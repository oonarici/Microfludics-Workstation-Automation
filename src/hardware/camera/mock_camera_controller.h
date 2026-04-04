/**
 * @file mock_camera_controller.h
 * @brief Mock camera controller with synthetic microscopy frames.
 * @author MWA Team
 * @date 2026-04-02
 *
 * Provides a simulated camera controller that implements
 * CameraControllerInterface without requiring real hardware. Generates
 * realistic synthetic microscopy frames with darkfield background,
 * microchannel walls, drifting particles, exposure/gain response, and
 * per-pixel read noise. Suitable for GUI development and unit testing.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QImage>
#include <QRect>
#include <QVector>
#include <random>

#include "hardware/camera/camera_controller_interface.h"

namespace mwa::hardware {

/**
 * @class MockCameraController
 * @brief Simulated imaging camera with synthetic microscopy frames.
 *
 * Implements CameraControllerInterface with in-memory state. The
 * connect() operation uses a 500 ms QTimer delay to simulate real
 * hardware latency. Each captured frame is a procedurally generated
 * 8-bit grayscale image that mimics a darkfield microscopy view:
 *
 * - Dark background (~20 base pixel value)
 * - Two horizontal bright lines simulating microchannel walls
 * - 5-15 bright circular particles drifting leftward between walls
 * - Exposure-dependent brightness scaling
 * - Gain-dependent brightness and noise amplification
 * - Gaussian read noise (sigma = 3.0 * gain_)
 * - Frame counter overlay in the top-left corner
 *
 * Frame rate adapts to exposure: interval = max(33 ms, exposure_ms).
 * Particles re-randomize every 30 frames.
 *
 * @see CameraControllerInterface
 */
class MockCameraController : public CameraControllerInterface {
  Q_OBJECT

 public:
  /**
   * @brief Construct a MockCameraController.
   *
   * Initialises the random number generator, particle set, and
   * capture timer. Default exposure is 10.0 ms, gain is 1.0, and
   * ROI is (0, 0, 640, 480).
   *
   * @param parent Optional QObject parent for Qt ownership.
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
   * Transitions state to DeviceState::kConnecting immediately,
   * then after 500 ms transitions to DeviceState::kConnected and
   * emits stateChanged().
   */
  void connectDevice() override;

  /**
   * @brief Disconnect from the simulated device immediately.
   *
   * Stops any active capture and transitions state to
   * DeviceState::kDisconnected.
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
   * @return @c true if state is DeviceState::kConnected.
   */
  [[nodiscard]] bool isConnected() const override;

  // CameraControllerInterface overrides

  /**
   * @brief Set the sensor exposure time and emit exposureChanged().
   *
   * If continuous capture is active, updates the timer interval to
   * match the new exposure-based frame rate.
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
   * @param roi Rectangle defining the active sensor area.
   */
  void setRoi(const QRect& roi) override;

  /**
   * @brief Return the current region of interest.
   *
   * @return Rectangle defining the active sensor area.
   */
  [[nodiscard]] QRect roi() const override;

  /**
   * @brief Capture a single synthetic microscopy frame and emit
   *        frameReady().
   */
  void grabSingle() override;

  /**
   * @brief Start continuous capture at an exposure-limited rate.
   *
   * Frame interval = max(33 ms, exposure_ms), so the maximum rate
   * is ~30 fps for short exposures.
   */
  void startContinuousCapture() override;

  /**
   * @brief Stop any active capture mode.
   */
  void stopCapture() override;

  /**
   * @brief Start a batch capture sequence.
   *
   * Captures @p count frames separated by @p interval_ms ms and
   * emits batchComplete() when done.
   *
   * @param count       Number of frames to capture.
   * @param interval_ms Interval between frames in milliseconds.
   */
  void startBatchCapture(int count, int interval_ms) override;

  /**
   * @brief Query whether any capture mode is active.
   *
   * @return @c true if capturing, @c false otherwise.
   */
  [[nodiscard]] bool isCapturing() const override;

  /**
   * @brief Return the most recently captured frame.
   *
   * @return The last QImage produced by the mock camera.
   */
  [[nodiscard]] QImage lastFrame() const override;

 private:
  /**
   * @brief Describes a single bright circular particle in the
   *        synthetic microscopy field of view.
   */
  struct Particle {
    double x;       ///< Centre X coordinate in pixels.
    double y;       ///< Centre Y coordinate in pixels.
    double radius;  ///< Radius in pixels (3-8).
    int brightness; ///< Peak pixel value before exposure/gain (180-255).
  };

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

  /// Mersenne Twister random number generator.
  std::mt19937 rng_;
  /// Monotonically increasing frame counter.
  int frame_counter_;
  /// Accumulated leftward particle drift in pixels.
  double particle_drift_x_;
  /// Current set of particles in the field of view.
  QVector<Particle> particles_;
  /// Frames elapsed since last particle regeneration.
  int frames_since_regen_;

  /**
   * @brief Generate a synthetic 8-bit grayscale microscopy frame.
   *
   * Produces an image matching the current ROI dimensions with:
   * - Dark background scaled by exposure
   * - Two horizontal channel wall lines
   * - Circular particles with leftward drift
   * - Exposure and gain brightness scaling
   * - Gaussian read noise
   * - Frame counter text overlay
   *
   * @return A QImage in Format_Grayscale8.
   */
  QImage generateMicroscopyFrame();

  /**
   * @brief Randomize the particle set for a new field of view.
   *
   * Generates 5-15 particles with random positions between the
   * channel walls, random radii (3-8 px), and random brightness
   * values (180-255).
   */
  void regenerateParticles();

  /**
   * @brief Compute the capture timer interval from exposure.
   *
   * @return max(33, static_cast<int>(exposure_)) in milliseconds.
   */
  [[nodiscard]] int captureIntervalMs() const;

  /**
   * @brief Slot called by capture_timer_ to produce each frame.
   */
  void onCaptureTimerTick();
};

}  // namespace mwa::hardware
