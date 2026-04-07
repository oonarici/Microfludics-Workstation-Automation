/**
 * @file main_window.cpp
 * @brief Main application window implementation for the MWA system.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Implements the MainWindow shell: menu bar, toolbar, dock areas,
 * central widget, and status bar. Settings are persisted via
 * SettingsManager. Logger signals update the status bar in real time.
 *
 * @copyright LGPL-3.0-or-later
 */

#include "app/main_window.h"

#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QKeySequence>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QVBoxLayout>

#include "analysis/experiment_session.h"
#include "analysis/session_serializer.h"
#include "core/settings_manager.h"
#include "gui/dialogs/new_session_dialog.h"
#include "hardware/camera/camera_controller_interface.h"
#include "hardware/led/led_controller_interface.h"
#include "hardware/network_analyzer/network_analyzer_controller_interface.h"
#include "hardware/pump/pump_controller_interface.h"
#include "hardware/signal_generator/signal_generator_controller_interface.h"
#include "hardware/stage/stage_controller_interface.h"
#include "gui/panels/analysis_panel.h"
#include "gui/panels/camera_panel.h"
#include "gui/panels/led_panel.h"
#include "gui/panels/pump_panel.h"
#include "gui/panels/signal_panel.h"
#include "gui/panels/stage_panel.h"
#include "gui/widgets/device_status_dashboard.h"
#include "gui/widgets/log_panel.h"
#include "hardware/camera/mock_camera_controller.h"
#include "hardware/led/mock_led_controller.h"
#include "hardware/network_analyzer/mock_network_analyzer_controller.h"
#include "hardware/pump/mock_pump_controller.h"
#include "hardware/signal_generator/mock_signal_generator_controller.h"
#include "hardware/stage/mock_stage_controller.h"

namespace mwa::app {

// ---- Constants --------------------------------------------------------

/// Settings group used for MainWindow geometry persistence.
static constexpr const char* kSettingsGroup = "MainWindow";
/// Settings key for window geometry bytes.
static constexpr const char* kKeyGeometry = "geometry";
/// Settings key for dock/toolbar state bytes.
static constexpr const char* kKeyState = "state";
/// Settings key for main toolbar visibility.
static constexpr const char* kKeyToolbarVisible = "toolbarVisible";
/// Settings key for status bar visibility.
static constexpr const char* kKeyStatusBarVisible = "statusBarVisible";

// ---- Constructor / Destructor -----------------------------------------

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
  setWindowTitle(
      QStringLiteral("MWA \u2014 Microfluidics Workstation Automation"));
  setMinimumSize(1024, 768);

  createActions();
  createMenus();
  createToolbar();
  createCentralWidget();
  createDocks();
  createDevicePanels();
  createStatusBar();
  connectSignals();
  restoreSettings();

  mwa::core::Logger::instance().logInfo(
      QStringLiteral("Application started"), QStringLiteral("MainWindow"));
}

MainWindow::~MainWindow() = default;

// ---- closeEvent -------------------------------------------------------

void MainWindow::closeEvent(QCloseEvent* event) {
  if (active_session_ && active_session_->isActive()) {
    QMessageBox msgbox(this);
    msgbox.setWindowTitle(QStringLiteral("Recording In Progress"));
    msgbox.setText(
        QStringLiteral(
            "A session is currently recording.\n"
            "Stop recording and close?"));
    msgbox.setIcon(QMessageBox::Warning);
    auto* btn_stop = msgbox.addButton(
        QStringLiteral("Stop && Close"), QMessageBox::AcceptRole);
    msgbox.addButton(QMessageBox::Cancel);
    msgbox.setDefaultButton(btn_stop);
    msgbox.exec();

    if (msgbox.clickedButton() != btn_stop) {
      event->ignore();
      return;
    }

    disconnectRecordingSignals();
    active_session_->end();
    active_session_->deleteLater();
    active_session_ = nullptr;
  }

  saveSettings();
  event->accept();
}

// ---- Private slots ----------------------------------------------------

void MainWindow::onNewLogEntry(const mwa::core::LogEntry& entry) {
  lbl_last_event_->setText(
      QStringLiteral("Last event: ") + entry.message);
}

void MainWindow::onActionExit() {
  close();
}

void MainWindow::onActionAbout() {
  QMessageBox::about(
      this,
      QStringLiteral("About MWA"),
      QStringLiteral(
          "<b>MWA \u2014 Microfluidics Workstation Automation</b>"
          "<br>Version: %1"
          "<br><br>A cross-platform desktop application for controlling "
          "microfluidic experiment workstations."
          "<br><br>Qt version: %2"
          "<br><br>License: LGPL-3.0-or-later")
          .arg(QCoreApplication::applicationVersion(),
               QStringLiteral(QT_VERSION_STR)));
}

// ---- createActions ----------------------------------------------------

void MainWindow::createActions() {
  // ---- File ----
  action_exit_ = new QAction(QStringLiteral("E&xit"), this);
  action_exit_->setObjectName(QStringLiteral("actionExit"));
#ifdef Q_OS_MAC
  action_exit_->setShortcut(QKeySequence(QStringLiteral("Ctrl+Q")));
#else
  action_exit_->setShortcut(QKeySequence(QStringLiteral("Alt+F4")));
#endif
  action_exit_->setStatusTip(QStringLiteral("Exit the application"));

  // ---- View ----
  action_toggle_left_dock_ =
      new QAction(QStringLiteral("&Device Panels"), this);
  action_toggle_left_dock_->setObjectName(
      QStringLiteral("actionToggleLeftDock"));
  action_toggle_left_dock_->setCheckable(true);
  action_toggle_left_dock_->setChecked(true);

  action_toggle_bottom_dock_ =
      new QAction(QStringLiteral("&Log Panel"), this);
  action_toggle_bottom_dock_->setObjectName(
      QStringLiteral("actionToggleBottomDock"));
  action_toggle_bottom_dock_->setCheckable(true);
  action_toggle_bottom_dock_->setChecked(true);

  action_toggle_analysis_panel_ =
      new QAction(QStringLiteral("&Analysis Panel"), this);
  action_toggle_analysis_panel_->setObjectName(
      QStringLiteral("actionToggleAnalysisPanel"));
  action_toggle_analysis_panel_->setCheckable(true);
  action_toggle_analysis_panel_->setChecked(true);

  action_toggle_toolbar_ =
      new QAction(QStringLiteral("&Toolbar"), this);
  action_toggle_toolbar_->setObjectName(
      QStringLiteral("actionToggleToolbar"));
  action_toggle_toolbar_->setCheckable(true);
  action_toggle_toolbar_->setChecked(true);

  action_toggle_status_bar_ =
      new QAction(QStringLiteral("&Status Bar"), this);
  action_toggle_status_bar_->setObjectName(
      QStringLiteral("actionToggleStatusBar"));
  action_toggle_status_bar_->setCheckable(true);
  action_toggle_status_bar_->setChecked(true);

  // ---- Devices ----
  action_connect_all_ =
      new QAction(QStringLiteral("Connect &All Devices"), this);
  action_connect_all_->setObjectName(QStringLiteral("actionConnectAll"));
  action_connect_all_->setShortcut(
      QKeySequence(QStringLiteral("Ctrl+Shift+C")));
  action_connect_all_->setToolTip(
      QStringLiteral("Connect All Devices (Ctrl+Shift+C)"));
  action_connect_all_->setEnabled(false);

  action_disconnect_all_ =
      new QAction(QStringLiteral("&Disconnect All Devices"), this);
  action_disconnect_all_->setObjectName(
      QStringLiteral("actionDisconnectAll"));
  action_disconnect_all_->setShortcut(
      QKeySequence(QStringLiteral("Ctrl+Shift+D")));
  action_disconnect_all_->setToolTip(
      QStringLiteral("Disconnect All Devices (Ctrl+Shift+D)"));
  action_disconnect_all_->setEnabled(false);

  action_device_settings_ =
      new QAction(QStringLiteral("Device &Settings..."), this);
  action_device_settings_->setObjectName(
      QStringLiteral("actionDeviceSettings"));
  action_device_settings_->setEnabled(false);

  // ---- Experiment ----
  action_new_experiment_ =
      new QAction(QStringLiteral("&New Experiment"), this);
  action_new_experiment_->setObjectName(
      QStringLiteral("actionNewExperiment"));
  action_new_experiment_->setShortcut(
      QKeySequence(QStringLiteral("Ctrl+N")));
  action_new_experiment_->setToolTip(
      QStringLiteral("New Experiment (Ctrl+N)"));
  action_new_experiment_->setEnabled(false);

  action_open_experiment_ =
      new QAction(QStringLiteral("&Open Experiment..."), this);
  action_open_experiment_->setObjectName(
      QStringLiteral("actionOpenExperiment"));
  action_open_experiment_->setShortcut(
      QKeySequence(QStringLiteral("Ctrl+O")));
  action_open_experiment_->setToolTip(
      QStringLiteral("Open Experiment (Ctrl+O)"));
  action_open_experiment_->setEnabled(false);

  action_save_experiment_ =
      new QAction(QStringLiteral("&Save Experiment"), this);
  action_save_experiment_->setObjectName(
      QStringLiteral("actionSaveExperiment"));
  action_save_experiment_->setShortcut(
      QKeySequence(QStringLiteral("Ctrl+S")));
  action_save_experiment_->setToolTip(
      QStringLiteral("Save Experiment (Ctrl+S)"));
  action_save_experiment_->setEnabled(false);

  action_start_experiment_ =
      new QAction(QStringLiteral("&Start"), this);
  action_start_experiment_->setObjectName(
      QStringLiteral("actionStartExperiment"));
  action_start_experiment_->setShortcut(QKeySequence(Qt::Key_F5));
  action_start_experiment_->setToolTip(
      QStringLiteral("Start Experiment (F5)"));
  action_start_experiment_->setEnabled(true);

  action_stop_experiment_ =
      new QAction(QStringLiteral("S&top"), this);
  action_stop_experiment_->setObjectName(
      QStringLiteral("actionStopExperiment"));
  action_stop_experiment_->setShortcut(QKeySequence(Qt::Key_F6));
  action_stop_experiment_->setToolTip(
      QStringLiteral("Stop Experiment (F6)"));
  action_stop_experiment_->setEnabled(false);

  // ---- Tools ----
  action_preferences_ =
      new QAction(QStringLiteral("&Preferences..."), this);
  action_preferences_->setObjectName(QStringLiteral("actionPreferences"));
  action_preferences_->setShortcut(
      QKeySequence(QStringLiteral("Ctrl+,")));
  action_preferences_->setEnabled(false);

  action_export_log_ =
      new QAction(QStringLiteral("&Export Log..."), this);
  action_export_log_->setObjectName(QStringLiteral("actionExportLog"));
  action_export_log_->setEnabled(false);

  // ---- Help ----
  action_about_ = new QAction(QStringLiteral("&About MWA..."), this);
  action_about_->setObjectName(QStringLiteral("actionAbout"));
  action_about_->setShortcut(QKeySequence(Qt::Key_F1));
  action_about_->setToolTip(QStringLiteral("About MWA"));

  action_about_qt_ = new QAction(QStringLiteral("About &Qt..."), this);
  action_about_qt_->setObjectName(QStringLiteral("actionAboutQt"));
}

// ---- createMenus ------------------------------------------------------

void MainWindow::createMenus() {
  // File
  auto* menu_file = menuBar()->addMenu(QStringLiteral("&File"));
  menu_file->setObjectName(QStringLiteral("menuFile"));
  menu_file->addAction(action_exit_);

  // View
  auto* menu_view = menuBar()->addMenu(QStringLiteral("&View"));
  menu_view->setObjectName(QStringLiteral("menuView"));
  menu_view->addAction(action_toggle_left_dock_);
  menu_view->addAction(action_toggle_bottom_dock_);
  menu_view->addAction(action_toggle_analysis_panel_);
  menu_view->addSeparator();
  menu_view->addAction(action_toggle_toolbar_);
  menu_view->addAction(action_toggle_status_bar_);

  // Devices
  auto* menu_devices = menuBar()->addMenu(QStringLiteral("&Devices"));
  menu_devices->setObjectName(QStringLiteral("menuDevices"));
  menu_devices->addAction(action_connect_all_);
  menu_devices->addAction(action_disconnect_all_);
  menu_devices->addSeparator();
  menu_devices->addAction(action_device_settings_);

  // Experiment
  auto* menu_experiment =
      menuBar()->addMenu(QStringLiteral("&Experiment"));
  menu_experiment->setObjectName(QStringLiteral("menuExperiment"));
  menu_experiment->addAction(action_new_experiment_);
  menu_experiment->addAction(action_open_experiment_);
  menu_experiment->addAction(action_save_experiment_);
  menu_experiment->addSeparator();
  menu_experiment->addAction(action_start_experiment_);
  menu_experiment->addAction(action_stop_experiment_);

  // Tools
  auto* menu_tools = menuBar()->addMenu(QStringLiteral("&Tools"));
  menu_tools->setObjectName(QStringLiteral("menuTools"));
  menu_tools->addAction(action_preferences_);
  menu_tools->addAction(action_export_log_);

  // Help
  auto* menu_help = menuBar()->addMenu(QStringLiteral("&Help"));
  menu_help->setObjectName(QStringLiteral("menuHelp"));
  menu_help->addAction(action_about_);
  menu_help->addAction(action_about_qt_);
}

// ---- createToolbar ----------------------------------------------------

void MainWindow::createToolbar() {
  main_toolbar_ = addToolBar(QStringLiteral("Main Toolbar"));
  main_toolbar_->setObjectName(QStringLiteral("mainToolbar"));
  main_toolbar_->setMovable(false);
  main_toolbar_->setIconSize(QSize(24, 24));

  main_toolbar_->addAction(action_new_experiment_);
  main_toolbar_->addAction(action_open_experiment_);
  main_toolbar_->addAction(action_save_experiment_);
  main_toolbar_->addSeparator();
  main_toolbar_->addAction(action_connect_all_);
  main_toolbar_->addAction(action_disconnect_all_);
  main_toolbar_->addSeparator();
  main_toolbar_->addAction(action_start_experiment_);
  main_toolbar_->addAction(action_stop_experiment_);
  main_toolbar_->addSeparator();
  main_toolbar_->addAction(action_about_);
}

// ---- createCentralWidget ----------------------------------------------

void MainWindow::createCentralWidget() {
  central_stack_ = new QStackedWidget(this);
  central_stack_->setObjectName(QStringLiteral("centralStack"));

  // Index 0 — placeholder (no camera connected)
  auto* placeholder_widget = new QWidget(central_stack_);
  placeholder_widget->setObjectName(QStringLiteral("placeholderWidget"));
  placeholder_widget->setStyleSheet(
      QStringLiteral("background-color: #2C3E50;"));

  auto* layout = new QVBoxLayout(placeholder_widget);

  auto* lbl_no_camera = new QLabel(
      QStringLiteral(
          "No camera connected\n"
          "Connect a camera to view live feed."),
      placeholder_widget);
  lbl_no_camera->setObjectName(QStringLiteral("lblNoCameraText"));
  lbl_no_camera->setAlignment(Qt::AlignCenter);
  lbl_no_camera->setStyleSheet(
      QStringLiteral("color: #95A5A6; font-size: 14pt;"));

  layout->addWidget(lbl_no_camera);

  central_stack_->addWidget(placeholder_widget);  // index 0
  setCentralWidget(central_stack_);
}

// ---- createDocks ------------------------------------------------------

void MainWindow::createDocks() {
  // Left dock — device panels placeholder
  dock_device_panels_ = new QDockWidget(
      QStringLiteral("Device Panels"), this);
  dock_device_panels_->setObjectName(QStringLiteral("dockDevicePanels"));
  dock_device_panels_->setFeatures(
      QDockWidget::DockWidgetClosable |
      QDockWidget::DockWidgetMovable  |
      QDockWidget::DockWidgetFloatable);
  dock_device_panels_->setMinimumWidth(280);
  dock_device_panels_->setMaximumWidth(400);

  auto* dashboard = new mwa::gui::DeviceStatusDashboard(this);
  dock_device_panels_->setWidget(dashboard);
  addDockWidget(Qt::LeftDockWidgetArea, dock_device_panels_);

  // Bottom dock — log panel placeholder
  dock_log_panel_ = new QDockWidget(QStringLiteral("Log"), this);
  dock_log_panel_->setObjectName(QStringLiteral("dockLogPanel"));
  dock_log_panel_->setFeatures(
      QDockWidget::DockWidgetClosable |
      QDockWidget::DockWidgetMovable  |
      QDockWidget::DockWidgetFloatable);
  dock_log_panel_->setMinimumHeight(100);

  auto* log_panel = new mwa::gui::LogPanel(this);
  dock_log_panel_->setWidget(log_panel);
  addDockWidget(Qt::BottomDockWidgetArea, dock_log_panel_);
  resizeDocks({dock_log_panel_}, {150}, Qt::Vertical);
}

// ---- createDevicePanels -----------------------------------------------

void MainWindow::createDevicePanels() {
  // Create device controllers (mock implementations for now).
  led_controller_ = new mwa::hardware::MockLedController(this);
  pump_controller_ = new mwa::hardware::MockPumpController(this);
  sig_gen_controller_ =
      new mwa::hardware::MockSignalGeneratorController(this);
  net_analyzer_controller_ =
      new mwa::hardware::MockNetworkAnalyzerController(this);
  camera_controller_ =
      new mwa::hardware::MockCameraController(this);
  stage_controller_ =
      new mwa::hardware::MockStageController(this);

  // Helper: create a device panel dock widget with standard features.
  auto makeDock = [this](const QString& title,
                         const QString& object_name,
                         QWidget* panel) {
    auto* dock = new QDockWidget(title, this);
    dock->setObjectName(object_name);
    dock->setFeatures(QDockWidget::DockWidgetClosable |
                      QDockWidget::DockWidgetMovable |
                      QDockWidget::DockWidgetFloatable);
    dock->setWidget(panel);
    addDockWidget(Qt::LeftDockWidgetArea, dock);
    return dock;
  };

  led_panel_ = new mwa::gui::LedPanel(this);
  led_panel_->setController(led_controller_);
  dock_led_panel_ = makeDock(
      QStringLiteral("LED"),
      QStringLiteral("dockLedPanel"), led_panel_);

  pump_panel_ = new mwa::gui::PumpPanel(this);
  pump_panel_->setController(pump_controller_);
  dock_pump_panel_ = makeDock(
      QStringLiteral("Syringe Pump"),
      QStringLiteral("dockPumpPanel"), pump_panel_);

  signal_panel_ = new mwa::gui::SignalPanel(this);
  signal_panel_->setSignalGeneratorController(sig_gen_controller_);
  signal_panel_->setNetworkAnalyzerController(
      net_analyzer_controller_);
  dock_signal_panel_ = makeDock(
      QStringLiteral("Signal / NA"),
      QStringLiteral("dockSignalPanel"), signal_panel_);

  camera_panel_ = new mwa::gui::CameraPanel(this);
  camera_panel_->setController(camera_controller_);
  dock_camera_panel_ = makeDock(
      QStringLiteral("Camera"),
      QStringLiteral("dockCameraPanel"), camera_panel_);

  stage_panel_ = new mwa::gui::StagePanel(this);
  stage_panel_->setController(stage_controller_);
  dock_stage_panel_ = makeDock(
      QStringLiteral("XYZ Stage"),
      QStringLiteral("dockStagePanel"), stage_panel_);

  // Analysis panel — right dock area, allows Left/Right/Bottom
  analysis_panel_ = new mwa::gui::AnalysisPanel(this);
  dock_analysis_panel_ = new QDockWidget(
      QStringLiteral("Analysis"), this);
  dock_analysis_panel_->setObjectName(
      QStringLiteral("dockAnalysisPanel"));
  dock_analysis_panel_->setFeatures(
      QDockWidget::DockWidgetClosable  |
      QDockWidget::DockWidgetMovable   |
      QDockWidget::DockWidgetFloatable);
  dock_analysis_panel_->setAllowedAreas(
      Qt::LeftDockWidgetArea  |
      Qt::RightDockWidgetArea |
      Qt::BottomDockWidgetArea);
  dock_analysis_panel_->setMinimumWidth(320);
  dock_analysis_panel_->setWidget(analysis_panel_);
  addDockWidget(Qt::RightDockWidgetArea, dock_analysis_panel_);

  // Tab the device panels together in the left dock area
  tabifyDockWidget(dock_device_panels_, dock_led_panel_);
  tabifyDockWidget(dock_led_panel_, dock_pump_panel_);
  tabifyDockWidget(dock_pump_panel_, dock_signal_panel_);
  tabifyDockWidget(dock_signal_panel_, dock_camera_panel_);
  tabifyDockWidget(dock_camera_panel_, dock_stage_panel_);

  // Show the dashboard (first tab) by default
  dock_device_panels_->raise();
}

// ---- createStatusBar --------------------------------------------------

void MainWindow::createStatusBar() {
  lbl_device_summary_ = new QLabel(QStringLiteral("Devices: \u2013"));
  lbl_device_summary_->setObjectName(QStringLiteral("lblDeviceSummary"));
  lbl_device_summary_->setMinimumWidth(200);

  lbl_last_event_ = new QLabel(
      QStringLiteral("Last event: Application started"));
  lbl_last_event_->setObjectName(QStringLiteral("lblLastEvent"));

  lbl_recording_indicator_ = new QLabel(this);
  lbl_recording_indicator_->setObjectName(
      QStringLiteral("lblRecordingIndicator"));
  lbl_recording_indicator_->setStyleSheet(
      QStringLiteral(
          "color: #E74C3C; font-weight: bold; padding: 0 8px;"));
  lbl_recording_indicator_->setVisible(false);

  lbl_version_ = new QLabel(QStringLiteral("MWA v0.1.0"));
  lbl_version_->setObjectName(QStringLiteral("lblVersion"));

  statusBar()->addWidget(lbl_device_summary_);
  statusBar()->addWidget(lbl_last_event_, 1);
  statusBar()->addPermanentWidget(lbl_recording_indicator_);
  statusBar()->addPermanentWidget(lbl_version_);
}

// ---- connectSignals ---------------------------------------------------

void MainWindow::connectSignals() {
  // File menu
  connect(action_exit_, &QAction::triggered,
          this, &MainWindow::onActionExit);

  // View menu — toggle left dock (bidirectional)
  connect(action_toggle_left_dock_, &QAction::toggled,
          dock_device_panels_, &QDockWidget::setVisible);
  connect(dock_device_panels_, &QDockWidget::visibilityChanged,
          action_toggle_left_dock_, &QAction::setChecked);

  // View menu — toggle bottom dock (bidirectional)
  connect(action_toggle_bottom_dock_, &QAction::toggled,
          dock_log_panel_, &QDockWidget::setVisible);
  connect(dock_log_panel_, &QDockWidget::visibilityChanged,
          action_toggle_bottom_dock_, &QAction::setChecked);

  // View menu — toggle analysis panel (bidirectional)
  connect(action_toggle_analysis_panel_, &QAction::toggled,
          dock_analysis_panel_, &QDockWidget::setVisible);
  connect(dock_analysis_panel_, &QDockWidget::visibilityChanged,
          action_toggle_analysis_panel_, &QAction::setChecked);

  // View menu — toggle toolbar (bidirectional)
  connect(action_toggle_toolbar_, &QAction::toggled,
          main_toolbar_, &QToolBar::setVisible);
  connect(main_toolbar_, &QToolBar::visibilityChanged,
          action_toggle_toolbar_, &QAction::setChecked);

  // View menu — toggle status bar
  connect(action_toggle_status_bar_, &QAction::toggled,
          statusBar(), &QStatusBar::setVisible);

  // Help menu
  connect(action_about_, &QAction::triggered,
          this, &MainWindow::onActionAbout);
  connect(action_about_qt_, &QAction::triggered,
          qApp, &QApplication::aboutQt);

  // Logger integration — update last-event label
  connect(&mwa::core::Logger::instance(), &mwa::core::Logger::newLogEntry,
          this, &MainWindow::onNewLogEntry);

  // Experiment recording
  connect(action_start_experiment_, &QAction::triggered,
          this, &MainWindow::onActionStartExperiment);
  connect(action_stop_experiment_, &QAction::triggered,
          this, &MainWindow::onActionStopExperiment);
}

// ---- restoreSettings --------------------------------------------------

void MainWindow::restoreSettings() {
  auto& settings = mwa::core::SettingsManager::instance();

  const QString grp = QLatin1String(kSettingsGroup);

  const QVariant geo = settings.value(
      grp, QLatin1String(kKeyGeometry));
  if (geo.isValid()) {
    restoreGeometry(geo.toByteArray());
  }

  const QVariant state = settings.value(
      grp, QLatin1String(kKeyState));
  if (state.isValid()) {
    restoreState(state.toByteArray());
  }

  const QVariant toolbar_visible = settings.value(
      grp, QLatin1String(kKeyToolbarVisible), true);
  main_toolbar_->setVisible(toolbar_visible.toBool());
  action_toggle_toolbar_->setChecked(toolbar_visible.toBool());

  const QVariant status_bar_visible = settings.value(
      grp, QLatin1String(kKeyStatusBarVisible), true);
  statusBar()->setVisible(status_bar_visible.toBool());
  action_toggle_status_bar_->setChecked(status_bar_visible.toBool());
}

// ---- saveSettings -----------------------------------------------------

void MainWindow::saveSettings() {
  auto& settings = mwa::core::SettingsManager::instance();
  const QString grp = QLatin1String(kSettingsGroup);

  settings.setValue(grp, QLatin1String(kKeyGeometry), saveGeometry());
  settings.setValue(grp, QLatin1String(kKeyState), saveState());
  settings.setValue(grp, QLatin1String(kKeyToolbarVisible),
                    main_toolbar_->isVisible());
  settings.setValue(grp, QLatin1String(kKeyStatusBarVisible),
                    statusBar()->isVisible());

  settings.saveAll();
}

// ---- Session recording slots ------------------------------------------

void MainWindow::onActionStartExperiment() {
  mwa::gui::NewSessionDialog dialog(this);
  if (dialog.exec() != QDialog::Accepted) {
    return;
  }

  active_session_ = new mwa::analysis::ExperimentSession(this);
  connect(active_session_,
          &mwa::analysis::ExperimentSession::sessionStarted,
          this, &MainWindow::onSessionStarted);
  connect(active_session_,
          &mwa::analysis::ExperimentSession::sessionEnded,
          this, &MainWindow::onSessionEnded);

  active_session_->start(dialog.name(), dialog.description());
  connectRecordingSignals();
}

void MainWindow::onActionStopExperiment() {
  disconnectRecordingSignals();
  active_session_->end();
  promptSaveSession();
  active_session_->deleteLater();
  active_session_ = nullptr;
}

void MainWindow::onSessionStarted(const QString& name) {
  action_start_experiment_->setEnabled(false);
  action_stop_experiment_->setEnabled(true);
  lbl_recording_indicator_->setText(
      QStringLiteral("\u25cf REC  ") + name);
  lbl_recording_indicator_->setVisible(true);
}

void MainWindow::onSessionEnded() {
  action_start_experiment_->setEnabled(true);
  action_stop_experiment_->setEnabled(false);
  lbl_recording_indicator_->setVisible(false);
}

void MainWindow::onVnaMeasurementComplete() {
  active_session_->addVnaMeasurement(
      net_analyzer_controller_->startFrequency(),
      net_analyzer_controller_->stopFrequency(),
      net_analyzer_controller_->numPoints(),
      net_analyzer_controller_->traceFrequencies(),
      net_analyzer_controller_->traceMagnitudes());
}

// ---- Recording signal management --------------------------------------

void MainWindow::connectRecordingSignals() {
  auto* s = active_session_;

  recording_connections_ << connect(
      led_controller_,
      &mwa::hardware::LedControllerInterface::intensityChanged,
      this, [this, s](double percent) {
        s->addLedSample(led_controller_->isPowerOn(), percent);
      });
  recording_connections_ << connect(
      led_controller_,
      &mwa::hardware::LedControllerInterface::powerStateChanged,
      this, [this, s](bool on) {
        s->addLedSample(on, led_controller_->intensity());
      });
  recording_connections_ << connect(
      pump_controller_,
      &mwa::hardware::PumpControllerInterface::positionChanged,
      this, [this, s](double uL) {
        s->addPumpSample(uL, pump_controller_->flowRate());
      });
  recording_connections_ << connect(
      sig_gen_controller_,
      &mwa::hardware::SignalGeneratorControllerInterface::frequencyChanged,
      this, [this, s](double hz) {
        s->addSigGenSample(hz, sig_gen_controller_->amplitude());
      });
  recording_connections_ << connect(
      camera_controller_,
      &mwa::hardware::CameraControllerInterface::frameReady,
      s, &mwa::analysis::ExperimentSession::addCameraFrame);
  recording_connections_ << connect(
      stage_controller_,
      &mwa::hardware::StageControllerInterface::positionChanged,
      s, &mwa::analysis::ExperimentSession::addStageSample);
  recording_connections_ << connect(
      net_analyzer_controller_,
      &mwa::hardware::NetworkAnalyzerControllerInterface::measurementComplete,
      this, &MainWindow::onVnaMeasurementComplete);
}

void MainWindow::disconnectRecordingSignals() {
  for (const auto& conn : recording_connections_) {
    disconnect(conn);
  }
  recording_connections_.clear();
}

// ---- Save prompt ------------------------------------------------------

void MainWindow::promptSaveSession() {
  QMessageBox msgbox(this);
  msgbox.setWindowTitle(QStringLiteral("Save Session"));
  msgbox.setText(
      QStringLiteral("Session \"%1\" has ended.\nSave to file?")
          .arg(active_session_->name()));
  msgbox.setIcon(QMessageBox::Question);
  auto* btn_save =
      msgbox.addButton(QStringLiteral("Save\u2026"), QMessageBox::AcceptRole);
  auto* btn_discard =
      msgbox.addButton(QStringLiteral("Discard"), QMessageBox::DestructiveRole);
  msgbox.setDefaultButton(btn_save);
  msgbox.exec();

  if (msgbox.clickedButton() != btn_save) {
    Q_UNUSED(btn_discard)
    return;
  }

  const QString safe_name =
      active_session_->name().simplified()
          .replace(QLatin1Char(' '), QLatin1Char('_'));
  const QString date_str =
      active_session_->startTime().toString(
          QStringLiteral("yyyy-MM-dd"));
  const QString default_name =
      safe_name + QLatin1Char('_') + date_str +
      QStringLiteral(".json");

  auto& settings = mwa::core::SettingsManager::instance();
  const QString last_path =
      settings.value(QStringLiteral("Session"),
                     QStringLiteral("lastFilePath")).toString();
  const QString start_dir =
      last_path.isEmpty()
          ? QDir::homePath()
          : QFileInfo(last_path).absoluteDir().absolutePath();

  const QString path = QFileDialog::getSaveFileName(
      this,
      QStringLiteral("Save Session"),
      start_dir + QDir::separator() + default_name,
      QStringLiteral("MWA Session (*.json)"));

  if (path.isEmpty()) {
    return;
  }

  if (!mwa::analysis::SessionSerializer::save(*active_session_, path)) {
    QMessageBox::warning(
        this,
        QStringLiteral("Save Failed"),
        QStringLiteral("Could not write session file:\n%1\n\n%2")
            .arg(path,
                 mwa::analysis::SessionSerializer::lastError()));
    return;
  }

  settings.setValue(QStringLiteral("Session"),
                    QStringLiteral("lastFilePath"), path);
  settings.saveAll();
}

}  // namespace mwa::app
