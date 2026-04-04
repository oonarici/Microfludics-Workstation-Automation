/**
 * @file mock_signal_generator_controller.cpp
 * @brief Mock signal generator controller implementation.
 * @author MWA Team
 * @date 2026-04-02
 *
 * Implements the MockSignalGeneratorController class defined in
 * mock_signal_generator_controller.h.  Adds realistic SCPI-style
 * simulation: parameter validation, command latency, frequency sweep
 * execution, instrument identity, and error injection.
 *
 * @copyright LGPL-3.0-or-later
 */

#include "hardware/signal_generator/mock_signal_generator_controller.h"

#include <QTimer>

namespace mwa::hardware {

static constexpr int kConnectDelayMs = 500;

// -------------------------------------------------------------------
// Construction / destruction
// -------------------------------------------------------------------

MockSignalGeneratorController::MockSignalGeneratorController(
    QObject* parent)
    : SignalGeneratorControllerInterface(parent),
      state_(DeviceState::kDisconnected),
      frequency_(1000.0),
      amplitude_(1.0),
      waveform_(Waveform::kSine),
      output_enabled_(false),
      sweep_start_hz_(0.0),
      sweep_stop_hz_(0.0),
      sweep_step_hz_(0.0),
      sweep_timer_(new QTimer(this)),
      is_sweeping_(false),
      current_sweep_freq_(0.0),
      simulate_error_(false),
      rng_(std::random_device{}()) {
  sweep_timer_->setInterval(kSweepStepIntervalMs);
  connect(sweep_timer_, &QTimer::timeout, this, [this]() {
    current_sweep_freq_ += sweep_step_hz_;
    if (current_sweep_freq_ >= sweep_stop_hz_) {
      current_sweep_freq_ = sweep_stop_hz_;
      frequency_ = current_sweep_freq_;
      emit frequencyChanged(frequency_);
      sweep_timer_->stop();
      is_sweeping_ = false;
      return;
    }
    frequency_ = current_sweep_freq_;
    emit frequencyChanged(frequency_);
  });
}

MockSignalGeneratorController::~MockSignalGeneratorController() {
  if (sweep_timer_->isActive()) {
    sweep_timer_->stop();
  }
}

// -------------------------------------------------------------------
// DeviceInterface overrides
// -------------------------------------------------------------------

void MockSignalGeneratorController::connectDevice() {
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

void MockSignalGeneratorController::disconnectDevice() {
  if (state_ == DeviceState::kDisconnected) {
    return;
  }
  stopSweep();
  state_ = DeviceState::kDisconnected;
  emit stateChanged(state_);
}

QString MockSignalGeneratorController::deviceName() const {
  return QStringLiteral("Mock Signal Generator Controller");
}

DeviceInterface::DeviceState
MockSignalGeneratorController::state() const {
  return state_;
}

bool MockSignalGeneratorController::isConnected() const {
  return state_ == DeviceState::kConnected;
}

// -------------------------------------------------------------------
// SignalGeneratorControllerInterface overrides
// -------------------------------------------------------------------

void MockSignalGeneratorController::setFrequency(double hz) {
  if (hz < kMinFrequencyHz || hz > kMaxFrequencyHz) {
    emit errorOccurred(
        QStringLiteral("Frequency out of range: %1 Hz")
            .arg(hz));
    return;
  }

  if (simulate_error_) {
    std::uniform_int_distribution<int> dist(0, 9);
    if (dist(rng_) == 0) {
      emit errorOccurred(
          QStringLiteral("Frequency lock failed"));
      return;
    }
  }

  frequency_ = hz;
  QTimer::singleShot(kCommandLatencyMs, this, [this]() {
    emit frequencyChanged(frequency_);
  });
}

double MockSignalGeneratorController::frequency() const {
  return frequency_;
}

void MockSignalGeneratorController::setAmplitude(double volts) {
  if (volts < kMinAmplitudeV || volts > kMaxAmplitudeV) {
    emit errorOccurred(
        QStringLiteral("Amplitude out of range: %1 V")
            .arg(volts));
    return;
  }

  amplitude_ = volts;
  QTimer::singleShot(kCommandLatencyMs, this, [this]() {
    emit amplitudeChanged(amplitude_);
  });
}

double MockSignalGeneratorController::amplitude() const {
  return amplitude_;
}

void MockSignalGeneratorController::setWaveform(
    Waveform waveform) {
  waveform_ = waveform;
  QTimer::singleShot(kCommandLatencyMs, this, [this]() {
    emit waveformChanged(waveform_);
  });
}

SignalGeneratorControllerInterface::Waveform
MockSignalGeneratorController::waveform() const {
  return waveform_;
}

void MockSignalGeneratorController::setOutputEnabled(
    bool enabled) {
  output_enabled_ = enabled;
  QTimer::singleShot(kCommandLatencyMs, this, [this]() {
    emit outputStateChanged(output_enabled_);
  });
}

bool MockSignalGeneratorController::isOutputEnabled() const {
  return output_enabled_;
}

void MockSignalGeneratorController::configureSweep(
    double start_hz, double stop_hz, double step_hz) {
  sweep_start_hz_ = start_hz;
  sweep_stop_hz_ = stop_hz;
  sweep_step_hz_ = step_hz;
}

// -------------------------------------------------------------------
// Enhanced simulation API
// -------------------------------------------------------------------

void MockSignalGeneratorController::startSweep() {
  if (is_sweeping_ || sweep_step_hz_ <= 0.0) {
    return;
  }
  is_sweeping_ = true;
  current_sweep_freq_ = sweep_start_hz_;
  frequency_ = current_sweep_freq_;
  emit frequencyChanged(frequency_);
  sweep_timer_->start();
}

void MockSignalGeneratorController::stopSweep() {
  if (!is_sweeping_) {
    return;
  }
  sweep_timer_->stop();
  is_sweeping_ = false;
}

bool MockSignalGeneratorController::isSweeping() const {
  return is_sweeping_;
}

QString MockSignalGeneratorController::instrumentIdentity() const {
  return QStringLiteral(
      "MOCK INSTRUMENTS,MWA-SG1000,SN001,V1.0");
}

void MockSignalGeneratorController::setSimulateError(
    bool enable) {
  simulate_error_ = enable;
}

}  // namespace mwa::hardware
