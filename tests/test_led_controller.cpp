/**
 * @file test_led_controller.cpp
 * @brief Tests for mwa::hardware::LedController (no hardware required).
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

#include "hardware/led/led_controller.h"

using mwa::hardware::DeviceInterface;
using mwa::hardware::LedController;

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------
class TestLedController : public QObject {
  Q_OBJECT

 private slots:
  // -- A. Construction & initial state --------------------------------------
  void test_initialState_isDisconnected();
  void test_initialState_isNotConnected();
  void test_initialState_intensityIsZero();
  void test_initialState_powerIsOff();
  void test_initialState_deviceName();

  // -- B. Port & baud configuration -----------------------------------------
  void test_setPortName_beforeConnect();
  void test_setBaudRate_beforeConnect();
  void test_defaultBaudRate_is9600();

  // -- C. Connected-state guards --------------------------------------------
  void test_setIntensity_noOpWhenDisconnected();
  void test_setIntensity_clampBelow_noOpWhenDisconnected();
  void test_setIntensity_clampAbove_noOpWhenDisconnected();
  void test_setPowerOn_noOpWhenDisconnected();
  void test_disconnectDevice_noOpWhenAlreadyDisconnected();
};

// ---------------------------------------------------------------------------
// A. Construction & initial state
// ---------------------------------------------------------------------------
void TestLedController::test_initialState_isDisconnected() {
  LedController ctrl;
  QCOMPARE(ctrl.state(), DeviceInterface::DeviceState::kDisconnected);
}

void TestLedController::test_initialState_isNotConnected() {
  LedController ctrl;
  QVERIFY(!ctrl.isConnected());
}

void TestLedController::test_initialState_intensityIsZero() {
  LedController ctrl;
  QCOMPARE(ctrl.intensity(), 0.0);
}

void TestLedController::test_initialState_powerIsOff() {
  LedController ctrl;
  QVERIFY(!ctrl.isPowerOn());
}

void TestLedController::test_initialState_deviceName() {
  LedController ctrl;
  QCOMPARE(ctrl.deviceName(), QStringLiteral("LED Controller"));
}

// ---------------------------------------------------------------------------
// B. Port & baud configuration
// ---------------------------------------------------------------------------
void TestLedController::test_setPortName_beforeConnect() {
  LedController ctrl;
  ctrl.setPortName(QStringLiteral("/dev/ttyUSB0"));
  QCOMPARE(ctrl.portName(), QStringLiteral("/dev/ttyUSB0"));
  QCOMPARE(ctrl.deviceName(),
           QStringLiteral("LED Controller (/dev/ttyUSB0)"));
}

void TestLedController::test_setBaudRate_beforeConnect() {
  LedController ctrl;
  ctrl.setBaudRate(115200);
  QCOMPARE(ctrl.baudRate(), 115200);
}

void TestLedController::test_defaultBaudRate_is9600() {
  LedController ctrl;
  QCOMPARE(ctrl.baudRate(), 9600);
}

// ---------------------------------------------------------------------------
// C. Connected-state guards
// ---------------------------------------------------------------------------
void TestLedController::test_setIntensity_noOpWhenDisconnected() {
  LedController ctrl;
  QSignalSpy spy(&ctrl, &LedController::intensityChanged);

  ctrl.setIntensity(50.0);
  QTest::qWait(100);

  // No signal emitted — command was not enqueued.
  QCOMPARE(spy.count(), 0);
  QCOMPARE(ctrl.intensity(), 0.0);
}

void TestLedController::test_setIntensity_clampBelow_noOpWhenDisconnected() {
  LedController ctrl;
  QSignalSpy spy(&ctrl, &LedController::intensityChanged);

  // Clamped to 0.0 — same as initial intensity, and device is disconnected.
  ctrl.setIntensity(-10.0);
  QTest::qWait(100);

  QCOMPARE(spy.count(), 0);
  QCOMPARE(ctrl.intensity(), 0.0);
}

void TestLedController::test_setIntensity_clampAbove_noOpWhenDisconnected() {
  LedController ctrl;
  QSignalSpy spy(&ctrl, &LedController::intensityChanged);

  // Clamped to 100.0 — device is disconnected, so no command is enqueued.
  ctrl.setIntensity(150.0);
  QTest::qWait(100);

  QCOMPARE(spy.count(), 0);
  QCOMPARE(ctrl.intensity(), 0.0);
}

void TestLedController::test_setPowerOn_noOpWhenDisconnected() {
  LedController ctrl;
  QSignalSpy spy(&ctrl, &LedController::powerStateChanged);

  ctrl.setPowerOn(true);
  QTest::qWait(100);

  QCOMPARE(spy.count(), 0);
  QVERIFY(!ctrl.isPowerOn());
}

void TestLedController::test_disconnectDevice_noOpWhenAlreadyDisconnected() {
  LedController ctrl;
  QSignalSpy spy(&ctrl, &LedController::stateChanged);

  ctrl.disconnectDevice();
  QTest::qWait(100);

  // Already disconnected — no state change emitted.
  QCOMPARE(spy.count(), 0);
}

// ---------------------------------------------------------------------------
QTEST_MAIN(TestLedController)
#include "test_led_controller.moc"
