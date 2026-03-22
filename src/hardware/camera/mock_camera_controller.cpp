/**
 * @file mock_camera_controller.cpp
 * @brief Mock camera controller implementation.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Implements the MockCameraController class defined in
 * mock_camera_controller.h.
 *
 * @copyright LGPL-3.0-or-later
 */

#include "hardware/camera/mock_camera_controller.h"

#include <QTimer>

namespace mwa::hardware {

static constexpr double kDefaultExposureMs    = 10.0;
static constexpr double kDefaultGain          = 1.0;
static constexpr int    kDefaultRoiWidth      = 640;
static constexpr int    kDefaultRoiHeight     = 480;
static constexpr int    kContinuousIntervalMs = 33;  // ~30 fps
static constexpr int    kCheckerSize          = 32;

MockCameraController::MockCameraController(QObject* parent)
    : CameraControllerInterface(parent),
      state_(DeviceState::kDisconnected),
      exposure_(kDefaultExposureMs),
      gain_(kDefaultGain),
      roi_(0, 0, kDefaultRoiWidth, kDefaultRoiHeight),
      is_capturing_(false),
      capture_timer_(new QTimer(this)),
      batch_remaining_(0) {
  connect(capture_timer_, &QTimer::timeout,
          this, &MockCameraController::onCaptureTimerTick);
}

MockCameraController::~MockCameraController() = default;

void MockCameraController::connectDevice() {
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

void MockCameraController::disconnectDevice() {
  if (state_ == DeviceState::kDisconnected) {
    return;
  }
  capture_timer_->stop();
  is_capturing_ = false;
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

void MockCameraController::setExposure(double ms) {
  exposure_ = ms;
  emit exposureChanged(exposure_);
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
}

QRect MockCameraController::roi() const {
  return roi_;
}

void MockCameraController::grabSingle() {
  last_frame_ = generateTestPattern();
  emit frameReady(last_frame_);
}

void MockCameraController::startContinuousCapture() {
  if (is_capturing_) {
    return;
  }
  is_capturing_    = true;
  batch_remaining_ = 0;
  capture_timer_->start(kContinuousIntervalMs);
}

void MockCameraController::stopCapture() {
  capture_timer_->stop();
  is_capturing_    = false;
  batch_remaining_ = 0;
}

void MockCameraController::startBatchCapture(int count, int interval_ms) {
  if (is_capturing_) {
    return;
  }
  is_capturing_    = true;
  batch_remaining_ = count;
  capture_timer_->start(interval_ms);
}

bool MockCameraController::isCapturing() const {
  return is_capturing_;
}

QImage MockCameraController::lastFrame() const {
  return last_frame_;
}

QImage MockCameraController::generateTestPattern() const {
  const int width  = roi_.width()  > 0 ? roi_.width()  : kDefaultRoiWidth;
  const int height = roi_.height() > 0 ? roi_.height() : kDefaultRoiHeight;

  QImage image(width, height, QImage::Format_RGB32);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const bool is_light =
          (((x / kCheckerSize) + (y / kCheckerSize)) % 2) == 0;
      image.setPixel(x, y, is_light ? 0xFFFFFFFF : 0xFF000000);
    }
  }

  return image;
}

void MockCameraController::onCaptureTimerTick() {
  last_frame_ = generateTestPattern();
  emit frameReady(last_frame_);

  if (batch_remaining_ > 0) {
    --batch_remaining_;
    if (batch_remaining_ == 0) {
      capture_timer_->stop();
      is_capturing_ = false;
      emit batchComplete();
    }
  }
}

}  // namespace mwa::hardware
