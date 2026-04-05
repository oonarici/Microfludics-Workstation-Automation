/**
 * @file test_mock_signal_generator_controller.cpp
 * @brief Adversarial tests for
 *        mwa::hardware::MockSignalGeneratorController.
 * @date 2026-03-22
 * @copyright LGPL-3.0-or-later
 */

#include <QObject>
#include <QSignalSpy>
#include <QString>
#include <QtTest>

#include "hardware/signal_generator/mock_signal_generator_controller.h"

using mwa::hardware::MockSignalGeneratorController;
using mwa::hardware::DeviceInterface;
using mwa::hardware::SignalGeneratorControllerInterface;
using DeviceState = DeviceInterface::DeviceState;
using DeviceType  = DeviceInterface::DeviceType;
using Waveform    = SignalGeneratorControllerInterface::Waveform;

class TestMockSignalGeneratorController : public QObject {
  Q_OBJECT

 private slots:
  // A. Construction & initial state
  void test_initialState_isDisconnected();
  void test_initialState_isConnectedFalse();
  void test_initialState_frequencyIs1000Hz();
  void test_initialState_amplitudeIs1V();
  void test_initialState_waveformIsSine();
  void test_initialState_outputIsDisabled();
  void test_deviceType_returnsSignalGenerator();
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
  void test_setFrequency_emitsFrequencyChanged();
  void test_setFrequency_getterReturnsSetValue();
  void test_setAmplitude_emitsAmplitudeChanged();
  void test_setAmplitude_getterReturnsSetValue();
  void test_setWaveform_sine_emitsWaveformChanged();
  void test_setWaveform_square_emitsWaveformChanged();
  void test_setWaveform_triangle_emitsWaveformChanged();
  void test_setWaveform_getterReturnsSetValue();
  void test_setOutputEnabled_true_emitsOutputStateChanged();
  void test_setOutputEnabled_false_emitsOutputStateChanged();
  void test_setOutputEnabled_getterReturnsSetValue();

  // E. Edge cases
  void test_configureSweep_valuesStoredCorrectly();
  void test_configureSweep_doesNotEmitAnySignal();
  void test_allWaveformTypes_canBeSet();
  void test_setFrequency_doesNotEmitStateChanged();
};

// ---------------------------------------------------------------------------
// A. Construction & initial state
// ---------------------------------------------------------------------------

void TestMockSignalGeneratorController::test_initialState_isDisconnected() {
  MockSignalGeneratorController ctrl;
  QCOMPARE(ctrl.state(), DeviceState::kDisconnected);
}

void TestMockSignalGeneratorController::
    test_initialState_isConnectedFalse() {
  MockSignalGeneratorController ctrl;
  QVERIFY2(!ctrl.isConnected(),
           "isConnected() must be false before connectDevice()");
}

void TestMockSignalGeneratorController::
    test_initialState_frequencyIs1000Hz() {
  MockSignalGeneratorController ctrl;
  QCOMPARE(ctrl.frequency(), 1000.0);
}

void TestMockSignalGeneratorController::test_initialState_amplitudeIs1V() {
  MockSignalGeneratorController ctrl;
  QCOMPARE(ctrl.amplitude(), 1.0);
}

void TestMockSignalGeneratorController::test_initialState_waveformIsSine() {
  MockSignalGeneratorController ctrl;
  QCOMPARE(ctrl.waveform(), Waveform::kSine);
}

void TestMockSignalGeneratorController::
    test_initialState_outputIsDisabled() {
  MockSignalGeneratorController ctrl;
  QVERIFY2(!ctrl.isOutputEnabled(),
           "Output must be disabled by default");
}

void TestMockSignalGeneratorController::
    test_deviceType_returnsSignalGenerator() {
  MockSignalGeneratorController ctrl;
  QCOMPARE(ctrl.deviceType(), DeviceType::kSignalGenerator);
}

void TestMockSignalGeneratorController::
    test_deviceName_returnsExpectedString() {
  MockSignalGeneratorController ctrl;
  QCOMPARE(ctrl.deviceName(),
           QStringLiteral("Mock Signal Generator Controller"));
}

// ---------------------------------------------------------------------------
// B. Connect / disconnect cycle
// ---------------------------------------------------------------------------

void TestMockSignalGeneratorController::
    test_connectDevice_emitsKConnectingThenKConnected() {
  MockSignalGeneratorController ctrl;
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

void TestMockSignalGeneratorController::
    test_connectDevice_stateIsConnectedAfterTimer() {
  MockSignalGeneratorController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  QCOMPARE(ctrl.state(), DeviceState::kConnected);
}

void TestMockSignalGeneratorController::
    test_connectDevice_isConnectedTrueAfterTimer() {
  MockSignalGeneratorController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  QVERIFY2(ctrl.isConnected(),
           "isConnected() must be true after the timer fires");
}

void TestMockSignalGeneratorController::
    test_disconnectDevice_emitsKDisconnected() {
  MockSignalGeneratorController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);

  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.disconnectDevice();
  QCOMPARE(spy.count(), 1);
  QCOMPARE(qvariant_cast<DeviceState>(spy.at(0).at(0)),
           DeviceState::kDisconnected);
}

void TestMockSignalGeneratorController::
    test_disconnectDevice_isConnectedFalse() {
  MockSignalGeneratorController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  ctrl.disconnectDevice();
  QVERIFY2(!ctrl.isConnected(),
           "isConnected() must be false after disconnectDevice()");
}

void TestMockSignalGeneratorController::
    test_disconnectDevice_stateIsDisconnected() {
  MockSignalGeneratorController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  ctrl.disconnectDevice();
  QCOMPARE(ctrl.state(), DeviceState::kDisconnected);
}

// ---------------------------------------------------------------------------
// C. Double-connect / double-disconnect guards
// ---------------------------------------------------------------------------

void TestMockSignalGeneratorController::test_doubleConnect_noExtraSignals() {
  MockSignalGeneratorController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.connectDevice();
  QTest::qWait(600);
  QCOMPARE(spy.count(), 0);
}

void TestMockSignalGeneratorController::
    test_connectWhileConnecting_noExtraSignals() {
  MockSignalGeneratorController ctrl;
  ctrl.connectDevice();
  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.connectDevice();
  QTest::qWait(600);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(qvariant_cast<DeviceState>(spy.at(0).at(0)),
           DeviceState::kConnected);
}

void TestMockSignalGeneratorController::
    test_doubleDisconnect_noExtraSignals() {
  MockSignalGeneratorController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  ctrl.disconnectDevice();
  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.disconnectDevice();
  QTest::qWait(100);
  QCOMPARE(spy.count(), 0);
}

// ---------------------------------------------------------------------------
// D. Device-specific methods & signals
// ---------------------------------------------------------------------------

void TestMockSignalGeneratorController::
    test_setFrequency_emitsFrequencyChanged() {
  MockSignalGeneratorController ctrl;
  QSignalSpy spy(
      &ctrl,
      &SignalGeneratorControllerInterface::frequencyChanged);
  ctrl.setFrequency(5000.0);
  QTest::qWait(100);  // Wait for 20ms command latency (extra margin for CI).
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toDouble(), 5000.0);
}

void TestMockSignalGeneratorController::
    test_setFrequency_getterReturnsSetValue() {
  MockSignalGeneratorController ctrl;
  ctrl.setFrequency(2500.0);
  QCOMPARE(ctrl.frequency(), 2500.0);
}

void TestMockSignalGeneratorController::
    test_setAmplitude_emitsAmplitudeChanged() {
  MockSignalGeneratorController ctrl;
  QSignalSpy spy(
      &ctrl,
      &SignalGeneratorControllerInterface::amplitudeChanged);
  ctrl.setAmplitude(3.3);
  QTest::qWait(100);  // Wait for 20ms command latency.
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toDouble(), 3.3);
}

void TestMockSignalGeneratorController::
    test_setAmplitude_getterReturnsSetValue() {
  MockSignalGeneratorController ctrl;
  ctrl.setAmplitude(5.0);
  QCOMPARE(ctrl.amplitude(), 5.0);
}

void TestMockSignalGeneratorController::
    test_setWaveform_sine_emitsWaveformChanged() {
  MockSignalGeneratorController ctrl;
  // Start at kSine (default), change to square first, then back to sine.
  // Use a scoped spy to guarantee the kSquare transition has completed
  // before setting up the real spy — avoids a qWait race on loaded CI.
  {
    QSignalSpy setup_spy(
        &ctrl,
        &SignalGeneratorControllerInterface::waveformChanged);
    ctrl.setWaveform(Waveform::kSquare);
    QTRY_COMPARE(setup_spy.count(), 1);
  }
  QSignalSpy spy(
      &ctrl,
      &SignalGeneratorControllerInterface::waveformChanged);
  ctrl.setWaveform(Waveform::kSine);
  QTRY_COMPARE(spy.count(), 1);
  QCOMPARE(
      qvariant_cast<Waveform>(spy.at(0).at(0)),
      Waveform::kSine);
}

void TestMockSignalGeneratorController::
    test_setWaveform_square_emitsWaveformChanged() {
  MockSignalGeneratorController ctrl;
  QSignalSpy spy(
      &ctrl,
      &SignalGeneratorControllerInterface::waveformChanged);
  ctrl.setWaveform(Waveform::kSquare);
  QTest::qWait(100);  // Wait for 20ms command latency.
  QCOMPARE(spy.count(), 1);
  QCOMPARE(
      qvariant_cast<Waveform>(spy.at(0).at(0)),
      Waveform::kSquare);
}

void TestMockSignalGeneratorController::
    test_setWaveform_triangle_emitsWaveformChanged() {
  MockSignalGeneratorController ctrl;
  QSignalSpy spy(
      &ctrl,
      &SignalGeneratorControllerInterface::waveformChanged);
  ctrl.setWaveform(Waveform::kTriangle);
  QTest::qWait(100);  // Wait for 20ms command latency.
  QCOMPARE(spy.count(), 1);
  QCOMPARE(
      qvariant_cast<Waveform>(spy.at(0).at(0)),
      Waveform::kTriangle);
}

void TestMockSignalGeneratorController::
    test_setWaveform_getterReturnsSetValue() {
  MockSignalGeneratorController ctrl;
  ctrl.setWaveform(Waveform::kTriangle);
  QCOMPARE(ctrl.waveform(), Waveform::kTriangle);
}

void TestMockSignalGeneratorController::
    test_setOutputEnabled_true_emitsOutputStateChanged() {
  MockSignalGeneratorController ctrl;
  QSignalSpy spy(
      &ctrl,
      &SignalGeneratorControllerInterface::outputStateChanged);
  ctrl.setOutputEnabled(true);
  QTest::qWait(100);  // Wait for 20ms command latency.
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toBool(), true);
}

void TestMockSignalGeneratorController::
    test_setOutputEnabled_false_emitsOutputStateChanged() {
  MockSignalGeneratorController ctrl;
  ctrl.setOutputEnabled(true);
  QTest::qWait(100);
  QSignalSpy spy(
      &ctrl,
      &SignalGeneratorControllerInterface::outputStateChanged);
  ctrl.setOutputEnabled(false);
  QTest::qWait(100);  // Wait for 20ms command latency.
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toBool(), false);
}

void TestMockSignalGeneratorController::
    test_setOutputEnabled_getterReturnsSetValue() {
  MockSignalGeneratorController ctrl;
  ctrl.setOutputEnabled(true);
  QVERIFY(ctrl.isOutputEnabled());
  ctrl.setOutputEnabled(false);
  QVERIFY(!ctrl.isOutputEnabled());
}

// ---------------------------------------------------------------------------
// E. Edge cases
// ---------------------------------------------------------------------------

void TestMockSignalGeneratorController::
    test_configureSweep_valuesStoredCorrectly() {
  // configureSweep() does not expose getters in the interface, so we verify
  // it does not corrupt other state and can be called without crashing.
  MockSignalGeneratorController ctrl;
  const double freq_before = ctrl.frequency();
  ctrl.configureSweep(100.0, 10000.0, 50.0);
  // Sweep must not corrupt the current output frequency.
  QCOMPARE(ctrl.frequency(), freq_before);
}

void TestMockSignalGeneratorController::
    test_configureSweep_doesNotEmitAnySignal() {
  MockSignalGeneratorController ctrl;
  QSignalSpy freq_spy(
      &ctrl,
      &SignalGeneratorControllerInterface::frequencyChanged);
  QSignalSpy state_spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.configureSweep(100.0, 20000.0, 100.0);
  QTest::qWait(100);
  QCOMPARE(freq_spy.count(), 0);
  QCOMPARE(state_spy.count(), 0);
}

void TestMockSignalGeneratorController::
    test_allWaveformTypes_canBeSet() {
  MockSignalGeneratorController ctrl;
  // Each waveform type must round-trip through the getter without crashing.
  ctrl.setWaveform(Waveform::kSine);
  QCOMPARE(ctrl.waveform(), Waveform::kSine);
  ctrl.setWaveform(Waveform::kSquare);
  QCOMPARE(ctrl.waveform(), Waveform::kSquare);
  ctrl.setWaveform(Waveform::kTriangle);
  QCOMPARE(ctrl.waveform(), Waveform::kTriangle);
}

void TestMockSignalGeneratorController::
    test_setFrequency_doesNotEmitStateChanged() {
  MockSignalGeneratorController ctrl;
  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.setFrequency(9999.0);
  QTest::qWait(100);
  QCOMPARE(spy.count(), 0);
}

// ---------------------------------------------------------------------------
QTEST_MAIN(TestMockSignalGeneratorController)
#include "test_mock_signal_generator_controller.moc"
