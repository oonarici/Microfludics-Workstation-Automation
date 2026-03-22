/**
 * @file mock_network_analyzer_controller.h
 * @brief Mock implementation of the network analyzer controller interface.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Provides a simulated network analyzer controller that implements
 * NetworkAnalyzerControllerInterface without requiring real hardware.
 * Suitable for GUI development and unit testing.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QVector>

#include "hardware/network_analyzer/network_analyzer_controller_interface.h"

namespace mwa::hardware {

/**
 * @class MockNetworkAnalyzerController
 * @brief Simulated network analyzer controller for hardware-free development.
 *
 * Implements NetworkAnalyzerControllerInterface with in-memory state. The
 * connect() operation uses a 500 ms QTimer delay to simulate real hardware
 * latency. Default frequency range is 1 MHz–100 MHz with 201 points.
 * measureSParameters() uses a 1 s QTimer to simulate measurement latency and
 * generates a fake sinusoidal trace.
 *
 * @see NetworkAnalyzerControllerInterface
 */
class MockNetworkAnalyzerController
    : public NetworkAnalyzerControllerInterface {
  Q_OBJECT

 public:
  /**
   * @brief Construct a MockNetworkAnalyzerController.
   *
   * @param parent Optional QObject parent for Qt ownership management.
   */
  explicit MockNetworkAnalyzerController(QObject* parent = nullptr);

  /**
   * @brief Virtual destructor.
   */
  ~MockNetworkAnalyzerController() override;

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
   * @return @c true if the device state is DeviceState::kConnected.
   */
  [[nodiscard]] bool isConnected() const override;

  // NetworkAnalyzerControllerInterface overrides

  /**
   * @brief Set the frequency sweep range and emit frequencyRangeChanged().
   *
   * @param start_hz  Start frequency in hertz.
   * @param stop_hz   Stop frequency in hertz.
   */
  void setFrequencyRange(double start_hz, double stop_hz) override;

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
   * @brief Set the number of measurement points and emit numPointsChanged().
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
   * Emits measurementStarted() immediately and after a 1 s simulated
   * delay generates a sinusoidal trace and emits measurementComplete().
   */
  void measureSParameters() override;

  /**
   * @brief Return the frequency axis of the most recent measurement trace.
   *
   * @return Vector of frequency values in hertz.
   */
  [[nodiscard]] QVector<double> traceFrequencies() const override;

  /**
   * @brief Return the magnitude axis of the most recent measurement trace.
   *
   * @return Vector of S-parameter magnitudes in dB.
   */
  [[nodiscard]] QVector<double> traceMagnitudes() const override;

  /**
   * @brief Query whether a measurement is currently in progress.
   *
   * @return @c true if a measurement is running, @c false otherwise.
   */
  [[nodiscard]] bool isMeasuring() const override;

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

  /**
   * @brief Generate simulated sinusoidal trace data and emit completion.
   *
   * Called internally after the simulated measurement delay.
   */
  void finishMeasurement();
};

}  // namespace mwa::hardware
