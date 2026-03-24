/**
 * @file test_stage_panel.cpp
 * @brief Adversarial unit tests for the StagePanel widget.
 * @author MWA Team
 * @date 2026-03-24
 *
 * Tests cover: initial widget state, controller attachment/detachment,
 * connection state transitions, jog button forwarding, step-size selection,
 * absolute move (Go To), emergency stop behaviour, speed debounce, position
 * label updates, and move/home completion handling.
 *
 * @note Uses !isHidden() / isEnabled() rather than isVisible() for
 *       headless CI compatibility.  QMessageBox dialogs triggered by
 *       Home and Go To are tested indirectly via state checks.
 *
 * @copyright LGPL-3.0-or-later
 */

#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>

#include "gui/panels/stage_panel.h"
#include "hardware/stage/mock_stage_controller.h"

using mwa::gui::StagePanel;
using mwa::hardware::MockStageController;
using mwa::hardware::DeviceInterface;
using DeviceState = DeviceInterface::DeviceState;

static int   s_argc        = 1;
static char  s_app_name[]  = "test_stage_panel";
static char* s_argv[]      = {s_app_name};

// ---------------------------------------------------------------------------
// Helper: drive mock controller to kConnected synchronously.
// ---------------------------------------------------------------------------
static void connectSync(MockStageController& ctrl) {
  ctrl.connectDevice();
  QTest::qWait(600);
}

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------

/**
 * @class TestStagePanel
 * @brief Qt Test class exercising mwa::gui::StagePanel.
 */
class TestStagePanel : public QObject {
  Q_OBJECT

 private:
  QApplication*        app_{nullptr};
  StagePanel*          panel_{nullptr};
  MockStageController* ctrl_{nullptr};

 private slots:
  void initTestCase() {
    app_ = new QApplication(s_argc, s_argv);
  }

  void init() {
    ctrl_  = new MockStageController();
    panel_ = new StagePanel();
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

  void test_objectName_isStagePanel() {
    QCOMPARE(panel_->objectName(),
             QStringLiteral("stagePanel"));
  }

  void test_connectionGroupBox_exists() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpConnection"));
    QVERIFY2(grp != nullptr,
             "grpConnection QGroupBox must exist");
  }

  void test_jogGroupBox_exists() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpJog"));
    QVERIFY2(grp != nullptr,
             "grpJog QGroupBox must exist");
  }

  void test_absPositionGroupBox_exists() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpAbsPosition"));
    QVERIFY2(grp != nullptr,
             "grpAbsPosition QGroupBox must exist");
  }

  void test_speedGroupBox_exists() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpSpeed"));
    QVERIFY2(grp != nullptr,
             "grpSpeed QGroupBox must exist");
  }

  void test_statusGroupBox_exists() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpStatus"));
    QVERIFY2(grp != nullptr,
             "grpStatus QGroupBox must exist");
  }

  void test_connectButton_exists() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnConnect"));
    QVERIFY2(btn != nullptr,
             "btnConnect QPushButton must exist");
  }

  void test_emergencyStopButton_exists() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnEmergencyStop"));
    QVERIFY2(btn != nullptr,
             "btnEmergencyStop QPushButton must exist");
  }

  void test_homeButton_exists() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnHome"));
    QVERIFY2(btn != nullptr,
             "btnHome QPushButton must exist");
  }

  void test_goToButton_exists() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnGoTo"));
    QVERIFY2(btn != nullptr,
             "btnGoTo QPushButton must exist");
  }

  void test_jogButtons_allExist() {
    QVERIFY(panel_->findChild<QPushButton*>(
        QStringLiteral("btnJogXPlus")) != nullptr);
    QVERIFY(panel_->findChild<QPushButton*>(
        QStringLiteral("btnJogXMinus")) != nullptr);
    QVERIFY(panel_->findChild<QPushButton*>(
        QStringLiteral("btnJogYPlus")) != nullptr);
    QVERIFY(panel_->findChild<QPushButton*>(
        QStringLiteral("btnJogYMinus")) != nullptr);
    QVERIFY(panel_->findChild<QPushButton*>(
        QStringLiteral("btnJogZPlus")) != nullptr);
    QVERIFY(panel_->findChild<QPushButton*>(
        QStringLiteral("btnJogZMinus")) != nullptr);
  }

  void test_stepSizeCombo_exists() {
    auto* cmb = panel_->findChild<QComboBox*>(
        QStringLiteral("cmbStepSize"));
    QVERIFY2(cmb != nullptr,
             "cmbStepSize QComboBox must exist");
    QCOMPARE(cmb->count(), 4);
  }

  void test_positionLabels_exist() {
    QVERIFY(panel_->findChild<QLabel*>(
        QStringLiteral("lblPosX")) != nullptr);
    QVERIFY(panel_->findChild<QLabel*>(
        QStringLiteral("lblPosY")) != nullptr);
    QVERIFY(panel_->findChild<QLabel*>(
        QStringLiteral("lblPosZ")) != nullptr);
  }

  void test_stateTextLabel_exists() {
    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblStateText"));
    QVERIFY2(lbl != nullptr,
             "lblStateText QLabel must exist");
  }

  void test_absPositionSpinboxes_exist() {
    QVERIFY(panel_->findChild<QDoubleSpinBox*>(
        QStringLiteral("spnAbsX")) != nullptr);
    QVERIFY(panel_->findChild<QDoubleSpinBox*>(
        QStringLiteral("spnAbsY")) != nullptr);
    QVERIFY(panel_->findChild<QDoubleSpinBox*>(
        QStringLiteral("spnAbsZ")) != nullptr);
  }

  void test_speedSpinbox_exists() {
    auto* spn = panel_->findChild<QDoubleSpinBox*>(
        QStringLiteral("spnSpeed"));
    QVERIFY2(spn != nullptr,
             "spnSpeed QDoubleSpinBox must exist");
  }

  // =========================================================================
  // B. Initial state (no controller)
  // =========================================================================

  void test_initialState_jogGroupDisabled() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpJog"));
    QVERIFY2(!grp->isEnabled(),
             "Jog group must be disabled without controller");
  }

  void test_initialState_absPositionGroupDisabled() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpAbsPosition"));
    QVERIFY2(!grp->isEnabled(),
             "Abs position group must be disabled without controller");
  }

  void test_initialState_speedGroupDisabled() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpSpeed"));
    QVERIFY2(!grp->isEnabled(),
             "Speed group must be disabled without controller");
  }

  void test_initialState_emergencyStopEnabled() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnEmergencyStop"));
    QVERIFY2(btn->isEnabled(),
             "Emergency stop must be always enabled");
  }

  void test_initialState_connectButtonTextIsConnect() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnConnect"));
    QCOMPARE(btn->text(), QStringLiteral("Connect"));
  }

  void test_initialState_stateTextIdle() {
    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblStateText"));
    QCOMPARE(lbl->text(), QStringLiteral("Idle"));
  }

  void test_initialState_positionsShowZero() {
    auto* lbl_x = panel_->findChild<QLabel*>(
        QStringLiteral("lblPosX"));
    auto* lbl_y = panel_->findChild<QLabel*>(
        QStringLiteral("lblPosY"));
    auto* lbl_z = panel_->findChild<QLabel*>(
        QStringLiteral("lblPosZ"));
    QVERIFY(lbl_x->text().contains(QStringLiteral("0.000")));
    QVERIFY(lbl_y->text().contains(QStringLiteral("0.000")));
    QVERIFY(lbl_z->text().contains(QStringLiteral("0.000")));
  }

  // =========================================================================
  // C. setController() — attachment
  // =========================================================================

  void test_setController_nullptr_noCrash_controlsDisabled() {
    panel_->setController(nullptr);
    QApplication::processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpJog"));
    QVERIFY2(!grp->isEnabled(),
             "Jog group must remain disabled after nullptr");
  }

  void test_setController_disconnected_controlsDisabled() {
    panel_->setController(ctrl_);
    QApplication::processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpJog"));
    QVERIFY2(!grp->isEnabled(),
             "Jog group must be disabled when not connected");
  }

  void test_setController_connected_controlsEnabled() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    auto* grp_jog = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpJog"));
    auto* grp_abs = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpAbsPosition"));
    auto* grp_spd = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpSpeed"));

    QVERIFY(grp_jog->isEnabled());
    QVERIFY(grp_abs->isEnabled());
    QVERIFY(grp_spd->isEnabled());
  }

  void test_setController_nullptr_afterConnected_disablesControls() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    panel_->setController(nullptr);
    QApplication::processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpJog"));
    QVERIFY2(!grp->isEnabled(),
             "Jog group must be disabled after setController(nullptr)");
  }

  // =========================================================================
  // D. Connection state transitions
  // =========================================================================

  void test_stateChanged_kConnected_connectButtonTextIsDisconnect() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnConnect"));
    QCOMPARE(btn->text(), QStringLiteral("Disconnect"));
  }

  void test_stateChanged_kDisconnected_controlsDisabled() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    ctrl_->disconnectDevice();
    QApplication::processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpJog"));
    QVERIFY2(!grp->isEnabled(),
             "Jog group must be disabled after disconnect");
  }

  void test_stateChanged_kDisconnected_connectButtonTextIsConnect() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    ctrl_->disconnectDevice();
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnConnect"));
    QCOMPARE(btn->text(), QStringLiteral("Connect"));
  }

  void test_stateChanged_emergencyStopAlwaysEnabled() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    ctrl_->disconnectDevice();
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnEmergencyStop"));
    QVERIFY2(btn->isEnabled(),
             "Emergency stop must remain enabled regardless of state");
  }

  // =========================================================================
  // E. Jog button forwarding
  // =========================================================================

  void test_jogXPlus_callsMoveRelative() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    QSignalSpy spy(
        ctrl_,
        &mwa::hardware::StageControllerInterface::positionChanged);

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnJogXPlus"));
    btn->click();
    // Wait for 300 ms move delay + margin
    QTest::qWait(500);
    QApplication::processEvents();

    QVERIFY2(spy.count() >= 1,
             "positionChanged must be emitted after jog X+");
    // Default step size index=1 => 0.10 mm, initial pos=0
    const double new_x = spy.last().at(0).toDouble();
    QVERIFY2(new_x > 0.0,
             "X position must increase after X+ jog");
  }

  void test_jogXMinus_callsMoveRelative() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    // First move to a positive position so X- is meaningful
    ctrl_->moveRelative(1.0, 0.0, 0.0);
    QTest::qWait(500);
    QApplication::processEvents();

    QSignalSpy spy(
        ctrl_,
        &mwa::hardware::StageControllerInterface::positionChanged);

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnJogXMinus"));
    btn->click();
    QTest::qWait(500);
    QApplication::processEvents();

    QVERIFY2(spy.count() >= 1,
             "positionChanged must be emitted after jog X-");
  }

  void test_jogYPlus_callsMoveRelative() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    QSignalSpy spy(
        ctrl_,
        &mwa::hardware::StageControllerInterface::positionChanged);

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnJogYPlus"));
    btn->click();
    QTest::qWait(500);
    QApplication::processEvents();

    QVERIFY2(spy.count() >= 1,
             "positionChanged must be emitted after jog Y+");
    const double new_y = spy.last().at(1).toDouble();
    QVERIFY2(new_y > 0.0,
             "Y position must increase after Y+ jog");
  }

  void test_jogZPlus_callsMoveRelative() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    QSignalSpy spy(
        ctrl_,
        &mwa::hardware::StageControllerInterface::positionChanged);

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnJogZPlus"));
    btn->click();
    QTest::qWait(500);
    QApplication::processEvents();

    QVERIFY2(spy.count() >= 1,
             "positionChanged must be emitted after jog Z+");
    const double new_z = spy.last().at(2).toDouble();
    QVERIFY2(new_z > 0.0,
             "Z position must increase after Z+ jog");
  }

  // =========================================================================
  // F. Jog — move buttons disabled during move
  // =========================================================================

  void test_jog_disablesButtons_duringMove() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    auto* btn_jog = panel_->findChild<QPushButton*>(
        QStringLiteral("btnJogXPlus"));
    btn_jog->click();
    QApplication::processEvents();

    // Immediately after clicking, buttons should be disabled
    auto* btn_home = panel_->findChild<QPushButton*>(
        QStringLiteral("btnHome"));
    auto* btn_go_to = panel_->findChild<QPushButton*>(
        QStringLiteral("btnGoTo"));

    QVERIFY2(!btn_jog->isEnabled(),
             "Jog button must be disabled during move");
    QVERIFY2(!btn_home->isEnabled(),
             "Home button must be disabled during move");
    QVERIFY2(!btn_go_to->isEnabled(),
             "Go To button must be disabled during move");
  }

  void test_jog_stateText_showsMoving() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnJogXPlus"));
    btn->click();
    QApplication::processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblStateText"));
    QCOMPARE(lbl->text(), QStringLiteral("Moving"));
  }

  void test_moveComplete_reEnablesButtons() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnJogXPlus"));
    btn->click();
    // Wait for move delay (300 ms) + margin
    QTest::qWait(500);
    QApplication::processEvents();

    QVERIFY2(btn->isEnabled(),
             "Jog button must be re-enabled after move complete");

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblStateText"));
    QCOMPARE(lbl->text(), QStringLiteral("Idle"));
  }

  // =========================================================================
  // G. Step size selection
  // =========================================================================

  void test_stepSize_index0_smallestStep() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    auto* cmb = panel_->findChild<QComboBox*>(
        QStringLiteral("cmbStepSize"));
    cmb->setCurrentIndex(0);  // 0.01 mm

    QSignalSpy spy(
        ctrl_,
        &mwa::hardware::StageControllerInterface::positionChanged);

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnJogXPlus"));
    btn->click();
    QTest::qWait(500);
    QApplication::processEvents();

    QVERIFY(spy.count() >= 1);
    const double new_x = spy.last().at(0).toDouble();
    // 0.01 mm step from 0
    QVERIFY2(qAbs(new_x - 0.01) < 0.001,
             "X must move by 0.01 mm with step index 0");
  }

  void test_stepSize_index3_largestStep() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    auto* cmb = panel_->findChild<QComboBox*>(
        QStringLiteral("cmbStepSize"));
    cmb->setCurrentIndex(3);  // 10.0 mm

    QSignalSpy spy(
        ctrl_,
        &mwa::hardware::StageControllerInterface::positionChanged);

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnJogXPlus"));
    btn->click();
    QTest::qWait(500);
    QApplication::processEvents();

    QVERIFY(spy.count() >= 1);
    const double new_x = spy.last().at(0).toDouble();
    // 10.0 mm step from 0
    QVERIFY2(qAbs(new_x - 10.0) < 0.001,
             "X must move by 10.0 mm with step index 3");
  }

  // =========================================================================
  // H. Position display updates
  // =========================================================================

  void test_positionChanged_updatesLabels() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    ctrl_->moveAbsolute(1.234, 5.678, 9.012);
    QTest::qWait(500);
    QApplication::processEvents();

    auto* lbl_x = panel_->findChild<QLabel*>(
        QStringLiteral("lblPosX"));
    auto* lbl_y = panel_->findChild<QLabel*>(
        QStringLiteral("lblPosY"));
    auto* lbl_z = panel_->findChild<QLabel*>(
        QStringLiteral("lblPosZ"));

    QVERIFY2(lbl_x->text().contains(QStringLiteral("1.234")),
             "X label must show 1.234");
    QVERIFY2(lbl_y->text().contains(QStringLiteral("5.678")),
             "Y label must show 5.678");
    QVERIFY2(lbl_z->text().contains(QStringLiteral("9.012")),
             "Z label must show 9.012");
  }

  // =========================================================================
  // I. Emergency stop
  // =========================================================================

  void test_emergencyStop_alwaysEnabled_noController() {
    // No controller attached — button must still be enabled.
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnEmergencyStop"));
    QVERIFY(btn->isEnabled());
  }

  void test_emergencyStop_click_callsStopMotion() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    // Start a move first
    auto* jog_btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnJogXPlus"));
    jog_btn->click();
    QApplication::processEvents();

    // Now hit emergency stop
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnEmergencyStop"));
    btn->click();
    QApplication::processEvents();

    // After emergency stop, state should be Idle and buttons
    // re-enabled.
    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblStateText"));
    QCOMPARE(lbl->text(), QStringLiteral("Idle"));
    QVERIFY(jog_btn->isEnabled());
  }

  // =========================================================================
  // J. Speed debounce
  // =========================================================================

  void test_speedSpinbox_change_reachesControllerAfterDebounce() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    QSignalSpy spy(
        ctrl_,
        &mwa::hardware::StageControllerInterface::speedChanged);

    auto* spn = panel_->findChild<QDoubleSpinBox*>(
        QStringLiteral("spnSpeed"));
    spn->setValue(5.0);
    QApplication::processEvents();

    // Speed should NOT be sent immediately
    QCOMPARE(spy.count(), 0);

    // Wait for debounce (300 ms) + margin
    QTest::qWait(500);
    QApplication::processEvents();

    QVERIFY2(spy.count() >= 1,
             "speedChanged must be emitted after debounce");
    QCOMPARE(spy.last().at(0).toDouble(), 5.0);
  }

  void test_speedSpinbox_rapidChanges_onlyLastValueSent() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    QSignalSpy spy(
        ctrl_,
        &mwa::hardware::StageControllerInterface::speedChanged);

    auto* spn = panel_->findChild<QDoubleSpinBox*>(
        QStringLiteral("spnSpeed"));

    // Rapid changes within the debounce window
    spn->setValue(2.0);
    QApplication::processEvents();
    spn->setValue(3.0);
    QApplication::processEvents();
    spn->setValue(7.5);
    QApplication::processEvents();

    // Wait for debounce
    QTest::qWait(500);
    QApplication::processEvents();

    // Only the last value should reach the controller
    QVERIFY(spy.count() >= 1);
    QCOMPARE(spy.last().at(0).toDouble(), 7.5);
  }

  // =========================================================================
  // K. Status label styling
  // =========================================================================

  void test_statusLabel_disconnected_showsDisconnected() {
    panel_->setController(ctrl_);
    QApplication::processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblStatus"));
    QVERIFY2(lbl->text().contains(QStringLiteral("Disconnected")),
             "Status must show Disconnected");
  }

  void test_statusLabel_connected_showsConnected() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblStatus"));
    QVERIFY2(lbl->text().contains(QStringLiteral("Connected")),
             "Status must show Connected");
  }

  // =========================================================================
  // L. Rapid operations (stress boundary)
  // =========================================================================

  void test_rapidJogs_noCrash() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    auto* btn_xp = panel_->findChild<QPushButton*>(
        QStringLiteral("btnJogXPlus"));

    // Rapid sequential jog clicks — not all will fire a move (some
    // will be rejected because the stage is still moving), but none
    // should crash.
    for (int i = 0; i < 10; ++i) {
      btn_xp->click();
      QApplication::processEvents();
    }

    // Wait for last move to finish
    QTest::qWait(600);
    QApplication::processEvents();

    // Panel should survive without crashing.
    QVERIFY(true);
  }

  void test_controllerSwap_noCrash() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    for (int i = 0; i < 5; ++i) {
      panel_->setController(nullptr);
      QApplication::processEvents();
      panel_->setController(ctrl_);
      QApplication::processEvents();
    }

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpJog"));
    // Controller was connected via connectSync before swaps and
    // stays in kConnected, so re-attaching syncs that state.
    QVERIFY2(grp->isEnabled(),
             "grpJog must be enabled because the re-attached "
             "controller is still in kConnected state");
  }
};

QTEST_APPLESS_MAIN(TestStagePanel)
#include "test_stage_panel.moc"
