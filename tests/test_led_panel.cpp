/**
 * @file test_led_panel.cpp
 * @brief Adversarial unit tests for the LedPanel widget.
 * @author MWA Team
 * @date 2026-03-23
 *
 * Tests cover: initial widget state, controller attachment/detachment,
 * signal forwarding through mock controller, connection-state transitions,
 * slider/spinbox synchronisation, power-toggle behaviour, and LCD intensity
 * readout updates.  Every assertion is designed to be capable of failing
 * if the implementation regresses.
 *
 * @note Uses !isHidden() / isEnabled() rather than isVisible() for
 *       headless CI compatibility.  QMessageBox confirmation dialogs
 *       triggered by the Disconnect button are tested indirectly via
 *       button-state checks rather than by auto-accepting dialogs, which
 *       is not safe in a headless environment.
 *
 * @copyright LGPL-3.0-or-later
 */

#include <QApplication>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QLCDNumber>
#include <QPushButton>
#include <QSignalSpy>
#include <QSlider>
#include <QTest>

#include "gui/panels/led_panel.h"
#include "hardware/led/mock_led_controller.h"

using mwa::gui::LedPanel;
using mwa::hardware::MockLedController;
using mwa::hardware::DeviceInterface;
using DeviceState = DeviceInterface::DeviceState;

static int   s_argc        = 1;
static char  s_app_name[]  = "test_led_panel";
static char* s_argv[]      = {s_app_name};

// ---------------------------------------------------------------------------
// Helper: drive mock controller to kConnected synchronously.
// ---------------------------------------------------------------------------
static void connectSync(MockLedController& ctrl) {
  ctrl.connectDevice();
  QTest::qWait(600);
}

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------

/**
 * @class TestLedPanel
 * @brief Qt Test class exercising mwa::gui::LedPanel.
 */
class TestLedPanel : public QObject {
  Q_OBJECT

 private:
  QApplication*      app_{nullptr};
  LedPanel*          panel_{nullptr};
  MockLedController* ctrl_{nullptr};

 private slots:
  void initTestCase() {
    app_ = new QApplication(s_argc, s_argv);
  }

  void init() {
    ctrl_  = new MockLedController();
    panel_ = new LedPanel();
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
   * @brief Panel must expose its objectName so tests can locate it
   *        programmatically.
   */
  void test_objectName_isLedPanel() {
    QCOMPARE(panel_->objectName(), QStringLiteral("ledPanel"));
  }

  /**
   * @brief Connection group box must exist with the expected object name.
   */
  void test_connectionGroupBox_exists() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpConnection"));
    QVERIFY2(grp != nullptr,
             "grpConnection QGroupBox must exist inside LedPanel");
  }

  /**
   * @brief Controls group box must exist.
   */
  void test_controlsGroupBox_exists() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpControls"));
    QVERIFY2(grp != nullptr,
             "grpControls QGroupBox must exist inside LedPanel");
  }

  /**
   * @brief Status group box must exist.
   */
  void test_statusGroupBox_exists() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpStatus"));
    QVERIFY2(grp != nullptr,
             "grpStatus QGroupBox must exist inside LedPanel");
  }

  /**
   * @brief Connect button must exist in the connection group.
   */
  void test_connectButton_exists() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnConnect"));
    QVERIFY2(btn != nullptr,
             "btnConnect QPushButton must exist inside LedPanel");
  }

  /**
   * @brief Power-toggle button must exist in the controls group.
   */
  void test_powerButton_exists() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnPower"));
    QVERIFY2(btn != nullptr,
             "btnPower QPushButton must exist inside LedPanel");
  }

  /**
   * @brief Intensity spinbox must exist.
   */
  void test_intensitySpinbox_exists() {
    auto* spn = panel_->findChild<QDoubleSpinBox*>(
        QStringLiteral("spnIntensity"));
    QVERIFY2(spn != nullptr,
             "spnIntensity QDoubleSpinBox must exist inside LedPanel");
  }

  /**
   * @brief Intensity slider must exist.
   */
  void test_intensitySlider_exists() {
    auto* sld = panel_->findChild<QSlider*>(
        QStringLiteral("sldIntensity"));
    QVERIFY2(sld != nullptr,
             "sldIntensity QSlider must exist inside LedPanel");
  }

  /**
   * @brief LCD intensity readout must exist.
   */
  void test_lcdIntensity_exists() {
    auto* lcd = panel_->findChild<QLCDNumber*>(
        QStringLiteral("lcdIntensity"));
    QVERIFY2(lcd != nullptr,
             "lcdIntensity QLCDNumber must exist inside LedPanel");
  }

  /**
   * @brief Power-value status label must exist.
   */
  void test_powerValueLabel_exists() {
    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblPowerValue"));
    QVERIFY2(lbl != nullptr,
             "lblPowerValue QLabel must exist inside LedPanel");
  }

  /**
   * @brief Device-state label must exist.
   */
  void test_deviceStateLabel_exists() {
    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblDeviceState"));
    QVERIFY2(lbl != nullptr,
             "lblDeviceState QLabel must exist inside LedPanel");
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
   * @brief Power button must not be checked initially.
   */
  void test_initialState_powerButtonUnchecked() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnPower"));
    QVERIFY2(!btn->isChecked(),
             "Power button must start unchecked");
  }

  /**
   * @brief LCD readout must display 0.0 on construction.
   */
  void test_initialState_lcdShowsZero() {
    auto* lcd = panel_->findChild<QLCDNumber*>(
        QStringLiteral("lcdIntensity"));
    QCOMPARE(lcd->value(), 0.0);
  }

  /**
   * @brief Intensity spinbox must be at 0.0 on construction.
   */
  void test_initialState_spinboxAtZero() {
    auto* spn = panel_->findChild<QDoubleSpinBox*>(
        QStringLiteral("spnIntensity"));
    QCOMPARE(spn->value(), 0.0);
  }

  /**
   * @brief Intensity slider must be at 0 on construction.
   */
  void test_initialState_sliderAtZero() {
    auto* sld = panel_->findChild<QSlider*>(
        QStringLiteral("sldIntensity"));
    QCOMPARE(sld->value(), 0);
  }

  /**
   * @brief Device-state label must read "Disconnected" on construction.
   */
  void test_initialState_deviceStateLabelDisconnected() {
    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblDeviceState"));
    QCOMPARE(lbl->text(), QStringLiteral("Disconnected"));
  }

  /**
   * @brief Connect button must read "Connect" on construction.
   */
  void test_initialState_connectButtonTextIsConnect() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnConnect"));
    QCOMPARE(btn->text(), QStringLiteral("Connect"));
  }

  /**
   * @brief Power button label must read "LED OFF" on construction.
   */
  void test_initialState_powerButtonTextIsOff() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnPower"));
    QCOMPARE(btn->text(), QStringLiteral("LED OFF"));
  }

  // =========================================================================
  // C. setController() — attach controller
  // =========================================================================

  /**
   * @brief setController(nullptr) must not crash and controls must stay
   *        disabled.
   */
  void test_setController_nullptr_noCrash_controlsDisabled() {
    panel_->setController(nullptr);
    QApplication::processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpControls"));
    QVERIFY2(!grp->isEnabled(),
             "Controls group must remain disabled after setController(nullptr)");
  }

  /**
   * @brief After attaching a controller that is still disconnected, controls
   *        must remain disabled.
   */
  void test_setController_disconnectedController_controlsStillDisabled() {
    panel_->setController(ctrl_);
    QApplication::processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpControls"));
    QVERIFY2(!grp->isEnabled(),
             "Controls must remain disabled when attached controller "
             "is still kDisconnected");
  }

  /**
   * @brief Attaching a controller wires intensityChanged so that a
   *        subsequent setIntensity() on the controller updates the LCD.
   */
  void test_setController_wiresIntensityChanged_lcdUpdates() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    ctrl_->setIntensity(55.0);
    QApplication::processEvents();

    auto* lcd = panel_->findChild<QLCDNumber*>(
        QStringLiteral("lcdIntensity"));
    QCOMPARE(lcd->value(), 55.0);
  }

  /**
   * @brief Attaching a controller wires powerStateChanged so that
   *        setPowerOn(true) updates the power-value label to contain "ON".
   */
  void test_setController_wiresPowerStateChanged_labelUpdates() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    ctrl_->setPowerOn(true);
    QApplication::processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblPowerValue"));
    QVERIFY2(lbl->text().contains(QStringLiteral("ON")),
             "lblPowerValue must display ON after setPowerOn(true)");
  }

  /**
   * @brief setController(nullptr) after a valid controller must disable
   *        controls (the old signal connections must be severed).
   */
  void test_setController_nullptrAfterValid_controlsDisabled() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    panel_->setController(nullptr);
    QApplication::processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpControls"));
    QVERIFY2(!grp->isEnabled(),
             "Controls group must be disabled after setController(nullptr) "
             "detaches the connected controller");
  }

  // =========================================================================
  // D. Connection state transitions
  // =========================================================================

  /**
   * @brief kConnecting state must disable the Connect button.
   */
  void test_stateChanged_kConnecting_connectButtonDisabled() {
    panel_->setController(ctrl_);
    // connectDevice() transitions to kConnecting synchronously.
    ctrl_->connectDevice();
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnConnect"));
    QVERIFY2(!btn->isEnabled(),
             "btnConnect must be disabled while kConnecting");
  }

  /**
   * @brief kConnected state must enable the Controls group.
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
   * @brief kConnected state must change the connect button text to
   *        "Disconnect".
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
   * @brief kConnected state must update the device-state label to
   *        "Connected".
   */
  void test_stateChanged_kConnected_deviceStateLabelConnected() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblDeviceState"));
    QCOMPARE(lbl->text(), QStringLiteral("Connected"));
  }

  /**
   * @brief kDisconnected state must disable the Controls group.
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
   * @brief After disconnect the connect button must revert to "Connect".
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

  // =========================================================================
  // E. Signal forwarding — controller receives the right calls
  // =========================================================================

  /**
   * @brief setIntensity() on the controller must be called exactly once
   *        when the spinbox finishes editing.
   *
   * We exercise onSpinboxEditingFinished indirectly by calling
   * QDoubleSpinBox::editingFinished() via QTest keyboard simulation is not
   * reliable headlessly; instead we use programmatic setValue + emit.
   */
  void test_spinboxEditingFinished_callsSetIntensity() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    QSignalSpy spy(
        ctrl_,
        &mwa::hardware::LedControllerInterface::intensityChanged);

    auto* spn = panel_->findChild<QDoubleSpinBox*>(
        QStringLiteral("spnIntensity"));
    spn->setValue(42.5);
    // Emit editingFinished manually (simulates user pressing Enter).
    emit spn->editingFinished();
    QApplication::processEvents();

    QVERIFY2(spy.count() >= 1,
             "intensityChanged must be emitted at least once after "
             "spinbox editingFinished");
    QCOMPARE(spy.last().at(0).toDouble(), 42.5);
  }

  /**
   * @brief Toggling the power button to ON must call setPowerOn(true) on
   *        the controller (evidenced by powerStateChanged).
   */
  void test_powerButton_toggleOn_callsSetPowerOnTrue() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    QSignalSpy spy(
        ctrl_,
        &mwa::hardware::LedControllerInterface::powerStateChanged);

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnPower"));
    // Simulate user clicking the checkable button (toggle from unchecked→checked).
    btn->setChecked(true);
    QApplication::processEvents();

    QVERIFY2(spy.count() >= 1,
             "powerStateChanged must be emitted when power button is toggled on");
    QCOMPARE(spy.last().at(0).toBool(), true);
  }

  /**
   * @brief Toggling the power button to OFF must call setPowerOn(false).
   */
  void test_powerButton_toggleOff_callsSetPowerOnFalse() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    ctrl_->setPowerOn(true);
    QApplication::processEvents();

    QSignalSpy spy(
        ctrl_,
        &mwa::hardware::LedControllerInterface::powerStateChanged);

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnPower"));
    btn->setChecked(false);
    QApplication::processEvents();

    QVERIFY2(spy.count() >= 1,
             "powerStateChanged must be emitted when power button is toggled off");
    QCOMPARE(spy.last().at(0).toBool(), false);
  }

  // =========================================================================
  // F. Slider/spinbox synchronisation
  // =========================================================================

  /**
   * @brief Moving the slider must update the spinbox display immediately
   *        (without triggering a controller call).
   */
  void test_sliderValueChanged_updatesSpinbox() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    auto* sld = panel_->findChild<QSlider*>(
        QStringLiteral("sldIntensity"));
    auto* spn = panel_->findChild<QDoubleSpinBox*>(
        QStringLiteral("spnIntensity"));

    // Move slider to 500 (= 50.0 %)
    sld->setValue(500);
    QApplication::processEvents();

    QCOMPARE(spn->value(), 50.0);
  }

  /**
   * @brief Slider value changed must NOT call setIntensity on the controller
   *        (only sliderReleased should forward the value).
   */
  void test_sliderValueChanged_doesNotCallSetIntensity() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    QSignalSpy spy(
        ctrl_,
        &mwa::hardware::LedControllerInterface::intensityChanged);

    auto* sld = panel_->findChild<QSlider*>(
        QStringLiteral("sldIntensity"));
    sld->setValue(700);
    QApplication::processEvents();

    QCOMPARE(spy.count(), 0);
  }

  /**
   * @brief Controller's intensityChanged must sync both spinbox and slider
   *        without emitting another setIntensity call.
   */
  void test_onIntensityChanged_syncsSpinboxAndSlider() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    ctrl_->setIntensity(75.0);
    QApplication::processEvents();

    auto* spn = panel_->findChild<QDoubleSpinBox*>(
        QStringLiteral("spnIntensity"));
    auto* sld = panel_->findChild<QSlider*>(
        QStringLiteral("sldIntensity"));

    QCOMPARE(spn->value(), 75.0);
    QCOMPARE(sld->value(), 750);  // 75.0 * kSliderScale(10)
  }

  /**
   * @brief Controller's intensityChanged must update the LCD readout.
   */
  void test_onIntensityChanged_updatesLcd() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    ctrl_->setIntensity(33.0);
    QApplication::processEvents();

    auto* lcd = panel_->findChild<QLCDNumber*>(
        QStringLiteral("lcdIntensity"));
    QCOMPARE(lcd->value(), 33.0);
  }

  // =========================================================================
  // G. Power state UI
  // =========================================================================

  /**
   * @brief After setPowerOn(true) the power-value label must contain "ON".
   */
  void test_onPowerStateChanged_true_labelContainsON() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    ctrl_->setPowerOn(true);
    QApplication::processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblPowerValue"));
    QVERIFY2(lbl->text().contains(QStringLiteral("ON")),
             "lblPowerValue must contain ON after power is turned on");
  }

  /**
   * @brief After setPowerOn(false) the power-value label must contain "OFF".
   */
  void test_onPowerStateChanged_false_labelContainsOFF() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    ctrl_->setPowerOn(true);
    ctrl_->setPowerOn(false);
    QApplication::processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblPowerValue"));
    QVERIFY2(lbl->text().contains(QStringLiteral("OFF")),
             "lblPowerValue must contain OFF after power is turned off");
  }

  /**
   * @brief After setPowerOn(true) the power button must be checked and
   *        display "LED ON".
   */
  void test_onPowerStateChanged_true_buttonCheckedAndTextOn() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    ctrl_->setPowerOn(true);
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnPower"));
    QVERIFY2(btn->isChecked(),
             "Power button must be checked after setPowerOn(true)");
    QCOMPARE(btn->text(), QStringLiteral("LED ON"));
  }

  /**
   * @brief After setPowerOn(false) the power button must be unchecked and
   *        display "LED OFF".
   */
  void test_onPowerStateChanged_false_buttonUncheckedAndTextOff() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    ctrl_->setPowerOn(true);
    ctrl_->setPowerOn(false);
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnPower"));
    QVERIFY2(!btn->isChecked(),
             "Power button must be unchecked after setPowerOn(false)");
    QCOMPARE(btn->text(), QStringLiteral("LED OFF"));
  }

  // =========================================================================
  // H. Button states per connection state
  // =========================================================================

  /**
   * @brief Connect button must be enabled when device is kDisconnected.
   */
  void test_buttonStates_disconnected_connectEnabled() {
    panel_->setController(ctrl_);
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnConnect"));
    QVERIFY2(btn->isEnabled(),
             "btnConnect must be enabled when device is kDisconnected");
  }

  /**
   * @brief Connect button must be re-enabled after kConnected.
   */
  void test_buttonStates_connected_connectEnabled() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnConnect"));
    QVERIFY2(btn->isEnabled(),
             "btnConnect must be enabled when device is kConnected");
  }

  /**
   * @brief setController(nullptr) after a connected controller — controls
   *        group must become disabled even though the group was enabled.
   */
  void test_setController_nullptr_afterConnected_controlsDisabled() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    // Verify controls are enabled BEFORE detach.
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpControls"));
    QVERIFY(grp->isEnabled());

    panel_->setController(nullptr);
    QApplication::processEvents();

    QVERIFY2(!grp->isEnabled(),
             "Controls group must be disabled after setController(nullptr) "
             "when previously connected");
  }

  // =========================================================================
  // I. Rapid operations (stress boundary)
  // =========================================================================

  /**
   * @brief Toggling the power button rapidly must not crash and each toggle
   *        must reach the controller.
   */
  void test_powerButton_rapidToggle_nocrash() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnPower"));

    QSignalSpy spy(
        ctrl_,
        &mwa::hardware::LedControllerInterface::powerStateChanged);

    for (int i = 0; i < 10; ++i) {
      btn->setChecked(i % 2 == 0);
      QApplication::processEvents();
    }

    QVERIFY2(spy.count() >= 10,
             "Each rapid power toggle must reach the controller");
  }

  /**
   * @brief Setting intensity to boundary values (0 and 100) must not crash
   *        and must display correctly in the LCD.
   */
  void test_intensityBoundary_zeroAndHundred_nocrash() {
    panel_->setController(ctrl_);
    connectSync(*ctrl_);
    QApplication::processEvents();

    ctrl_->setIntensity(0.0);
    QApplication::processEvents();

    auto* lcd = panel_->findChild<QLCDNumber*>(
        QStringLiteral("lcdIntensity"));
    QCOMPARE(lcd->value(), 0.0);

    ctrl_->setIntensity(100.0);
    QApplication::processEvents();
    QCOMPARE(lcd->value(), 100.0);
  }
};

QTEST_APPLESS_MAIN(TestLedPanel)
#include "test_led_panel.moc"
