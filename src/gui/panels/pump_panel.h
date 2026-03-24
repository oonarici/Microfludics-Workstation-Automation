/**
 * @file pump_panel.h
 * @brief Syringe pump control panel widget.
 * @author MWA Team
 * @date 2026-03-23
 *
 * Declares the PumpPanel widget, which provides a complete UI for
 * connecting to, controlling, and monitoring a syringe pump through
 * the PumpControllerInterface. The panel is divided into three group
 * boxes: Connection, Controls, and Status.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QLCDNumber>
#include <QProgressBar>
#include <QPushButton>
#include <QComboBox>
#include <QWidget>

#include "hardware/pump/pump_controller_interface.h"

namespace mwa::gui {

/**
 * @class PumpPanel
 * @brief Widget for controlling a syringe pump.
 *
 * PumpPanel presents three sections:
 *  - **Connection** — port selector, status dot and text,
 *                     connect/disconnect button.
 *  - **Controls**   — flow rate spinbox, target volume spinbox,
 *                     start/stop/refill action buttons.
 *  - **Status**     — plunger position LCD, pump sub-state dot and
 *                     text, infusion progress bar, error label.
 *
 * All hardware interaction is delegated to a PumpControllerInterface
 * instance supplied via setController(). The Controls group box is
 * disabled while the device is not connected. An internal
 * @c is_refilling_ flag distinguishes the refilling state from
 * infusing when updating button availability.
 *
 * @note Stop and Refill show a QMessageBox::question confirmation
 *       before issuing the corresponding controller command.
 *
 * @see mwa::hardware::PumpControllerInterface
 */
class PumpPanel : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Construct the pump panel with all sub-widgets.
   *
   * Builds the three QGroupBox sections and wires all internal
   * widget-to-widget connections. The panel starts with the Controls
   * group disabled and shows a gray "Disconnected" status.
   *
   * @param parent Optional parent widget for Qt ownership.
   */
  explicit PumpPanel(QWidget* parent = nullptr);

  /**
   * @brief Attach a pump controller and wire all signal/slot
   *        connections.
   *
   * Disconnects any previously attached controller's signals, then
   * connects stateChanged(), positionChanged(), infusionStarted(),
   * infusionStopped(), flowRateChanged(), and errorOccurred() from
   * the new controller to slots in this panel. Passing @c nullptr
   * clears the controller reference without crashing.
   *
   * @param controller Pointer to the pump controller to attach.
   *                   May be @c nullptr to detach.
   */
  void setController(
      mwa::hardware::PumpControllerInterface* controller);

 private slots:
  /**
   * @brief Handle device state changes from the controller.
   *
   * Updates the status dot/text labels, enables/disables the Controls
   * group, and toggles the Connect/Disconnect button text.
   *
   * @param new_state The updated device connection state.
   */
  void onStateChanged(
      mwa::hardware::DeviceInterface::DeviceState new_state);

  /**
   * @brief Handle plunger position updates from the controller.
   *
   * Updates the LCD readout and the progress bar relative to the
   * current target volume.
   *
   * @param uL Current plunger position in microlitres.
   */
  void onPositionChanged(double uL);

  /**
   * @brief Handle flow rate changes from the controller.
   *
   * Syncs the spinbox display to the confirmed flow rate without
   * triggering a setFlowRate() call.
   *
   * @param uL_per_min Confirmed flow rate in µL/min.
   */
  void onFlowRateChanged(double uL_per_min);

  /**
   * @brief Handle the infusion started signal from the controller.
   *
   * Updates pump sub-state labels to "Infusing" (blue) and adjusts
   * button enabled states.
   */
  void onInfusionStarted();

  /**
   * @brief Handle the infusion stopped signal from the controller.
   *
   * Updates pump sub-state labels to "Idle" (green) and re-enables
   * appropriate buttons.
   */
  void onInfusionStopped();

  /**
   * @brief Display an error message from the controller.
   *
   * Makes lbl_error_ visible and sets its text to the error message.
   * Also updates the sub-state to Error (red).
   *
   * @param message Human-readable error description.
   */
  void onErrorOccurred(const QString& message);

  /**
   * @brief Handle the Connect / Disconnect button click.
   *
   * If disconnected, calls connectDevice(). If connected, shows a
   * confirmation dialog before calling disconnectDevice().
   */
  void onConnectClicked();

  /**
   * @brief Send the flow rate value to the controller.
   *
   * Called on QDoubleSpinBox::valueChanged for spn_flow_rate_.
   *
   * @param value New flow rate in µL/min.
   */
  void onFlowRateValueChanged(double value);

  /**
   * @brief Send the target volume value to the controller.
   *
   * Called on QDoubleSpinBox::valueChanged for spn_volume_.
   *
   * @param value New target volume in µL.
   */
  void onVolumeValueChanged(double value);

  /**
   * @brief Start an infusion run without confirmation.
   *
   * Calls startInfusion() on the controller and disables spinboxes
   * along with btn_start_ and btn_refill_.
   */
  void onStartClicked();

  /**
   * @brief Stop the active infusion after user confirmation.
   *
   * Shows a QMessageBox::question dialog; on Yes calls stopInfusion().
   */
  void onStopClicked();

  /**
   * @brief Retract the plunger to refill after user confirmation.
   *
   * Shows a QMessageBox::question dialog; on Yes calls refill() and
   * sets is_refilling_ = true.
   */
  void onRefillClicked();

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
   * @brief Apply colour-coded stylesheet to the status dot labels.
   *
   * @param color Hex colour string (e.g. "#27AE60").
   */
  void applyStatusDotColor(const QString& color);

  /**
   * @brief Update the pump sub-state dot and text labels.
   *
   * @param color Hex colour string for the dot.
   * @param text  Human-readable state string.
   */
  void updateSubState(const QString& color,
                      const QString& text);

  /**
   * @brief Refresh control button enabled states based on pump state.
   *
   * @param is_connected  @c true if the device is connected.
   * @param is_infusing   @c true if the pump is actively infusing.
   * @param is_refilling  @c true if the pump is in refill mode.
   */
  void updateButtonStates(bool is_connected,
                          bool is_infusing,
                          bool is_refilling);

  // ---- Connection group ---------------------------------------------------
  QGroupBox*   grp_connection_{nullptr};  ///< Connection section.
  QComboBox*   cmb_port_{nullptr};        ///< Port/device selector.
  QLabel*      lbl_status_dot_{nullptr};  ///< 12x12 coloured dot.
  QLabel*      lbl_status_text_{nullptr}; ///< Status text label.
  QPushButton* btn_connect_{nullptr};     ///< Connect/Disconnect button.

  // ---- Controls group -----------------------------------------------------
  QGroupBox*      grp_controls_{nullptr};   ///< Controls section.
  QDoubleSpinBox* spn_flow_rate_{nullptr};  ///< Flow rate spinbox.
  QDoubleSpinBox* spn_volume_{nullptr};     ///< Target volume spinbox.
  QPushButton*    btn_start_{nullptr};      ///< Start Infusion button.
  QPushButton*    btn_stop_{nullptr};       ///< Stop button.
  QPushButton*    btn_refill_{nullptr};     ///< Refill button.

  // ---- Status group -------------------------------------------------------
  QGroupBox*   grp_status_{nullptr};       ///< Status section.
  QLCDNumber*  lcd_position_{nullptr};     ///< Position readout in µL.
  QLabel*      lbl_state_dot_{nullptr};    ///< Coloured dot for sub-state.
  QLabel*      lbl_state_text_{nullptr};   ///< Sub-state text label.
  QProgressBar* prg_infusion_{nullptr};   ///< Infusion progress bar.
  QLabel*      lbl_error_{nullptr};        ///< Error message (hidden).

  // ---- State --------------------------------------------------------------
  /// Non-owning pointer to the attached pump controller.
  mwa::hardware::PumpControllerInterface* controller_{nullptr};
  /// Tracks whether the pump is in a refill operation.
  bool is_refilling_{false};

  // ---- Constants ----------------------------------------------------------
  /// Color hex for connected / idle state.
  static constexpr auto kColorConnected    = "#27AE60";
  /// Color hex for disconnected state.
  static constexpr auto kColorDisconnected = "#95A5A6";
  /// Color hex for error state.
  static constexpr auto kColorError        = "#E74C3C";
  /// Color hex for connecting state.
  static constexpr auto kColorConnecting   = "#F39C12";
  /// Color hex for active infusing / refilling state.
  static constexpr auto kColorActive       = "#2980B9";
  /// Dot widget fixed size in pixels.
  static constexpr int kDotSize = 12;
};

}  // namespace mwa::gui
