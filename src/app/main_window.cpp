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
#include <QKeySequence>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QVBoxLayout>

#include "core/settings_manager.h"

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
  createStatusBar();
  connectSignals();
  restoreSettings();

  // Log the application start event.
  mwa::core::Logger::instance().logInfo(
      QStringLiteral("Application started"), QStringLiteral("MainWindow"));
}

MainWindow::~MainWindow() = default;

// ---- closeEvent -------------------------------------------------------

void MainWindow::closeEvent(QCloseEvent* event) {
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
  action_start_experiment_->setEnabled(false);

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
  placeholder_widget->setLayout(layout);

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

  auto* device_placeholder = new QWidget(dock_device_panels_);
  device_placeholder->setObjectName(
      QStringLiteral("wgtDevicePanelsPlaceholder"));
  device_placeholder->setStyleSheet(
      QStringLiteral("background-color: #ECF0F1;"));

  auto* device_layout = new QVBoxLayout(device_placeholder);
  auto* lbl_device = new QLabel(
      QStringLiteral("Device panels will appear here."),
      device_placeholder);
  lbl_device->setObjectName(
      QStringLiteral("lblDevicePanelsPlaceholder"));
  lbl_device->setAlignment(Qt::AlignCenter);
  lbl_device->setStyleSheet(
      QStringLiteral("color: #95A5A6; font-size: 12pt;"));
  device_layout->addWidget(lbl_device);
  device_placeholder->setLayout(device_layout);

  dock_device_panels_->setWidget(device_placeholder);
  addDockWidget(Qt::LeftDockWidgetArea, dock_device_panels_);

  // Bottom dock — log panel placeholder
  dock_log_panel_ = new QDockWidget(QStringLiteral("Log"), this);
  dock_log_panel_->setObjectName(QStringLiteral("dockLogPanel"));
  dock_log_panel_->setFeatures(
      QDockWidget::DockWidgetClosable |
      QDockWidget::DockWidgetMovable  |
      QDockWidget::DockWidgetFloatable);
  dock_log_panel_->setMinimumHeight(100);

  auto* log_placeholder = new QWidget(dock_log_panel_);
  log_placeholder->setObjectName(
      QStringLiteral("wgtLogPanelPlaceholder"));
  log_placeholder->setStyleSheet(
      QStringLiteral("background-color: #ECF0F1;"));

  auto* log_layout = new QVBoxLayout(log_placeholder);
  auto* lbl_log = new QLabel(
      QStringLiteral("Log panel will appear here (MWA-02-E)."),
      log_placeholder);
  lbl_log->setObjectName(QStringLiteral("lblLogPanelPlaceholder"));
  lbl_log->setAlignment(Qt::AlignCenter);
  lbl_log->setStyleSheet(
      QStringLiteral("color: #95A5A6; font-size: 12pt;"));
  log_layout->addWidget(lbl_log);
  log_placeholder->setLayout(log_layout);

  dock_log_panel_->setWidget(log_placeholder);
  addDockWidget(Qt::BottomDockWidgetArea, dock_log_panel_);
  resizeDocks({dock_log_panel_}, {150}, Qt::Vertical);
}

// ---- createStatusBar --------------------------------------------------

void MainWindow::createStatusBar() {
  lbl_device_summary_ = new QLabel(QStringLiteral("Devices: \u2013"));
  lbl_device_summary_->setObjectName(QStringLiteral("lblDeviceSummary"));
  lbl_device_summary_->setMinimumWidth(200);

  lbl_last_event_ = new QLabel(
      QStringLiteral("Last event: Application started"));
  lbl_last_event_->setObjectName(QStringLiteral("lblLastEvent"));

  lbl_version_ = new QLabel(QStringLiteral("MWA v0.1.0"));
  lbl_version_->setObjectName(QStringLiteral("lblVersion"));

  statusBar()->addWidget(lbl_device_summary_);
  statusBar()->addWidget(lbl_last_event_, 1);
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

}  // namespace mwa::app
