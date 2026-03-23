/**
 * @file mock_camera_controller.h
 * @brief Mock implementation of the camera controller interface.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Provides a simulated camera controller that implements
 * CameraControllerInterface without requiring real hardware. Suitable for
 * GUI development and unit testing.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QImage>
#include <QRect>

#include "hardware/camera/camera_controller_interface.h"

namespace mwa::hardware {

/**
 * @class MockCameraController
 * @brief Simulated imaging camera controller for hardware-free development.
 *
 * Implements CameraControllerInterface with in-memory state. The connect()
 * operation uses a 500 ms QTimer delay to simulate real hardware latency.
 * Exposure defaults to 10.0 ms, gain to 1.0, and ROI to (0, 0, 640, 480).
 * grabSingle() generates a checkerboard test-pattern QImage. Continuous
 * capture uses a QTimer at approximately 30 fps.
 *
 * @see CameraControllerInterface
 */
class MockCameraController : public CameraControllerInterface {
  Q_OBJECT

 public:
  /**
   * @brief Construct a MockCameraController.
   *
   * @param parent Optional QObject parent for Qt ownership management.
   */
  explicit MockCameraController(QObject* parent = nullptr);

  /**
   * @brief Virtual destructor.
   */
  ~MockCameraController() override;

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
   * @return The string "Mock Camera Controller".
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

  // CameraControllerInterface overrides

  /**
   * @brief Set the sensor exposure time and emit exposureChanged().
   *
   * @param ms Exposure duration in milliseconds.
   */
  void setExposure(double ms) override;

  /**
   * @brief Return the current sensor exposure time.
   *
   * @return Exposure duration in milliseconds.
   */
  [[nodiscard]] double exposure() const override;

  /**
   * @brief Set the analogue gain and emit gainChanged().
   *
   * @param gain Gain multiplier (1.0 = unity gain).
   */
  void setGain(double gain) override;

  /**
   * @brief Return the current analogue gain.
   *
   * @return Gain multiplier.
   */
  [[nodiscard]] double gain() const override;

  /**
   * @brief Set the region of interest on the sensor.
   *
   * @param roi Rectangle defining the active sensor area in pixels.
   */
  void setRoi(const QRect& roi) override;

  /**
   * @brief Return the current region of interest.
   *
   * @return Rectangle defining the active sensor area in pixels.
   */
  [[nodiscard]] QRect roi() const override;

  /**
   * @brief Capture a single checkerboard test-pattern frame synchronously
   *        and emit frameReady().
   */
  void grabSingle() override;

  /**
   * @brief Start continuous frame capture at ~30 fps using a QTimer.
   *
   * Each tick generates a test-pattern frame and emits frameReady().
   */
  void startContinuousCapture() override;

  /**
   * @brief Stop any active capture mode.
   */
  void stopCapture() override;

  /**
   * @brief Start a batch capture sequence.
   *
   * Captures @p count frames separated by @p interval_ms milliseconds and
   * emits batchComplete() when done.
   *
   * @param count       Number of frames to capture.
   * @param interval_ms Interval between frames in milliseconds.
   */
  void startBatchCapture(int count, int interval_ms) override;

  /**
   * @brief Query whether any capture mode is currently active.
   *
   * @return @c true if a capture is in progress, @c false otherwise.
   */
  [[nodiscard]] bool isCapturing() const override;

  /**
   * @brief Return the most recently captured frame.
   *
   * @return The last QImage produced by the mock camera.
   */
  [[nodiscard]] QImage lastFrame() const override;

 private:
  /// Current connection state of the device.
  DeviceState state_;
  /// Current sensor exposure time in milliseconds.
  double exposure_;
  /// Current analogue gain multiplier.
  double gain_;
  /// Current region of interest.
  QRect roi_;
  /// Most recently produced frame.
  QImage last_frame_;
  /// Timer used for continuous and batch capture.
  QTimer* capture_timer_;
  /// Remaining frame count for batch capture (0 = continuous).
  int batch_remaining_;
  /// Cached checkerboard pattern, invalidated when ROI changes.
  mutable QImage cached_pattern_;

  /**
   * @brief Return a checkerboard test-pattern QImage matching the ROI size.
   *
   * Caches the pattern and only regenerates when the ROI dimensions change.
   *
   * @return A QImage with a 32x32 pixel checkerboard pattern.
   */
  QImage generateTestPattern() const;

  /**
   * @brief Slot called by capture_timer_ to produce each frame.
   */
  void onCaptureTimerTick();
};

}  // namespace mwa::hardware
