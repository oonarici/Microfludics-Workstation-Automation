/**
 * @file test_pump_panel.cpp
 * @brief Adversarial unit tests for the PumpPanel widget.
 * @author MWA Team
 * @date 2026-03-23
 *
 * Tests cover: initial widget state, controller attachment/detachment,
 * signal forwarding, connection-state transitions (kDisconnected →
 * kConnecting → kConnected), infusion start/stop UI updates, progress-bar
 * behaviour driven by positionChanged, error-label visibility from
 * errorOccurred, and the is_refilling_ state-machine implicit in the UI
 * (refill button disabled while infusing, start disabled while refilling).
 *
 * @note QMessageBox confirmation dialogs (Stop, Refill, Disconnect) require
 *       interactive input and cannot be auto-accepted in a headless environment.
 *       Those code paths are therefore exercised via direct controller signal
 *       emission rather than button clicks.
 *
 * @copyright LGPL-3.0-or-later
 */

#include <QApplication>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QLCDNumber>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>

#include "gui/panels/pump_panel.h"
#include "hardware/pump/mock_pump_controller.h"

using mwa::gui::PumpPanel;
using mwa::hardware::MockPumpController;
using mwa::hardware::DeviceInterface;
using DeviceState = DeviceInterface::DeviceState;

static int   s_argc       = 1;
static char  s_app_name[] = "test_pump_panel";
static char* s_argv[]     = {s_app_name};

// ---------------------------------------------------------------------------
// Helper: drive the mock controller to kConnected synchronously.
// ---------------------------------------------------------------------------
static void connectSync(MockPumpController& ctrl) {
  ctrl.connectDevice();
  QTest::qWait(600);
}

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------

/**
 * @class TestPumpPanel
 * @brief Qt Test class exercising mwa::gui::PumpPanel.
 */
class TestPumpPanel : public QObject {
  Q_OBJECT

 private:
  QApplication*      app_{nullptr};
  PumpPanel*         panel_{nullptr};
  MockPumpController* ctrl_{nullptr};

 private slots:
  void initTestCase() {
    app_ = new QApplication(s_argc, s_argv);
  }

  void init() {
    ctrl_  = new MockPumpController();
    panel_ = new PumpPanel();
    QApplication::processEvents();
  }

  void cleanup() {
    delete panel_;
    panel_ = nullptr;
    delete ctrl_;
    ctrl_ = nullptr;
  }

  void cleanupTestCase() {
    delete app_;
    app_ = nullptr;
  }

  // =========================================================================
  // A. Widget structure
  // =========================================================================

  /**
   * @brief Panel must expose the expected object name.
   */
  void test_objectName_isPumpPanel() {
    QCOMPARE(panel_->objectName(), QStringLiteral("pumpPanel"));
  }

  /**
   * @brief Connection group box must exist.
   */
  void test_connectionGroupBox_exists() {
    QVERIFY2(panel_->findChild<QGroupBox*>(
                 QStringLiteral("grpConnection")) != nullptr,
             "grpConnection QGroupBox must exist inside PumpPanel");
  }

  /**
   * @brief Controls group box must exist.
   */
  void test_controlsGroupBox_exists() {
    QVERIFY2(panel_->findChild<QGroupBox*>(
                 QStringLiteral("grpControls")) != nullptr,
             "grpControls QGroupBox must exist inside PumpPanel");
  }

  /**
   * @brief Status group box must exist.
   */
  void test_statusGroupBox_exists() {
    QVERIFY2(panel_->findChild<QGroupBox*>(
                 QStringLiteral("grpStatus")) != nullptr,
             "grpStatus QGroupBox must exist inside PumpPanel");
  }

  /**
   * @brief Connect button must exist.
   */
  void test_connectButton_exists() {
    QVERIFY2(panel_->findChild<QPushButton*>(
                 QStringLiteral("btnConnect")) != nullptr,
             "btnConnect QPushButton must exist inside PumpPanel");
  }

  /**
   * @brief Start-Infusion button must exist.
   */
  void test_startButton_exists() {
    QVERIFY2(panel_->findChild<QPushButton*>(
                 QStringLiteral("btnStart")) != nullptr,
             "btnStart QPushButton must exist inside PumpPanel");
  }

  /**
   * @brief Stop button must exist.
   */
  void test_stopButton_exists() {
    QVERIFY2(panel_->findChild<QPushButton*>(
                 QStringLiteral("btnStop")) != nullptr,
             "btnStop QPushButton must exist inside PumpPanel");
  }

  /**
   * @brief Refill button must exist.
   */
  void test_refillButton_exists() {
    QVERIFY2(panel_->findChild<QPushButton*>(
                 QStringLiteral("btnRefill")) != nullptr,
             "btnRefill QPushButton must exist inside PumpPanel");
  }

  /**
   * @brief Flow-rate spinbox must exist.
   */
  void test_flowRateSpinbox_exists() {
    QVERIFY2(panel_->findChild<QDoubleSpinBox*>(
                 QStringLiteral("spnFlowRate")) != nullptr,
             "spnFlowRate QDoubleSpinBox must exist inside PumpPanel");
  }

  /**
   * @brief Volume spinbox must exist.
   */
  void test_volumeSpinbox_exists() {
    QVERIFY2(panel_->findChild<QDoubleSpinBox*>(
                 QStringLiteral("spnVolume")) != nullptr,
             "spnVolume QDoubleSpinBox must exist inside PumpPanel");
  }

  /**
   * @brief Position LCD must exist.
   */
  void test_positionLcd_exists() {
    QVERIFY2(panel_->findChild<QLCDNumber*>(
                 QStringLiteral("lcdPosition")) != nullptr,
             "lcdPosition QLCDNumber must exist inside PumpPanel");
  }

  /**
   * @brief Infusion progress bar must exist.
   */
  void test_progressBar_exists() {
    QVERIFY2(panel_->findChild<QProgressBar*>(
                 QStringLiteral("prgInfusion")) != nullptr,
             "prgInfusion QProgressBar must exist inside PumpPanel");
  }

  /**
   * @brief Error label must exist.
   */
  void test_errorLabel_exists() {
    QVERIFY2(panel_->findChild<QLabel*>(
                 QStringLiteral("lblError")) != nullptr,
             "lblError QLabel must exist inside PumpPanel");
  }

  /**
   * @brief State-text label must exist.
   */
  void test_stateTextLabel_exists() {
    QVERIFY2(panel_->findChild<QLabel*>(
                 QStringLiteral("lblStateText")) != nullptr,
             "lblStateText QLabel must exist inside PumpPanel");
  }

  // =========================================================================
  // B. Initial state (no controller attached)
  // =========================================================================

  /**
   * @brief Controls group must be disabled before a controller is attached.
   */
  void test_initialState_controlsGroupDisabled() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpControls"));
    QVERIFY2(!grp->isEnabled(),
             "Controls group must be disabled when no controller is attached");
  }

  /**
   * @brief Stop button must be disabled before connection.
   */
  void test_initialState_stopButtonDisabled() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnStop"));
    QVERIFY2(!btn->isEnabled(),
             "btnStop must be disabled on construction");
  }

  /**
   * @brief Progress bar must start at 0.
   */
  void test_initialState_progressBarAtZero() {
    auto* prg = panel_->findChild<QProgressBar*>(
        QStringLiteral("prgInfusion"));
    QCOMPARE(prg->value(), 0);
  }

  /**
   * @brief Error label must be hidden initially.
   */
  void test_initialState_errorLabelHidden() {
    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblError"));
    QVERIFY2(lbl->isHidden(),
             "lblError must be hidden on construction");
  }

  /**
   * @brief Position LCD must display 0.0 initially.
   */
  void test_initialState_positionLcdAtZero() {
    auto* lcd = panel_->findChild<QLCDNumber*>(
        QStringLiteral("lcdPosition"));
    QCOMPARE(lcd->value(), 0.0);
  }

  /**
   * @brief Status text must read "Disconnected" initially.
   */
  void test_initialState_statusTextDisconnected() {
    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblStatusText"));
    QCOMPARE(lbl->text(), QStringLiteral("Disconnected"));
  }

  // =========================================================================
  // C. setController() — attach / detach
  // =========================================================================

  /**
   * @brief setController(nullptr) must not crash and controls must remain
   *        disabled.
   */
  void test_setController_nullptr_noCrash_controlsDisabled() {
    panel_->setController(nullptr);
    QApplication::processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpControls"));
    QVERIFY2(!grp->isEnabled(),
             "Controls must remain disabled after setController(nullptr)");
  }

  /**
   * @brief Attaching a disconnected controller must keep controls disabled.
   */
  void test_setController_disconnected_controlsStillDisabled() {
    panel_->setController(ctrl_);
    QApplication::processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpControls"));
    QVERIFY2(!grp->isEnabled(),
             "Controls must remain disabled when attached controller "
             "is still kDisconnected");
  }

  /**
   * @brief setController(nullptr) after a connected controller must
   *        disable the controls group.
   */
  void test_setController_nullptr_afterConnected_disablesControls() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpControls"));
    QVERIFY(grp->isEnabled());  // pre-condition

    panel_->setController(nullptr);
    QApplication::processEvents();

    QVERIFY2(!grp->isEnabled(),
             "Controls group must be disabled after setController(nullptr) "
             "when previously connected");
  }

  // =========================================================================
  // D. Connection state transitions
  // =========================================================================

  /**
   * @brief kConnecting state must disable the Connect button.
   */
  void test_stateChanged_kConnecting_connectButtonDisabled() {
    panel_->setController(ctrl_);
    ctrl_->connectDevice();  // Synchronously emits kConnecting.
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnConnect"));
    QVERIFY2(!btn->isEnabled(),
             "btnConnect must be disabled while kConnecting");
  }

  /**
   * @brief kConnected must enable the controls group.
   */
  void test_stateChanged_kConnected_controlsEnabled() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpControls"));
    QVERIFY2(grp->isEnabled(),
             "Controls group must be enabled after kConnected");
  }

  /**
   * @brief kConnected must change the connect button text to "Disconnect".
   */
  void test_stateChanged_kConnected_connectButtonTextIsDisconnect() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnConnect"));
    QCOMPARE(btn->text(), QStringLiteral("Disconnect"));
  }

  /**
   * @brief kConnected must update the status text label.
   */
  void test_stateChanged_kConnected_statusTextConnected() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblStatusText"));
    QCOMPARE(lbl->text(), QStringLiteral("Connected"));
  }

  /**
   * @brief kDisconnected must disable the controls group.
   */
  void test_stateChanged_kDisconnected_controlsDisabled() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    ctrl_->disconnectDevice();
    QApplication::processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpControls"));
    QVERIFY2(!grp->isEnabled(),
             "Controls group must be disabled after kDisconnected");
  }

  /**
   * @brief kDisconnected must revert connect button text to "Connect".
   */
  void test_stateChanged_kDisconnected_connectButtonTextIsConnect() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    ctrl_->disconnectDevice();
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnConnect"));
    QCOMPARE(btn->text(), QStringLiteral("Connect"));
  }

  /**
   * @brief kDisconnected must reset the progress bar to 0.
   */
  void test_stateChanged_kDisconnected_progressBarReset() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);

    // Simulate position reaching 50 % of target.
    ctrl_->setTargetVolume(100.0);
    emit ctrl_->positionChanged(50.0);
    QApplication::processEvents();

    ctrl_->disconnectDevice();
    QApplication::processEvents();

    auto* prg = panel_->findChild<QProgressBar*>(
        QStringLiteral("prgInfusion"));
    QCOMPARE(prg->value(), 0);
  }

  // =========================================================================
  // E. Button enable/disable per pump state
  // =========================================================================

  /**
   * @brief After connect, Start and Refill are enabled; Stop is disabled.
   */
  void test_buttonStates_connected_idle() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    auto* btn_start  = panel_->findChild<QPushButton*>(
        QStringLiteral("btnStart"));
    auto* btn_stop   = panel_->findChild<QPushButton*>(
        QStringLiteral("btnStop"));
    auto* btn_refill = panel_->findChild<QPushButton*>(
        QStringLiteral("btnRefill"));

    QVERIFY2(btn_start->isEnabled(),
             "btnStart must be enabled when idle + connected");
    QVERIFY2(!btn_stop->isEnabled(),
             "btnStop must be disabled when idle + connected");
    QVERIFY2(btn_refill->isEnabled(),
             "btnRefill must be enabled when idle + connected");
  }

  /**
   * @brief After infusionStarted signal: Stop enabled, Start+Refill disabled.
   */
  void test_buttonStates_infusing_stopEnabledStartRefillDisabled() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    // Trigger infusionStarted via controller — this is the correct path
    // (avoids the QMessageBox from clicking Start directly).
    ctrl_->startInfusion();
    QApplication::processEvents();

    auto* btn_start  = panel_->findChild<QPushButton*>(
        QStringLiteral("btnStart"));
    auto* btn_stop   = panel_->findChild<QPushButton*>(
        QStringLiteral("btnStop"));
    auto* btn_refill = panel_->findChild<QPushButton*>(
        QStringLiteral("btnRefill"));

    QVERIFY2(!btn_start->isEnabled(),
             "btnStart must be disabled while infusing");
    QVERIFY2(btn_stop->isEnabled(),
             "btnStop must be enabled while infusing");
    QVERIFY2(!btn_refill->isEnabled(),
             "btnRefill must be disabled while infusing");
  }

  /**
   * @brief After infusionStopped signal: Start+Refill re-enabled, Stop
   *        disabled.
   */
  void test_buttonStates_afterStop_startAndRefillEnabled() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    ctrl_->startInfusion();
    QApplication::processEvents();

    ctrl_->stopInfusion();
    QApplication::processEvents();

    auto* btn_start  = panel_->findChild<QPushButton*>(
        QStringLiteral("btnStart"));
    auto* btn_stop   = panel_->findChild<QPushButton*>(
        QStringLiteral("btnStop"));
    auto* btn_refill = panel_->findChild<QPushButton*>(
        QStringLiteral("btnRefill"));

    QVERIFY2(btn_start->isEnabled(),
             "btnStart must be re-enabled after infusion stops");
    QVERIFY2(!btn_stop->isEnabled(),
             "btnStop must be disabled after infusion stops");
    QVERIFY2(btn_refill->isEnabled(),
             "btnRefill must be re-enabled after infusion stops");
  }

  // =========================================================================
  // F. Progress bar updates from positionChanged
  // =========================================================================

  /**
   * @brief positionChanged(50) with targetVolume 100 must set progress
   *        bar to 50 %.
   */
  void test_positionChanged_updatesProgressBar() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);

    ctrl_->setTargetVolume(100.0);
    emit ctrl_->positionChanged(50.0);
    QApplication::processEvents();

    auto* prg = panel_->findChild<QProgressBar*>(
        QStringLiteral("prgInfusion"));
    QCOMPARE(prg->value(), 50);
  }

  /**
   * @brief positionChanged must update the LCD position readout.
   */
  void test_positionChanged_updatesLcd() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);

    emit ctrl_->positionChanged(42.5);
    QApplication::processEvents();

    auto* lcd = panel_->findChild<QLCDNumber*>(
        QStringLiteral("lcdPosition"));
    QCOMPARE(lcd->value(), 42.5);
  }

  /**
   * @brief positionChanged exceeding targetVolume must clamp progress to 100.
   */
  void test_positionChanged_overflow_progressClampedAt100() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);

    ctrl_->setTargetVolume(10.0);
    emit ctrl_->positionChanged(99.0);  // Way beyond target.
    QApplication::processEvents();

    auto* prg = panel_->findChild<QProgressBar*>(
        QStringLiteral("prgInfusion"));
    QVERIFY2(prg->value() <= 100,
             "Progress bar must not exceed 100 when position overshoots target");
  }

  // =========================================================================
  // G. Flow rate sync from controller
  // =========================================================================

  /**
   * @brief flowRateChanged signal from the controller must sync the spinbox.
   */
  void test_flowRateChanged_syncsSpinbox() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    ctrl_->setFlowRate(25.5);
    QApplication::processEvents();

    auto* spn = panel_->findChild<QDoubleSpinBox*>(
        QStringLiteral("spnFlowRate"));
    QCOMPARE(spn->value(), 25.5);
  }

  // =========================================================================
  // H. Error state
  // =========================================================================

  /**
   * @brief errorOccurred signal must make the error label visible and
   *        populate it with the error message.
   */
  void test_errorOccurred_showsErrorLabel() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    emit ctrl_->errorOccurred(
        QStringLiteral("Pump stall detected"));
    QApplication::processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblError"));
    QVERIFY2(!lbl->isHidden(),
             "lblError must be visible after errorOccurred");
    QVERIFY2(lbl->text().contains(
                 QStringLiteral("Pump stall detected")),
             "lblError must contain the error message text");
  }

  /**
   * @brief errorOccurred must update the sub-state text to "Error".
   */
  void test_errorOccurred_stateTextIsError() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    emit ctrl_->errorOccurred(QStringLiteral("hardware fault"));
    QApplication::processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblStateText"));
    QCOMPARE(lbl->text(), QStringLiteral("Error"));
  }

  /**
   * @brief After errorOccurred, Stop button must be disabled (not infusing).
   */
  void test_errorOccurred_stopButtonDisabled() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    ctrl_->startInfusion();
    QApplication::processEvents();

    emit ctrl_->errorOccurred(QStringLiteral("error!"));
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnStop"));
    QVERIFY2(!btn->isEnabled(),
             "btnStop must be disabled after an error");
  }

  // =========================================================================
  // I. infusionStarted / infusionStopped UI updates
  // =========================================================================

  /**
   * @brief infusionStarted must update sub-state text to "Infusing".
   */
  void test_infusionStarted_stateTextIsInfusing() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    ctrl_->startInfusion();
    QApplication::processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblStateText"));
    QCOMPARE(lbl->text(), QStringLiteral("Infusing"));
  }

  /**
   * @brief infusionStopped must update sub-state text to "Idle".
   */
  void test_infusionStopped_stateTextIsIdle() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    ctrl_->startInfusion();
    ctrl_->stopInfusion();
    QApplication::processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblStateText"));
    QCOMPARE(lbl->text(), QStringLiteral("Idle"));
  }

  /**
   * @brief infusionStopped must reset the progress bar to 0.
   */
  void test_infusionStopped_progressBarReset() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);

    ctrl_->setTargetVolume(100.0);
    emit ctrl_->positionChanged(60.0);
    ctrl_->startInfusion();
    QApplication::processEvents();

    ctrl_->stopInfusion();
    QApplication::processEvents();

    auto* prg = panel_->findChild<QProgressBar*>(
        QStringLiteral("prgInfusion"));
    QCOMPARE(prg->value(), 0);
  }

  // =========================================================================
  // J. Signal forwarding — panel → controller
  // =========================================================================

  /**
   * @brief Changing the flow-rate spinbox value must call setFlowRate() on
   *        the controller (evidenced by flowRateChanged).
   */
  void test_flowRateSpinbox_valueChanged_callsSetFlowRate() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    QSignalSpy spy(
        ctrl_,
        &mwa::hardware::PumpControllerInterface::flowRateChanged);

    auto* spn = panel_->findChild<QDoubleSpinBox*>(
        QStringLiteral("spnFlowRate"));
    spn->setValue(15.0);
    QApplication::processEvents();

    QVERIFY2(spy.count() >= 1,
             "flowRateChanged must be emitted when flow-rate spinbox changes");
  }

  /**
   * @brief Changing the volume spinbox must call setTargetVolume() on the
   *        controller.  We verify indirectly that the value is stored.
   */
  void test_volumeSpinbox_valueChanged_callsSetTargetVolume() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    auto* spn = panel_->findChild<QDoubleSpinBox*>(
        QStringLiteral("spnVolume"));
    spn->setValue(250.0);
    QApplication::processEvents();

    QCOMPARE(ctrl_->targetVolume(), 250.0);
  }
};

QTEST_APPLESS_MAIN(TestPumpPanel)
#include "test_pump_panel.moc"
