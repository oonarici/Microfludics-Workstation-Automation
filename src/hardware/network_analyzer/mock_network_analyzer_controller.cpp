/**
 * @file mock_network_analyzer_controller.cpp
 * @brief Mock network analyzer controller implementation.
 * @author MWA Team
 * @date 2026-04-02
 *
 * Implements the MockNetworkAnalyzerController class defined in
 * mock_network_analyzer_controller.h. Generates Lorentzian S-parameter
 * traces with configurable resonances, Gaussian noise, measurement
 * progress reporting, and error injection.
 *
 * @copyright LGPL-3.0-or-later
 */

#include "hardware/network_analyzer/mock_network_analyzer_controller.h"

#include <cmath>
#include <random>

#include <QTimer>

namespace mwa::hardware {

// -- Named constants -----------------------------------------------

/// Simulated connection latency in milliseconds.
static constexpr int kConnectDelayMs = 500;

/// Default sweep start frequency in hertz (1 MHz).
static constexpr double kDefaultStartFrequency = 1.0e6;

/// Default sweep stop frequency in hertz (100 MHz).
static constexpr double kDefaultStopFrequency = 100.0e6;

/// Default number of measurement points.
static constexpr int kDefaultNumPoints = 201;

/// Milliseconds of simulated acquisition time per point.
static constexpr int kMsPerPoint = 5;

/// Minimum simulated measurement time in milliseconds.
static constexpr int kMinMeasurementMs = 500;

/// Maximum simulated measurement time in milliseconds.
static constexpr int kMaxMeasurementMs = 10000;

/// Default off-resonance background level in dB.
static constexpr double kDefaultBackgroundDb = -8.0;

/// Default Gaussian noise standard deviation in dB.
static constexpr double kDefaultNoiseStdDevDb = 0.5;

/// Primary PZT resonance centre frequency in hertz.
static constexpr double kPrimaryResonanceHz = 3.2e6;

/// Primary PZT resonance depth in dB.
static constexpr double kPrimaryDepthDb = 35.0;

/// Primary PZT resonance 3 dB bandwidth in hertz.
static constexpr double kPrimaryBandwidthHz = 50.0e3;

/// Secondary PZT resonance centre frequency in hertz.
static constexpr double kSecondaryResonanceHz = 6.5e6;

/// Secondary PZT resonance depth in dB.
static constexpr double kSecondaryDepthDb = 18.0;

/// Secondary PZT resonance 3 dB bandwidth in hertz.
static constexpr double kSecondaryBandwidthHz = 80.0e3;

/// Number of progress increments during a measurement (10 %).
static constexpr int kProgressSteps = 10;

/// Error injection period: 1 in every N measurements fails.
static constexpr int kErrorInjectionPeriod = 5;

// -- Constructor / Destructor --------------------------------------

MockNetworkAnalyzerController::MockNetworkAnalyzerController(
    QObject* parent)
    : NetworkAnalyzerControllerInterface(parent),
      state_(DeviceState::kDisconnected),
      start_frequency_(kDefaultStartFrequency),
      stop_frequency_(kDefaultStopFrequency),
      num_points_(kDefaultNumPoints),
      is_measuring_(false),
      background_level_db_(kDefaultBackgroundDb),
      noise_std_dev_db_(kDefaultNoiseStdDevDb),
      simulate_error_(false),
      measurement_count_(0),
      progress_timer_(new QTimer(this)),
      progress_percent_(0),
      rng_(std::random_device{}()) {
  // Default resonances: PZT transducer characterisation.
  resonances_.append(
      {kPrimaryResonanceHz, kPrimaryDepthDb,
       kPrimaryBandwidthHz});
  resonances_.append(
      {kSecondaryResonanceHz, kSecondaryDepthDb,
       kSecondaryBandwidthHz});
}

MockNetworkAnalyzerController::~MockNetworkAnalyzerController() =
    default;

// -- DeviceInterface overrides -------------------------------------

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

DeviceInterface::DeviceState
MockNetworkAnalyzerController::state() const {
  return state_;
}

bool MockNetworkAnalyzerController::isConnected() const {
  return state_ == DeviceState::kConnected;
}

// -- NetworkAnalyzerControllerInterface overrides -------------------

void MockNetworkAnalyzerController::setFrequencyRange(
    double start_hz, double stop_hz) {
  start_frequency_ = start_hz;
  stop_frequency_ = stop_hz;
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
  ++measurement_count_;
  emit measurementStarted();

  // Scaled measurement time: 5 ms per point, clamped.
  const int measurement_ms = qBound(
      kMinMeasurementMs, num_points_ * kMsPerPoint,
      kMaxMeasurementMs);

  // Start progress reporting at 10 % intervals.
  progress_percent_ = 0;
  const int progress_interval_ms =
      measurement_ms / kProgressSteps;
  progress_timer_->start(progress_interval_ms);

  connect(progress_timer_, &QTimer::timeout, this, [this]() {
    progress_percent_ += kProgressSteps;  // +10 each tick
    if (progress_percent_ >= 100) {
      progress_timer_->stop();
      progress_percent_ = 100;
    }
    emit measurementProgress(progress_percent_);
  });

  // Schedule measurement completion.
  QTimer::singleShot(measurement_ms, this, [this]() {
    // Stop progress timer if still running.
    progress_timer_->stop();
    disconnect(progress_timer_, &QTimer::timeout,
               this, nullptr);

    // Error injection: 1 in kErrorInjectionPeriod fails.
    if (simulate_error_ &&
        (measurement_count_ % kErrorInjectionPeriod == 0)) {
      is_measuring_ = false;
      emit errorOccurred(QStringLiteral(
          "Measurement timeout: "
          "instrument not responding"));
      return;
    }

    finishMeasurement();
  });
}

void MockNetworkAnalyzerController::finishMeasurement() {
  trace_frequencies_.clear();
  trace_magnitudes_.clear();

  const int n = num_points_;
  const double f_min = start_frequency_;
  const double f_max = stop_frequency_;
  const double span =
      (n > 1) ? static_cast<double>(n - 1) : 1.0;
  const double step = (f_max - f_min) / span;

  trace_frequencies_.reserve(n);
  trace_magnitudes_.reserve(n);

  // Gaussian noise generator.
  std::normal_distribution<double> noise_dist(
      0.0, noise_std_dev_db_);

  for (int i = 0; i < n; ++i) {
    const double f = f_min + i * step;
    trace_frequencies_.append(f);

    // Lorentzian model: S(f) = background - sum of dips.
    double magnitude = background_level_db_;
    for (const auto& r : resonances_) {
      const double half_bw = r.bandwidth_hz / 2.0;
      const double delta = f - r.freq_hz;
      const double denom =
          1.0 + (delta / half_bw) * (delta / half_bw);
      magnitude -= r.depth_db / denom;
    }

    // Add Gaussian noise (skip if std dev is zero).
    if (noise_std_dev_db_ > 0.0) {
      magnitude += noise_dist(rng_);
    }

    trace_magnitudes_.append(magnitude);
  }

  is_measuring_ = false;
  emit measurementComplete();
}

QVector<double>
MockNetworkAnalyzerController::traceFrequencies() const {
  return trace_frequencies_;
}

QVector<double>
MockNetworkAnalyzerController::traceMagnitudes() const {
  return trace_magnitudes_;
}

bool MockNetworkAnalyzerController::isMeasuring() const {
  return is_measuring_;
}

// -- Resonance model configuration ---------------------------------

void MockNetworkAnalyzerController::setResonances(
    const QVector<Resonance>& resonances) {
  resonances_ = resonances;
}

QVector<Resonance>
MockNetworkAnalyzerController::resonances() const {
  return resonances_;
}

void MockNetworkAnalyzerController::setBackgroundLevel(double db) {
  background_level_db_ = db;
}

void MockNetworkAnalyzerController::setNoiseStdDev(double db) {
  noise_std_dev_db_ = db;
}

// -- Error injection -----------------------------------------------

void MockNetworkAnalyzerController::setSimulateError(bool enable) {
  simulate_error_ = enable;
  if (enable) {
    measurement_count_ = 0;
  }
}

}  // namespace mwa::hardware
