/**
 * @file mock_led_controller.cpp
 * @brief Mock LED controller implementation.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Implements the MockLedController class defined in mock_led_controller.h.
 *
 * @copyright LGPL-3.0-or-later
 */

#include "hardware/led/mock_led_controller.h"

#include <QTimer>

namespace mwa::hardware {

MockLedController::MockLedController(QObject* parent)
    : LedControllerInterface(parent),
      state_(DeviceState::kDisconnected),
      intensity_(0.0),
      power_on_(false) {}

MockLedController::~MockLedController() = default;

void MockLedController::connectDevice() {
  if (state_ == DeviceState::kConnected ||
      state_ == DeviceState::kConnecting) {
    return;
  }
  state_ = DeviceState::kConnecting;
  emit stateChanged(state_);

  QTimer::singleShot(500, this, [this]() {
    state_ = DeviceState::kConnected;
    emit stateChanged(state_);
  });
}

void MockLedController::disconnectDevice() {
  if (state_ == DeviceState::kDisconnected) {
    return;
  }
  state_ = DeviceState::kDisconnected;
  emit stateChanged(state_);
}

QString MockLedController::deviceName() const {
  return QStringLiteral("Mock LED Controller");
}

DeviceInterface::DeviceState MockLedController::state() const {
  return state_;
}

bool MockLedController::isConnected() const {
  return state_ == DeviceState::kConnected;
}

void MockLedController::setIntensity(double percent) {
  intensity_ = percent;
  emit intensityChanged(intensity_);
}

double MockLedController::intensity() const {
  return intensity_;
}

void MockLedController::setPowerOn(bool on) {
  power_on_ = on;
  emit powerStateChanged(power_on_);
}

bool MockLedController::isPowerOn() const {
  return power_on_;
}

}  // namespace mwa::hardware
