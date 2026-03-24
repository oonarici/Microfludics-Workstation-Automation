/**
 * @file signal_panel.h
 * @brief Combined Signal Generator and Network Analyzer control panel.
 * @author MWA Team
 * @date 2026-03-23
 *
 * Declares the SignalPanel widget, which provides a tabbed UI for
 * controlling both a signal/waveform generator and a network/impedance
 * analyzer. Tab 0 exposes output configuration, sweep configuration,
 * and output-status readouts for the signal generator. Tab 1 exposes
 * sweep configuration, S-parameter measurement triggering, and
 * measurement status for the network analyzer.
 *
 * The panel owns no hardware resources directly; all hardware
 * interaction is delegated to controller instances supplied via
 * setSignalGeneratorController() and setNetworkAnalyzerController().
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QWidget>

#include "gui/panels/panel_colors.h"
#include "hardware/network_analyzer/network_analyzer_controller_interface.h"
#include "hardware/signal_generator/signal_generator_controller_interface.h"

namespace mwa::gui {

/**
 * @class SignalPanel
 * @brief Tabbed widget for controlling a signal generator and a network
 *        analyzer.
 *
 * SignalPanel presents two tabs inside a shared connection group box:
 *  - **Signal Generator** — frequency/amplitude/waveform output
 *    configuration, frequency sweep setup, and a live output-status
 *    readout with output enable/disable toggle.
 *  - **Network Analyzer** — sweep frequency range and point count
 *    configuration, S-parameter measurement trigger, an indeterminate
 *    progress bar shown during measurement, and a measurement-status
 *    summary grid.
 *
 * Passing @c nullptr to either setter disables the corresponding tab.
 * The entire panel (both tabs) is disabled while the device is not
 * connected. All frequency values are stored and transmitted in hertz;
 * the helper convertToHz() converts from the selected unit combo index,
 * and formatFrequency() selects the most readable unit for display.
 *
 * @note Double-clicking any read-only display label copies its text
 *       to the system clipboard.
 *
 * @see mwa::hardware::SignalGeneratorControllerInterface
 * @see mwa::hardware::NetworkAnalyzerControllerInterface
 */
class SignalPanel : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Construct the signal panel with all sub-widgets.
   *
   * Builds the connection group box, the QTabWidget with both
   * device tabs, and wires all internal widget-to-widget connections.
   * Both tabs start disabled and the connection status shows
   * "Disconnected".
   *
   * @param parent Optional parent widget for Qt ownership.
   */
  explicit SignalPanel(QWidget* parent = nullptr);

  /**
   * @brief Attach a signal generator controller and wire signals.
   *
   * Disconnects any previously attached signal generator controller's
   * signals, then connects stateChanged(), frequencyChanged(),
   * amplitudeChanged(), waveformChanged(), and outputStateChanged()
   * from the new controller to slots in this panel. Tab 0 is disabled
   * when @p controller is @c nullptr.
   *
   * @param controller Pointer to the signal generator controller to
   *                   attach. May be @c nullptr to detach.
   */
  void setSignalGeneratorController(
      mwa::hardware::SignalGeneratorControllerInterface* controller);

  /**
   * @brief Attach a network analyzer controller and wire signals.
   *
   * Disconnects any previously attached network analyzer controller's
   * signals, then connects stateChanged(), measurementStarted(),
   * measurementComplete(), frequencyRangeChanged(), and
   * numPointsChanged() from the new controller to slots in this panel.
   * Tab 1 is disabled when @p controller is @c nullptr.
   *
   * @param controller Pointer to the network analyzer controller to
   *                   attach. May be @c nullptr to detach.
   */
  void setNetworkAnalyzerController(
      mwa::hardware::NetworkAnalyzerControllerInterface* controller);

 protected:
  /**
   * @brief Intercept double-click events on display labels for clipboard copy.
   *
   * Routes QEvent::MouseButtonDblClick events on the read-only display
   * labels (lbl_sig_freq_display_, lbl_sig_amp_display_,
   * lbl_sig_waveform_display_, lbl_na_sparam_display_) to the
   * corresponding clipboard-copy slots.
   *
   * @param watched The object that received the event.
   * @param event   The intercepted event.
   * @return @c true if the event was consumed, @c false otherwise.
   */
  bool eventFilter(QObject* watched, QEvent* event) override;

 private slots:
  // ---- Shared / Connection ------------------------------------------------

  /**
   * @brief Handle the Connect / Disconnect button click.
   *
   * Determines which controller is active based on the selected tab
   * and the current state of that controller. If disconnected,
   * calls connectDevice(). If connected, shows a confirmation dialog
   * before calling disconnectDevice().
   */
  void onConnectClicked();

  // ---- Signal Generator slots ---------------------------------------------

  /**
   * @brief Handle device state changes from the signal generator.
   *
   * Updates the connection status label and enables/disables Tab 0
   * controls accordingly.
   *
   * @param new_state Updated connection state.
   */
  void onSigStateChanged(
      mwa::hardware::DeviceInterface::DeviceState new_state);

  /**
   * @brief Send the current frequency spinbox value to the controller.
   *
   * Converts value + unit to Hz and calls setFrequency(). Triggered
   * by QDoubleSpinBox::valueChanged and QComboBox::currentIndexChanged
   * for the frequency controls.
   */
  void onSigFrequencyChanged();

  /**
   * @brief Send the amplitude spinbox value to the controller.
   *
   * Triggered by QDoubleSpinBox::valueChanged on spn_sig_amplitude_.
   *
   * @param volts New amplitude value in volts.
   */
  void onSigAmplitudeChanged(double volts);

  /**
   * @brief Send the selected waveform to the controller.
   *
   * Triggered by QComboBox::currentIndexChanged on cmb_sig_waveform_.
   *
   * @param index Combo index: 0=Sine, 1=Square, 2=Triangle.
   */
  void onSigWaveformChanged(int index);

  /**
   * @brief Apply the configured frequency sweep to the controller.
   *
   * Reads start, stop, and step values with their respective unit
   * combos, converts all to Hz, and calls configureSweep().
   */
  void onSigApplySweep();

  /**
   * @brief Toggle signal output enable state.
   *
   * Called when btn_sig_output_enable_ is toggled. Turning OFF shows
   * a QMessageBox confirmation dialog. Turning ON proceeds without
   * confirmation.
   *
   * @param checked @c true if the button was toggled to ON.
   */
  void onSigOutputToggled(bool checked);

  /**
   * @brief Update display labels from a confirmed frequency change.
   *
   * Blocks signals on spn_sig_frequency_ and cmb_sig_freq_unit_,
   * updates the spinbox/unit to the canonical Hz value, and refreshes
   * lbl_sig_freq_display_.
   *
   * @param hz Confirmed output frequency in hertz.
   */
  void onSigFrequencyConfirmed(double hz);

  /**
   * @brief Update the amplitude display from a confirmed controller value.
   *
   * Blocks signals on spn_sig_amplitude_ and updates
   * lbl_sig_amp_display_.
   *
   * @param volts Confirmed output amplitude in volts.
   */
  void onSigAmplitudeConfirmed(double volts);

  /**
   * @brief Update the waveform display from a confirmed controller value.
   *
   * Blocks signals on cmb_sig_waveform_ and updates
   * lbl_sig_waveform_display_.
   *
   * @param waveform Confirmed output waveform shape.
   */
  void onSigWaveformConfirmed(
      mwa::hardware::SignalGeneratorControllerInterface::Waveform waveform);

  /**
   * @brief Update the output enable button from a confirmed controller state.
   *
   * Blocks signals on btn_sig_output_enable_ and updates its text
   * and checked state.
   *
   * @param enabled @c true if output is now enabled.
   */
  void onSigOutputStateConfirmed(bool enabled);

  // ---- Network Analyzer slots ---------------------------------------------

  /**
   * @brief Handle device state changes from the network analyzer.
   *
   * Updates the measurement-status dot and text, and enables/disables
   * Tab 1 controls accordingly.
   *
   * @param new_state Updated connection state.
   */
  void onNaStateChanged(
      mwa::hardware::DeviceInterface::DeviceState new_state);

  /**
   * @brief Trigger an asynchronous S-parameter measurement.
   *
   * Disables btn_na_measure_, shows prg_na_measurement_, updates
   * state dot to "Measuring...", then calls measureSParameters().
   */
  void onNaMeasureClicked();

  /**
   * @brief React to measurement completion.
   *
   * Hides the progress bar, re-enables the measure button, reads
   * trace data from the controller, updates lbl_na_sparam_display_
   * with a summary, and sets measurement state to "Complete".
   */
  void onNaMeasurementComplete();

  /**
   * @brief Update the frequency range display from the controller.
   *
   * @param start New sweep start frequency in hertz.
   * @param stop  New sweep stop frequency in hertz.
   */
  void onNaFrequencyRangeChanged(double start, double stop);

  /**
   * @brief Update the points summary label from the controller.
   *
   * @param points New number of frequency measurement points.
   */
  void onNaNumPointsChanged(int points);

  // ---- Clipboard helpers --------------------------------------------------

  /**
   * @brief Copy the signal frequency display text to the clipboard.
   *
   * Connected to the doubleClicked pseudo-signal of
   * lbl_sig_freq_display_.
   */
  void onSigFreqDisplayDoubleClicked();

  /**
   * @brief Copy the signal amplitude display text to the clipboard.
   *
   * Connected to the doubleClicked pseudo-signal of
   * lbl_sig_amp_display_.
   */
  void onSigAmpDisplayDoubleClicked();

  /**
   * @brief Copy the signal waveform display text to the clipboard.
   *
   * Connected to the doubleClicked pseudo-signal of
   * lbl_sig_waveform_display_.
   */
  void onSigWaveformDisplayDoubleClicked();

  /**
   * @brief Copy the S-parameter display text to the clipboard.
   *
   * Connected to the doubleClicked pseudo-signal of
   * lbl_na_sparam_display_.
   */
  void onNaSparamDisplayDoubleClicked();

 private:
  // ---- Builder helpers ----------------------------------------------------

  /**
   * @brief Build and return the shared Connection group box.
   *
   * @return Pointer to the created QGroupBox (owned by this widget).
   */
  QGroupBox* createConnectionGroup();

  /**
   * @brief Build and return the Signal Generator tab widget.
   *
   * @return Pointer to the created QWidget (owned by tab_widget_).
   */
  QWidget* createSignalGeneratorTab();

  /**
   * @brief Build and return the Network Analyzer tab widget.
   *
   * @return Pointer to the created QWidget (owned by tab_widget_).
   */
  QWidget* createNetworkAnalyzerTab();

  /**
   * @brief Build the Signal Generator Output Configuration group box.
   *
   * @param parent Parent widget for ownership.
   * @return Pointer to the created QGroupBox.
   */
  QGroupBox* createSigOutputConfigGroup(QWidget* parent);

  /**
   * @brief Build the Signal Generator Sweep Configuration group box.
   *
   * @param parent Parent widget for ownership.
   * @return Pointer to the created QGroupBox.
   */
  QGroupBox* createSigSweepGroup(QWidget* parent);

  /**
   * @brief Build the Signal Generator Output Status group box.
   *
   * @param parent Parent widget for ownership.
   * @return Pointer to the created QGroupBox.
   */
  QGroupBox* createSigOutputStatusGroup(QWidget* parent);

  /**
   * @brief Build the Network Analyzer Sweep Configuration group box.
   *
   * @param parent Parent widget for ownership.
   * @return Pointer to the created QGroupBox.
   */
  QGroupBox* createNaSweepConfigGroup(QWidget* parent);

  /**
   * @brief Build the Network Analyzer Measurement group box.
   *
   * @param parent Parent widget for ownership.
   * @return Pointer to the created QGroupBox.
   */
  QGroupBox* createNaMeasurementGroup(QWidget* parent);

  /**
   * @brief Build the Network Analyzer Measurement Status group box.
   *
   * @param parent Parent widget for ownership.
   * @return Pointer to the created QGroupBox.
   */
  QGroupBox* createNaMeasStatusGroup(QWidget* parent);

  // ---- Utility ------------------------------------------------------------

  /**
   * @brief Create a QComboBox populated with Hz / kHz / MHz items.
   *
   * @param parent Parent widget for Qt ownership.
   * @param default_index Index to select on creation (0=Hz, 1=kHz,
   *                      2=MHz).
   * @return Pointer to the new QComboBox.
   */
  QComboBox* createFreqUnitCombo(QWidget* parent, int default_index);

  /**
   * @brief Convert a frequency value to hertz using the unit combo index.
   *
   * @param value      Raw frequency value as displayed in a spinbox.
   * @param unit_index Unit combo index: 0=Hz, 1=kHz, 2=MHz.
   * @return Frequency in hertz.
   */
  [[nodiscard]] double convertToHz(double value, int unit_index) const;

  /**
   * @brief Format a frequency in hertz for display, auto-selecting unit.
   *
   * Selects MHz when ≥ 1e6 Hz, kHz when ≥ 1e3 Hz, otherwise Hz.
   * Always shows 6 decimal places.
   *
   * @param hz Frequency in hertz.
   * @return Human-readable frequency string, e.g. "1.000000 MHz".
   */
  [[nodiscard]] QString formatFrequency(double hz) const;

  /**
   * @brief Apply the stylesheet for the connection status dot.
   *
   * @param new_state Current device state determining the dot colour.
   */
  void applyStatusStyle(
      mwa::hardware::DeviceInterface::DeviceState new_state);

  /**
   * @brief Update the NA measurement-state dot and text label.
   *
   * @param color   CSS hex color for the 12×12 dot.
   * @param text    Text for lbl_na_state_text_.
   */
  void setNaMeasurementState(const QString& color,
                             const QString& text);

  /**
   * @brief Enable or disable all controls in the signal generator tab.
   *
   * @param enabled @c true to enable controls.
   */
  void setSigTabEnabled(bool enabled);

  /**
   * @brief Enable or disable all controls in the network analyzer tab.
   *
   * @param enabled @c true to enable controls.
   */
  void setNaTabEnabled(bool enabled);

  // ---- Connection group widgets -------------------------------------------
  QGroupBox*   grp_connection_{nullptr};  ///< Shared connection section.
  QComboBox*   cmb_port_{nullptr};        ///< Editable GPIB/port selector.
  QLabel*      lbl_status_{nullptr};      ///< 14×14 coloured state dot.
  QPushButton* btn_connect_{nullptr};     ///< Connect/Disconnect button.

  // ---- Tab widget ---------------------------------------------------------
  QTabWidget* tab_widget_{nullptr};  ///< Hosts Signal Gen and NA tabs.

  // ---- Signal Generator: Output Configuration -----------------------------
  QGroupBox*      grp_sig_output_config_{nullptr};  ///< Output config group.
  QDoubleSpinBox* spn_sig_frequency_{nullptr};       ///< Freq value spinbox.
  QComboBox*      cmb_sig_freq_unit_{nullptr};       ///< Hz/kHz/MHz selector.
  QDoubleSpinBox* spn_sig_amplitude_{nullptr};       ///< Amplitude spinbox.
  QComboBox*      cmb_sig_waveform_{nullptr};        ///< Waveform selector.

  // ---- Signal Generator: Sweep Configuration ------------------------------
  QGroupBox*      grp_sig_sweep_{nullptr};             ///< Sweep config group.
  QDoubleSpinBox* spn_sig_sweep_start_{nullptr};       ///< Sweep start.
  QComboBox*      cmb_sig_sweep_start_unit_{nullptr};  ///< Sweep start unit.
  QDoubleSpinBox* spn_sig_sweep_stop_{nullptr};        ///< Sweep stop.
  QComboBox*      cmb_sig_sweep_stop_unit_{nullptr};   ///< Sweep stop unit.
  QDoubleSpinBox* spn_sig_sweep_step_{nullptr};        ///< Sweep step.
  QComboBox*      cmb_sig_sweep_step_unit_{nullptr};   ///< Sweep step unit.
  QPushButton*    btn_sig_apply_sweep_{nullptr};       ///< Apply sweep button.

  // ---- Signal Generator: Output Status ------------------------------------
  QGroupBox*   grp_sig_output_status_{nullptr};   ///< Output status group.
  QPushButton* btn_sig_output_enable_{nullptr};   ///< Checkable output toggle.
  QLabel*      lbl_sig_freq_display_{nullptr};    ///< Freq readout label.
  QLabel*      lbl_sig_amp_display_{nullptr};     ///< Amplitude readout label.
  QLabel*      lbl_sig_waveform_display_{nullptr};///< Waveform readout label.

  // ---- Network Analyzer: Sweep Configuration ------------------------------
  QGroupBox*      grp_na_sweep_config_{nullptr};        ///< NA sweep group.
  QDoubleSpinBox* spn_na_start_freq_{nullptr};          ///< NA start freq.
  QComboBox*      cmb_na_start_freq_unit_{nullptr};     ///< NA start unit.
  QDoubleSpinBox* spn_na_stop_freq_{nullptr};           ///< NA stop freq.
  QComboBox*      cmb_na_stop_freq_unit_{nullptr};      ///< NA stop unit.
  QSpinBox*       spn_na_points_{nullptr};               ///< Point count.

  // ---- Network Analyzer: Measurement --------------------------------------
  QGroupBox*    grp_na_measurement_{nullptr};      ///< Measurement group.
  QPushButton*  btn_na_measure_{nullptr};          ///< Trigger measurement.
  QLabel*       lbl_na_sparam_display_{nullptr};   ///< S-param data display.
  QProgressBar* prg_na_measurement_{nullptr};      ///< Indeterminate progress.

  // ---- Network Analyzer: Measurement Status -------------------------------
  QGroupBox* grp_na_meas_status_{nullptr};      ///< Measurement status group.
  QLabel*    lbl_na_state_dot_{nullptr};        ///< 12×12 coloured state dot.
  QLabel*    lbl_na_state_text_{nullptr};       ///< "Idle"/"Measuring…" text.
  QLabel*    lbl_na_range_summary_{nullptr};    ///< Frequency range summary.
  QLabel*    lbl_na_points_summary_{nullptr};   ///< Points count summary.

  // ---- Controller references (non-owning) ---------------------------------
  /// Non-owning pointer to the attached signal generator controller.
  mwa::hardware::SignalGeneratorControllerInterface* sig_controller_{
      nullptr};
  /// Non-owning pointer to the attached network analyzer controller.
  mwa::hardware::NetworkAnalyzerControllerInterface* na_controller_{
      nullptr};

  // ---- Constants ----------------------------------------------------------
  /// Frequency unit multipliers indexed by combo position.
  static constexpr double kUnitMultipliers[3] = {1.0, 1000.0, 1000000.0};

  /// Default frequency spinbox maximum (999999.999999 in chosen unit).
  static constexpr double kFreqMax    = 999999.999999;
  /// Default frequency spinbox minimum (1 µHz expressed in Hz).
  static constexpr double kFreqMin    = 0.000001;
  /// Frequency spinbox decimal places.
  static constexpr int    kFreqDecimals = 6;
};

}  // namespace mwa::gui
