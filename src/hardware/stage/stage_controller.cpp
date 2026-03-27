/**
 * @file stage_controller.cpp
 * @brief Hardware XYZ stage controller implementation.
 * @author MWA Team
 * @date 2026-03-25
 *
 * Implements the StageController class defined in stage_controller.h.
 *
 * @copyright LGPL-3.0-or-later
 */

#include "hardware/stage/stage_controller.h"

#include <QSerialPort>
#include <QStringList>

#include <algorithm>

#include "core/logger.h"
#include "hardware/serial_utils.h"

namespace mwa::hardware {

StageController::StageController(QObject* parent)
    : StageControllerInterface(parent),
      command_queue_(std::make_unique<CommandQueue>()) {}

StageController::~StageController() {
  command_queue_->shutdown();
  // Worker thread has stopped (shutdown blocks until idle) — safe to
  // destroy the port even though it was created on the worker thread.
}

void StageController::setPortName(const QString& name) {
  QMutexLocker lock(&mutex_);
  if (state_ != DeviceState::kDisconnected) {
    return;
  }
  port_name_ = name;
}

QString StageController::portName() const {
  QMutexLocker lock(&mutex_);
  return port_name_;
}

void StageController::setBaudRate(qint32 baud) {
  QMutexLocker lock(&mutex_);
  if (state_ != DeviceState::kDisconnected) {
    return;
  }
  baud_rate_ = baud;
}

qint32 StageController::baudRate() const {
  QMutexLocker lock(&mutex_);
  return baud_rate_;
}

void StageController::connectDevice() {
  QString name;
  qint32 baud{};
  {
    QMutexLocker lock(&mutex_);
    if (state_ == DeviceState::kConnected ||
        state_ == DeviceState::kConnecting) {
      return;
    }
    state_ = DeviceState::kConnecting;
    name = port_name_;
    baud = baud_rate_;
  }
  emit stateChanged(DeviceState::kConnecting);

  command_queue_->enqueue(
      [this, name, baud]() {
        if (!port_) {
          port_ = std::make_unique<QSerialPort>();
        }

        if (!serial_utils::openPort(port_.get(), name, baud)) {
          const QString err = port_->errorString();
          mwa::core::Logger::instance().logError(
              QStringLiteral(
                  "StageController: failed to open %1 — %2")
                  .arg(name, err));
          setState(DeviceState::kError);
          emit errorOccurred(err);
          return;
        }

        mwa::core::Logger::instance().logInfo(
            QStringLiteral(
                "StageController: connected on %1 @ %2 baud")
                .arg(name)
                .arg(baud));
        setState(DeviceState::kConnected);
      },
      kConnectTimeoutMs);
}

void StageController::disconnectDevice() {
  {
    QMutexLocker lock(&mutex_);
    if (state_ == DeviceState::kDisconnected) {
      return;
    }
    // Transition state immediately so command guards reject new requests
    // before the worker thread closes the port.
    state_ = DeviceState::kDisconnected;
    position_x_ = kDefaultPosition;
    position_y_ = kDefaultPosition;
    position_z_ = kDefaultPosition;
    speed_ = kDefaultSpeed;
    is_moving_ = false;
  }
  emit stateChanged(DeviceState::kDisconnected);

  command_queue_->enqueue([this]() {
    if (port_ && port_->isOpen()) {
      port_->close();
    }
    mwa::core::Logger::instance().logInfo(
        QStringLiteral("StageController: disconnected"));
  });
}

QString StageController::deviceName() const {
  QMutexLocker lock(&mutex_);
  if (port_name_.isEmpty()) {
    return QStringLiteral("Stage Controller");
  }
  return QStringLiteral("Stage Controller (%1)").arg(port_name_);
}

DeviceInterface::DeviceState StageController::state() const {
  QMutexLocker lock(&mutex_);
  return state_;
}

bool StageController::isConnected() const {
  QMutexLocker lock(&mutex_);
  return state_ == DeviceState::kConnected;
}

void StageController::home() {
  {
    QMutexLocker lock(&mutex_);
    if (state_ != DeviceState::kConnected) {
      return;
    }
  }

  command_queue_->enqueue(
      [this]() {
        MovingGuard guard(*this);

        const QString resp = serial_utils::sendCommand(
            port_.get(), QStringLiteral("HOME"), kResponseWaitMs);

        if (serial_utils::isOkResponse(resp)) {
          {
            QMutexLocker lock(&mutex_);
            position_x_ = kDefaultPosition;
            position_y_ = kDefaultPosition;
            position_z_ = kDefaultPosition;
          }
          emit positionChanged(0.0, 0.0, 0.0);
          emit homeComplete();
        } else {
          const QString err = serial_utils::formatCommandError(
              QStringLiteral("StageController"),
              QStringLiteral("HOME"), resp);
          mwa::core::Logger::instance().logWarning(err);
          emit errorOccurred(err);
        }
      },
      kMoveTimeoutMs);
}

void StageController::moveAbsolute(double x, double y, double z) {
  {
    QMutexLocker lock(&mutex_);
    if (state_ != DeviceState::kConnected) {
      return;
    }
  }

  command_queue_->enqueue(
      [this, x, y, z]() {
        MovingGuard guard(*this);

        const QString cmd =
            QStringLiteral("MOVE %1 %2 %3")
                .arg(x, 0, 'f', 4)
                .arg(y, 0, 'f', 4)
                .arg(z, 0, 'f', 4);
        const QString resp =
            serial_utils::sendCommand(port_.get(), cmd,
                                      kResponseWaitMs);

        if (serial_utils::isOkResponse(resp)) {
          {
            QMutexLocker lock(&mutex_);
            position_x_ = x;
            position_y_ = y;
            position_z_ = z;
          }
          emit positionChanged(x, y, z);
          emit moveComplete();
        } else {
          const QString err = serial_utils::formatCommandError(
              QStringLiteral("StageController"),
              QStringLiteral("MOVE"), resp);
          mwa::core::Logger::instance().logWarning(err);
          emit errorOccurred(err);
        }
      },
      kMoveTimeoutMs);
}

void StageController::moveRelative(double dx, double dy, double dz) {
  {
    QMutexLocker lock(&mutex_);
    if (state_ != DeviceState::kConnected) {
      return;
    }
  }

  command_queue_->enqueue(
      [this, dx, dy, dz]() {
        MovingGuard guard(*this);

        const QString cmd =
            QStringLiteral("RMOVE %1 %2 %3")
                .arg(dx, 0, 'f', 4)
                .arg(dy, 0, 'f', 4)
                .arg(dz, 0, 'f', 4);
        const QString resp =
            serial_utils::sendCommand(port_.get(), cmd,
                                      kResponseWaitMs);

        if (serial_utils::isOkResponse(resp)) {
          double new_x{};
          double new_y{};
          double new_z{};
          {
            QMutexLocker lock(&mutex_);
            position_x_ += dx;
            position_y_ += dy;
            position_z_ += dz;
            new_x = position_x_;
            new_y = position_y_;
            new_z = position_z_;
          }
          emit positionChanged(new_x, new_y, new_z);
          emit moveComplete();
        } else {
          const QString err = serial_utils::formatCommandError(
              QStringLiteral("StageController"),
              QStringLiteral("RMOVE"), resp);
          mwa::core::Logger::instance().logWarning(err);
          emit errorOccurred(err);
        }
      },
      kMoveTimeoutMs);
}

void StageController::stopMotion() {
  {
    QMutexLocker lock(&mutex_);
    if (state_ != DeviceState::kConnected) {
      return;
    }
  }

  command_queue_->enqueue(
      [this]() {
        const QString resp = serial_utils::sendCommand(
            port_.get(), QStringLiteral("STOP"), kResponseWaitMs);

        {
          QMutexLocker lock(&mutex_);
          is_moving_ = false;
        }

        if (!serial_utils::isOkResponse(resp)) {
          const QString err = serial_utils::formatCommandError(
              QStringLiteral("StageController"),
              QStringLiteral("STOP"), resp);
          mwa::core::Logger::instance().logWarning(err);
          emit errorOccurred(err);
        }

        // Stage may have halted at an intermediate position.
        queryPosition();
      },
      kCommandTimeoutMs);
}

double StageController::positionX() const {
  QMutexLocker lock(&mutex_);
  return position_x_;
}

double StageController::positionY() const {
  QMutexLocker lock(&mutex_);
  return position_y_;
}

double StageController::positionZ() const {
  QMutexLocker lock(&mutex_);
  return position_z_;
}

void StageController::setSpeed(double mm_per_s) {
  const double clamped = std::max(0.0, mm_per_s);

  {
    QMutexLocker lock(&mutex_);
    if (state_ != DeviceState::kConnected || speed_ == clamped) {
      return;
    }
  }

  command_queue_->enqueue(
      [this, clamped]() {
        const QString cmd =
            QStringLiteral("SPEED %1").arg(clamped, 0, 'f', 2);
        const QString resp =
            serial_utils::sendCommand(port_.get(), cmd,
                                      kResponseWaitMs);

        if (serial_utils::isOkResponse(resp)) {
          {
            QMutexLocker lock(&mutex_);
            speed_ = clamped;
          }
          emit speedChanged(clamped);
        } else {
          const QString err = serial_utils::formatCommandError(
              QStringLiteral("StageController"),
              QStringLiteral("SPEED"), resp);
          mwa::core::Logger::instance().logWarning(err);
          emit errorOccurred(err);
        }
      },
      kCommandTimeoutMs);
}

double StageController::speed() const {
  QMutexLocker lock(&mutex_);
  return speed_;
}

bool StageController::isMoving() const {
  QMutexLocker lock(&mutex_);
  return is_moving_;
}

bool StageController::parsePosition(const QString& response,
                                     double& x, double& y, double& z) {
  const QStringList parts =
      response.split(QLatin1Char(' '), Qt::SkipEmptyParts);
  if (parts.size() < 3) {
    return false;
  }

  bool ok_x = false;
  bool ok_y = false;
  bool ok_z = false;
  const double px = parts[0].toDouble(&ok_x);
  const double py = parts[1].toDouble(&ok_y);
  const double pz = parts[2].toDouble(&ok_z);

  if (!ok_x || !ok_y || !ok_z) {
    return false;
  }

  x = px;
  y = py;
  z = pz;
  return true;
}

void StageController::setState(DeviceState new_state) {
  {
    QMutexLocker lock(&mutex_);
    state_ = new_state;
  }
  emit stateChanged(new_state);
}

void StageController::queryPosition() {
  const QString resp = serial_utils::sendCommand(
      port_.get(), QStringLiteral("POS?"), kResponseWaitMs);
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;

  if (parsePosition(resp, x, y, z)) {
    {
      QMutexLocker lock(&mutex_);
      position_x_ = x;
      position_y_ = y;
      position_z_ = z;
    }
    emit positionChanged(x, y, z);
  }
}

}  // namespace mwa::hardware
