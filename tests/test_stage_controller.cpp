/**
 * @file test_stage_controller.cpp
 * @brief Tests for mwa::hardware::StageController (no hardware required).
 * @date 2026-03-27
 *
 * Exercises construction, initial state, port/baud configuration, and
 * the connected-state guards that prevent commands from being enqueued
 * when the device is disconnected.
 *
 * @copyright LGPL-3.0-or-later
 */

#include <QObject>
#include <QSignalSpy>
#include <QString>
#include <QtTest>

#include "hardware/stage/stage_controller.h"

using mwa::hardware::DeviceInterface;
using mwa::hardware::StageController;

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------
class TestStageController : public QObject {
  Q_OBJECT

 private slots:
  // -- A. Construction & initial state --------------------------------------
  void test_initialState_isDisconnected();
  void test_initialState_isNotConnected();
  void test_initialState_positionsAreZero();
  void test_initialState_speedIsOne();
  void test_initialState_notMoving();
  void test_initialState_deviceName();

  // -- B. Port & baud configuration -----------------------------------------
  void test_setPortName_beforeConnect();
  void test_setBaudRate_beforeConnect();
  void test_defaultBaudRate_is115200();

  // -- C. Connected-state guards --------------------------------------------
  void test_home_noOpWhenDisconnected();
  void test_moveAbsolute_noOpWhenDisconnected();
  void test_moveRelative_noOpWhenDisconnected();
  void test_stopMotion_noOpWhenDisconnected();
  void test_setSpeed_noOpWhenDisconnected();
  void test_disconnectDevice_noOpWhenAlreadyDisconnected();
};

// ---------------------------------------------------------------------------
// A. Construction & initial state
// ---------------------------------------------------------------------------
void TestStageController::test_initialState_isDisconnected() {
  StageController ctrl;
  QCOMPARE(ctrl.state(), DeviceInterface::DeviceState::kDisconnected);
}

void TestStageController::test_initialState_isNotConnected() {
  StageController ctrl;
  QVERIFY(!ctrl.isConnected());
}

void TestStageController::test_initialState_positionsAreZero() {
  StageController ctrl;
  QCOMPARE(ctrl.positionX(), 0.0);
  QCOMPARE(ctrl.positionY(), 0.0);
  QCOMPARE(ctrl.positionZ(), 0.0);
}

void TestStageController::test_initialState_speedIsOne() {
  StageController ctrl;
  QCOMPARE(ctrl.speed(), 1.0);
}

void TestStageController::test_initialState_notMoving() {
  StageController ctrl;
  QVERIFY(!ctrl.isMoving());
}

void TestStageController::test_initialState_deviceName() {
  StageController ctrl;
  QCOMPARE(ctrl.deviceName(), QStringLiteral("Stage Controller"));
}

// ---------------------------------------------------------------------------
// B. Port & baud configuration
// ---------------------------------------------------------------------------
void TestStageController::test_setPortName_beforeConnect() {
  StageController ctrl;
  ctrl.setPortName(QStringLiteral("COM3"));
  QCOMPARE(ctrl.portName(), QStringLiteral("COM3"));
  QCOMPARE(ctrl.deviceName(),
           QStringLiteral("Stage Controller (COM3)"));
}

void TestStageController::test_setBaudRate_beforeConnect() {
  StageController ctrl;
  ctrl.setBaudRate(9600);
  QCOMPARE(ctrl.baudRate(), 9600);
}

void TestStageController::test_defaultBaudRate_is115200() {
  StageController ctrl;
  QCOMPARE(ctrl.baudRate(), 115200);
}

// ---------------------------------------------------------------------------
// C. Connected-state guards
// ---------------------------------------------------------------------------
void TestStageController::test_home_noOpWhenDisconnected() {
  StageController ctrl;
  QSignalSpy spy(&ctrl, &StageController::homeComplete);

  ctrl.home();
  QTest::qWait(100);

  QCOMPARE(spy.count(), 0);
}

void TestStageController::test_moveAbsolute_noOpWhenDisconnected() {
  StageController ctrl;
  QSignalSpy spy(&ctrl, &StageController::moveComplete);

  ctrl.moveAbsolute(10.0, 20.0, 5.0);
  QTest::qWait(100);

  QCOMPARE(spy.count(), 0);
  QCOMPARE(ctrl.positionX(), 0.0);
}

void TestStageController::test_moveRelative_noOpWhenDisconnected() {
  StageController ctrl;
  QSignalSpy spy(&ctrl, &StageController::moveComplete);

  ctrl.moveRelative(1.0, 2.0, 3.0);
  QTest::qWait(100);

  QCOMPARE(spy.count(), 0);
  QCOMPARE(ctrl.positionX(), 0.0);
}

void TestStageController::test_stopMotion_noOpWhenDisconnected() {
  StageController ctrl;
  QSignalSpy spy(&ctrl, &StageController::errorOccurred);

  ctrl.stopMotion();
  QTest::qWait(100);

  // No error emitted — command silently ignored.
  QCOMPARE(spy.count(), 0);
}

void TestStageController::test_setSpeed_noOpWhenDisconnected() {
  StageController ctrl;
  QSignalSpy spy(&ctrl, &StageController::speedChanged);

  ctrl.setSpeed(10.0);
  QTest::qWait(100);

  QCOMPARE(spy.count(), 0);
  QCOMPARE(ctrl.speed(), 1.0);
}

void TestStageController::test_disconnectDevice_noOpWhenAlreadyDisconnected() {
  StageController ctrl;
  QSignalSpy spy(&ctrl, &StageController::stateChanged);

  ctrl.disconnectDevice();
  QTest::qWait(100);

  QCOMPARE(spy.count(), 0);
}

// ---------------------------------------------------------------------------
QTEST_MAIN(TestStageController)
#include "test_stage_controller.moc"
