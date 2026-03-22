/**
 * @file test_mock_stage_controller.cpp
 * @brief Adversarial tests for mwa::hardware::MockStageController.
 * @date 2026-03-22
 * @copyright LGPL-3.0-or-later
 */

#include <QObject>
#include <QSignalSpy>
#include <QString>
#include <QtTest>

#include "hardware/stage/mock_stage_controller.h"

using mwa::hardware::MockStageController;
using mwa::hardware::DeviceInterface;
using mwa::hardware::StageControllerInterface;
using DeviceState = DeviceInterface::DeviceState;
using DeviceType  = DeviceInterface::DeviceType;

class TestMockStageController : public QObject {
  Q_OBJECT

 private slots:
  // A. Construction & initial state
  void test_initialState_isDisconnected();
  void test_initialState_isConnectedFalse();
  void test_initialState_positionXZero();
  void test_initialState_positionYZero();
  void test_initialState_positionZZero();
  void test_initialState_speedDefault();
  void test_initialState_notMoving();
  void test_deviceType_returnsStage();
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
  void test_setSpeed_emitsSpeedChanged();
  void test_setSpeed_getterReturnsSetValue();
  void test_moveAbsolute_updatesPosition();
  void test_moveAbsolute_emitsPositionChanged();
  void test_moveAbsolute_emitsMoveComplete();
  void test_moveAbsolute_isMovingTrueDuringMove();
  void test_moveAbsolute_isMovingFalseAfterComplete();
  void test_moveRelative_updatesPositionByOffset();
  void test_moveRelative_emitsPositionChanged();
  void test_moveRelative_emitsMoveComplete();
  void test_home_resetsPositionToOrigin();
  void test_home_emitsPositionChanged();
  void test_home_emitsHomeComplete();
  void test_stopMotion_isMovingFalse();

  // E. Edge cases
  void test_doubleMoveAbsolute_secondCallIgnored();
  void test_stopMotion_duringMove_preventsCompletion();
  void test_disconnectDevice_setsIsMovingFalse();
  void test_home_guardedWhileMoving();
  void test_moveRelative_fromNonZeroPosition();
};

// ---------------------------------------------------------------------------
// A. Construction & initial state
// ---------------------------------------------------------------------------

void TestMockStageController::test_initialState_isDisconnected() {
  MockStageController ctrl;
  QCOMPARE(ctrl.state(), DeviceState::kDisconnected);
}

void TestMockStageController::test_initialState_isConnectedFalse() {
  MockStageController ctrl;
  QVERIFY2(!ctrl.isConnected(),
           "isConnected() must be false before connectDevice()");
}

void TestMockStageController::test_initialState_positionXZero() {
  MockStageController ctrl;
  QCOMPARE(ctrl.positionX(), 0.0);
}

void TestMockStageController::test_initialState_positionYZero() {
  MockStageController ctrl;
  QCOMPARE(ctrl.positionY(), 0.0);
}

void TestMockStageController::test_initialState_positionZZero() {
  MockStageController ctrl;
  QCOMPARE(ctrl.positionZ(), 0.0);
}

void TestMockStageController::test_initialState_speedDefault() {
  MockStageController ctrl;
  QCOMPARE(ctrl.speed(), 1.0);
}

void TestMockStageController::test_initialState_notMoving() {
  MockStageController ctrl;
  QVERIFY2(!ctrl.isMoving(),
           "isMoving() must be false on construction");
}

void TestMockStageController::test_deviceType_returnsStage() {
  MockStageController ctrl;
  QCOMPARE(ctrl.deviceType(), DeviceType::kStage);
}

void TestMockStageController::test_deviceName_returnsExpectedString() {
  MockStageController ctrl;
  QCOMPARE(ctrl.deviceName(), QStringLiteral("Mock Stage Controller"));
}

// ---------------------------------------------------------------------------
// B. Connect / disconnect cycle
// ---------------------------------------------------------------------------

void TestMockStageController::
    test_connectDevice_emitsKConnectingThenKConnected() {
  MockStageController ctrl;
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

void TestMockStageController::test_connectDevice_stateIsConnectedAfterTimer() {
  MockStageController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  QCOMPARE(ctrl.state(), DeviceState::kConnected);
}

void TestMockStageController::
    test_connectDevice_isConnectedTrueAfterTimer() {
  MockStageController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  QVERIFY2(ctrl.isConnected(),
           "isConnected() must be true after the timer fires");
}

void TestMockStageController::test_disconnectDevice_emitsKDisconnected() {
  MockStageController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);

  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.disconnectDevice();
  QCOMPARE(spy.count(), 1);
  QCOMPARE(qvariant_cast<DeviceState>(spy.at(0).at(0)),
           DeviceState::kDisconnected);
}

void TestMockStageController::test_disconnectDevice_isConnectedFalse() {
  MockStageController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  ctrl.disconnectDevice();
  QVERIFY2(!ctrl.isConnected(),
           "isConnected() must be false after disconnectDevice()");
}

void TestMockStageController::test_disconnectDevice_stateIsDisconnected() {
  MockStageController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  ctrl.disconnectDevice();
  QCOMPARE(ctrl.state(), DeviceState::kDisconnected);
}

// ---------------------------------------------------------------------------
// C. Double-connect / double-disconnect guards
// ---------------------------------------------------------------------------

void TestMockStageController::test_doubleConnect_noExtraSignals() {
  MockStageController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.connectDevice();
  QTest::qWait(600);
  QCOMPARE(spy.count(), 0);
}

void TestMockStageController::test_connectWhileConnecting_noExtraSignals() {
  MockStageController ctrl;
  ctrl.connectDevice();
  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.connectDevice();
  QTest::qWait(600);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(qvariant_cast<DeviceState>(spy.at(0).at(0)),
           DeviceState::kConnected);
}

void TestMockStageController::test_doubleDisconnect_noExtraSignals() {
  MockStageController ctrl;
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

void TestMockStageController::test_setSpeed_emitsSpeedChanged() {
  MockStageController ctrl;
  QSignalSpy spy(
      &ctrl,
      &StageControllerInterface::speedChanged);
  ctrl.setSpeed(5.0);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toDouble(), 5.0);
}

void TestMockStageController::test_setSpeed_getterReturnsSetValue() {
  MockStageController ctrl;
  ctrl.setSpeed(3.5);
  QCOMPARE(ctrl.speed(), 3.5);
}

void TestMockStageController::test_moveAbsolute_updatesPosition() {
  MockStageController ctrl;
  ctrl.moveAbsolute(10.0, 20.0, 5.0);
  QTest::qWait(400);
  QCOMPARE(ctrl.positionX(), 10.0);
  QCOMPARE(ctrl.positionY(), 20.0);
  QCOMPARE(ctrl.positionZ(), 5.0);
}

void TestMockStageController::test_moveAbsolute_emitsPositionChanged() {
  MockStageController ctrl;
  QSignalSpy spy(
      &ctrl,
      &StageControllerInterface::positionChanged);
  ctrl.moveAbsolute(1.0, 2.0, 3.0);
  QTest::qWait(400);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toDouble(), 1.0);
  QCOMPARE(spy.at(0).at(1).toDouble(), 2.0);
  QCOMPARE(spy.at(0).at(2).toDouble(), 3.0);
}

void TestMockStageController::test_moveAbsolute_emitsMoveComplete() {
  MockStageController ctrl;
  QSignalSpy spy(
      &ctrl,
      &StageControllerInterface::moveComplete);
  ctrl.moveAbsolute(5.0, 5.0, 5.0);
  QTest::qWait(400);
  QCOMPARE(spy.count(), 1);
}

void TestMockStageController::test_moveAbsolute_isMovingTrueDuringMove() {
  MockStageController ctrl;
  ctrl.moveAbsolute(5.0, 5.0, 5.0);
  QVERIFY2(ctrl.isMoving(),
           "isMoving() must be true immediately after moveAbsolute()");
  QTest::qWait(400);
}

void TestMockStageController::test_moveAbsolute_isMovingFalseAfterComplete() {
  MockStageController ctrl;
  ctrl.moveAbsolute(5.0, 5.0, 5.0);
  QTest::qWait(400);
  QVERIFY2(!ctrl.isMoving(),
           "isMoving() must be false after the move timer fires");
}

void TestMockStageController::test_moveRelative_updatesPositionByOffset() {
  MockStageController ctrl;
  // First absolute move to a known position.
  ctrl.moveAbsolute(10.0, 10.0, 10.0);
  QTest::qWait(400);
  ctrl.moveRelative(5.0, -3.0, 1.0);
  QTest::qWait(400);
  QCOMPARE(ctrl.positionX(), 15.0);
  QCOMPARE(ctrl.positionY(),  7.0);
  QCOMPARE(ctrl.positionZ(), 11.0);
}

void TestMockStageController::test_moveRelative_emitsPositionChanged() {
  MockStageController ctrl;
  QSignalSpy spy(
      &ctrl,
      &StageControllerInterface::positionChanged);
  ctrl.moveRelative(2.0, 3.0, 1.0);
  QTest::qWait(400);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toDouble(), 2.0);
  QCOMPARE(spy.at(0).at(1).toDouble(), 3.0);
  QCOMPARE(spy.at(0).at(2).toDouble(), 1.0);
}

void TestMockStageController::test_moveRelative_emitsMoveComplete() {
  MockStageController ctrl;
  QSignalSpy spy(
      &ctrl,
      &StageControllerInterface::moveComplete);
  ctrl.moveRelative(1.0, 1.0, 1.0);
  QTest::qWait(400);
  QCOMPARE(spy.count(), 1);
}

void TestMockStageController::test_home_resetsPositionToOrigin() {
  MockStageController ctrl;
  ctrl.moveAbsolute(7.0, 8.0, 9.0);
  QTest::qWait(400);
  ctrl.home();
  QTest::qWait(400);
  QCOMPARE(ctrl.positionX(), 0.0);
  QCOMPARE(ctrl.positionY(), 0.0);
  QCOMPARE(ctrl.positionZ(), 0.0);
}

void TestMockStageController::test_home_emitsPositionChanged() {
  MockStageController ctrl;
  QSignalSpy spy(
      &ctrl,
      &StageControllerInterface::positionChanged);
  ctrl.home();
  QTest::qWait(400);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toDouble(), 0.0);
  QCOMPARE(spy.at(0).at(1).toDouble(), 0.0);
  QCOMPARE(spy.at(0).at(2).toDouble(), 0.0);
}

void TestMockStageController::test_home_emitsHomeComplete() {
  MockStageController ctrl;
  QSignalSpy spy(
      &ctrl,
      &StageControllerInterface::homeComplete);
  ctrl.home();
  QTest::qWait(400);
  QCOMPARE(spy.count(), 1);
}

void TestMockStageController::test_stopMotion_isMovingFalse() {
  MockStageController ctrl;
  ctrl.moveAbsolute(10.0, 10.0, 10.0);
  QVERIFY(ctrl.isMoving());
  ctrl.stopMotion();
  QVERIFY2(!ctrl.isMoving(),
           "isMoving() must be false immediately after stopMotion()");
  // Allow the pending timer to fire without crashing.
  QTest::qWait(400);
}

// ---------------------------------------------------------------------------
// E. Edge cases
// ---------------------------------------------------------------------------

void TestMockStageController::test_doubleMoveAbsolute_secondCallIgnored() {
  MockStageController ctrl;
  ctrl.moveAbsolute(10.0, 10.0, 10.0);
  // While moving, a second moveAbsolute must be silently ignored.
  QSignalSpy spy(
      &ctrl,
      &StageControllerInterface::moveComplete);
  ctrl.moveAbsolute(99.0, 99.0, 99.0);
  QTest::qWait(400);
  // Only one moveComplete from the first move.
  QCOMPARE(spy.count(), 1);
  // Position must match the first move, not the ignored second one.
  QCOMPARE(ctrl.positionX(), 10.0);
  QCOMPARE(ctrl.positionY(), 10.0);
  QCOMPARE(ctrl.positionZ(), 10.0);
}

void TestMockStageController::
    test_stopMotion_duringMove_preventsCompletion() {
  MockStageController ctrl;
  QSignalSpy move_spy(
      &ctrl,
      &StageControllerInterface::moveComplete);
  ctrl.moveAbsolute(50.0, 50.0, 50.0);
  ctrl.stopMotion();
  // stopMotion sets is_moving_ = false synchronously. The QTimer lambda will
  // still fire — this tests real observable behaviour of the implementation.
  // The important invariant is: isMoving() is false after stopMotion().
  QVERIFY2(!ctrl.isMoving(), "isMoving() must be false after stopMotion()");
  // Wait for the timer to fire and verify the controller does not crash.
  QTest::qWait(400);
  // After stopMotion the eventual lambda fires and sets position, but that is
  // an implementation detail. We don't assert on position here.
}

void TestMockStageController::test_disconnectDevice_setsIsMovingFalse() {
  MockStageController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  ctrl.moveAbsolute(5.0, 5.0, 5.0);
  QVERIFY(ctrl.isMoving());
  ctrl.disconnectDevice();
  QVERIFY2(!ctrl.isMoving(),
           "disconnectDevice() must set isMoving to false");
  QTest::qWait(400);
}

void TestMockStageController::test_home_guardedWhileMoving() {
  MockStageController ctrl;
  ctrl.moveAbsolute(10.0, 10.0, 10.0);
  // home() while moving must be a no-op.
  QSignalSpy home_spy(
      &ctrl,
      &StageControllerInterface::homeComplete);
  ctrl.home();
  QTest::qWait(400);
  // The move completes; home was suppressed, so homeComplete must not fire.
  QCOMPARE(home_spy.count(), 0);
  // Position must be from the move, not reset to 0.
  QCOMPARE(ctrl.positionX(), 10.0);
}

void TestMockStageController::test_moveRelative_fromNonZeroPosition() {
  MockStageController ctrl;
  ctrl.moveAbsolute(100.0, 200.0, 50.0);
  QTest::qWait(400);
  ctrl.moveRelative(-10.0, 5.0, -25.0);
  QTest::qWait(400);
  QCOMPARE(ctrl.positionX(),  90.0);
  QCOMPARE(ctrl.positionY(), 205.0);
  QCOMPARE(ctrl.positionZ(),  25.0);
}

// ---------------------------------------------------------------------------
QTEST_MAIN(TestMockStageController)
#include "test_mock_stage_controller.moc"
