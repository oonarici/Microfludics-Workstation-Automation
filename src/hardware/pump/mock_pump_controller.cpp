/**
 * @file mock_pump_controller.cpp
 * @brief Mock syringe pump controller implementation.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Implements the MockPumpController class defined in mock_pump_controller.h.
 *
 * @copyright LGPL-3.0-or-later
 */

#include "hardware/pump/mock_pump_controller.h"

#include <QTimer>

namespace mwa::hardware {

static constexpr int kConnectDelayMs = 500;

MockPumpController::MockPumpController(QObject* parent)
    : PumpControllerInterface(parent),
      state_(DeviceState::kDisconnected),
      flow_rate_(10.0),
      target_volume_(100.0),
      position_(0.0),
      is_infusing_(false) {}

MockPumpController::~MockPumpController() = default;

void MockPumpController::connectDevice() {
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

void MockPumpController::disconnectDevice() {
  if (state_ == DeviceState::kDisconnected) {
    return;
  }
  state_ = DeviceState::kDisconnected;
  emit stateChanged(state_);
}

QString MockPumpController::deviceName() const {
  return QStringLiteral("Mock Pump Controller");
}

DeviceInterface::DeviceState MockPumpController::state() const {
  return state_;
}

bool MockPumpController::isConnected() const {
  return state_ == DeviceState::kConnected;
}

void MockPumpController::setFlowRate(double uL_per_min) {
  flow_rate_ = uL_per_min;
  emit flowRateChanged(flow_rate_);
}

double MockPumpController::flowRate() const {
  return flow_rate_;
}

void MockPumpController::setTargetVolume(double uL) {
  target_volume_ = uL;
}

double MockPumpController::targetVolume() const {
  return target_volume_;
}

void MockPumpController::startInfusion() {
  if (is_infusing_) {
    return;
  }
  is_infusing_ = true;
  emit infusionStarted();
}

void MockPumpController::stopInfusion() {
  if (!is_infusing_) {
    return;
  }
  is_infusing_ = false;
  emit infusionStopped();
}

void MockPumpController::refill() {
  is_infusing_ = false;
  position_ = 0.0;
  emit positionChanged(position_);
}

double MockPumpController::currentPosition() const {
  return position_;
}

bool MockPumpController::isInfusing() const {
  return is_infusing_;
}

}  // namespace mwa::hardware
