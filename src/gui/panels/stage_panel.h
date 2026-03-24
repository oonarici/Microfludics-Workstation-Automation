/**
 * @file stage_panel.h
 * @brief Motorized XYZ stage control panel widget.
 * @author MWA Team
 * @date 2026-03-23
 *
 * Declares the StagePanel widget, which provides a complete UI for
 * connecting to, jogging, and commanding an XYZ translation stage through
 * the StageControllerInterface. The panel is divided into five group boxes
 * (Connection, Jog, Absolute Position, Speed, Status) plus a persistent
 * Emergency Stop button.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QWidget>

#include "hardware/stage/stage_controller_interface.h"

namespace mwa::gui {

/**
 * @class StagePanel
 * @brief Widget for controlling a motorized XYZ translation stage.
 *
 * StagePanel presents five sections plus an emergency stop button:
 *  - **Connection**        — port selector, status label,
 *                            connect/disconnect.
 *  - **Jog**               — step-size combo, XY directional grid,
 *                            Z axis buttons, and a Home button.
 *  - **Absolute Position** — X/Y/Z target spinboxes and a Go To button
 *                            with a confirmation dialog.
 *  - **Speed**             — travel speed spinbox with 300 ms debounce.
 *  - **Status**            — monospace position labels and state text.
 *  - **Emergency Stop**    — always-enabled red button that immediately
 *                            halts all stage motion without confirmation.
 *
 * The panel owns no hardware resources directly; all hardware interaction
 * is delegated to a StageControllerInterface instance supplied via
 * setController(). Jog, Absolute Position, and Speed groups are disabled
 * while the device is disconnected.
 *
 * @note The emergency stop button is always enabled regardless of
 *       connection state so that the operator can halt motion at any time.
 *
 * @see mwa::hardware::StageControllerInterface
 */
class StagePanel : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Construct the stage panel with all sub-widgets.
   *
   * Builds all five group boxes, the emergency stop button, and wires
   * all internal widget-to-widget connections. The panel starts with
   * Jog, Absolute Position, and Speed groups disabled and shows
   * "● Disconnected" in the status label.
   *
   * @param parent Optional parent widget for Qt ownership.
   */
  explicit StagePanel(QWidget* parent = nullptr);

  /**
   * @brief Attach a stage controller and wire all signal/slot connections.
   *
   * Disconnects any previously attached controller's signals, then
   * connects stateChanged(), positionChanged(), moveComplete(), and
   * homeComplete() from the new controller to the corresponding slots
   * in this panel. Passing @c nullptr clears the controller reference
   * without crashing.
   *
   * @param controller Pointer to the stage controller to attach.
   *                   May be @c nullptr to detach.
   */
  void setController(
      mwa::hardware::StageControllerInterface* controller);

 private slots:
  /**
   * @brief Handle device state changes from the controller.
   *
   * Updates the status label, enables/disables control groups, and
   * toggles the Connect/Disconnect button text.
   *
   * @param new_state The updated device connection state.
   */
  void onStateChanged(
      mwa::hardware::DeviceInterface::DeviceState new_state);

  /**
   * @brief Update position labels when the stage moves.
   *
   * Formats each axis value to three decimal places and updates
   * the corresponding monospace QLabel.
   *
   * @param x Current X position in millimetres.
   * @param y Current Y position in millimetres.
   * @param z Current Z position in millimetres.
   */
  void onPositionChanged(double x, double y, double z);

  /**
   * @brief Handle move completion from the controller.
   *
   * Re-enables jog, home, and go-to controls and updates the
   * state label to "Idle".
   */
  void onMoveComplete();

  /**
   * @brief Handle home sequence completion from the controller.
   *
   * Re-enables all motion controls and updates the state label
   * to "Idle".
   */
  void onHomeComplete();

  /**
   * @brief Handle the Connect / Disconnect button click.
   *
   * If the device is disconnected, calls connectDevice() on the
   * controller. If connected, shows a confirmation dialog before
   * calling disconnectDevice().
   */
  void onConnectClicked();

  /**
   * @brief Jog the stage in the positive X direction.
   *
   * Reads the current step size from cmb_step_size_ and calls
   * moveRelative(+step, 0, 0) on the controller.
   */
  void onJogXPlus();

  /**
   * @brief Jog the stage in the negative X direction.
   *
   * Reads the current step size from cmb_step_size_ and calls
   * moveRelative(-step, 0, 0) on the controller.
   */
  void onJogXMinus();

  /**
   * @brief Jog the stage in the positive Y direction.
   *
   * Reads the current step size from cmb_step_size_ and calls
   * moveRelative(0, +step, 0) on the controller.
   */
  void onJogYPlus();

  /**
   * @brief Jog the stage in the negative Y direction.
   *
   * Reads the current step size from cmb_step_size_ and calls
   * moveRelative(0, -step, 0) on the controller.
   */
  void onJogYMinus();

  /**
   * @brief Jog the stage in the positive Z direction.
   *
   * Reads the current step size from cmb_step_size_ and calls
   * moveRelative(0, 0, +step) on the controller.
   */
  void onJogZPlus();

  /**
   * @brief Jog the stage in the negative Z direction.
   *
   * Reads the current step size from cmb_step_size_ and calls
   * moveRelative(0, 0, -step) on the controller.
   */
  void onJogZMinus();

  /**
   * @brief Home all stage axes with a confirmation dialog.
   *
   * Asks the operator to confirm, then calls home() and disables
   * motion controls until homeComplete() fires.
   */
  void onHomeClicked();

  /**
   * @brief Move to the absolute position specified in the spinboxes.
   *
   * Shows a confirmation dialog with the target coordinates, then
   * calls moveAbsolute() and disables motion controls until
   * moveComplete() fires.
   */
  void onGoToClicked();

  /**
   * @brief Immediately halt all stage motion without confirmation.
   *
   * Calls stopMotion() on the controller and re-enables motion controls.
   */
  void onEmergencyStop();

  /**
   * @brief Flush the debounced speed value to the controller.
   *
   * Called when the 300 ms speed debounce timer fires.
   */
  void onSpeedDebounced();

 private:
  /**
   * @brief Build and return the Connection group box.
   *
   * @return Pointer to the created QGroupBox (owned by this widget).
   */
  QGroupBox* createConnectionGroup();

  /**
   * @brief Build and return the Jog group box.
   *
   * @return Pointer to the created QGroupBox (owned by this widget).
   */
  QGroupBox* createJogGroup();

  /**
   * @brief Build and return the Absolute Position group box.
   *
   * @return Pointer to the created QGroupBox (owned by this widget).
   */
  QGroupBox* createAbsPositionGroup();

  /**
   * @brief Build and return the Speed group box.
   *
   * @return Pointer to the created QGroupBox (owned by this widget).
   */
  QGroupBox* createSpeedGroup();

  /**
   * @brief Build and return the Status group box.
   *
   * @return Pointer to the created QGroupBox (owned by this widget).
   */
  QGroupBox* createStatusGroup();

  /**
   * @brief Build and return the Emergency Stop button.
   *
   * The button is full-width, 48 px tall, red background, always enabled.
   *
   * @return Pointer to the created QPushButton (owned by this widget).
   */
  QPushButton* createEmergencyStopButton();

  /**
   * @brief Apply the colour-coded status stylesheet to lbl_status_.
   *
   * @param new_state Current device state determining the dot colour.
   */
  void applyStatusStyle(
      mwa::hardware::DeviceInterface::DeviceState new_state);

  /**
   * @brief Enable or disable motion control groups.
   *
   * @param enabled @c true to enable jog, abs-position, and speed
   *                controls; @c false to disable them.
   */
  void setMotionControlsEnabled(bool enabled);

  /**
   * @brief Enable or disable individual motion buttons during a move.
   *
   * Disables all jog, home, and go-to buttons while the stage is moving
   * or homing. The emergency stop button is never affected.
   *
   * @param enabled @c true to enable buttons, @c false to disable.
   */
  void setMoveButtonsEnabled(bool enabled);

  /**
   * @brief Return the step size selected in cmb_step_size_ in mm.
   *
   * @return Selected step size in millimetres.
   */
  [[nodiscard]] double currentStepSize() const;

  // ---- Connection group ---------------------------------------------------
  QGroupBox*   grp_connection_{nullptr};  ///< Connection section.
  QComboBox*   cmb_port_{nullptr};        ///< Port/device selector.
  QLabel*      lbl_status_{nullptr};      ///< Coloured dot + state text.
  QPushButton* btn_connect_{nullptr};     ///< Connect/Disconnect button.

  // ---- Jog group ----------------------------------------------------------
  QGroupBox*   grp_jog_{nullptr};          ///< Jog section.
  QComboBox*   cmb_step_size_{nullptr};    ///< Step size selector.
  QPushButton* btn_jog_y_plus_{nullptr};   ///< Jog +Y button.
  QPushButton* btn_jog_x_minus_{nullptr};  ///< Jog -X button.
  QPushButton* btn_home_{nullptr};         ///< Home all axes button.
  QPushButton* btn_jog_x_plus_{nullptr};   ///< Jog +X button.
  QPushButton* btn_jog_y_minus_{nullptr};  ///< Jog -Y button.
  QPushButton* btn_jog_z_plus_{nullptr};   ///< Jog +Z button.
  QPushButton* btn_jog_z_minus_{nullptr};  ///< Jog -Z button.

  // ---- Absolute position group --------------------------------------------
  QGroupBox*      grp_abs_position_{nullptr};  ///< Absolute position section.
  QDoubleSpinBox* spn_abs_x_{nullptr};         ///< Target X in mm.
  QDoubleSpinBox* spn_abs_y_{nullptr};         ///< Target Y in mm.
  QDoubleSpinBox* spn_abs_z_{nullptr};         ///< Target Z in mm.
  QPushButton*    btn_go_to_{nullptr};         ///< Execute move button.

  // ---- Speed group --------------------------------------------------------
  QGroupBox*      grp_speed_{nullptr};    ///< Speed section.
  QDoubleSpinBox* spn_speed_{nullptr};    ///< Travel speed in mm/s.
  QTimer*         timer_speed_{nullptr};  ///< 300 ms debounce timer.

  // ---- Status group -------------------------------------------------------
  QGroupBox*   grp_status_{nullptr};        ///< Status section.
  QLabel*      lbl_pos_x_{nullptr};         ///< Current X position (mono).
  QLabel*      lbl_pos_y_{nullptr};         ///< Current Y position (mono).
  QLabel*      lbl_pos_z_{nullptr};         ///< Current Z position (mono).
  QLabel*      lbl_state_text_{nullptr};    ///< Stage motion state text.

  // ---- Emergency stop -----------------------------------------------------
  QPushButton* btn_emergency_stop_{nullptr};  ///< Always-enabled stop button.

  // ---- State --------------------------------------------------------------
  /// Non-owning pointer to the attached stage controller.
  mwa::hardware::StageControllerInterface* controller_{nullptr};

  // ---- Constants ----------------------------------------------------------
  /// Minimum jog button size.
  static constexpr int kJogButtonMinW = 52;
  /// Minimum jog button height.
  static constexpr int kJogButtonMinH = 36;
  /// Emergency stop button height.
  static constexpr int kEmergencyStopHeight = 48;
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
  /// Color hex for moving/homing state.
  static constexpr auto kColorMoving       = "#2980B9";
};

}  // namespace mwa::gui
