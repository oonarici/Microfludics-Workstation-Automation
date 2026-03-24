/**
 * @file camera_panel.h
 * @brief Camera control and acquisition panel widget.
 * @author MWA Team
 * @date 2026-03-23
 *
 * Declares the CameraPanel widget, which provides a complete UI for
 * connecting to, configuring, and capturing images from a camera device
 * through the CameraControllerInterface. The panel is divided into four
 * group boxes: Connection, Acquisition Settings, Capture, and Status.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QWidget>

#include "hardware/camera/camera_controller_interface.h"

namespace mwa::gui {

/**
 * @class CameraPanel
 * @brief Widget for controlling a camera device and capturing images.
 *
 * CameraPanel presents four sections:
 *  - **Connection**           — device selector, status label,
 *                               connect/disconnect.
 *  - **Acquisition Settings** — exposure, gain, and region-of-interest
 *                               controls with 300 ms debounce.
 *  - **Capture**              — single-frame grab, continuous live view,
 *                               and timed batch capture.
 *  - **Status**               — capture state label, frame counter, and
 *                               a 160×120 scaled live preview.
 *
 * The panel owns no hardware resources directly; all hardware interaction
 * is delegated to a CameraControllerInterface instance supplied via
 * setController(). Acquisition Settings and Capture groups are disabled
 * while the device is disconnected.
 *
 * @note Exposure, gain, and ROI values are forwarded to the controller
 *       only after a 300 ms debounce to prevent flooding the device.
 *
 * @see mwa::hardware::CameraControllerInterface
 */
class CameraPanel : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Construct the camera panel with all sub-widgets.
   *
   * Builds the four QGroupBox sections and wires all internal
   * widget-to-widget connections. The panel starts with the Acquisition
   * and Capture groups disabled and shows "● Disconnected" in the
   * status label.
   *
   * @param parent Optional parent widget for Qt ownership.
   */
  explicit CameraPanel(QWidget* parent = nullptr);

  /**
   * @brief Attach a camera controller and wire all signal/slot connections.
   *
   * Disconnects any previously attached controller's signals, then
   * connects stateChanged(), frameReady(), and batchComplete() from the
   * new controller to the corresponding slots in this panel. Passing
   * @c nullptr clears the controller reference without crashing.
   *
   * @param controller Pointer to the camera controller to attach.
   *                   May be @c nullptr to detach.
   */
  void setController(
      mwa::hardware::CameraControllerInterface* controller);

 private slots:
  /**
   * @brief Handle device state changes from the controller.
   *
   * Updates the status label, enables/disables Acquisition and Capture
   * groups, and toggles the Connect/Disconnect button text.
   *
   * @param new_state The updated device connection state.
   */
  void onStateChanged(
      mwa::hardware::DeviceInterface::DeviceState new_state);

  /**
   * @brief Handle a new frame from the controller.
   *
   * Scales the image to 160×120 and updates the preview label.
   * Updates the frame counter and capture state. Preview updates
   * are throttled to at most one every 100 ms.
   *
   * @param frame The newly captured image from the controller.
   */
  void onFrameReady(const QImage& frame);

  /**
   * @brief Handle batch capture completion from the controller.
   *
   * Re-enables capture controls and updates the state label to "Idle".
   */
  void onBatchComplete();

  /**
   * @brief Handle the Connect / Disconnect button click.
   *
   * If the device is disconnected, calls connectDevice() on the
   * controller. If connected, shows a confirmation dialog before
   * calling disconnectDevice().
   */
  void onConnectClicked();

  /**
   * @brief Grab a single frame from the camera.
   *
   * Disables the capture buttons and calls grabSingle() on the
   * controller. Controls are re-enabled when frameReady() fires.
   */
  void onGrabSingleClicked();

  /**
   * @brief Toggle continuous live-view capture on or off.
   *
   * When toggled on, calls startContinuousCapture() and updates
   * the button label to "Stop Continuous". When toggled off, calls
   * stopCapture() and resets the label.
   *
   * @param checked @c true if continuous capture is being started.
   */
  void onContinuousToggled(bool checked);

  /**
   * @brief Start a batch capture sequence.
   *
   * Reads count and interval from the batch spinboxes and calls
   * startBatchCapture() on the controller.
   */
  void onStartBatchClicked();

  /**
   * @brief Stop any active capture immediately.
   *
   * Calls stopCapture() on the controller and resets the UI state.
   */
  void onStopClicked();

  /**
   * @brief Reset the ROI spinboxes to the full-sensor default.
   *
   * Sets X=0, Y=0, W=1920, H=1080 and schedules an ROI debounce.
   */
  void onResetRoiClicked();

  /**
   * @brief Flush the debounced exposure value to the controller.
   *
   * Called when the 300 ms exposure debounce timer fires.
   */
  void onExposureDebounced();

  /**
   * @brief Flush the debounced gain value to the controller.
   *
   * Called when the 300 ms gain debounce timer fires.
   */
  void onGainDebounced();

  /**
   * @brief Flush the debounced ROI value to the controller.
   *
   * Called when the 300 ms ROI debounce timer fires.
   */
  void onRoiDebounced();

 private:
  /**
   * @brief Build and return the Connection group box.
   *
   * @return Pointer to the created QGroupBox (owned by this widget).
   */
  QGroupBox* createConnectionGroup();

  /**
   * @brief Build and return the Acquisition Settings group box.
   *
   * @return Pointer to the created QGroupBox (owned by this widget).
   */
  QGroupBox* createAcquisitionGroup();

  /**
   * @brief Build and return the Capture group box.
   *
   * @return Pointer to the created QGroupBox (owned by this widget).
   */
  QGroupBox* createCaptureGroup();

  /**
   * @brief Build and return the Status group box.
   *
   * @return Pointer to the created QGroupBox (owned by this widget).
   */
  QGroupBox* createStatusGroup();

  /**
   * @brief Apply the colour-coded status stylesheet to lbl_status_.
   *
   * @param new_state Current device state determining the dot colour.
   */
  void applyStatusStyle(
      mwa::hardware::DeviceInterface::DeviceState new_state);

  /**
   * @brief Enable or disable acquisition and capture widgets.
   *
   * @param connected @c true to enable controls, @c false to disable.
   */
  void setControlsEnabled(bool connected);

  /**
   * @brief Update capture-related button states for an active capture.
   *
   * Disables acquisition settings and shows the Stop button while any
   * capture is active.
   *
   * @param capturing @c true when a capture is in progress.
   */
  void setCaptureActive(bool capturing);

  // ---- Connection group ---------------------------------------------------
  QGroupBox*   grp_connection_{nullptr};  ///< Connection section.
  QComboBox*   cmb_camera_{nullptr};      ///< Device selector.
  QLabel*      lbl_status_{nullptr};      ///< Coloured dot + state text.
  QPushButton* btn_connect_{nullptr};     ///< Connect/Disconnect button.

  // ---- Acquisition Settings group -----------------------------------------
  QGroupBox*      grp_acquisition_{nullptr};  ///< Acquisition section.
  QDoubleSpinBox* spn_exposure_{nullptr};     ///< Exposure in ms.
  QDoubleSpinBox* spn_gain_{nullptr};         ///< Analogue gain multiplier.
  QSpinBox*       spn_roi_x_{nullptr};        ///< ROI origin X in pixels.
  QSpinBox*       spn_roi_y_{nullptr};        ///< ROI origin Y in pixels.
  QSpinBox*       spn_roi_w_{nullptr};        ///< ROI width in pixels.
  QSpinBox*       spn_roi_h_{nullptr};        ///< ROI height in pixels.
  QPushButton*    btn_reset_roi_{nullptr};    ///< Reset ROI to full sensor.

  // ---- Capture group ------------------------------------------------------
  QGroupBox*   grp_capture_{nullptr};       ///< Capture section.
  QPushButton* btn_grab_single_{nullptr};   ///< Grab a single frame.
  QPushButton* btn_continuous_{nullptr};    ///< Checkable continuous toggle.
  QSpinBox*    spn_batch_count_{nullptr};   ///< Batch frame count.
  QSpinBox*    spn_batch_interval_{nullptr};///< Batch interval in ms.
  QPushButton* btn_start_batch_{nullptr};   ///< Start batch capture.
  QPushButton* btn_stop_{nullptr};          ///< Stop any active capture.

  // ---- Status group -------------------------------------------------------
  QGroupBox*   grp_status_{nullptr};         ///< Status section.
  QLabel*      lbl_capture_state_{nullptr};  ///< "Idle" / "Capturing" / etc.
  QLabel*      lbl_frame_count_{nullptr};    ///< Frame counter display.
  QLabel*      lbl_preview_{nullptr};        ///< 160×120 image preview.

  // ---- Debounce timers ----------------------------------------------------
  QTimer* timer_exposure_{nullptr};  ///< 300 ms exposure debounce.
  QTimer* timer_gain_{nullptr};      ///< 300 ms gain debounce.
  QTimer* timer_roi_{nullptr};       ///< 300 ms ROI debounce.

  // ---- State --------------------------------------------------------------
  /// Non-owning pointer to the attached camera controller.
  mwa::hardware::CameraControllerInterface* controller_{nullptr};

  /// Elapsed timer used to throttle preview updates to 100 ms.
  QElapsedTimer preview_throttle_;

  /// Running count of frames received since last capture start.
  int frame_count_{0};

  /// Total frames expected for the current batch (0 = non-batch).
  int batch_total_{0};

  /// Frames received so far in the current batch.
  int batch_received_{0};

  // ---- Constants ----------------------------------------------------------
  /// Minimum preview update interval in milliseconds.
  static constexpr qint64 kPreviewThrottleMs = 100;
  /// Default ROI width (full-sensor).
  static constexpr int kDefaultRoiWidth = 1920;
  /// Default ROI height (full-sensor).
  static constexpr int kDefaultRoiHeight = 1080;
  /// Preview label width in pixels.
  static constexpr int kPreviewWidth = 160;
  /// Preview label height in pixels.
  static constexpr int kPreviewHeight = 120;
  /// Debounce interval in milliseconds.
  static constexpr int kDebounceMs = 300;
  /// Color hex for connected/idle state.
  static constexpr auto kColorConnected    = "#27AE60";
  /// Color hex for disconnected state.
  static constexpr auto kColorDisconnected = "#95A5A6";
  /// Color hex for error state.
  static constexpr auto kColorError        = "#E74C3C";
  /// Color hex for connecting state.
  static constexpr auto kColorConnecting   = "#F39C12";
  /// Color hex for capturing state.
  static constexpr auto kColorCapturing    = "#2980B9";
};

}  // namespace mwa::gui
