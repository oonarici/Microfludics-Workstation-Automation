/**
 * @file mock_camera_controller.cpp
 * @brief Mock camera controller with synthetic microscopy frames.
 * @author MWA Team
 * @date 2026-04-02
 *
 * Implements the MockCameraController class defined in
 * mock_camera_controller.h. Generates 8-bit grayscale frames that
 * simulate darkfield microscopy with channel walls, drifting
 * particles, exposure/gain response, and Gaussian read noise.
 *
 * @copyright LGPL-3.0-or-later
 */

#include "hardware/camera/mock_camera_controller.h"

#include <QFont>
#include <QPainter>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include <random>

namespace mwa::hardware {

// ── Named constants ────────────────────────────────────────────────

/// Simulated connection delay in milliseconds.
static constexpr int kConnectDelayMs = 500;

/// Default sensor exposure time in milliseconds.
static constexpr double kDefaultExposureMs = 10.0;

/// Default analogue gain multiplier (unity).
static constexpr double kDefaultGain = 1.0;

/// Default ROI width in pixels.
static constexpr int kDefaultRoiWidth = 640;

/// Default ROI height in pixels.
static constexpr int kDefaultRoiHeight = 480;

/// Minimum capture interval (ms), caps frame rate at ~30 fps.
static constexpr int kMinIntervalMs = 33;

/// Darkfield background base pixel value (before scaling).
static constexpr double kBackgroundBase = 20.0;

/// Channel wall base pixel value (before scaling).
static constexpr double kWallBase = 200.0;

/// Channel wall thickness in pixels.
static constexpr int kWallThickness = 3;

/// Relative Y position of the top channel wall (fraction of height).
static constexpr double kTopWallFraction = 0.25;

/// Relative Y position of the bottom wall (fraction of height).
static constexpr double kBottomWallFraction = 0.75;

/// Reference exposure for "well-exposed" brightness (ms).
static constexpr double kReferenceExposureMs = 25.0;

/// Standard deviation base for read noise (multiplied by gain).
static constexpr double kNoiseBaseSigma = 3.0;

/// Leftward particle drift per frame in pixels.
static constexpr double kDriftPerFrame = 2.0;

/// Minimum number of particles per field of view.
static constexpr int kMinParticles = 5;

/// Maximum number of particles per field of view.
static constexpr int kMaxParticles = 15;

/// Minimum particle radius in pixels.
static constexpr double kMinRadius = 3.0;

/// Maximum particle radius in pixels.
static constexpr double kMaxRadius = 8.0;

/// Minimum particle peak brightness (before scaling).
static constexpr int kMinBrightness = 180;

/// Maximum particle peak brightness (before scaling).
static constexpr int kMaxBrightness = 255;

/// Frames between particle regeneration.
static constexpr int kRegenInterval = 30;

/// Font size for the frame counter overlay.
static constexpr int kOverlayFontSize = 10;

/// Pixel value floor.
static constexpr int kPixelMin = 0;

/// Pixel value ceiling.
static constexpr int kPixelMax = 255;

// ── Construction / destruction ─────────────────────────────────────

MockCameraController::MockCameraController(QObject* parent)
    : CameraControllerInterface(parent),
      state_(DeviceState::kDisconnected),
      exposure_(kDefaultExposureMs),
      gain_(kDefaultGain),
      roi_(0, 0, kDefaultRoiWidth, kDefaultRoiHeight),
      capture_timer_(new QTimer(this)),
      batch_remaining_(0),
      rng_(std::random_device{}()),
      frame_counter_(0),
      particle_drift_x_(0.0),
      frames_since_regen_(0) {
  connect(capture_timer_, &QTimer::timeout,
          this, &MockCameraController::onCaptureTimerTick);
  regenerateParticles();
}

MockCameraController::~MockCameraController() = default;

// ── DeviceInterface overrides ──────────────────────────────────────

void MockCameraController::connectDevice() {
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

void MockCameraController::disconnectDevice() {
  if (state_ == DeviceState::kDisconnected) {
    return;
  }
  capture_timer_->stop();
  batch_remaining_ = 0;
  state_ = DeviceState::kDisconnected;
  emit stateChanged(state_);
}

QString MockCameraController::deviceName() const {
  return QStringLiteral("Mock Camera Controller");
}

DeviceInterface::DeviceState MockCameraController::state() const {
  return state_;
}

bool MockCameraController::isConnected() const {
  return state_ == DeviceState::kConnected;
}

// ── CameraControllerInterface overrides ────────────────────────────

void MockCameraController::setExposure(double ms) {
  exposure_ = ms;
  emit exposureChanged(exposure_);

  // Update timer interval if continuous capture is active.
  if (capture_timer_->isActive() && batch_remaining_ == 0) {
    capture_timer_->setInterval(captureIntervalMs());
  }
}

double MockCameraController::exposure() const {
  return exposure_;
}

void MockCameraController::setGain(double gain) {
  gain_ = gain;
  emit gainChanged(gain_);
}

double MockCameraController::gain() const {
  return gain_;
}

void MockCameraController::setRoi(const QRect& roi) {
  roi_ = roi;
  regenerateParticles();
}

QRect MockCameraController::roi() const {
  return roi_;
}

void MockCameraController::grabSingle() {
  last_frame_ = generateMicroscopyFrame();
  emit frameReady(last_frame_);
}

void MockCameraController::startContinuousCapture() {
  if (capture_timer_->isActive()) {
    return;
  }
  batch_remaining_ = 0;
  capture_timer_->start(captureIntervalMs());
}

void MockCameraController::stopCapture() {
  capture_timer_->stop();
  batch_remaining_ = 0;
}

void MockCameraController::startBatchCapture(int count,
                                              int interval_ms) {
  if (capture_timer_->isActive()) {
    return;
  }
  batch_remaining_ = count;
  capture_timer_->start(interval_ms);
}

bool MockCameraController::isCapturing() const {
  return capture_timer_->isActive();
}

QImage MockCameraController::lastFrame() const {
  return last_frame_;
}

// ── Private helpers ────────────────────────────────────────────────

int MockCameraController::captureIntervalMs() const {
  return qMax(kMinIntervalMs,
              static_cast<int>(exposure_));
}

void MockCameraController::regenerateParticles() {
  const int w = roi_.width() > 0
                    ? roi_.width() : kDefaultRoiWidth;
  const int h = roi_.height() > 0
                    ? roi_.height() : kDefaultRoiHeight;

  const double top_wall =
      h * kTopWallFraction + kWallThickness;
  const double bottom_wall =
      h * kBottomWallFraction - kWallThickness;

  std::uniform_int_distribution<int> count_dist(
      kMinParticles, kMaxParticles);
  std::uniform_real_distribution<double> x_dist(0.0, w);
  std::uniform_real_distribution<double> y_dist(
      top_wall, bottom_wall);
  std::uniform_real_distribution<double> r_dist(
      kMinRadius, kMaxRadius);
  std::uniform_int_distribution<int> b_dist(
      kMinBrightness, kMaxBrightness);

  const int count = count_dist(rng_);
  particles_.clear();
  particles_.reserve(count);
  for (int i = 0; i < count; ++i) {
    particles_.append(Particle{
        x_dist(rng_), y_dist(rng_),
        r_dist(rng_), b_dist(rng_)});
  }

  particle_drift_x_ = 0.0;
  frames_since_regen_ = 0;
}

QImage MockCameraController::generateMicroscopyFrame() {
  const int w = roi_.width() > 0
                    ? roi_.width() : kDefaultRoiWidth;
  const int h = roi_.height() > 0
                    ? roi_.height() : kDefaultRoiHeight;

  // Exposure brightness scale: 25 ms is "well exposed".
  const double exposure_scale =
      exposure_ / kReferenceExposureMs;

  // Noise distribution: sigma scales with gain.
  std::normal_distribution<double> noise(
      0.0, kNoiseBaseSigma * gain_);

  // -- Regenerate particles periodically ──────────────────
  ++frames_since_regen_;
  if (frames_since_regen_ >= kRegenInterval) {
    regenerateParticles();
  }

  // Advance particle drift.
  particle_drift_x_ += kDriftPerFrame;

  // -- Allocate 8-bit grayscale image ─────────────────────
  QImage image(w, h, QImage::Format_Grayscale8);

  // Channel wall Y positions.
  const int top_y =
      static_cast<int>(h * kTopWallFraction);
  const int bot_y =
      static_cast<int>(h * kBottomWallFraction);

  // -- Fill pixels ────────────────────────────────────────
  for (int y = 0; y < h; ++y) {
    auto* row = image.scanLine(y);

    // Determine base value for this row.
    bool is_wall = false;
    if ((y >= top_y && y < top_y + kWallThickness) ||
        (y >= bot_y && y < bot_y + kWallThickness)) {
      is_wall = true;
    }

    const double row_base =
        is_wall ? kWallBase : kBackgroundBase;

    for (int x = 0; x < w; ++x) {
      double value = row_base;

      // Add particle contribution (soft circle falloff).
      if (!is_wall) {
        for (const auto& p : particles_) {
          double px =
              std::fmod(p.x - particle_drift_x_,
                        static_cast<double>(w));
          if (px < 0.0) {
            px += w;
          }
          const double dx = x - px;
          const double dy = y - p.y;
          const double dist_sq = dx * dx + dy * dy;
          const double r_sq = p.radius * p.radius;
          if (dist_sq < r_sq) {
            // Quadratic fall-off from centre.
            const double frac =
                1.0 - dist_sq / r_sq;
            value += p.brightness * frac;
          }
        }
      }

      // Apply exposure scaling.
      value *= exposure_scale;

      // Apply gain.
      value *= gain_;

      // Add read noise.
      value += noise(rng_);

      // Clamp to [0, 255].
      const int clamped = std::clamp(
          static_cast<int>(std::round(value)),
          kPixelMin, kPixelMax);
      row[x] = static_cast<uchar>(clamped);
    }
  }

  // -- Frame counter overlay ──────────────────────────────
  ++frame_counter_;

  // QPainter needs a non-indexed format for text rendering,
  // so we temporarily convert, paint, and convert back.
  QImage overlay =
      image.convertToFormat(QImage::Format_ARGB32);
  {
    QPainter painter(&overlay);
    QFont font;
    font.setPixelSize(kOverlayFontSize);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(
        4, kOverlayFontSize + 2,
        QStringLiteral("F: %1").arg(frame_counter_));
  }
  image =
      overlay.convertToFormat(QImage::Format_Grayscale8);

  return image;
}

void MockCameraController::onCaptureTimerTick() {
  last_frame_ = generateMicroscopyFrame();
  emit frameReady(last_frame_);

  if (batch_remaining_ > 0) {
    --batch_remaining_;
    if (batch_remaining_ == 0) {
      capture_timer_->stop();
      emit batchComplete();
    }
  }
}

}  // namespace mwa::hardware
