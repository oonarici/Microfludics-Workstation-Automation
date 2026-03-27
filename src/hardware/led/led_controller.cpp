/**
 * @file led_controller.cpp
 * @brief Hardware LED controller implementation.
 * @author MWA Team
 * @date 2026-03-25
 *
 * Implements the LedController class defined in led_controller.h.
 *
 * @copyright LGPL-3.0-or-later
 */

#include "hardware/led/led_controller.h"

#include <QSerialPort>

#include <algorithm>

#include "core/logger.h"
#include "hardware/serial_utils.h"

namespace mwa::hardware {

LedController::LedController(QObject* parent)
    : LedControllerInterface(parent),
      command_queue_(std::make_unique<CommandQueue>()) {}

LedController::~LedController() {
  command_queue_->shutdown();
  // Worker thread has stopped (shutdown blocks until idle) — safe to
  // destroy the port even though it was created on the worker thread.
}

void LedController::setPortName(const QString& name) {
  QMutexLocker lock(&mutex_);
  if (state_ != DeviceState::kDisconnected) {
    return;
  }
  port_name_ = name;
}

QString LedController::portName() const {
  QMutexLocker lock(&mutex_);
  return port_name_;
}

void LedController::setBaudRate(qint32 baud) {
  QMutexLocker lock(&mutex_);
  if (state_ != DeviceState::kDisconnected) {
    return;
  }
  baud_rate_ = baud;
}

qint32 LedController::baudRate() const {
  QMutexLocker lock(&mutex_);
  return baud_rate_;
}

void LedController::connectDevice() {
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
              QStringLiteral("LedController: failed to open %1 — %2")
                  .arg(name, err));
          setState(DeviceState::kError);
          emit errorOccurred(err);
          return;
        }

        mwa::core::Logger::instance().logInfo(
            QStringLiteral("LedController: connected on %1 @ %2 baud")
                .arg(name)
                .arg(baud));
        setState(DeviceState::kConnected);
      },
      kConnectTimeoutMs);
}

void LedController::disconnectDevice() {
  {
    QMutexLocker lock(&mutex_);
    if (state_ == DeviceState::kDisconnected) {
      return;
    }
    // Transition state immediately so command guards reject new requests
    // before the worker thread closes the port.
    state_ = DeviceState::kDisconnected;
    power_on_ = false;
    intensity_ = 0.0;
  }
  emit stateChanged(DeviceState::kDisconnected);

  command_queue_->enqueue([this]() {
    if (port_ && port_->isOpen()) {
      port_->close();
    }
    mwa::core::Logger::instance().logInfo(
        QStringLiteral("LedController: disconnected"));
  });
}

QString LedController::deviceName() const {
  QMutexLocker lock(&mutex_);
  if (port_name_.isEmpty()) {
    return QStringLiteral("LED Controller");
  }
  return QStringLiteral("LED Controller (%1)").arg(port_name_);
}

DeviceInterface::DeviceState LedController::state() const {
  QMutexLocker lock(&mutex_);
  return state_;
}

bool LedController::isConnected() const {
  QMutexLocker lock(&mutex_);
  return state_ == DeviceState::kConnected;
}

void LedController::setIntensity(double percent) {
  const double clamped = std::clamp(percent, 0.0, 100.0);

  {
    QMutexLocker lock(&mutex_);
    if (state_ != DeviceState::kConnected || intensity_ == clamped) {
      return;
    }
  }

  command_queue_->enqueue(
      [this, clamped]() {
        const QString resp = serial_utils::sendCommand(
            port_.get(),
            QStringLiteral("INT %1").arg(clamped, 0, 'f', 1),
            kResponseWaitMs);

        if (serial_utils::isOkResponse(resp)) {
          {
            QMutexLocker lock(&mutex_);
            intensity_ = clamped;
          }
          emit intensityChanged(clamped);
        } else {
          const QString err = serial_utils::formatCommandError(
              QStringLiteral("LedController"),
              QStringLiteral("setIntensity"), resp);
          mwa::core::Logger::instance().logWarning(err);
          emit errorOccurred(err);
        }
      },
      kCommandTimeoutMs);
}

double LedController::intensity() const {
  QMutexLocker lock(&mutex_);
  return intensity_;
}

void LedController::setPowerOn(bool on) {
  {
    QMutexLocker lock(&mutex_);
    if (state_ != DeviceState::kConnected || power_on_ == on) {
      return;
    }
  }

  command_queue_->enqueue(
      [this, on]() {
        const QString cmd =
            on ? QStringLiteral("ON") : QStringLiteral("OFF");
        const QString resp =
            serial_utils::sendCommand(port_.get(), cmd, kResponseWaitMs);

        if (serial_utils::isOkResponse(resp)) {
          {
            QMutexLocker lock(&mutex_);
            power_on_ = on;
          }
          emit powerStateChanged(on);
        } else {
          const QString action = QStringLiteral("setPowerOn(%1)")
              .arg(on ? QStringLiteral("true")
                      : QStringLiteral("false"));
          const QString err = serial_utils::formatCommandError(
              QStringLiteral("LedController"), action, resp);
          mwa::core::Logger::instance().logWarning(err);
          emit errorOccurred(err);
        }
      },
      kCommandTimeoutMs);
}

bool LedController::isPowerOn() const {
  QMutexLocker lock(&mutex_);
  return power_on_;
}

void LedController::setState(DeviceState new_state) {
  {
    QMutexLocker lock(&mutex_);
    state_ = new_state;
  }
  emit stateChanged(new_state);
}

}  // namespace mwa::hardware
