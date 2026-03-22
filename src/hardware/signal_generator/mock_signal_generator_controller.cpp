/**
 * @file mock_signal_generator_controller.cpp
 * @brief Mock signal generator controller implementation.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Implements the MockSignalGeneratorController class defined in
 * mock_signal_generator_controller.h.
 *
 * @copyright LGPL-3.0-or-later
 */

#include "hardware/signal_generator/mock_signal_generator_controller.h"

#include <QTimer>

namespace mwa::hardware {

MockSignalGeneratorController::MockSignalGeneratorController(QObject* parent)
    : SignalGeneratorControllerInterface(parent),
      state_(DeviceState::kDisconnected),
      frequency_(1000.0),
      amplitude_(1.0),
      waveform_(Waveform::kSine),
      output_enabled_(false),
      sweep_start_hz_(0.0),
      sweep_stop_hz_(0.0),
      sweep_step_hz_(0.0) {}

MockSignalGeneratorController::~MockSignalGeneratorController() = default;

void MockSignalGeneratorController::connectDevice() {
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

void MockSignalGeneratorController::disconnectDevice() {
  if (state_ == DeviceState::kDisconnected) {
    return;
  }
  state_ = DeviceState::kDisconnected;
  emit stateChanged(state_);
}

QString MockSignalGeneratorController::deviceName() const {
  return QStringLiteral("Mock Signal Generator Controller");
}

DeviceInterface::DeviceState MockSignalGeneratorController::state() const {
  return state_;
}

bool MockSignalGeneratorController::isConnected() const {
  return state_ == DeviceState::kConnected;
}

void MockSignalGeneratorController::setFrequency(double hz) {
  frequency_ = hz;
  emit frequencyChanged(frequency_);
}

double MockSignalGeneratorController::frequency() const {
  return frequency_;
}

void MockSignalGeneratorController::setAmplitude(double volts) {
  amplitude_ = volts;
  emit amplitudeChanged(amplitude_);
}

double MockSignalGeneratorController::amplitude() const {
  return amplitude_;
}

void MockSignalGeneratorController::setWaveform(Waveform waveform) {
  waveform_ = waveform;
  emit waveformChanged(waveform_);
}

SignalGeneratorControllerInterface::Waveform
MockSignalGeneratorController::waveform() const {
  return waveform_;
}

void MockSignalGeneratorController::setOutputEnabled(bool enabled) {
  output_enabled_ = enabled;
  emit outputStateChanged(output_enabled_);
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

}  // namespace mwa::hardware
