/**
 * @file mock_signal_generator_controller.h
 * @brief Mock implementation of the signal generator controller interface.
 * @author MWA Team
 * @date 2026-04-02
 *
 * Provides a simulated signal generator controller that implements
 * SignalGeneratorControllerInterface without requiring real hardware.
 * Includes realistic SCPI-style behaviour: parameter validation, command
 * latency simulation, frequency sweep execution, instrument identity
 * query, and error injection for robustness testing.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <random>

#include <QTimer>

#include "hardware/signal_generator/signal_generator_controller_interface.h"

namespace mwa::hardware {

/// Minimum allowable output frequency in hertz (1 uHz).
inline constexpr double kMinFrequencyHz = 0.000001;
/// Maximum allowable output frequency in hertz (120 MHz).
inline constexpr double kMaxFrequencyHz = 120'000'000.0;
/// Minimum allowable peak amplitude in volts.
inline constexpr double kMinAmplitudeV = 0.001;
/// Maximum allowable peak amplitude in volts.
inline constexpr double kMaxAmplitudeV = 20.0;
/// Simulated command processing latency in milliseconds.
inline constexpr int kCommandLatencyMs = 20;
/// Interval between sweep frequency steps in milliseconds.
inline constexpr int kSweepStepIntervalMs = 100;

/**
 * @class MockSignalGeneratorController
 * @brief Simulated signal generator controller for hardware-free
 *        development.
 *
 * Implements SignalGeneratorControllerInterface with in-memory state
 * and realistic SCPI-like simulation features:
 *
 * - **Parameter validation** — frequency and amplitude are clamped to
 *   instrument-legal ranges; out-of-range requests emit
 *   errorOccurred() without changing the stored value.
 * - **Command latency** — every set* method stores the value
 *   immediately but defers the corresponding signal by
 *   @ref kCommandLatencyMs to exercise GUI responsiveness.
 * - **Frequency sweep** — startSweep() steps from the configured
 *   start to stop frequency at @ref kSweepStepIntervalMs intervals.
 * - **Instrument identity** — instrumentIdentity() returns a
 *   SCPI-style *IDN? response string.
 * - **Error injection** — when enabled, setFrequency() has a 10 %
 *   random chance of failing with a "Frequency lock failed" error.
 *
 * The connect() operation uses a 500 ms QTimer delay to simulate
 * real hardware latency.  Frequency defaults to 1000.0 Hz, amplitude
 * to 1.0 V, waveform to Waveform::kSine, and output is disabled by
 * default.
 *
 * @see SignalGeneratorControllerInterface
 */
class MockSignalGeneratorController
    : public SignalGeneratorControllerInterface {
  Q_OBJECT

 public:
  /**
   * @brief Construct a MockSignalGeneratorController.
   *
   * @param parent Optional QObject parent for Qt ownership
   *               management.
   */
  explicit MockSignalGeneratorController(
      QObject* parent = nullptr);

  /**
   * @brief Virtual destructor.
   */
  ~MockSignalGeneratorController() override;

  // -- DeviceInterface overrides ----------------------------------

  /**
   * @brief Initiate a simulated 500 ms asynchronous connection.
   *
   * Transitions state to DeviceState::kConnecting immediately,
   * then after 500 ms transitions to DeviceState::kConnected and
   * emits stateChanged().
   */
  void connectDevice() override;

  /**
   * @brief Disconnect from the simulated device immediately.
   *
   * Stops any active sweep, transitions state to
   * DeviceState::kDisconnected and emits stateChanged().
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
   * @return @c true if state is DeviceState::kConnected.
   */
  [[nodiscard]] bool isConnected() const override;

  // -- SignalGeneratorControllerInterface overrides ----------------

  /**
   * @brief Set the output frequency with validation and latency.
   *
   * The value is stored immediately if it falls within
   * [@ref kMinFrequencyHz, @ref kMaxFrequencyHz].  The
   * frequencyChanged() signal is emitted after a
   * @ref kCommandLatencyMs delay.  Out-of-range values emit
   * errorOccurred() and leave the frequency unchanged.
   *
   * When error injection is enabled via setSimulateError(), there
   * is a 10 % random chance that the call fails with
   * errorOccurred("Frequency lock failed") without changing the
   * stored value.
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
   * @brief Set the output amplitude with validation and latency.
   *
   * The value is stored immediately if it falls within
   * [@ref kMinAmplitudeV, @ref kMaxAmplitudeV].  The
   * amplitudeChanged() signal is emitted after a
   * @ref kCommandLatencyMs delay.  Out-of-range values emit
   * errorOccurred() and leave the amplitude unchanged.
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
   * @brief Set the output waveform shape with latency.
   *
   * The value is stored immediately and waveformChanged() is
   * emitted after a @ref kCommandLatencyMs delay.
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
   * @brief Enable or disable the output with latency.
   *
   * The value is stored immediately and outputStateChanged() is
   * emitted after a @ref kCommandLatencyMs delay.
   *
   * @param enabled @c true to enable output, @c false to disable.
   */
  void setOutputEnabled(bool enabled) override;

  /**
   * @brief Query whether the signal output is currently enabled.
   *
   * @return @c true if enabled, @c false otherwise.
   */
  [[nodiscard]] bool isOutputEnabled() const override;

  /**
   * @brief Configure a frequency sweep.
   *
   * Stores the start, stop and step parameters for a subsequent
   * call to startSweep().  Does not emit any signal.
   *
   * @param start_hz Start frequency of the sweep in hertz.
   * @param stop_hz  Stop frequency of the sweep in hertz.
   * @param step_hz  Frequency step size in hertz.
   */
  void configureSweep(
      double start_hz, double stop_hz, double step_hz) override;

  // -- Enhanced simulation API ------------------------------------

  /**
   * @brief Begin executing the configured frequency sweep.
   *
   * Starts stepping from sweep_start_hz_ to sweep_stop_hz_ in
   * increments of sweep_step_hz_ at @ref kSweepStepIntervalMs
   * intervals.  Each step updates frequency_ and emits
   * frequencyChanged().  When the sweep reaches or exceeds
   * sweep_stop_hz_ the timer stops automatically.
   *
   * Has no effect if a sweep is already running or if
   * sweep_step_hz_ is non-positive.
   */
  void startSweep();

  /**
   * @brief Halt an active frequency sweep.
   *
   * Stops the sweep timer and sets the sweeping flag to @c false.
   * The frequency remains at the last swept value.
   */
  void stopSweep();

  /**
   * @brief Query whether a frequency sweep is currently active.
   *
   * @return @c true while a sweep is in progress.
   */
  [[nodiscard]] bool isSweeping() const;

  /**
   * @brief Return the SCPI-style instrument identity string.
   *
   * Mimics the *IDN? query of a real SCPI instrument.
   *
   * @return A comma-separated identity string in the form
   *         "Manufacturer,Model,Serial,Version".
   */
  [[nodiscard]] QString instrumentIdentity() const;

  /**
   * @brief Enable or disable random error injection.
   *
   * When enabled, setFrequency() has a 10 % random chance of
   * failing with errorOccurred("Frequency lock failed") instead
   * of applying the requested value.
   *
   * @param enable @c true to enable, @c false to disable.
   */
  void setSimulateError(bool enable);

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
  /// Timer driving the frequency sweep steps.
  QTimer* sweep_timer_;
  /// Whether a frequency sweep is currently executing.
  bool is_sweeping_;
  /// Tracks the current frequency position during a sweep.
  double current_sweep_freq_;
  /// Whether error injection is enabled.
  bool simulate_error_;
  /// Pseudo-random engine for error injection.
  std::mt19937 rng_;
};

}  // namespace mwa::hardware
