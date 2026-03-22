/**
 * @file mock_signal_generator_controller.h
 * @brief Mock implementation of the signal generator controller interface.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Provides a simulated signal generator controller that implements
 * SignalGeneratorControllerInterface without requiring real hardware.
 * Suitable for GUI development and unit testing.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include "hardware/signal_generator/signal_generator_controller_interface.h"

namespace mwa::hardware {

/**
 * @class MockSignalGeneratorController
 * @brief Simulated signal generator controller for hardware-free development.
 *
 * Implements SignalGeneratorControllerInterface with in-memory state. The
 * connect() operation uses a 500 ms QTimer delay to simulate real hardware
 * latency. Frequency defaults to 1000.0 Hz, amplitude to 1.0 V, waveform
 * to Waveform::kSine, and output is disabled by default.
 *
 * @see SignalGeneratorControllerInterface
 */
class MockSignalGeneratorController : public SignalGeneratorControllerInterface {
  Q_OBJECT

 public:
  /**
   * @brief Construct a MockSignalGeneratorController.
   *
   * @param parent Optional QObject parent for Qt ownership management.
   */
  explicit MockSignalGeneratorController(QObject* parent = nullptr);

  /**
   * @brief Virtual destructor.
   */
  ~MockSignalGeneratorController() override;

  // DeviceInterface overrides

  /**
   * @brief Initiate a simulated 500 ms asynchronous connection.
   *
   * Transitions state to DeviceState::kConnecting immediately, then after
   * 500 ms transitions to DeviceState::kConnected and emits stateChanged().
   */
  void connectDevice() override;

  /**
   * @brief Disconnect from the simulated device immediately.
   *
   * Transitions state to DeviceState::kDisconnected and emits stateChanged().
   */
  void disconnectDevice() override;

  /**
   * @brief Return the display name for this mock device.
   *
   * @return The string "Mock Signal Generator Controller".
   */
  [[nodiscard]] QString deviceName() const override;

  /**
   * @brief Return the current connection state.
   *
   * @return The current DeviceState value.
   */
  [[nodiscard]] DeviceState state() const override;

  /**
   * @brief Query whether the device is fully connected.
   *
   * @return @c true if the device state is DeviceState::kConnected.
   */
  [[nodiscard]] bool isConnected() const override;

  // SignalGeneratorControllerInterface overrides

  /**
   * @brief Set the output frequency and emit frequencyChanged().
   *
   * @param hz Output frequency in hertz.
   */
  void setFrequency(double hz) override;

  /**
   * @brief Return the current output frequency.
   *
   * @return Output frequency in hertz.
   */
  [[nodiscard]] double frequency() const override;

  /**
   * @brief Set the output amplitude and emit amplitudeChanged().
   *
   * @param volts Peak amplitude in volts.
   */
  void setAmplitude(double volts) override;

  /**
   * @brief Return the current output amplitude.
   *
   * @return Peak amplitude in volts.
   */
  [[nodiscard]] double amplitude() const override;

  /**
   * @brief Set the output waveform shape and emit waveformChanged().
   *
   * @param waveform The desired Waveform value.
   */
  void setWaveform(Waveform waveform) override;

  /**
   * @brief Return the current output waveform shape.
   *
   * @return The current Waveform value.
   */
  [[nodiscard]] Waveform waveform() const override;

  /**
   * @brief Enable or disable the output and emit outputStateChanged().
   *
   * @param enabled @c true to enable the output, @c false to disable.
   */
  void setOutputEnabled(bool enabled) override;

  /**
   * @brief Query whether the signal output is currently enabled.
   *
   * @return @c true if the output is enabled, @c false otherwise.
   */
  [[nodiscard]] bool isOutputEnabled() const override;

  /**
   * @brief Configure a frequency sweep (stored internally, no signal emitted).
   *
   * @param start_hz  Start frequency of the sweep in hertz.
   * @param stop_hz   Stop frequency of the sweep in hertz.
   * @param step_hz   Frequency step size in hertz.
   */
  void configureSweep(
      double start_hz, double stop_hz, double step_hz) override;

 private:
  /// Current connection state of the device.
  DeviceState state_;
  /// Current output frequency in hertz.
  double frequency_;
  /// Current output amplitude in volts.
  double amplitude_;
  /// Current output waveform shape.
  Waveform waveform_;
  /// Whether the signal output is enabled.
  bool output_enabled_;
  /// Sweep start frequency in hertz.
  double sweep_start_hz_;
  /// Sweep stop frequency in hertz.
  double sweep_stop_hz_;
  /// Sweep step size in hertz.
  double sweep_step_hz_;
};

}  // namespace mwa::hardware
