/**
 * @file test_mock_pump_controller.cpp
 * @brief Adversarial tests for mwa::hardware::MockPumpController.
 * @date 2026-03-22
 * @copyright LGPL-3.0-or-later
 */

#include <QObject>
#include <QSignalSpy>
#include <QString>
#include <QtTest>

#include "hardware/pump/mock_pump_controller.h"

using mwa::hardware::MockPumpController;
using mwa::hardware::DeviceInterface;
using DeviceState = DeviceInterface::DeviceState;
using DeviceType  = DeviceInterface::DeviceType;

class TestMockPumpController : public QObject {
  Q_OBJECT

 private slots:
  // A. Construction & initial state
  void test_initialState_isDisconnected();
  void test_initialState_isConnectedFalse();
  void test_initialState_flowRateDefault();
  void test_initialState_targetVolumeDefault();
  void test_initialState_positionZero();
  void test_initialState_notInfusing();
  void test_deviceType_returnsPump();
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

  // D. Device-specific methods & signals
  void test_setFlowRate_emitsFlowRateChanged();
  void test_setFlowRate_getterReturnsSetValue();
  void test_setTargetVolume_getterReturnsSetValue();
  void test_startInfusion_emitsInfusionStarted();
  void test_startInfusion_isInfusingTrue();
  void test_stopInfusion_emitsInfusionStopped();
  void test_stopInfusion_isInfusingFalse();
  void test_refill_resetsPositionToZero();
  void test_refill_emitsPositionChanged();
  void test_refill_stopsInfusion();

  // E. Edge cases
  void test_doubleStartInfusion_noExtraSignals();
  void test_doubleStopInfusion_noExtraSignals();
  void test_refillWhileInfusing_stopsInfusionAndResets();
  void test_setFlowRate_doesNotEmitStateChanged();
};

// ---------------------------------------------------------------------------
// A. Construction & initial state
// ---------------------------------------------------------------------------

void TestMockPumpController::test_initialState_isDisconnected() {
  MockPumpController ctrl;
  QCOMPARE(ctrl.state(), DeviceState::kDisconnected);
}

void TestMockPumpController::test_initialState_isConnectedFalse() {
  MockPumpController ctrl;
  QVERIFY2(!ctrl.isConnected(),
           "isConnected() must be false before connectDevice()");
}

void TestMockPumpController::test_initialState_flowRateDefault() {
  MockPumpController ctrl;
  QCOMPARE(ctrl.flowRate(), 10.0);
}

void TestMockPumpController::test_initialState_targetVolumeDefault() {
  MockPumpController ctrl;
  QCOMPARE(ctrl.targetVolume(), 100.0);
}

void TestMockPumpController::test_initialState_positionZero() {
  MockPumpController ctrl;
  QCOMPARE(ctrl.currentPosition(), 0.0);
}

void TestMockPumpController::test_initialState_notInfusing() {
  MockPumpController ctrl;
  QVERIFY2(!ctrl.isInfusing(),
           "Pump must not be infusing on construction");
}

void TestMockPumpController::test_deviceType_returnsPump() {
  MockPumpController ctrl;
  QCOMPARE(ctrl.deviceType(), DeviceType::kPump);
}

void TestMockPumpController::test_deviceName_returnsExpectedString() {
  MockPumpController ctrl;
  QCOMPARE(ctrl.deviceName(), QStringLiteral("Mock Pump Controller"));
}

// ---------------------------------------------------------------------------
// B. Connect / disconnect cycle
// ---------------------------------------------------------------------------

void TestMockPumpController::
    test_connectDevice_emitsKConnectingThenKConnected() {
  MockPumpController ctrl;
  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);

  ctrl.connectDevice();
  QVERIFY2(spy.count() >= 1,
           "stateChanged(kConnecting) must fire synchronously");
  QCOMPARE(qvariant_cast<DeviceState>(spy.at(0).at(0)),
           DeviceState::kConnecting);

  QTest::qWait(600);
  QCOMPARE(spy.count(), 2);
  QCOMPARE(qvariant_cast<DeviceState>(spy.at(1).at(0)),
           DeviceState::kConnected);
}

void TestMockPumpController::test_connectDevice_stateIsConnectedAfterTimer() {
  MockPumpController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  QCOMPARE(ctrl.state(), DeviceState::kConnected);
}

void TestMockPumpController::test_connectDevice_isConnectedTrueAfterTimer() {
  MockPumpController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  QVERIFY2(ctrl.isConnected(),
           "isConnected() must be true after the connection timer fires");
}

void TestMockPumpController::test_disconnectDevice_emitsKDisconnected() {
  MockPumpController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);

  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.disconnectDevice();
  QCOMPARE(spy.count(), 1);
  QCOMPARE(qvariant_cast<DeviceState>(spy.at(0).at(0)),
           DeviceState::kDisconnected);
}

void TestMockPumpController::test_disconnectDevice_isConnectedFalse() {
  MockPumpController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  ctrl.disconnectDevice();
  QVERIFY2(!ctrl.isConnected(),
           "isConnected() must be false after disconnectDevice()");
}

void TestMockPumpController::test_disconnectDevice_stateIsDisconnected() {
  MockPumpController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  ctrl.disconnectDevice();
  QCOMPARE(ctrl.state(), DeviceState::kDisconnected);
}

// ---------------------------------------------------------------------------
// C. Double-connect / double-disconnect guards
// ---------------------------------------------------------------------------

void TestMockPumpController::test_doubleConnect_noExtraSignals() {
  MockPumpController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.connectDevice();
  QTest::qWait(600);
  QCOMPARE(spy.count(), 0);
}

void TestMockPumpController::test_connectWhileConnecting_noExtraSignals() {
  MockPumpController ctrl;
  ctrl.connectDevice();
  // Now in kConnecting — second call must be silently ignored.
  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.connectDevice();
  QTest::qWait(600);
  // Only the kConnected emission from the first timer is allowed.
  QCOMPARE(spy.count(), 1);
  QCOMPARE(qvariant_cast<DeviceState>(spy.at(0).at(0)),
           DeviceState::kConnected);
}

void TestMockPumpController::test_doubleDisconnect_noExtraSignals() {
  MockPumpController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  ctrl.disconnectDevice();
  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.disconnectDevice();
  QTest::qWait(50);
  QCOMPARE(spy.count(), 0);
}

// ---------------------------------------------------------------------------
// D. Device-specific methods & signals
// ---------------------------------------------------------------------------

void TestMockPumpController::test_setFlowRate_emitsFlowRateChanged() {
  MockPumpController ctrl;
  QSignalSpy spy(
      &ctrl,
      &mwa::hardware::PumpControllerInterface::flowRateChanged);
  ctrl.setFlowRate(25.0);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toDouble(), 25.0);
}

void TestMockPumpController::test_setFlowRate_getterReturnsSetValue() {
  MockPumpController ctrl;
  ctrl.setFlowRate(55.5);
  QCOMPARE(ctrl.flowRate(), 55.5);
}

void TestMockPumpController::test_setTargetVolume_getterReturnsSetValue() {
  MockPumpController ctrl;
  ctrl.setTargetVolume(250.0);
  QCOMPARE(ctrl.targetVolume(), 250.0);
}

void TestMockPumpController::test_startInfusion_emitsInfusionStarted() {
  MockPumpController ctrl;
  QSignalSpy spy(
      &ctrl,
      &mwa::hardware::PumpControllerInterface::infusionStarted);
  ctrl.startInfusion();
  QCOMPARE(spy.count(), 1);
}

void TestMockPumpController::test_startInfusion_isInfusingTrue() {
  MockPumpController ctrl;
  ctrl.startInfusion();
  QVERIFY2(ctrl.isInfusing(),
           "isInfusing() must be true immediately after startInfusion()");
}

void TestMockPumpController::test_stopInfusion_emitsInfusionStopped() {
  MockPumpController ctrl;
  ctrl.startInfusion();
  QSignalSpy spy(
      &ctrl,
      &mwa::hardware::PumpControllerInterface::infusionStopped);
  ctrl.stopInfusion();
  QCOMPARE(spy.count(), 1);
}

void TestMockPumpController::test_stopInfusion_isInfusingFalse() {
  MockPumpController ctrl;
  ctrl.startInfusion();
  ctrl.stopInfusion();
  QVERIFY2(!ctrl.isInfusing(),
           "isInfusing() must be false after stopInfusion()");
}

void TestMockPumpController::test_refill_resetsPositionToZero() {
  MockPumpController ctrl;
  // Force a non-zero position indirectly by calling refill from a
  // non-zero position context — here we just verify the post-condition.
  ctrl.refill();
  QCOMPARE(ctrl.currentPosition(), 0.0);
}

void TestMockPumpController::test_refill_emitsPositionChanged() {
  MockPumpController ctrl;
  QSignalSpy spy(
      &ctrl,
      &mwa::hardware::PumpControllerInterface::positionChanged);
  ctrl.refill();
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toDouble(), 0.0);
}

void TestMockPumpController::test_refill_stopsInfusion() {
  MockPumpController ctrl;
  ctrl.startInfusion();
  QVERIFY(ctrl.isInfusing());
  ctrl.refill();
  QVERIFY2(!ctrl.isInfusing(),
           "refill() must stop an active infusion");
}

// ---------------------------------------------------------------------------
// E. Edge cases
// ---------------------------------------------------------------------------

void TestMockPumpController::test_doubleStartInfusion_noExtraSignals() {
  MockPumpController ctrl;
  ctrl.startInfusion();
  QSignalSpy spy(
      &ctrl,
      &mwa::hardware::PumpControllerInterface::infusionStarted);
  ctrl.startInfusion();
  QCOMPARE(spy.count(), 0);
}

void TestMockPumpController::test_doubleStopInfusion_noExtraSignals() {
  MockPumpController ctrl;
  ctrl.startInfusion();
  ctrl.stopInfusion();
  QSignalSpy spy(
      &ctrl,
      &mwa::hardware::PumpControllerInterface::infusionStopped);
  ctrl.stopInfusion();
  QCOMPARE(spy.count(), 0);
}

void TestMockPumpController::
    test_refillWhileInfusing_stopsInfusionAndResets() {
  MockPumpController ctrl;
  ctrl.startInfusion();
  ctrl.refill();
  QVERIFY2(!ctrl.isInfusing(),
           "refill() while infusing must stop infusion");
  QCOMPARE(ctrl.currentPosition(), 0.0);
}

void TestMockPumpController::test_setFlowRate_doesNotEmitStateChanged() {
  MockPumpController ctrl;
  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.setFlowRate(30.0);
  QTest::qWait(50);
  QCOMPARE(spy.count(), 0);
}

// ---------------------------------------------------------------------------
QTEST_MAIN(TestMockPumpController)
#include "test_mock_pump_controller.moc"
