/**
 * @file basler_camera_controller.h
 * @brief Hardware camera driver skeleton for Basler Pylon cameras.
 * @author onuronarici
 * @date 2026-04-05
 *
 * Declares the BaslerCameraController class, which drives Basler
 * industrial cameras (ace, dart, pulse series) via the Pylon C++ SDK.
 * Image acquisition uses the Pylon instant-camera grab engine with
 * software-triggered batch mode and free-running continuous mode.
 * All camera operations are serialised through a CommandQueue running
 * on a dedicated worker thread to keep the GUI responsive.
 *
 * This is a **header-only skeleton** -- the .cpp implementation will
 * be written when physical hardware becomes available.  The header is
 * fully documented so that the implementation can be written directly
 * from the Doxygen comments and the protocol specification in
 * `docs/protocols/basler_pylon_spec.md`.
 *
 * @note This header is not currently compiled via CMake.  When the
 *       .cpp implementation is written, both files will be added to a
 *       `if(Pylon_FOUND)` conditional block in
 *       `src/hardware/CMakeLists.txt`.
 *
 * @note The GUI always works against CameraControllerInterface*.
 *       On platforms without hardware, MockCameraController is used
 *       instead.
 *
 * @see CameraControllerInterface
 * @see CommandQueue
 * @see MockCameraController
 * @see docs/protocols/basler_pylon_spec.md
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QImage>
#include <QMutex>
#include <QRect>
#include <QString>

#include <memory>

#include "hardware/camera/camera_controller_interface.h"
#include "hardware/command_queue.h"

// Forward declarations of Pylon SDK types -- full headers included in
// the .cpp only.  This avoids requiring the Pylon SDK to be installed
// just to parse this header in IDE / static-analysis tools.
namespace Pylon {
class CBaslerUniversalInstantCamera;
class CImageFormatConverter;
}  // namespace Pylon

namespace mwa::hardware {

// ── Constants ────────────────────────────────────────────────────

/// Default sensor exposure time in milliseconds.
inline constexpr double kCameraDefaultExposureMs = 10.0;

/// Default analogue gain multiplier (1.0 = unity / 0 dB).
inline constexpr double kCameraDefaultGain = 1.0;

/// Default region of interest width in pixels.
inline constexpr int kCameraDefaultRoiWidth = 640;

/// Default region of interest height in pixels.
inline constexpr int kCameraDefaultRoiHeight = 480;

/// Timeout for the Pylon connect / open sequence in milliseconds.
inline constexpr int kCameraConnectTimeoutMs = 10000;

/// Timeout for a single GrabOne or RetrieveResult call in
/// milliseconds.
inline constexpr int kCameraGrabTimeoutMs = 5000;

/// Minimum capture interval in milliseconds (~30 fps cap).
inline constexpr int kCameraMinCaptureIntervalMs = 33;

/**
 * @class BaslerCameraController
 * @brief Hardware camera controller using the Basler Pylon SDK.
 *
 * Implements CameraControllerInterface for real Basler industrial
 * cameras.  All camera operations (parameter changes, frame grabs)
 * are submitted to an internal CommandQueue so that only one Pylon
 * call is in flight at a time and the GUI thread is never blocked.
 *
 * ### Unit conversions
 *
 * The CameraControllerInterface uses **milliseconds** for exposure
 * and a **linear multiplier** for gain, while the Pylon GenICam
 * nodes use **microseconds** and **decibels** respectively:
 *
 * | Quantity | MWA unit         | Pylon unit        | Conversion              |
 * |----------|------------------|-------------------|-------------------------|
 * | Exposure | milliseconds     | microseconds      | `us = ms * 1000`        |
 * | Gain     | linear multiplier| decibels (dB)     | `dB = 20 * log10(mult)` |
 *
 * @code
 *   // Exposure: MWA ms -> Pylon us
 *   double exposure_us = exposure_ms * 1000.0;
 *   double exposure_ms = exposure_us / 1000.0;
 *
 *   // Gain: MWA multiplier -> Pylon dB
 *   double gain_db = 20.0 * std::log10(gain_multiplier);
 *   double gain_multiplier = std::pow(10.0, gain_db / 20.0);
 * @endcode
 *
 * ### Typical usage
 * @code
 *   auto cam = std::make_unique<BaslerCameraController>();
 *   cam->setSerialNumber("12345678");    // optional filter
 *   cam->connectDevice();                // async
 *   // ... after stateChanged(kConnected) ...
 *   cam->setExposure(5.0);              // 5 ms
 *   cam->setGain(2.0);                  // 6.02 dB
 *   cam->setRoi(QRect(100, 100, 320, 240));
 *   cam->grabSingle();                  // async -- emits frameReady()
 *   cam->startContinuousCapture();      // live preview
 *   cam->stopCapture();
 * @endcode
 *
 * ### Connection workflow (enqueued on CommandQueue)
 * 1. Call `Pylon::PylonInitialize()` if not already initialised
 *    (reference-counted; the app calls this once at startup)
 * 2. Obtain `Pylon::CTlFactory::GetInstance()`
 * 3. If a serial number was set via setSerialNumber():
 *    - Create a `CDeviceInfo` filter with the serial number
 *    - `factory.CreateDevice(info)` to open that specific camera
 *    Otherwise:
 *    - `factory.CreateFirstDevice()` to open the first camera found
 * 4. Construct `CBaslerUniversalInstantCamera` around the device
 * 5. `camera_->Open()` -- opens the GenICam transport
 * 6. Read sensor dimensions from `SensorWidth` / `SensorHeight`
 *    and cache as max_sensor_width_ / max_sensor_height_
 * 7. Set pixel format to Mono8:
 *    `camera_->PixelFormat.SetValue(PixelFormat_Mono8)`
 * 8. Apply cached exposure (converted to microseconds):
 *    `camera_->ExposureTime.SetValue(exposure_ms_ * 1000.0)`
 * 9. Apply cached gain (converted to decibels):
 *    `camera_->Gain.SetValue(20.0 * log10(gain_))`
 * 10. Apply cached ROI (reset offsets first, then set dimensions):
 *     `camera_->OffsetX.SetValue(0); camera_->OffsetY.SetValue(0);`
 *     `camera_->Width.SetValue(roi_.width());`
 *     `camera_->Height.SetValue(roi_.height());`
 *     `camera_->OffsetX.SetValue(roi_.x());`
 *     `camera_->OffsetY.SetValue(roi_.y());`
 * 11. Create the ImageFormatConverter for Mono8 -> QImage conversion
 * 12. Transition to DeviceState::kConnected and emit stateChanged()
 *
 * If any step fails (Pylon::GenericException), the state transitions
 * to kError and errorOccurred() is emitted with the exception
 * description.
 *
 * ### Disconnection workflow
 * 1. Stop any active capture via stopCapture()
 * 2. `camera_->Close()` -- close the GenICam transport
 * 3. Destroy camera_ and converter_ smart pointers
 * 4. Transition to DeviceState::kDisconnected
 *
 * ### Single-frame grab workflow (grabSingle)
 * 1. Enqueue on CommandQueue:
 * 2. `camera_->GrabOne(kCameraGrabTimeoutMs, grab_result)`
 * 3. If `grab_result->GrabSucceeded()`:
 *    - Convert buffer to QImage via pylonToQImage()
 *    - Cache as last_frame_ under mutex_
 *    - Emit frameReady(last_frame_)
 * 4. Otherwise emit errorOccurred() with the grab error description
 *
 * ### Continuous capture workflow (startContinuousCapture)
 * 1. Set is_capturing_ = true
 * 2. `camera_->StartGrabbing(GrabStrategy_LatestImageOnly)`
 * 3. Loop on worker thread:
 *    - `camera_->RetrieveResult(kCameraGrabTimeoutMs, grab_result)`
 *    - Convert to QImage, cache, emit frameReady()
 *    - Check is_capturing_ flag; break if false
 * 4. `camera_->StopGrabbing()` when loop exits
 *
 * ### Batch capture workflow (startBatchCapture)
 * 1. Set is_capturing_ = true
 * 2. Enable software trigger:
 *    `camera_->TriggerMode.SetValue(TriggerMode_On)`
 *    `camera_->TriggerSource.SetValue(TriggerSource_Software)`
 * 3. `camera_->StartGrabbing(count, GrabStrategy_OneByOne)`
 * 4. For each frame (i = 0 .. count-1):
 *    a. `camera_->WaitForFrameTriggerReady(kCameraGrabTimeoutMs)`
 *    b. `camera_->ExecuteSoftwareTrigger()`
 *    c. `camera_->RetrieveResult(kCameraGrabTimeoutMs, grab_result)`
 *    d. Convert to QImage, cache, emit frameReady()
 *    e. If not last frame: `QThread::msleep(interval_ms)`
 * 5. Restore free-run mode:
 *    `camera_->TriggerMode.SetValue(TriggerMode_Off)`
 * 6. `camera_->StopGrabbing()`
 * 7. Set is_capturing_ = false
 * 8. Emit batchComplete()
 *
 * @note Non-copyable, non-movable.
 *
 * @see CameraControllerInterface
 * @see CommandQueue
 * @see docs/protocols/basler_pylon_spec.md
 */
class BaslerCameraController : public CameraControllerInterface {
  Q_OBJECT

 public:
  /**
   * @brief Construct a BaslerCameraController.
   *
   * The controller starts in DeviceState::kDisconnected with default
   * parameters: 10 ms exposure, 1.0x gain, 640x480 ROI at origin.
   * Call setSerialNumber() to target a specific camera before
   * connectDevice().
   *
   * @param parent Optional QObject parent for Qt ownership management.
   */
  explicit BaslerCameraController(QObject* parent = nullptr);

  /**
   * @brief Destructor -- disconnects the camera and shuts down the
   *        command queue.
   */
  ~BaslerCameraController() override;

  // Non-copyable, non-movable.
  BaslerCameraController(const BaslerCameraController&) = delete;
  BaslerCameraController& operator=(
      const BaslerCameraController&) = delete;
  BaslerCameraController(BaslerCameraController&&) = delete;
  BaslerCameraController& operator=(
      BaslerCameraController&&) = delete;

  // ── Configuration (call before connectDevice) ────────────────

  /**
   * @brief Set the camera serial number to open a specific device.
   *
   * If not called, connectDevice() will open the first available
   * Basler camera.  The serial number is matched against the
   * `CDeviceInfo::GetSerialNumber()` field during device enumeration.
   *
   * @warning Not thread-safe -- call only while disconnected.
   * @param serial The camera serial number string.
   */
  void setSerialNumber(const QString& serial);

  /**
   * @brief Return the currently configured camera serial number.
   *
   * @return The serial number filter, or an empty string if none
   *         was set (open-first-available mode).
   */
  [[nodiscard]] QString serialNumber() const;

  // ── DeviceInterface overrides ────────────────────────────────

  /**
   * @brief Open the Pylon transport and initialise the camera.
   *
   * Transitions immediately to DeviceState::kConnecting, then
   * enqueues the full connection sequence on the CommandQueue
   * (see class-level documentation for the 12-step workflow).
   *
   * All Pylon calls are wrapped in a try/catch for
   * `Pylon::GenericException`.  On failure the state transitions
   * to kError and errorOccurred() is emitted.
   *
   * @pre Pylon SDK must be globally initialised
   *      (PylonInitialize() called at app startup).
   */
  void connectDevice() override;

  /**
   * @brief Close the camera connection.
   *
   * Enqueues the shutdown sequence on the CommandQueue:
   * 1. stopCapture() if capturing
   * 2. camera_->Close()
   * 3. Destroy camera_ and converter_
   * 4. Transition to kDisconnected and emit stateChanged()
   */
  void disconnectDevice() override;

  /**
   * @brief Return the display name for this device.
   *
   * @return A string in the form "Basler Camera (<serial>)" if a
   *         serial number was configured, or "Basler Camera" if
   *         using first-available mode.
   */
  [[nodiscard]] QString deviceName() const override;

  /**
   * @brief Return the current connection state (thread-safe).
   *
   * @return The current DeviceState value, read under a mutex.
   */
  [[nodiscard]] DeviceState state() const override;

  /**
   * @brief Query whether the camera is fully connected (thread-safe).
   *
   * @return @c true if the device state is DeviceState::kConnected.
   */
  [[nodiscard]] bool isConnected() const override;

  // ── CameraControllerInterface overrides ──────────────────────

  /**
   * @brief Set the sensor exposure time via the Pylon SDK.
   *
   * Converts from MWA milliseconds to Pylon microseconds
   * (`us = ms * 1000.0`) and enqueues on the CommandQueue:
   *
   * 1. `camera_->ExposureTime.SetValue(exposure_us)`
   * 2. Read back: `camera_->ExposureTime.GetValue()` (the camera
   *    may clamp to the nearest supported value)
   * 3. Convert back to ms and cache under mutex_
   * 4. Emit exposureChanged(ms)
   *
   * @param ms Exposure duration in milliseconds.
   *
   * @pre Device must be connected.
   */
  void setExposure(double ms) override;

  /**
   * @brief Return the cached sensor exposure time (thread-safe).
   *
   * @return Exposure duration in milliseconds.
   */
  [[nodiscard]] double exposure() const override;

  /**
   * @brief Set the analogue gain via the Pylon SDK.
   *
   * Converts from MWA linear multiplier to Pylon decibels
   * (`dB = 20 * log10(multiplier)`) and enqueues on the
   * CommandQueue:
   *
   * 1. `camera_->Gain.SetValue(gain_db)`
   * 2. Read back: `camera_->Gain.GetValue()` (camera may clamp)
   * 3. Convert back to multiplier and cache under mutex_
   * 4. Emit gainChanged(multiplier)
   *
   * @param gain Gain multiplier (1.0 = unity / 0 dB).
   *
   * @pre Device must be connected.
   */
  void setGain(double gain) override;

  /**
   * @brief Return the cached analogue gain (thread-safe).
   *
   * @return Gain multiplier.
   */
  [[nodiscard]] double gain() const override;

  /**
   * @brief Set the region of interest via the Pylon SDK.
   *
   * ROI changes require a specific order to avoid constraint
   * violations (`OffsetX + Width <= SensorWidth`).  Enqueues
   * on the CommandQueue:
   *
   * 1. Reset offsets to zero:
   *    `camera_->OffsetX.SetValue(0)`
   *    `camera_->OffsetY.SetValue(0)`
   * 2. Set dimensions:
   *    `camera_->Width.SetValue(roi.width())`
   *    `camera_->Height.SetValue(roi.height())`
   * 3. Set offsets:
   *    `camera_->OffsetX.SetValue(roi.x())`
   *    `camera_->OffsetY.SetValue(roi.y())`
   * 4. Read back all four parameters and construct the actual
   *    QRect (camera may quantise to alignment requirements)
   * 5. Cache under mutex_
   *
   * @param roi Rectangle defining the active sensor area in pixels.
   *
   * @pre Device must be connected.
   * @pre roi must satisfy: `roi.x() + roi.width() <= SensorWidth`
   *      and `roi.y() + roi.height() <= SensorHeight`.
   */
  void setRoi(const QRect& roi) override;

  /**
   * @brief Return the cached region of interest (thread-safe).
   *
   * @return Rectangle defining the active sensor area in pixels.
   */
  [[nodiscard]] QRect roi() const override;

  /**
   * @brief Capture a single frame asynchronously.
   *
   * Enqueues on the CommandQueue:
   * 1. `camera_->GrabOne(kCameraGrabTimeoutMs, grab_result)`
   * 2. If succeeded: convert via pylonToQImage(), cache, emit
   *    frameReady()
   * 3. If failed: emit errorOccurred() with the grab error
   *    description from `grab_result->GetErrorDescription()`
   *
   * @pre Device must be connected and not currently capturing.
   */
  void grabSingle() override;

  /**
   * @brief Start continuous capture using GrabStrategy_LatestImageOnly.
   *
   * Enqueues the continuous grab loop on the CommandQueue (see
   * class-level documentation for the full workflow).  Each frame
   * is converted to QImage and emitted via frameReady().  The loop
   * runs until stopCapture() clears the is_capturing_ flag.
   *
   * Old frames are automatically dropped by the Pylon grab engine
   * (LatestImageOnly strategy) to prevent frame-buffer backlog
   * when the GUI cannot keep up.
   *
   * @pre Device must be connected and not currently capturing.
   */
  void startContinuousCapture() override;

  /**
   * @brief Stop any active capture mode.
   *
   * Sets is_capturing_ to @c false (thread-safe).  The active
   * grab loop or batch sequence checks this flag and exits
   * cleanly, calling `camera_->StopGrabbing()` before returning.
   */
  void stopCapture() override;

  /**
   * @brief Start a software-triggered batch capture.
   *
   * Enqueues the batch capture sequence on the CommandQueue (see
   * class-level documentation for the full workflow).  Each frame
   * is emitted via frameReady() as it is captured.  When all
   * @p count frames have been acquired, batchComplete() is emitted.
   *
   * Software triggering is used to achieve precise inter-frame
   * timing independent of the camera's internal frame rate.
   *
   * @param count       Number of frames to capture.
   * @param interval_ms Delay between consecutive frames in ms.
   *
   * @pre Device must be connected and not currently capturing.
   */
  void startBatchCapture(int count, int interval_ms) override;

  /**
   * @brief Query whether any capture mode is active (thread-safe).
   *
   * @return @c true while a single grab, continuous loop, or batch
   *         sequence is running on the CommandQueue.
   */
  [[nodiscard]] bool isCapturing() const override;

  /**
   * @brief Return the most recently captured frame (thread-safe).
   *
   * @return The last QImage produced by the camera, or a null
   *         QImage if no frame has been captured yet.
   */
  [[nodiscard]] QImage lastFrame() const override;

 private:
  // ── Private helpers ──────────────────────────────────────────

  /**
   * @brief Thread-safe setter for the device state.
   *
   * Updates state_ under mutex_ and emits stateChanged().
   *
   * @param new_state The new device state.
   */
  void setState(DeviceState new_state);

  /**
   * @brief Convert a Pylon grab result buffer to a QImage.
   *
   * Copies the raw image buffer from the grab result into a
   * `QImage::Format_Grayscale8` image.  Handles stride mismatch
   * between the camera buffer and QImage by copying row-by-row
   * when necessary.
   *
   * @code
   *   const int w = grab_result->GetWidth();
   *   const int h = grab_result->GetHeight();
   *   const size_t stride = grab_result->GetStride();
   *   const auto* src = static_cast<const uint8_t*>(
   *       grab_result->GetBuffer());
   *
   *   QImage image(w, h, QImage::Format_Grayscale8);
   *   if (stride == static_cast<size_t>(image.bytesPerLine())) {
   *     memcpy(image.bits(), src, h * stride);
   *   } else {
   *     for (int row = 0; row < h; ++row) {
   *       memcpy(image.scanLine(row), src + row * stride, w);
   *     }
   *   }
   *   return image;
   * @endcode
   *
   * @param grab_result Pointer to the Pylon grab result (passed
   *        as `void*` to avoid exposing the Pylon CGrabResultPtr
   *        type in the header).
   * @return A QImage in Format_Grayscale8, or a null QImage on
   *         failure.
   *
   * @note In the .cpp implementation, the parameter type will be
   *       `const Pylon::CGrabResultPtr&`.  The `void*` signature
   *       here avoids a Pylon SDK header dependency.
   */
  [[nodiscard]] QImage pylonToQImage(const void* grab_result) const;

  /**
   * @brief Convert an MWA exposure value (ms) to Pylon units (us).
   *
   * @param ms Exposure in milliseconds.
   * @return Exposure in microseconds.
   */
  [[nodiscard]] static double exposureMsToUs(double ms);

  /**
   * @brief Convert a Pylon exposure value (us) to MWA units (ms).
   *
   * @param us Exposure in microseconds.
   * @return Exposure in milliseconds.
   */
  [[nodiscard]] static double exposureUsToMs(double us);

  /**
   * @brief Convert an MWA gain multiplier to Pylon decibels.
   *
   * Uses the formula `dB = 20 * log10(multiplier)`.
   * A multiplier of 1.0 yields 0 dB.
   *
   * @param multiplier Linear gain multiplier (must be > 0).
   * @return Gain in decibels.
   */
  [[nodiscard]] static double gainMultiplierToDb(double multiplier);

  /**
   * @brief Convert a Pylon gain value (dB) to MWA multiplier.
   *
   * Uses the formula `multiplier = 10^(dB / 20)`.
   *
   * @param db Gain in decibels.
   * @return Linear gain multiplier.
   */
  [[nodiscard]] static double gainDbToMultiplier(double db);

  // ── Members ──────────────────────────────────────────────────

  /// Serialises all Pylon SDK calls onto a single worker thread.
  std::unique_ptr<CommandQueue> command_queue_;

  /// Pylon instant-camera handle.  Created during connectDevice(),
  /// destroyed during disconnectDevice().
  std::unique_ptr<Pylon::CBaslerUniversalInstantCamera> camera_;

  /// Pylon image format converter (Mono8 output).  Created during
  /// connectDevice().
  std::unique_ptr<Pylon::CImageFormatConverter> converter_;

  mutable QMutex mutex_;  ///< Guards all cached state below.

  // ── Cached state (read/written under mutex_) ─────────────────

  DeviceState state_{DeviceState::kDisconnected};  ///< Connection state.
  double exposure_ms_{kCameraDefaultExposureMs};   ///< Exposure (ms).
  double gain_{kCameraDefaultGain};                ///< Gain multiplier.
  QRect roi_{0, 0, kCameraDefaultRoiWidth,
             kCameraDefaultRoiHeight};             ///< Region of interest.
  bool is_capturing_{false};                       ///< Capture in progress.
  QImage last_frame_;                              ///< Last captured frame.

  // ── Sensor limits (read from camera during connect) ──────────

  int max_sensor_width_{0};   ///< Maximum sensor width in pixels.
  int max_sensor_height_{0};  ///< Maximum sensor height in pixels.

  // ── Configuration (set before connectDevice) ──────────────────

  /// Camera serial number filter.  Empty = open first available.
  QString serial_number_;
};

}  // namespace mwa::hardware
