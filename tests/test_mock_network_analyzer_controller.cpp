/**
 * @file test_mock_network_analyzer_controller.cpp
 * @brief Adversarial tests for
 *        mwa::hardware::MockNetworkAnalyzerController.
 * @date 2026-03-22
 * @copyright LGPL-3.0-or-later
 */

#include <QObject>
#include <QSignalSpy>
#include <QString>
#include <QVector>
#include <QtTest>

#include "hardware/network_analyzer/mock_network_analyzer_controller.h"

using mwa::hardware::MockNetworkAnalyzerController;
using mwa::hardware::DeviceInterface;
using mwa::hardware::NetworkAnalyzerControllerInterface;
using DeviceState = DeviceInterface::DeviceState;
using DeviceType  = DeviceInterface::DeviceType;

class TestMockNetworkAnalyzerController : public QObject {
  Q_OBJECT

 private slots:
  // A. Construction & initial state
  void test_initialState_isDisconnected();
  void test_initialState_isConnectedFalse();
  void test_initialState_startFrequency1MHz();
  void test_initialState_stopFrequency100MHz();
  void test_initialState_numPoints201();
  void test_initialState_notMeasuring();
  void test_initialState_traceFrequenciesEmpty();
  void test_initialState_traceMagnitudesEmpty();
  void test_deviceType_returnsNetworkAnalyzer();
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
  void test_setFrequencyRange_emitsFrequencyRangeChanged();
  void test_setFrequencyRange_gettersReturnSetValues();
  void test_setNumPoints_emitsNumPointsChanged();
  void test_setNumPoints_getterReturnsSetValue();
  void test_measureSParameters_emitsMeasurementStarted();
  void test_measureSParameters_isMeasuringTrueWhileRunning();
  void test_measureSParameters_emitsMeasurementComplete();
  void test_measureSParameters_isMeasuringFalseAfterComplete();
  void test_measureSParameters_traceFrequenciesPopulated();
  void test_measureSParameters_traceMagnitudesPopulated();
  void test_measureSParameters_traceCountMatchesNumPoints();

  // E. Edge cases
  void test_doubleMeasure_noExtraStartedSignal();
  void test_measureSParameters_traceFirstFreqEqualsStart();
  void test_measureSParameters_traceLastFreqEqualsStop();
};

// ---------------------------------------------------------------------------
// A. Construction & initial state
// ---------------------------------------------------------------------------

void TestMockNetworkAnalyzerController::test_initialState_isDisconnected() {
  MockNetworkAnalyzerController ctrl;
  QCOMPARE(ctrl.state(), DeviceState::kDisconnected);
}

void TestMockNetworkAnalyzerController::
    test_initialState_isConnectedFalse() {
  MockNetworkAnalyzerController ctrl;
  QVERIFY2(!ctrl.isConnected(),
           "isConnected() must be false before connectDevice()");
}

void TestMockNetworkAnalyzerController::
    test_initialState_startFrequency1MHz() {
  MockNetworkAnalyzerController ctrl;
  QCOMPARE(ctrl.startFrequency(), 1.0e6);
}

void TestMockNetworkAnalyzerController::
    test_initialState_stopFrequency100MHz() {
  MockNetworkAnalyzerController ctrl;
  QCOMPARE(ctrl.stopFrequency(), 100.0e6);
}

void TestMockNetworkAnalyzerController::test_initialState_numPoints201() {
  MockNetworkAnalyzerController ctrl;
  QCOMPARE(ctrl.numPoints(), 201);
}

void TestMockNetworkAnalyzerController::test_initialState_notMeasuring() {
  MockNetworkAnalyzerController ctrl;
  QVERIFY2(!ctrl.isMeasuring(),
           "isMeasuring() must be false on construction");
}

void TestMockNetworkAnalyzerController::
    test_initialState_traceFrequenciesEmpty() {
  MockNetworkAnalyzerController ctrl;
  QVERIFY2(ctrl.traceFrequencies().isEmpty(),
           "traceFrequencies() must be empty before the first measurement");
}

void TestMockNetworkAnalyzerController::
    test_initialState_traceMagnitudesEmpty() {
  MockNetworkAnalyzerController ctrl;
  QVERIFY2(ctrl.traceMagnitudes().isEmpty(),
           "traceMagnitudes() must be empty before the first measurement");
}

void TestMockNetworkAnalyzerController::
    test_deviceType_returnsNetworkAnalyzer() {
  MockNetworkAnalyzerController ctrl;
  QCOMPARE(ctrl.deviceType(), DeviceType::kNetworkAnalyzer);
}

void TestMockNetworkAnalyzerController::
    test_deviceName_returnsExpectedString() {
  MockNetworkAnalyzerController ctrl;
  QCOMPARE(ctrl.deviceName(),
           QStringLiteral("Mock Network Analyzer Controller"));
}

// ---------------------------------------------------------------------------
// B. Connect / disconnect cycle
// ---------------------------------------------------------------------------

void TestMockNetworkAnalyzerController::
    test_connectDevice_emitsKConnectingThenKConnected() {
  MockNetworkAnalyzerController ctrl;
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

void TestMockNetworkAnalyzerController::
    test_connectDevice_stateIsConnectedAfterTimer() {
  MockNetworkAnalyzerController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  QCOMPARE(ctrl.state(), DeviceState::kConnected);
}

void TestMockNetworkAnalyzerController::
    test_connectDevice_isConnectedTrueAfterTimer() {
  MockNetworkAnalyzerController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  QVERIFY2(ctrl.isConnected(),
           "isConnected() must be true after the timer fires");
}

void TestMockNetworkAnalyzerController::
    test_disconnectDevice_emitsKDisconnected() {
  MockNetworkAnalyzerController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);

  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.disconnectDevice();
  QCOMPARE(spy.count(), 1);
  QCOMPARE(qvariant_cast<DeviceState>(spy.at(0).at(0)),
           DeviceState::kDisconnected);
}

void TestMockNetworkAnalyzerController::
    test_disconnectDevice_isConnectedFalse() {
  MockNetworkAnalyzerController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  ctrl.disconnectDevice();
  QVERIFY2(!ctrl.isConnected(),
           "isConnected() must be false after disconnectDevice()");
}

void TestMockNetworkAnalyzerController::
    test_disconnectDevice_stateIsDisconnected() {
  MockNetworkAnalyzerController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  ctrl.disconnectDevice();
  QCOMPARE(ctrl.state(), DeviceState::kDisconnected);
}

// ---------------------------------------------------------------------------
// C. Double-connect / double-disconnect guards
// ---------------------------------------------------------------------------

void TestMockNetworkAnalyzerController::test_doubleConnect_noExtraSignals() {
  MockNetworkAnalyzerController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.connectDevice();
  QTest::qWait(600);
  QCOMPARE(spy.count(), 0);
}

void TestMockNetworkAnalyzerController::
    test_connectWhileConnecting_noExtraSignals() {
  MockNetworkAnalyzerController ctrl;
  ctrl.connectDevice();
  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.connectDevice();
  QTest::qWait(600);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(qvariant_cast<DeviceState>(spy.at(0).at(0)),
           DeviceState::kConnected);
}

void TestMockNetworkAnalyzerController::
    test_doubleDisconnect_noExtraSignals() {
  MockNetworkAnalyzerController ctrl;
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

void TestMockNetworkAnalyzerController::
    test_setFrequencyRange_emitsFrequencyRangeChanged() {
  MockNetworkAnalyzerController ctrl;
  QSignalSpy spy(
      &ctrl,
      &NetworkAnalyzerControllerInterface::frequencyRangeChanged);
  ctrl.setFrequencyRange(500.0e3, 50.0e6);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toDouble(), 500.0e3);
  QCOMPARE(spy.at(0).at(1).toDouble(), 50.0e6);
}

void TestMockNetworkAnalyzerController::
    test_setFrequencyRange_gettersReturnSetValues() {
  MockNetworkAnalyzerController ctrl;
  ctrl.setFrequencyRange(2.0e6, 80.0e6);
  QCOMPARE(ctrl.startFrequency(), 2.0e6);
  QCOMPARE(ctrl.stopFrequency(), 80.0e6);
}

void TestMockNetworkAnalyzerController::
    test_setNumPoints_emitsNumPointsChanged() {
  MockNetworkAnalyzerController ctrl;
  QSignalSpy spy(
      &ctrl,
      &NetworkAnalyzerControllerInterface::numPointsChanged);
  ctrl.setNumPoints(401);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toInt(), 401);
}

void TestMockNetworkAnalyzerController::
    test_setNumPoints_getterReturnsSetValue() {
  MockNetworkAnalyzerController ctrl;
  ctrl.setNumPoints(101);
  QCOMPARE(ctrl.numPoints(), 101);
}

void TestMockNetworkAnalyzerController::
    test_measureSParameters_emitsMeasurementStarted() {
  MockNetworkAnalyzerController ctrl;
  QSignalSpy spy(
      &ctrl,
      &NetworkAnalyzerControllerInterface::measurementStarted);
  ctrl.measureSParameters();
  // measurementStarted must be synchronous.
  QCOMPARE(spy.count(), 1);
  // Allow the async timer to fire to avoid leaking a pending QTimer.
  QTest::qWait(1100);
}

void TestMockNetworkAnalyzerController::
    test_measureSParameters_isMeasuringTrueWhileRunning() {
  MockNetworkAnalyzerController ctrl;
  ctrl.measureSParameters();
  QVERIFY2(ctrl.isMeasuring(),
           "isMeasuring() must be true immediately after measureSParameters()");
  QTest::qWait(1100);
}

void TestMockNetworkAnalyzerController::
    test_measureSParameters_emitsMeasurementComplete() {
  MockNetworkAnalyzerController ctrl;
  QSignalSpy spy(
      &ctrl,
      &NetworkAnalyzerControllerInterface::measurementComplete);
  ctrl.measureSParameters();
  QTest::qWait(1100);
  QCOMPARE(spy.count(), 1);
}

void TestMockNetworkAnalyzerController::
    test_measureSParameters_isMeasuringFalseAfterComplete() {
  MockNetworkAnalyzerController ctrl;
  ctrl.measureSParameters();
  QTest::qWait(1100);
  QVERIFY2(!ctrl.isMeasuring(),
           "isMeasuring() must be false after measurementComplete fires");
}

void TestMockNetworkAnalyzerController::
    test_measureSParameters_traceFrequenciesPopulated() {
  MockNetworkAnalyzerController ctrl;
  ctrl.measureSParameters();
  QTest::qWait(1100);
  QVERIFY2(!ctrl.traceFrequencies().isEmpty(),
           "traceFrequencies() must be non-empty after measurement");
}

void TestMockNetworkAnalyzerController::
    test_measureSParameters_traceMagnitudesPopulated() {
  MockNetworkAnalyzerController ctrl;
  ctrl.measureSParameters();
  QTest::qWait(1100);
  QVERIFY2(!ctrl.traceMagnitudes().isEmpty(),
           "traceMagnitudes() must be non-empty after measurement");
}

void TestMockNetworkAnalyzerController::
    test_measureSParameters_traceCountMatchesNumPoints() {
  MockNetworkAnalyzerController ctrl;
  ctrl.setNumPoints(51);
  ctrl.measureSParameters();
  QTest::qWait(1100);
  QCOMPARE(ctrl.traceFrequencies().size(), 51);
  QCOMPARE(ctrl.traceMagnitudes().size(), 51);
}

// ---------------------------------------------------------------------------
// E. Edge cases
// ---------------------------------------------------------------------------

void TestMockNetworkAnalyzerController::
    test_doubleMeasure_noExtraStartedSignal() {
  MockNetworkAnalyzerController ctrl;
  QSignalSpy spy(
      &ctrl,
      &NetworkAnalyzerControllerInterface::measurementStarted);
  ctrl.measureSParameters();
  // Second call while measuring must be silently ignored.
  ctrl.measureSParameters();
  QCOMPARE(spy.count(), 1);
  QTest::qWait(1100);
}

void TestMockNetworkAnalyzerController::
    test_measureSParameters_traceFirstFreqEqualsStart() {
  MockNetworkAnalyzerController ctrl;
  ctrl.setFrequencyRange(5.0e6, 50.0e6);
  ctrl.setNumPoints(11);
  ctrl.measureSParameters();
  QTest::qWait(1100);
  QVERIFY(!ctrl.traceFrequencies().isEmpty());
  QCOMPARE(ctrl.traceFrequencies().first(), 5.0e6);
}

void TestMockNetworkAnalyzerController::
    test_measureSParameters_traceLastFreqEqualsStop() {
  MockNetworkAnalyzerController ctrl;
  ctrl.setFrequencyRange(5.0e6, 50.0e6);
  ctrl.setNumPoints(11);
  ctrl.measureSParameters();
  QTest::qWait(1100);
  QVERIFY(!ctrl.traceFrequencies().isEmpty());
  QCOMPARE(ctrl.traceFrequencies().last(), 50.0e6);
}

// ---------------------------------------------------------------------------
QTEST_MAIN(TestMockNetworkAnalyzerController)
#include "test_mock_network_analyzer_controller.moc"
