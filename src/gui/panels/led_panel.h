/**
 * @file led_panel.h
 * @brief LED illumination control panel widget.
 * @author MWA Team
 * @date 2026-03-23
 *
 * Declares the LedPanel widget, which provides a complete UI for
 * connecting to, configuring, and monitoring an LED illumination
 * source through the LedControllerInterface. The panel is divided
 * into three group boxes: Connection, Controls, and Status.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QLCDNumber>
#include <QPushButton>
#include <QSlider>
#include <QComboBox>
#include <QWidget>

#include "hardware/led/led_controller_interface.h"

namespace mwa::gui {

/**
 * @class LedPanel
 * @brief Widget for controlling an LED illumination source.
 *
 * LedPanel presents three sections:
 *  - **Connection** — device selector, status label, connect/disconnect.
 *  - **Controls**   — power toggle, intensity spinbox and slider (synced).
 *  - **Status**     — confirmed power state, LCD intensity readout,
 *                     device connection state label.
 *
 * The panel owns no hardware resources directly; all hardware interaction
 * is delegated to a LedControllerInterface instance supplied via
 * setController(). The Controls group box is disabled while the device
 * is not connected.
 *
 * @note Intensity is sent to the controller only on editingFinished
 *       (spinbox) or sliderReleased (slider), not during dragging.
 *
 * @see mwa::hardware::LedControllerInterface
 */
class LedPanel : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Construct the LED panel with all sub-widgets.
   *
   * Builds the three QGroupBox sections and wires all internal
   * widget-to-widget connections. The panel starts with the Controls
   * group disabled and shows "● Disconnected" in the status label.
   *
   * @param parent Optional parent widget for Qt ownership.
   */
  explicit LedPanel(QWidget* parent = nullptr);

  /**
   * @brief Attach a LED controller and wire all signal/slot connections.
   *
   * Disconnects any previously attached controller's signals, then
   * connects stateChanged(), intensityChanged(), and powerStateChanged()
   * from the new controller to the corresponding slots in this panel.
   * Passing @c nullptr clears the controller reference without crashing.
   *
   * @param controller Pointer to the LED controller to attach.
   *                   May be @c nullptr to detach.
   */
  void setController(
      mwa::hardware::LedControllerInterface* controller);

 private slots:
  /**
   * @brief Handle device state changes from the controller.
   *
   * Updates the status labels, enables/disables the Controls group,
   * and toggles the Connect/Disconnect button text.
   *
   * @param new_state The updated device connection state.
   */
  void onStateChanged(
      mwa::hardware::DeviceInterface::DeviceState new_state);

  /**
   * @brief Handle intensity updates from the controller.
   *
   * Updates the LCD readout and, with signals blocked, keeps the
   * spinbox and slider in sync with the confirmed value.
   *
   * @param percent Confirmed intensity in the range [0.0, 100.0].
   */
  void onIntensityChanged(double percent);

  /**
   * @brief Handle power state changes from the controller.
   *
   * Updates btn_power_ text and lbl_power_value_ color/text.
   *
   * @param on @c true if the LED is now powered on.
   */
  void onPowerStateChanged(bool on);

  /**
   * @brief Handle the Connect / Disconnect button click.
   *
   * If the device is disconnected, calls connectDevice() on the
   * controller. If connected, shows a confirmation dialog before
   * calling disconnectDevice().
   */
  void onConnectClicked();

  /**
   * @brief Toggle LED power without confirmation.
   *
   * Calls setPowerOn() on the controller with the new checked state.
   *
   * @param checked @c true if the button was toggled on (LED ON).
   */
  void onPowerToggled(bool checked);

  /**
   * @brief Send the spinbox value to the controller after editing.
   *
   * Called on QDoubleSpinBox::editingFinished. Syncs the slider and
   * forwards the value to the controller's setIntensity().
   */
  void onSpinboxEditingFinished();

  /**
   * @brief Send the slider value to the controller after release.
   *
   * Called on QSlider::sliderReleased. Converts the slider integer
   * (0–1000) to a percentage and forwards it to setIntensity().
   */
  void onSliderReleased();

  /**
   * @brief Keep spinbox in sync while the slider moves (display only).
   *
   * Called on QSlider::valueChanged. Updates the spinbox display
   * without triggering a setIntensity() call.
   *
   * @param value Raw slider value in the range [0, 1000].
   */
  void onSliderValueChanged(int value);

 private:
  /**
   * @brief Build and return the Connection group box.
   *
   * @return Pointer to the created QGroupBox (owned by this widget).
   */
  QGroupBox* createConnectionGroup();

  /**
   * @brief Build and return the Controls group box.
   *
   * @return Pointer to the created QGroupBox (owned by this widget).
   */
  QGroupBox* createControlsGroup();

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

  // ---- Connection group ---------------------------------------------------
  QGroupBox*    grp_connection_{nullptr};  ///< Connection section.
  QComboBox*    cmb_device_{nullptr};      ///< Device selector.
  QLabel*       lbl_status_{nullptr};      ///< Coloured dot + state text.
  QPushButton*  btn_connect_{nullptr};     ///< Connect/Disconnect button.

  // ---- Controls group -----------------------------------------------------
  QGroupBox*      grp_controls_{nullptr};   ///< Controls section.
  QPushButton*    btn_power_{nullptr};      ///< Checkable power toggle.
  QDoubleSpinBox* spn_intensity_{nullptr};  ///< Intensity percentage.
  QSlider*        sld_intensity_{nullptr};  ///< Intensity slider (0–1000).

  // ---- Status group -------------------------------------------------------
  QGroupBox*  grp_status_{nullptr};         ///< Status section.
  QLabel*     lbl_power_value_{nullptr};    ///< "● ON" / "● OFF" label.
  QLCDNumber* lcd_intensity_{nullptr};      ///< Confirmed intensity readout.
  QLabel*     lbl_device_state_{nullptr};   ///< Connection state text.

  // ---- State --------------------------------------------------------------
  /// Non-owning pointer to the attached LED controller.
  mwa::hardware::LedControllerInterface* controller_{nullptr};

  // ---- Constants ----------------------------------------------------------
  /// Slider integer scale factor (slider 0–1000 ↔ percent 0.0–100.0).
  static constexpr int kSliderScale = 10;
  /// Color hex for connected state.
  static constexpr auto kColorConnected    = "#27AE60";
  /// Color hex for disconnected state.
  static constexpr auto kColorDisconnected = "#95A5A6";
  /// Color hex for error state.
  static constexpr auto kColorError        = "#E74C3C";
  /// Color hex for connecting state.
  static constexpr auto kColorConnecting   = "#F39C12";
};

}  // namespace mwa::gui
