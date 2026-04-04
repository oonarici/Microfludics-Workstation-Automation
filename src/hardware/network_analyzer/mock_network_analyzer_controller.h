/**
 * @file mock_network_analyzer_controller.h
 * @brief Mock implementation of the network analyzer controller interface.
 * @author MWA Team
 * @date 2026-04-02
 *
 * Provides a simulated network analyzer controller that implements
 * NetworkAnalyzerControllerInterface without requiring real hardware.
 * Generates physically realistic Lorentzian S-parameter traces with
 * configurable resonances, noise, and error injection.
 * Suitable for GUI development and unit testing.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <random>

#include <QTimer>
#include <QVector>

#include "hardware/network_analyzer/network_analyzer_controller_interface.h"

namespace mwa::hardware {

/**
 * @struct Resonance
 * @brief Describes a single Lorentzian resonance dip in an S-parameter
 *        trace.
 *
 * Used by MockNetworkAnalyzerController to build realistic simulated
 * measurement data. Each resonance represents a PZT transducer mode
 * or other acoustic feature visible in the reflection coefficient.
 */
struct Resonance {
  double freq_hz;       ///< Center frequency of the resonance in Hz.
  double depth_db;      ///< Depth of the resonance dip in dB.
  double bandwidth_hz;  ///< 3 dB bandwidth of the resonance in Hz.
};

/**
 * @class MockNetworkAnalyzerController
 * @brief Simulated network analyzer controller for hardware-free
 *        development.
 *
 * Implements NetworkAnalyzerControllerInterface with in-memory state.
 * The connect() operation uses a 500 ms QTimer delay to simulate real
 * hardware latency. Default frequency range is 1 MHz -- 100 MHz with
 * 201 points.
 *
 * measureSParameters() uses a scaled QTimer delay (5 ms per point,
 * clamped to 500 -- 10 000 ms) and generates a realistic Lorentzian
 * trace composed of configurable resonance dips over a flat
 * background, with additive Gaussian noise.
 *
 * Features:
 * - Configurable resonance model (setResonances())
 * - Adjustable background level and noise amplitude
 * - Measurement progress reporting (measurementProgress signal)
 * - Deterministic error injection for testing error-handling paths
 *
 * @see NetworkAnalyzerControllerInterface
 * @see Resonance
 */
class MockNetworkAnalyzerController
    : public NetworkAnalyzerControllerInterface {
  Q_OBJECT

 public:
  /**
   * @brief Construct a MockNetworkAnalyzerController.
   *
   * Initialises default resonances representing a typical PZT
   * transducer characterisation (primary 3.2 MHz, secondary
   * 6.5 MHz).
   *
   * @param parent Optional QObject parent for Qt ownership
   *               management.
   */
  explicit MockNetworkAnalyzerController(
      QObject* parent = nullptr);

  /**
   * @brief Virtual destructor.
   */
  ~MockNetworkAnalyzerController() override;

  // -- DeviceInterface overrides -----------------------------------

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
   * Transitions state to DeviceState::kDisconnected and emits
   * stateChanged().
   */
  void disconnectDevice() override;

  /**
   * @brief Return the display name for this mock device.
   *
   * @return The string "Mock Network Analyzer Controller".
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
   * @return @c true if the device state is
   *         DeviceState::kConnected.
   */
  [[nodiscard]] bool isConnected() const override;

  // -- NetworkAnalyzerControllerInterface overrides -----------------

  /**
   * @brief Set the frequency sweep range and emit
   *        frequencyRangeChanged().
   *
   * @param start_hz Start frequency in hertz.
   * @param stop_hz  Stop frequency in hertz.
   */
  void setFrequencyRange(double start_hz,
                         double stop_hz) override;

  /**
   * @brief Return the sweep start frequency.
   *
   * @return Start frequency in hertz.
   */
  [[nodiscard]] double startFrequency() const override;

  /**
   * @brief Return the sweep stop frequency.
   *
   * @return Stop frequency in hertz.
   */
  [[nodiscard]] double stopFrequency() const override;

  /**
   * @brief Set the number of measurement points and emit
   *        numPointsChanged().
   *
   * @param points Number of evenly-spaced frequency points.
   */
  void setNumPoints(int points) override;

  /**
   * @brief Return the number of measurement points in the sweep.
   *
   * @return Number of frequency points.
   */
  [[nodiscard]] int numPoints() const override;

  /**
   * @brief Trigger an asynchronous S-parameter measurement.
   *
   * Emits measurementStarted() immediately and uses a scaled
   * timer (5 ms per point, clamped to 500 -- 10 000 ms) to
   * simulate instrument acquisition time. Generates a Lorentzian
   * resonance trace and emits measurementComplete(). Emits
   * measurementProgress() at every 10 % increment during the
   * measurement.
   *
   * If error injection is enabled, 1 in 5 measurements will fail
   * with an errorOccurred() signal instead of completing.
   */
  void measureSParameters() override;

  /**
   * @brief Return the frequency axis of the most recent
   *        measurement trace.
   *
   * @return Vector of frequency values in hertz.
   */
  [[nodiscard]] QVector<double> traceFrequencies() const override;

  /**
   * @brief Return the magnitude axis of the most recent
   *        measurement trace.
   *
   * @return Vector of S-parameter magnitudes in dB.
   */
  [[nodiscard]] QVector<double> traceMagnitudes() const override;

  /**
   * @brief Query whether a measurement is currently in progress.
   *
   * @return @c true if a measurement is running, @c false
   *         otherwise.
   */
  [[nodiscard]] bool isMeasuring() const override;

  // -- Resonance model configuration --------------------------------

  /**
   * @brief Replace the resonance model with the given list.
   *
   * @param resonances Vector of Resonance descriptors. An empty
   *        vector yields a flat background-only trace.
   */
  void setResonances(const QVector<Resonance>& resonances);

  /**
   * @brief Return the current resonance model.
   *
   * @return Vector of Resonance descriptors.
   */
  [[nodiscard]] QVector<Resonance> resonances() const;

  /**
   * @brief Set the off-resonance background level.
   *
   * @param db Background level in dB (default -8.0).
   */
  void setBackgroundLevel(double db);

  /**
   * @brief Set the standard deviation of additive Gaussian noise.
   *
   * @param db Noise standard deviation in dB (default 0.5).
   *           Set to 0.0 for deterministic traces.
   */
  void setNoiseStdDev(double db);

  // -- Error injection ----------------------------------------------

  /**
   * @brief Enable or disable simulated measurement errors.
   *
   * When enabled, every 5th measurement (starting from the first
   * after enabling) will fail and emit errorOccurred() with a
   * timeout message instead of producing trace data.
   *
   * @param enable @c true to activate error injection.
   */
  void setSimulateError(bool enable);

 signals:
  /**
   * @brief Emitted periodically during a measurement to report
   *        progress.
   *
   * @param percent Completion percentage (0 -- 100).
   */
  void measurementProgress(int percent);

 private:
  /// Current connection state of the device.
  DeviceState state_;
  /// Sweep start frequency in hertz.
  double start_frequency_;
  /// Sweep stop frequency in hertz.
  double stop_frequency_;
  /// Number of measurement points.
  int num_points_;
  /// Whether a measurement is currently in progress.
  bool is_measuring_;
  /// Frequency axis of the last completed measurement.
  QVector<double> trace_frequencies_;
  /// Magnitude axis of the last completed measurement in dB.
  QVector<double> trace_magnitudes_;

  /// Lorentzian resonance descriptors for trace generation.
  QVector<Resonance> resonances_;
  /// Off-resonance reflection level in dB.
  double background_level_db_;
  /// Standard deviation of additive Gaussian noise in dB.
  double noise_std_dev_db_;
  /// Whether simulated measurement errors are enabled.
  bool simulate_error_;
  /// Total measurements triggered (for error injection cycling).
  int measurement_count_;
  /// Timer that fires periodically during a measurement to emit
  /// progress updates.
  QTimer* progress_timer_;
  /// Current progress percentage (0 -- 100) during a measurement.
  int progress_percent_;

  /// Mersenne Twister PRNG for Gaussian noise generation.
  std::mt19937 rng_;

  /**
   * @brief Generate a Lorentzian resonance trace and emit
   *        measurementComplete().
   *
   * Called internally after the simulated measurement delay.
   * Computes S(f) = background - sum of Lorentzian dips, then
   * adds Gaussian noise to each point.
   */
  void finishMeasurement();
};

}  // namespace mwa::hardware
