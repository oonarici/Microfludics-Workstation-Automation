/**
 * @file mock_network_analyzer_controller.cpp
 * @brief Mock network analyzer controller implementation.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Implements the MockNetworkAnalyzerController class defined in
 * mock_network_analyzer_controller.h.
 *
 * @copyright LGPL-3.0-or-later
 */

#include "hardware/network_analyzer/mock_network_analyzer_controller.h"

#include <QtMath>
#include <QTimer>

namespace mwa::hardware {

static constexpr int    kConnectDelayMs        = 500;
static constexpr double kDefaultStartFrequency = 1.0e6;   // 1 MHz
static constexpr double kDefaultStopFrequency  = 100.0e6; // 100 MHz
static constexpr int    kDefaultNumPoints      = 201;
static constexpr int    kMeasurementDelayMs    = 1000;

MockNetworkAnalyzerController::MockNetworkAnalyzerController(QObject* parent)
    : NetworkAnalyzerControllerInterface(parent),
      state_(DeviceState::kDisconnected),
      start_frequency_(kDefaultStartFrequency),
      stop_frequency_(kDefaultStopFrequency),
      num_points_(kDefaultNumPoints),
      is_measuring_(false) {}

MockNetworkAnalyzerController::~MockNetworkAnalyzerController() = default;

void MockNetworkAnalyzerController::connectDevice() {
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

void MockNetworkAnalyzerController::disconnectDevice() {
  if (state_ == DeviceState::kDisconnected) {
    return;
  }
  state_ = DeviceState::kDisconnected;
  emit stateChanged(state_);
}

QString MockNetworkAnalyzerController::deviceName() const {
  return QStringLiteral("Mock Network Analyzer Controller");
}

DeviceInterface::DeviceState MockNetworkAnalyzerController::state() const {
  return state_;
}

bool MockNetworkAnalyzerController::isConnected() const {
  return state_ == DeviceState::kConnected;
}

void MockNetworkAnalyzerController::setFrequencyRange(
    double start_hz, double stop_hz) {
  start_frequency_ = start_hz;
  stop_frequency_  = stop_hz;
  emit frequencyRangeChanged(start_frequency_, stop_frequency_);
}

double MockNetworkAnalyzerController::startFrequency() const {
  return start_frequency_;
}

double MockNetworkAnalyzerController::stopFrequency() const {
  return stop_frequency_;
}

void MockNetworkAnalyzerController::setNumPoints(int points) {
  num_points_ = points;
  emit numPointsChanged(num_points_);
}

int MockNetworkAnalyzerController::numPoints() const {
  return num_points_;
}

void MockNetworkAnalyzerController::measureSParameters() {
  if (is_measuring_) {
    return;
  }
  is_measuring_ = true;
  emit measurementStarted();

  QTimer::singleShot(kMeasurementDelayMs, this, [this]() {
    finishMeasurement();
  });
}

void MockNetworkAnalyzerController::finishMeasurement() {
  trace_frequencies_.clear();
  trace_magnitudes_.clear();

  const int    n     = num_points_;
  const double f_min = start_frequency_;
  const double f_max = stop_frequency_;
  const double span  = (n > 1) ? static_cast<double>(n - 1) : 1.0;
  const double step  = (f_max - f_min) / span;

  trace_frequencies_.reserve(n);
  trace_magnitudes_.reserve(n);

  for (int i = 0; i < n; ++i) {
    trace_frequencies_.append(f_min + i * step);
    // Simulate a simple resonance: sinusoidal dip around centre frequency.
    trace_magnitudes_.append(-20.0 * qSin(M_PI * (i / span)));
  }

  is_measuring_ = false;
  emit measurementComplete();
}

QVector<double> MockNetworkAnalyzerController::traceFrequencies() const {
  return trace_frequencies_;
}

QVector<double> MockNetworkAnalyzerController::traceMagnitudes() const {
  return trace_magnitudes_;
}

bool MockNetworkAnalyzerController::isMeasuring() const {
  return is_measuring_;
}

}  // namespace mwa::hardware
