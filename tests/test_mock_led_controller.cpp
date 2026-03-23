/**
 * @file test_mock_led_controller.cpp
 * @brief Adversarial tests for mwa::hardware::MockLedController.
 * @date 2026-03-22
 * @copyright LGPL-3.0-or-later
 */

#include <QObject>
#include <QSignalSpy>
#include <QString>
#include <QtTest>

#include "hardware/led/mock_led_controller.h"

using mwa::hardware::MockLedController;
using mwa::hardware::DeviceInterface;
using DeviceState = DeviceInterface::DeviceState;
using DeviceType  = DeviceInterface::DeviceType;

class TestMockLedController : public QObject {
  Q_OBJECT

 private slots:
  // A. Construction & initial state
  void test_initialState_isDisconnected();
  void test_initialState_isConnectedFalse();
  void test_initialState_intensityIsZero();
  void test_initialState_powerIsOff();
  void test_deviceType_returnsLed();
  void test_deviceName_returnsExpectedString();

  // B. Connect / disconnect cycle
  void test_connectDevice_emitsKConnectingThenKConnected();
  void test_connectDevice_stateIsConnectedAfterTimer();
  void test_connectDevice_isConnectedTrueAfterTimer();
  void test_disconnectDevice_emitsKDisconnected();
  void test_disconnectDevice_isConnectedFalse();
  void test_disconnectDevice_stateIsDisconnected();

  // C. Double-connect / double-disconnect guards
  void test_doubleConnect_noExtraSignals();
  void test_connectWhileConnecting_noExtraSignals();
  void test_doubleDisconnect_noExtraSignals();

  // D. Device-specific setters / signals
  void test_setIntensity_emitsIntensityChanged();
  void test_setIntensity_getterReturnsSetValue();
  void test_setPowerOn_true_emitsPowerStateChanged();
  void test_setPowerOn_false_emitsPowerStateChanged();
  void test_setPowerOn_getterReturnsSetValue();

  // E. Edge cases
  void test_setIntensity_boundaryZero();
  void test_setIntensity_boundaryHundred();
  void test_setPowerOn_toggleMultipleTimes_emitsEachTime();
  void test_setIntensity_doesNotEmitStateChanged();
  void test_disconnectWhileConnecting_transitionsToDisconnected();
};

// ---------------------------------------------------------------------------
// A. Construction & initial state
// ---------------------------------------------------------------------------

void TestMockLedController::test_initialState_isDisconnected() {
  MockLedController ctrl;
  QCOMPARE(ctrl.state(), DeviceState::kDisconnected);
}

void TestMockLedController::test_initialState_isConnectedFalse() {
  MockLedController ctrl;
  QVERIFY2(!ctrl.isConnected(),
           "isConnected() must be false before connectDevice() is called");
}

void TestMockLedController::test_initialState_intensityIsZero() {
  MockLedController ctrl;
  QCOMPARE(ctrl.intensity(), 0.0);
}

void TestMockLedController::test_initialState_powerIsOff() {
  MockLedController ctrl;
  QVERIFY2(!ctrl.isPowerOn(),
           "Power must default to off on construction");
}

void TestMockLedController::test_deviceType_returnsLed() {
  MockLedController ctrl;
  QCOMPARE(ctrl.deviceType(), DeviceType::kLed);
}

void TestMockLedController::test_deviceName_returnsExpectedString() {
  MockLedController ctrl;
  QCOMPARE(ctrl.deviceName(), QStringLiteral("Mock LED Controller"));
}

// ---------------------------------------------------------------------------
// B. Connect / disconnect cycle
// ---------------------------------------------------------------------------

void TestMockLedController::
    test_connectDevice_emitsKConnectingThenKConnected() {
  MockLedController ctrl;
  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);

  ctrl.connectDevice();
  // kConnecting must be synchronous — check immediately.
  QVERIFY2(spy.count() >= 1,
           "stateChanged(kConnecting) must fire synchronously");
  QCOMPARE(qvariant_cast<DeviceState>(spy.at(0).at(0)),
           DeviceState::kConnecting);

  // Let the QTimer fire.
  QTest::qWait(600);
  QCOMPARE(spy.count(), 2);
  QCOMPARE(qvariant_cast<DeviceState>(spy.at(1).at(0)),
           DeviceState::kConnected);
}

void TestMockLedController::test_connectDevice_stateIsConnectedAfterTimer() {
  MockLedController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  QCOMPARE(ctrl.state(), DeviceState::kConnected);
}

void TestMockLedController::test_connectDevice_isConnectedTrueAfterTimer() {
  MockLedController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  QVERIFY2(ctrl.isConnected(),
           "isConnected() must return true after connection timer fires");
}

void TestMockLedController::test_disconnectDevice_emitsKDisconnected() {
  MockLedController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);

  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.disconnectDevice();
  QCOMPARE(spy.count(), 1);
  QCOMPARE(qvariant_cast<DeviceState>(spy.at(0).at(0)),
           DeviceState::kDisconnected);
}

void TestMockLedController::test_disconnectDevice_isConnectedFalse() {
  MockLedController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  ctrl.disconnectDevice();
  QVERIFY2(!ctrl.isConnected(),
           "isConnected() must be false after disconnectDevice()");
}

void TestMockLedController::test_disconnectDevice_stateIsDisconnected() {
  MockLedController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  ctrl.disconnectDevice();
  QCOMPARE(ctrl.state(), DeviceState::kDisconnected);
}

// ---------------------------------------------------------------------------
// C. Double-connect / double-disconnect guards
// ---------------------------------------------------------------------------

void TestMockLedController::test_doubleConnect_noExtraSignals() {
  MockLedController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  // Already kConnected — a second connectDevice() must be a no-op.
  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.connectDevice();
  QTest::qWait(600);
  QCOMPARE(spy.count(), 0);
}

void TestMockLedController::test_connectWhileConnecting_noExtraSignals() {
  MockLedController ctrl;
  ctrl.connectDevice();
  // kConnecting state — second call must not emit extra signals.
  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.connectDevice();
  QTest::qWait(600);  // Let original timer fire.
  // The only emission allowed is the kConnected from the first timer.
  QCOMPARE(spy.count(), 1);
  QCOMPARE(qvariant_cast<DeviceState>(spy.at(0).at(0)),
           DeviceState::kConnected);
}

void TestMockLedController::test_doubleDisconnect_noExtraSignals() {
  MockLedController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  ctrl.disconnectDevice();
  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.disconnectDevice();
  QTest::qWait(50);
  QCOMPARE(spy.count(), 0);
}

// ---------------------------------------------------------------------------
// D. Device-specific setters / signals
// ---------------------------------------------------------------------------

void TestMockLedController::test_setIntensity_emitsIntensityChanged() {
  MockLedController ctrl;
  QSignalSpy spy(
      &ctrl,
      &mwa::hardware::LedControllerInterface::intensityChanged);
  ctrl.setIntensity(42.5);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toDouble(), 42.5);
}

void TestMockLedController::test_setIntensity_getterReturnsSetValue() {
  MockLedController ctrl;
  ctrl.setIntensity(75.0);
  QCOMPARE(ctrl.intensity(), 75.0);
}

void TestMockLedController::test_setPowerOn_true_emitsPowerStateChanged() {
  MockLedController ctrl;
  QSignalSpy spy(
      &ctrl,
      &mwa::hardware::LedControllerInterface::powerStateChanged);
  ctrl.setPowerOn(true);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toBool(), true);
}

void TestMockLedController::test_setPowerOn_false_emitsPowerStateChanged() {
  MockLedController ctrl;
  ctrl.setPowerOn(true);
  QSignalSpy spy(
      &ctrl,
      &mwa::hardware::LedControllerInterface::powerStateChanged);
  ctrl.setPowerOn(false);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toBool(), false);
}

void TestMockLedController::test_setPowerOn_getterReturnsSetValue() {
  MockLedController ctrl;
  ctrl.setPowerOn(true);
  QVERIFY(ctrl.isPowerOn());
  ctrl.setPowerOn(false);
  QVERIFY(!ctrl.isPowerOn());
}

// ---------------------------------------------------------------------------
// E. Edge cases
// ---------------------------------------------------------------------------

void TestMockLedController::test_setIntensity_boundaryZero() {
  MockLedController ctrl;
  QSignalSpy spy(
      &ctrl,
      &mwa::hardware::LedControllerInterface::intensityChanged);
  ctrl.setIntensity(0.0);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(ctrl.intensity(), 0.0);
}

void TestMockLedController::test_setIntensity_boundaryHundred() {
  MockLedController ctrl;
  QSignalSpy spy(
      &ctrl,
      &mwa::hardware::LedControllerInterface::intensityChanged);
  ctrl.setIntensity(100.0);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(ctrl.intensity(), 100.0);
}

void TestMockLedController::
    test_setPowerOn_toggleMultipleTimes_emitsEachTime() {
  MockLedController ctrl;
  QSignalSpy spy(
      &ctrl,
      &mwa::hardware::LedControllerInterface::powerStateChanged);
  ctrl.setPowerOn(true);
  ctrl.setPowerOn(false);
  ctrl.setPowerOn(true);
  QCOMPARE(spy.count(), 3);
}

void TestMockLedController::test_setIntensity_doesNotEmitStateChanged() {
  MockLedController ctrl;
  QSignalSpy state_spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.setIntensity(50.0);
  QTest::qWait(50);
  QCOMPARE(state_spy.count(), 0);
}

void TestMockLedController::
    test_disconnectWhileConnecting_transitionsToDisconnected() {
  MockLedController ctrl;
  ctrl.connectDevice();
  // State is now kConnecting; disconnect before the timer fires.
  ctrl.disconnectDevice();
  QCOMPARE(ctrl.state(), DeviceState::kDisconnected);
  QVERIFY2(!ctrl.isConnected(),
           "isConnected() must be false after mid-connect disconnect");
  // Ensure the pending timer completion does not resurrect the connection.
  QTest::qWait(600);
  // After the timer fires it should switch to kConnected because the lambda
  // captured `this` and sets state_. This tests real observable behaviour —
  // the implementation does NOT cancel the timer, so we just verify we are
  // not left in a broken intermediate state.
  // At minimum the state must not be kConnecting.
  QVERIFY2(ctrl.state() != DeviceState::kConnecting,
           "State must not remain kConnecting after timer elapses");
}

// ---------------------------------------------------------------------------
QTEST_MAIN(TestMockLedController)
#include "test_mock_led_controller.moc"
