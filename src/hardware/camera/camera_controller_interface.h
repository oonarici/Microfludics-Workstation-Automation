/**
 * @file camera_controller_interface.h
 * @brief Abstract interface for imaging camera controllers.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Defines the pure abstract CameraControllerInterface that all camera
 * controller implementations must fulfil. Extends DeviceInterface with
 * camera-specific exposure, gain, ROI, and capture controls.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QImage>
#include <QRect>

#include "hardware/device_interface.h"

namespace mwa::hardware {

/**
 * @class CameraControllerInterface
 * @brief Pure abstract interface for imaging camera controllers.
 *
 * Extends DeviceInterface with camera-specific pure virtual methods for
 * controlling exposure time, analogue gain, region of interest, and both
 * single-frame and continuous/batch capture modes. All concrete camera
 * controller implementations must inherit this interface.
 *
 * @see DeviceInterface
 * @see MockCameraController
 */
class CameraControllerInterface : public DeviceInterface {
  Q_OBJECT

 public:
  /**
   * @brief Protected constructor — only concrete subclasses may call this.
   *
   * @param parent Optional QObject parent for Qt ownership management.
   */
  explicit CameraControllerInterface(QObject* parent = nullptr)
      : DeviceInterface(parent) {}

  /**
   * @brief Virtual destructor.
   */
  ~CameraControllerInterface() override = default;

  /**
   * @brief Return the category of this device.
   *
   * @return DeviceType::kCamera for all camera controller implementations.
   */
  [[nodiscard]] DeviceType deviceType() const override {
    return DeviceType::kCamera;
  }

  /**
   * @brief Set the sensor exposure time.
   *
   * @param ms Exposure duration in milliseconds.
   */
  virtual void setExposure(double ms) = 0;

  /**
   * @brief Return the current sensor exposure time.
   *
   * @return Exposure duration in milliseconds.
   */
  [[nodiscard]] virtual double exposure() const = 0;

  /**
   * @brief Set the analogue gain.
   *
   * @param gain Gain multiplier (1.0 = unity gain).
   */
  virtual void setGain(double gain) = 0;

  /**
   * @brief Return the current analogue gain.
   *
   * @return Gain multiplier.
   */
  [[nodiscard]] virtual double gain() const = 0;

  /**
   * @brief Set the region of interest on the sensor.
   *
   * @param roi Rectangle defining the active sensor area in pixels.
   */
  virtual void setRoi(const QRect& roi) = 0;

  /**
   * @brief Return the current region of interest.
   *
   * @return Rectangle defining the active sensor area in pixels.
   */
  [[nodiscard]] virtual QRect roi() const = 0;

  /**
   * @brief Asynchronously capture a single frame.
   *
   * Emits frameReady() with the captured QImage when the frame is available.
   */
  virtual void grabSingle() = 0;

  /**
   * @brief Start continuous frame capture at the maximum supported rate.
   *
   * Repeatedly emits frameReady() until stopCapture() is called.
   */
  virtual void startContinuousCapture() = 0;

  /**
   * @brief Stop any active single, continuous, or batch capture.
   */
  virtual void stopCapture() = 0;

  /**
   * @brief Start a batch capture of a fixed number of frames.
   *
   * Captures @p count frames separated by @p interval_ms milliseconds and
   * emits batchComplete() when all frames have been acquired.
   *
   * @param count       Number of frames to capture.
   * @param interval_ms Interval between frames in milliseconds.
   */
  virtual void startBatchCapture(int count, int interval_ms) = 0;

  /**
   * @brief Query whether any capture mode is currently active.
   *
   * @return @c true if a capture is in progress, @c false otherwise.
   */
  [[nodiscard]] virtual bool isCapturing() const = 0;

  /**
   * @brief Return the most recently captured frame.
   *
   * @return The last QImage produced by the camera, or a null QImage if no
   *         frame has been captured yet.
   */
  [[nodiscard]] virtual QImage lastFrame() const = 0;

 signals:
  /**
   * @brief Emitted when a new frame is available.
   *
   * @param frame The newly captured image.
   */
  void frameReady(const QImage& frame);

  /**
   * @brief Emitted when a batch capture sequence completes.
   */
  void batchComplete();

  /**
   * @brief Emitted when the exposure time changes.
   *
   * @param ms The new exposure duration in milliseconds.
   */
  void exposureChanged(double ms);

  /**
   * @brief Emitted when the analogue gain changes.
   *
   * @param gain The new gain multiplier.
   */
  void gainChanged(double gain);
};

}  // namespace mwa::hardware
