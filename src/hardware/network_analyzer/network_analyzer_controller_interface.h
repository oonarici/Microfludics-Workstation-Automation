/**
 * @file network_analyzer_controller_interface.h
 * @brief Abstract interface for network/impedance analyzer controllers.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Defines the pure abstract NetworkAnalyzerControllerInterface that all
 * network analyzer controller implementations must fulfil. Extends
 * DeviceInterface with S-parameter measurement and frequency sweep controls.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QVector>

#include "hardware/device_interface.h"

namespace mwa::hardware {

/**
 * @class NetworkAnalyzerControllerInterface
 * @brief Pure abstract interface for network/impedance analyzer controllers.
 *
 * Extends DeviceInterface with network-analyzer-specific pure virtual methods
 * for configuring frequency sweep ranges, number of measurement points, and
 * triggering asynchronous S-parameter measurements. All concrete network
 * analyzer implementations must inherit this interface.
 *
 * @see DeviceInterface
 * @see MockNetworkAnalyzerController
 */
class NetworkAnalyzerControllerInterface : public DeviceInterface {
  Q_OBJECT

 public:
  /**
   * @brief Constructor — forwards the QObject parent to DeviceInterface.
   *
   * @param parent Optional QObject parent for Qt ownership management.
   */
  explicit NetworkAnalyzerControllerInterface(QObject* parent = nullptr)
      : DeviceInterface(parent) {}

  /**
   * @brief Virtual destructor.
   */
  ~NetworkAnalyzerControllerInterface() override = default;

  /**
   * @brief Return the category of this device.
   *
   * @return DeviceType::kNetworkAnalyzer for all network analyzer
   *         implementations.
   */
  [[nodiscard]] DeviceType deviceType() const override {
    return DeviceType::kNetworkAnalyzer;
  }

  /**
   * @brief Set the frequency sweep range.
   *
   * @param start_hz  Start frequency in hertz.
   * @param stop_hz   Stop frequency in hertz.
   */
  virtual void setFrequencyRange(double start_hz, double stop_hz) = 0;

  /**
   * @brief Return the sweep start frequency.
   *
   * @return Start frequency in hertz.
   */
  [[nodiscard]] virtual double startFrequency() const = 0;

  /**
   * @brief Return the sweep stop frequency.
   *
   * @return Stop frequency in hertz.
   */
  [[nodiscard]] virtual double stopFrequency() const = 0;

  /**
   * @brief Set the number of measurement points in the sweep.
   *
   * @param points Number of evenly-spaced frequency points.
   */
  virtual void setNumPoints(int points) = 0;

  /**
   * @brief Return the number of measurement points in the sweep.
   *
   * @return Number of frequency points.
   */
  [[nodiscard]] virtual int numPoints() const = 0;

  /**
   * @brief Trigger an asynchronous S-parameter measurement.
   *
   * Emits measurementStarted() immediately and measurementComplete() when
   * the measurement finishes. Trace data can be retrieved via
   * traceFrequencies() and traceMagnitudes() after measurementComplete().
   */
  virtual void measureSParameters() = 0;

  /**
   * @brief Return the frequency axis of the most recent measurement trace.
   *
   * @return Vector of frequency values in hertz.
   */
  [[nodiscard]] virtual QVector<double> traceFrequencies() const = 0;

  /**
   * @brief Return the magnitude axis of the most recent measurement trace.
   *
   * @return Vector of S-parameter magnitudes in dB.
   */
  [[nodiscard]] virtual QVector<double> traceMagnitudes() const = 0;

  /**
   * @brief Query whether a measurement is currently in progress.
   *
   * @return @c true if a measurement is running, @c false otherwise.
   */
  [[nodiscard]] virtual bool isMeasuring() const = 0;

 signals:
  /**
   * @brief Emitted when a measurement run begins.
   */
  void measurementStarted();

  /**
   * @brief Emitted when a measurement run completes.
   */
  void measurementComplete();

  /**
   * @brief Emitted when the frequency sweep range changes.
   *
   * @param start  New start frequency in hertz.
   * @param stop   New stop frequency in hertz.
   */
  void frequencyRangeChanged(double start, double stop);

  /**
   * @brief Emitted when the number of measurement points changes.
   *
   * @param points The new number of frequency points.
   */
  void numPointsChanged(int points);
};

}  // namespace mwa::hardware
