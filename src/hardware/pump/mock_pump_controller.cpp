/**
 * @file mock_pump_controller.cpp
 * @brief Mock syringe pump controller implementation.
 * @author MWA Team
 * @date 2026-04-02
 *
 * Implements the MockPumpController class defined in
 * mock_pump_controller.h. Provides timed dispensing and refill
 * simulation, syringe-size-dependent flow rate validation, and
 * optional error injection for testing.
 *
 * @copyright LGPL-3.0-or-later
 */

#include "hardware/pump/mock_pump_controller.h"

#include <QRandomGenerator>
#include <QTimer>

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace mwa::hardware {

// ── Constants ────────────────────────────────────────────────

/// Simulated connection delay in milliseconds.
static constexpr int kConnectDelayMs = 500;

/// Probability threshold (0–100) for simulated stall errors.
static constexpr int kErrorProbabilityPercent = 10;

/// Number of entries in the syringe-size-to-max-flow-rate table.
static constexpr int kSyringeTableSize = 4;

/**
 * @brief Lookup table mapping syringe volume (µL) to maximum
 *        flow rate (µL/min). Must be sorted by volume ascending.
 */
static constexpr std::array<
    std::pair<double, double>, kSyringeTableSize>
    kSyringeMaxFlowRates{{
        {100.0, 15.0},
        {250.0, 40.0},
        {500.0, 75.0},
        {1000.0, 150.0},
    }};

// ── Construction / Destruction ───────────────────────────────

MockPumpController::MockPumpController(QObject* parent)
    : PumpControllerInterface(parent),
      state_(DeviceState::kDisconnected),
      flow_rate_(10.0),
      target_volume_(100.0),
      position_(0.0),
      is_infusing_(false),
      infusion_timer_(new QTimer(this)),
      refill_timer_(new QTimer(this)),
      syringe_volume_(kDefaultSyringeVolume),
      dispensed_volume_(0.0),
      is_refilling_(false),
      simulate_error_(false) {
  connect(infusion_timer_, &QTimer::timeout,
          this, &MockPumpController::onInfusionTick);
  connect(refill_timer_, &QTimer::timeout,
          this, &MockPumpController::onRefillTick);
}

MockPumpController::~MockPumpController() = default;

// ── DeviceInterface overrides ────────────────────────────────

void MockPumpController::connectDevice() {
  if (state_ == DeviceState::kConnected ||
      state_ == DeviceState::kConnecting) {
    return;
  }
  state_ = DeviceState::kConnecting;
  emit stateChanged(state_);

  QTimer::singleShot(kConnectDelayMs, this, [this]() {
    state_ = DeviceState::kConnected;
    position_ = syringe_volume_;
    emit positionChanged(position_);
    emit stateChanged(state_);
  });
}

void MockPumpController::disconnectDevice() {
  if (state_ == DeviceState::kDisconnected) {
    return;
  }
  stopInfusionTimer();
  stopRefillTimer();
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

// ── PumpControllerInterface overrides ────────────────────────

void MockPumpController::setFlowRate(double uL_per_min) {
  if (!isConnected()) {
    emit errorOccurred(
        QStringLiteral("Cannot set flow rate: not connected"));
    return;
  }

  const double max_rate = maxFlowRateForSyringe();
  if (uL_per_min < kMinFlowRate || uL_per_min > max_rate) {
    emit errorOccurred(
        QStringLiteral("Flow rate %.3f µL/min out of range "
                       "[%.3f, %.1f]")
            .arg(uL_per_min)
            .arg(kMinFlowRate)
            .arg(max_rate));
    return;
  }

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
  if (is_infusing_ || !isConnected()) {
    return;
  }
  if (is_refilling_) {
    emit errorOccurred(
        QStringLiteral("Cannot infuse while refilling"));
    return;
  }

  // Error injection: 10 % chance of simulated stall.
  if (simulate_error_) {
    const int roll = QRandomGenerator::global()->bounded(100);
    if (roll < kErrorProbabilityPercent) {
      emit errorOccurred(
          QStringLiteral("Simulated mechanical stall"));
      return;
    }
  }

  dispensed_volume_ = 0.0;
  is_infusing_ = true;
  infusion_timer_->start(kInfusionTickMs);
  emit infusionStarted();
}

void MockPumpController::stopInfusion() {
  if (!is_infusing_) {
    return;
  }
  stopInfusionTimer();
  emit infusionStopped();
}

void MockPumpController::refill() {
  if (is_infusing_) {
    emit errorOccurred(
        QStringLiteral("Cannot refill while infusing"));
    return;
  }
  if (is_refilling_) {
    return;
  }
  if (!isConnected()) {
    return;
  }

  is_refilling_ = true;
  refill_timer_->start(kInfusionTickMs);
}

double MockPumpController::currentPosition() const {
  return position_;
}

bool MockPumpController::isInfusing() const {
  return is_infusing_;
}

// ── Mock-specific methods ────────────────────────────────────

void MockPumpController::setSyringeVolume(double uL) {
  if (uL > 0.0) {
    syringe_volume_ = uL;
  }
}

double MockPumpController::syringeVolume() const {
  return syringe_volume_;
}

void MockPumpController::setSimulateError(bool enable) {
  simulate_error_ = enable;
}

bool MockPumpController::simulateError() const {
  return simulate_error_;
}

double MockPumpController::maxFlowRateForSyringe() const {
  const double vol = syringe_volume_;

  // Below smallest table entry — use fallback.
  if (vol <= kSyringeMaxFlowRates.front().first) {
    return (vol == kSyringeMaxFlowRates.front().first)
               ? kSyringeMaxFlowRates.front().second
               : kFallbackMaxFlowRate;
  }
  // Above largest table entry — use fallback.
  if (vol >= kSyringeMaxFlowRates.back().first) {
    return (vol == kSyringeMaxFlowRates.back().first)
               ? kSyringeMaxFlowRates.back().second
               : kFallbackMaxFlowRate;
  }

  // Linear interpolation between bracketing entries.
  for (int i = 0; i < kSyringeTableSize - 1; ++i) {
    const auto& [v0, r0] = kSyringeMaxFlowRates[
        static_cast<std::size_t>(i)];
    const auto& [v1, r1] = kSyringeMaxFlowRates[
        static_cast<std::size_t>(i + 1)];
    if (vol >= v0 && vol <= v1) {
      const double t = (vol - v0) / (v1 - v0);
      return r0 + t * (r1 - r0);
    }
  }

  return kFallbackMaxFlowRate;  // Should not be reached.
}

bool MockPumpController::isRefilling() const {
  return is_refilling_;
}

double MockPumpController::dispensedVolume() const {
  return dispensed_volume_;
}

// ── Private helpers ──────────────────────────────────────────

void MockPumpController::onInfusionTick() {
  const double tick_seconds =
      static_cast<double>(kInfusionTickMs) / 1000.0;
  const double dispensed_per_tick =
      (flow_rate_ / 60.0) * tick_seconds;

  position_ -= dispensed_per_tick;
  dispensed_volume_ += dispensed_per_tick;

  // Syringe empty — clamp and stop with error.
  if (position_ <= 0.0) {
    position_ = 0.0;
    stopInfusionTimer();
    emit positionChanged(position_);
    emit errorOccurred(QStringLiteral("Syringe empty"));
    emit infusionStopped();
    return;
  }

  // Target volume reached — stop normally.
  if (dispensed_volume_ >= target_volume_) {
    stopInfusionTimer();
    emit positionChanged(position_);
    emit infusionStopped();
    return;
  }

  emit positionChanged(position_);
}

void MockPumpController::onRefillTick() {
  const double tick_seconds =
      static_cast<double>(kInfusionTickMs) / 1000.0;
  const double aspirated_per_tick =
      (kRefillFlowRate / 60.0) * tick_seconds;

  position_ += aspirated_per_tick;

  // Syringe full — clamp and stop.
  if (position_ >= syringe_volume_) {
    position_ = syringe_volume_;
    stopRefillTimer();
    emit positionChanged(position_);
    return;
  }

  emit positionChanged(position_);
}

void MockPumpController::stopInfusionTimer() {
  infusion_timer_->stop();
  is_infusing_ = false;
}

void MockPumpController::stopRefillTimer() {
  refill_timer_->stop();
  is_refilling_ = false;
}

}  // namespace mwa::hardware
