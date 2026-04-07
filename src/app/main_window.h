/**
 * @file main_window.h
 * @brief Main application window for the MWA system.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Declares the MainWindow class which serves as the top-level shell of
 * the Microfluidics Workstation Automation application. It hosts all
 * device panels as dockable widgets, provides a central workspace for
 * the camera live view, and exposes all primary actions via menu bar
 * and toolbar.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QCloseEvent>
#include <QDockWidget>
#include <QLabel>
#include <QList>
#include <QMainWindow>
#include <QMenu>
#include <QMetaObject>
#include <QStackedWidget>
#include <QToolBar>

#include "core/logger.h"

// Forward declarations for device panels
namespace mwa::gui {
class LedPanel;
class PumpPanel;
class SignalPanel;
class CameraPanel;
class StagePanel;
class AnalysisPanel;
}  // namespace mwa::gui

// Forward declaration for session data model
namespace mwa::analysis {
class ExperimentSession;
}  // namespace mwa::analysis

// Forward declarations for device controller interfaces
namespace mwa::hardware {
class LedControllerInterface;
class PumpControllerInterface;
class SignalGeneratorControllerInterface;
class NetworkAnalyzerControllerInterface;
class CameraControllerInterface;
class StageControllerInterface;
}  // namespace mwa::hardware

namespace mwa::app {

/**
 * @class MainWindow
 * @brief Top-level application window shell for the MWA system.
 *
 * MainWindow provides the full application chrome: menu bar, main
 * toolbar, left dock area (for device panels), bottom dock area (for
 * the log panel), a stacked central widget (placeholder / camera view),
 * and a status bar. Dock geometry and toolbar visibility are persisted
 * via SettingsManager across sessions.
 *
 * All device-specific actions are disabled in this shell; they will be
 * wired to backend logic in subsequent sprints.
 *
 * @see mwa::core::Logger
 * @see mwa::core::SettingsManager
 */
class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  /**
   * @brief Construct the main window.
   *
   * Builds the full UI (menu bar, toolbar, docks, central widget,
   * status bar), restores persisted geometry and dock state via
   * SettingsManager, and connects the Logger::newLogEntry() signal
   * to update the status bar last-event label.
   *
   * @param parent Optional parent widget; usually nullptr for a
   *               top-level window.
   */
  explicit MainWindow(QWidget* parent = nullptr);

  /**
   * @brief Destroy the main window.
   */
  ~MainWindow() override;

 protected:
  /**
   * @brief Handle window close events.
   *
   * Persists geometry and dock state via SettingsManager before
   * accepting the close event. If an experiment is running, prompts
   * the user for confirmation before closing.
   *
   * @param event The close event to accept or ignore.
   */
  void closeEvent(QCloseEvent* event) override;

 private slots:
  /**
   * @brief Update the status bar last-event label on a new log entry.
   *
   * @param entry The new log entry emitted by Logger.
   */
  void onNewLogEntry(const mwa::core::LogEntry& entry);

  /**
   * @brief Handle File > Exit action.
   *
   * Calls close(), which triggers closeEvent(). If an experiment is
   * running, the user is prompted before the window closes.
   */
  void onActionExit();

  /**
   * @brief Handle Help > About MWA action.
   *
   * Displays an About dialog via QMessageBox::about() with the
   * application name, version, and license summary.
   */
  void onActionAbout();

  /**
   * @brief Handle Experiment > Start action (F5).
   *
   * Opens NewSessionDialog. On acceptance, creates an ExperimentSession,
   * wires all device telemetry signals into it, and begins recording.
   */
  void onActionStartExperiment();

  /**
   * @brief Handle Experiment > Stop action (F6).
   *
   * Disconnects recording signals, ends the active session, and prompts
   * the user to save the session to a JSON file.
   */
  void onActionStopExperiment();

  /**
   * @brief Update UI to reflect a recording session has started.
   *
   * @param name The session name passed to ExperimentSession::start().
   */
  void onSessionStarted(const QString& name);

  /**
   * @brief Update UI to reflect the active session has ended.
   */
  void onSessionEnded();

  /**
   * @brief Forward a completed VNA sweep into the active session.
   *
   * Called when NetworkAnalyzerControllerInterface::measurementComplete()
   * fires. Reads trace data from the controller and calls
   * ExperimentSession::addVnaMeasurement().
   */
  void onVnaMeasurementComplete();

 private:
  // ---- Setup helpers (called once from constructor) ----

  /**
   * @brief Create and configure all QAction objects.
   */
  void createActions();

  /**
   * @brief Build the menu bar from the created actions.
   */
  void createMenus();

  /**
   * @brief Build the main toolbar from the created actions.
   */
  void createToolbar();

  /**
   * @brief Build the central stacked widget with placeholder.
   */
  void createCentralWidget();

  /**
   * @brief Build the left and bottom dock widgets.
   */
  void createDocks();

  /**
   * @brief Build the status bar with its three labels.
   */
  void createStatusBar();

  /**
   * @brief Create mock controllers and device panels in left dock area.
   */
  void createDevicePanels();

  /**
   * @brief Connect all signals and slots after widgets are created.
   */
  void connectSignals();

  /**
   * @brief Restore window geometry and dock state from SettingsManager.
   */
  void restoreSettings();

  /**
   * @brief Persist window geometry and dock state via SettingsManager.
   */
  void saveSettings();

  /**
   * @brief Wire all device telemetry signals into the active session.
   *
   * Stores each connection in recording_connections_ so they can be
   * cleanly disconnected by disconnectRecordingSignals().
   *
   * @pre active_session_ must be non-null and active.
   */
  void connectRecordingSignals();

  /**
   * @brief Disconnect all device-to-session telemetry connections.
   *
   * Disconnects every connection stored in recording_connections_ and
   * clears the list.
   */
  void disconnectRecordingSignals();

  /**
   * @brief Prompt the user to save the just-ended session to a JSON file.
   *
   * Shows a Save/Discard dialog. If the user chooses Save, opens a file
   * dialog and writes the session via SessionSerializer. Persists the
   * chosen path to SettingsManager.
   *
   * @pre active_session_ must be non-null and ended (not active).
   */
  void promptSaveSession();

  // ---- Actions ----

  // File menu
  QAction* action_exit_{nullptr};

  // View menu
  QAction* action_toggle_left_dock_{nullptr};
  QAction* action_toggle_bottom_dock_{nullptr};
  QAction* action_toggle_analysis_panel_{nullptr};
  QAction* action_toggle_toolbar_{nullptr};
  QAction* action_toggle_status_bar_{nullptr};

  // Devices menu
  QAction* action_connect_all_{nullptr};
  QAction* action_disconnect_all_{nullptr};
  QAction* action_device_settings_{nullptr};

  // Experiment menu
  QAction* action_new_experiment_{nullptr};
  QAction* action_open_experiment_{nullptr};
  QAction* action_save_experiment_{nullptr};
  QAction* action_start_experiment_{nullptr};
  QAction* action_stop_experiment_{nullptr};

  // Tools menu
  QAction* action_preferences_{nullptr};
  QAction* action_export_log_{nullptr};

  // Help menu
  QAction* action_about_{nullptr};
  QAction* action_about_qt_{nullptr};

  // ---- Widgets ----

  QToolBar* main_toolbar_{nullptr};
  QStackedWidget* central_stack_{nullptr};

  QDockWidget* dock_device_panels_{nullptr};
  QDockWidget* dock_log_panel_{nullptr};

  // Device panel docks
  QDockWidget* dock_led_panel_{nullptr};
  QDockWidget* dock_pump_panel_{nullptr};
  QDockWidget* dock_signal_panel_{nullptr};
  QDockWidget* dock_camera_panel_{nullptr};
  QDockWidget* dock_stage_panel_{nullptr};
  QDockWidget* dock_analysis_panel_{nullptr};

  // Device panels (owned by their dock widgets)
  mwa::gui::LedPanel* led_panel_{nullptr};
  mwa::gui::PumpPanel* pump_panel_{nullptr};
  mwa::gui::SignalPanel* signal_panel_{nullptr};
  mwa::gui::CameraPanel* camera_panel_{nullptr};
  mwa::gui::StagePanel* stage_panel_{nullptr};
  mwa::gui::AnalysisPanel* analysis_panel_{nullptr};

  // Device controllers (owned by this window; currently mock implementations)
  mwa::hardware::LedControllerInterface* led_controller_{nullptr};
  mwa::hardware::PumpControllerInterface* pump_controller_{nullptr};
  mwa::hardware::SignalGeneratorControllerInterface*
      sig_gen_controller_{nullptr};
  mwa::hardware::NetworkAnalyzerControllerInterface*
      net_analyzer_controller_{nullptr};
  mwa::hardware::CameraControllerInterface* camera_controller_{nullptr};
  mwa::hardware::StageControllerInterface* stage_controller_{nullptr};

  // Status bar labels (owned by the status bar via addWidget)
  QLabel* lbl_device_summary_{nullptr};
  QLabel* lbl_last_event_{nullptr};
  QLabel* lbl_recording_indicator_{nullptr};
  QLabel* lbl_version_{nullptr};

  // Session recording state
  /// Active experiment session; null when not recording.
  mwa::analysis::ExperimentSession* active_session_{nullptr};
  /// Device-to-session signal connections active during recording.
  QList<QMetaObject::Connection> recording_connections_;
};

}  // namespace mwa::app
