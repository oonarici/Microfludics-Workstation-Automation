/**
 * @file mock_stage_controller.cpp
 * @brief Mock XYZ stage controller implementation.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Implements the MockStageController class defined in
 * mock_stage_controller.h.
 *
 * @copyright LGPL-3.0-or-later
 */

#include "hardware/stage/mock_stage_controller.h"

#include <QTimer>

namespace mwa::hardware {

static constexpr int    kConnectDelayMs  = 500;
static constexpr double kDefaultSpeed    = 1.0;
static constexpr int    kMoveDelayMs     = 300;

MockStageController::MockStageController(QObject* parent)
    : StageControllerInterface(parent),
      state_(DeviceState::kDisconnected),
      position_x_(0.0),
      position_y_(0.0),
      position_z_(0.0),
      speed_(kDefaultSpeed),
      is_moving_(false) {}

MockStageController::~MockStageController() = default;

void MockStageController::connectDevice() {
  if (state_ == DeviceState::kConnected ||
      state_ == DeviceState::kConnecting) {
    return;
  }
  state_ = DeviceState::kConnecting;
  emit stateChanged(state_);

  QTimer::singleShot(kConnectDelayMs, this, [this]() {
    state_ = DeviceState::kConnected;
    emit stateChanged(state_);
  });
}

void MockStageController::disconnectDevice() {
  if (state_ == DeviceState::kDisconnected) {
    return;
  }
  is_moving_ = false;
  state_ = DeviceState::kDisconnected;
  emit stateChanged(state_);
}

QString MockStageController::deviceName() const {
  return QStringLiteral("Mock Stage Controller");
}

DeviceInterface::DeviceState MockStageController::state() const {
  return state_;
}

bool MockStageController::isConnected() const {
  return state_ == DeviceState::kConnected;
}

void MockStageController::home() {
  if (is_moving_) {
    return;
  }
  is_moving_ = true;

  QTimer::singleShot(kMoveDelayMs, this, [this]() {
    position_x_ = 0.0;
    position_y_ = 0.0;
    position_z_ = 0.0;
    is_moving_ = false;
    emit positionChanged(position_x_, position_y_, position_z_);
    emit homeComplete();
  });
}

void MockStageController::moveAbsolute(double x, double y, double z) {
  doMove(x, y, z);
}

void MockStageController::moveRelative(double dx, double dy, double dz) {
  doMove(position_x_ + dx, position_y_ + dy, position_z_ + dz);
}

void MockStageController::stopMotion() {
  is_moving_ = false;
}

double MockStageController::positionX() const {
  return position_x_;
}

double MockStageController::positionY() const {
  return position_y_;
}

double MockStageController::positionZ() const {
  return position_z_;
}

void MockStageController::setSpeed(double mm_per_s) {
  speed_ = mm_per_s;
  emit speedChanged(speed_);
}

double MockStageController::speed() const {
  return speed_;
}

bool MockStageController::isMoving() const {
  return is_moving_;
}

void MockStageController::doMove(double tx, double ty, double tz) {
  if (is_moving_) {
    return;
  }
  is_moving_ = true;

  QTimer::singleShot(kMoveDelayMs, this, [this, tx, ty, tz]() {
    position_x_ = tx;
    position_y_ = ty;
    position_z_ = tz;
    is_moving_ = false;
    emit positionChanged(position_x_, position_y_, position_z_);
    emit moveComplete();
  });
}

}  // namespace mwa::hardware
