/**
 * @file test_mock_camera_controller.cpp
 * @brief Adversarial tests for mwa::hardware::MockCameraController.
 * @date 2026-03-22
 * @copyright LGPL-3.0-or-later
 */

#include <QImage>
#include <QObject>
#include <QRect>
#include <QSignalSpy>
#include <QString>
#include <QtTest>

#include "hardware/camera/mock_camera_controller.h"

using mwa::hardware::MockCameraController;
using mwa::hardware::DeviceInterface;
using mwa::hardware::CameraControllerInterface;
using DeviceState = DeviceInterface::DeviceState;
using DeviceType  = DeviceInterface::DeviceType;

class TestMockCameraController : public QObject {
  Q_OBJECT

 private slots:
  // A. Construction & initial state
  void test_initialState_isDisconnected();
  void test_initialState_isConnectedFalse();
  void test_initialState_exposureIs10ms();
  void test_initialState_gainIs1();
  void test_initialState_roiIs640x480();
  void test_initialState_notCapturing();
  void test_initialState_lastFrameIsNull();
  void test_deviceType_returnsCamera();
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
  void test_setExposure_emitsExposureChanged();
  void test_setExposure_getterReturnsSetValue();
  void test_setGain_emitsGainChanged();
  void test_setGain_getterReturnsSetValue();
  void test_setRoi_getterReturnsSetValue();
  void test_grabSingle_emitsFrameReady();
  void test_grabSingle_lastFrameIsNotNull();
  void test_grabSingle_frameSizeMatchesRoi();
  void test_startContinuousCapture_isCapturingTrue();
  void test_startContinuousCapture_emitsFrameReadyOverTime();
  void test_stopCapture_isCapturingFalse();
  void test_stopCapture_stopsFrameEmissions();

  // E. Edge cases — batch capture
  void test_batchCapture_emitsExactlyNFrames();
  void test_batchCapture_emitsBatchComplete();
  void test_batchCapture_isCapturingFalseAfterComplete();
  void test_doubleContinuousCapture_noExtraCapture();
  void test_disconnectDevice_stopsCaptureTimer();
};

// ---------------------------------------------------------------------------
// A. Construction & initial state
// ---------------------------------------------------------------------------

void TestMockCameraController::test_initialState_isDisconnected() {
  MockCameraController ctrl;
  QCOMPARE(ctrl.state(), DeviceState::kDisconnected);
}

void TestMockCameraController::test_initialState_isConnectedFalse() {
  MockCameraController ctrl;
  QVERIFY2(!ctrl.isConnected(),
           "isConnected() must be false before connectDevice()");
}

void TestMockCameraController::test_initialState_exposureIs10ms() {
  MockCameraController ctrl;
  QCOMPARE(ctrl.exposure(), 10.0);
}

void TestMockCameraController::test_initialState_gainIs1() {
  MockCameraController ctrl;
  QCOMPARE(ctrl.gain(), 1.0);
}

void TestMockCameraController::test_initialState_roiIs640x480() {
  MockCameraController ctrl;
  QCOMPARE(ctrl.roi(), QRect(0, 0, 640, 480));
}

void TestMockCameraController::test_initialState_notCapturing() {
  MockCameraController ctrl;
  QVERIFY2(!ctrl.isCapturing(),
           "isCapturing() must be false on construction");
}

void TestMockCameraController::test_initialState_lastFrameIsNull() {
  MockCameraController ctrl;
  QVERIFY2(ctrl.lastFrame().isNull(),
           "lastFrame() must be null before any grab");
}

void TestMockCameraController::test_deviceType_returnsCamera() {
  MockCameraController ctrl;
  QCOMPARE(ctrl.deviceType(), DeviceType::kCamera);
}

void TestMockCameraController::test_deviceName_returnsExpectedString() {
  MockCameraController ctrl;
  QCOMPARE(ctrl.deviceName(), QStringLiteral("Mock Camera Controller"));
}

// ---------------------------------------------------------------------------
// B. Connect / disconnect cycle
// ---------------------------------------------------------------------------

void TestMockCameraController::
    test_connectDevice_emitsKConnectingThenKConnected() {
  MockCameraController ctrl;
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

void TestMockCameraController::
    test_connectDevice_stateIsConnectedAfterTimer() {
  MockCameraController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  QCOMPARE(ctrl.state(), DeviceState::kConnected);
}

void TestMockCameraController::
    test_connectDevice_isConnectedTrueAfterTimer() {
  MockCameraController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  QVERIFY2(ctrl.isConnected(),
           "isConnected() must be true after the connection timer fires");
}

void TestMockCameraController::test_disconnectDevice_emitsKDisconnected() {
  MockCameraController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);

  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.disconnectDevice();
  QCOMPARE(spy.count(), 1);
  QCOMPARE(qvariant_cast<DeviceState>(spy.at(0).at(0)),
           DeviceState::kDisconnected);
}

void TestMockCameraController::test_disconnectDevice_isConnectedFalse() {
  MockCameraController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  ctrl.disconnectDevice();
  QVERIFY2(!ctrl.isConnected(),
           "isConnected() must be false after disconnectDevice()");
}

void TestMockCameraController::test_disconnectDevice_stateIsDisconnected() {
  MockCameraController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  ctrl.disconnectDevice();
  QCOMPARE(ctrl.state(), DeviceState::kDisconnected);
}

// ---------------------------------------------------------------------------
// C. Double-connect / double-disconnect guards
// ---------------------------------------------------------------------------

void TestMockCameraController::test_doubleConnect_noExtraSignals() {
  MockCameraController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.connectDevice();
  QTest::qWait(600);
  QCOMPARE(spy.count(), 0);
}

void TestMockCameraController::test_connectWhileConnecting_noExtraSignals() {
  MockCameraController ctrl;
  ctrl.connectDevice();
  QSignalSpy spy(&ctrl, &DeviceInterface::stateChanged);
  ctrl.connectDevice();
  QTest::qWait(600);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(qvariant_cast<DeviceState>(spy.at(0).at(0)),
           DeviceState::kConnected);
}

void TestMockCameraController::test_doubleDisconnect_noExtraSignals() {
  MockCameraController ctrl;
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

void TestMockCameraController::test_setExposure_emitsExposureChanged() {
  MockCameraController ctrl;
  QSignalSpy spy(
      &ctrl,
      &CameraControllerInterface::exposureChanged);
  ctrl.setExposure(25.0);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toDouble(), 25.0);
}

void TestMockCameraController::test_setExposure_getterReturnsSetValue() {
  MockCameraController ctrl;
  ctrl.setExposure(50.0);
  QCOMPARE(ctrl.exposure(), 50.0);
}

void TestMockCameraController::test_setGain_emitsGainChanged() {
  MockCameraController ctrl;
  QSignalSpy spy(
      &ctrl,
      &CameraControllerInterface::gainChanged);
  ctrl.setGain(4.0);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toDouble(), 4.0);
}

void TestMockCameraController::test_setGain_getterReturnsSetValue() {
  MockCameraController ctrl;
  ctrl.setGain(2.5);
  QCOMPARE(ctrl.gain(), 2.5);
}

void TestMockCameraController::test_setRoi_getterReturnsSetValue() {
  MockCameraController ctrl;
  const QRect new_roi(100, 50, 320, 240);
  ctrl.setRoi(new_roi);
  QCOMPARE(ctrl.roi(), new_roi);
}

void TestMockCameraController::test_grabSingle_emitsFrameReady() {
  MockCameraController ctrl;
  QSignalSpy spy(
      &ctrl,
      &CameraControllerInterface::frameReady);
  ctrl.grabSingle();
  QCOMPARE(spy.count(), 1);
}

void TestMockCameraController::test_grabSingle_lastFrameIsNotNull() {
  MockCameraController ctrl;
  ctrl.grabSingle();
  QVERIFY2(!ctrl.lastFrame().isNull(),
           "lastFrame() must not be null after grabSingle()");
}

void TestMockCameraController::test_grabSingle_frameSizeMatchesRoi() {
  MockCameraController ctrl;
  const QRect custom_roi(0, 0, 128, 96);
  ctrl.setRoi(custom_roi);
  ctrl.grabSingle();
  const QImage frame = ctrl.lastFrame();
  QCOMPARE(frame.width(),  custom_roi.width());
  QCOMPARE(frame.height(), custom_roi.height());
}

void TestMockCameraController::test_startContinuousCapture_isCapturingTrue() {
  MockCameraController ctrl;
  ctrl.startContinuousCapture();
  QVERIFY2(ctrl.isCapturing(),
           "isCapturing() must be true immediately after startContinuousCapture");
  ctrl.stopCapture();
}

void TestMockCameraController::
    test_startContinuousCapture_emitsFrameReadyOverTime() {
  MockCameraController ctrl;
  QSignalSpy spy(
      &ctrl,
      &CameraControllerInterface::frameReady);
  ctrl.startContinuousCapture();
  // At ~30 fps (33 ms interval), wait ~100 ms — expect at least 2 frames.
  QTest::qWait(150);
  ctrl.stopCapture();
  QVERIFY2(spy.count() >= 2,
           "At least 2 frameReady signals expected during 150 ms of capture");
}

void TestMockCameraController::test_stopCapture_isCapturingFalse() {
  MockCameraController ctrl;
  ctrl.startContinuousCapture();
  ctrl.stopCapture();
  QVERIFY2(!ctrl.isCapturing(),
           "isCapturing() must be false after stopCapture()");
}

void TestMockCameraController::test_stopCapture_stopsFrameEmissions() {
  MockCameraController ctrl;
  ctrl.startContinuousCapture();
  QTest::qWait(100);
  ctrl.stopCapture();
  QSignalSpy spy(
      &ctrl,
      &CameraControllerInterface::frameReady);
  // No further frames should arrive after stopCapture().
  QTest::qWait(150);
  QCOMPARE(spy.count(), 0);
}

// ---------------------------------------------------------------------------
// E. Edge cases — batch capture
// ---------------------------------------------------------------------------

void TestMockCameraController::test_batchCapture_emitsExactlyNFrames() {
  MockCameraController ctrl;
  QSignalSpy spy(
      &ctrl,
      &CameraControllerInterface::frameReady);
  // 3 frames at 50 ms each; total ~150 ms + margin
  ctrl.startBatchCapture(3, 50);
  QTest::qWait(500);
  QCOMPARE(spy.count(), 3);
}

void TestMockCameraController::test_batchCapture_emitsBatchComplete() {
  MockCameraController ctrl;
  QSignalSpy spy(
      &ctrl,
      &CameraControllerInterface::batchComplete);
  ctrl.startBatchCapture(2, 50);
  QTest::qWait(400);
  QCOMPARE(spy.count(), 1);
}

void TestMockCameraController::
    test_batchCapture_isCapturingFalseAfterComplete() {
  MockCameraController ctrl;
  ctrl.startBatchCapture(2, 50);
  QTest::qWait(400);
  QVERIFY2(!ctrl.isCapturing(),
           "isCapturing() must be false after batch completes");
}

void TestMockCameraController::test_doubleContinuousCapture_noExtraCapture() {
  MockCameraController ctrl;
  ctrl.startContinuousCapture();
  // Second call while capturing must be a no-op.
  QSignalSpy spy(
      &ctrl,
      &CameraControllerInterface::frameReady);
  ctrl.startContinuousCapture();
  // Frame count should be consistent with only one timer active.
  QTest::qWait(100);
  const int count_a = spy.count();
  ctrl.stopCapture();
  // Frames per 100 ms at 33 ms/frame ≈ 3; two timers would double this.
  // We verify count is below an impossibly high threshold for two timers.
  QVERIFY2(count_a <= 6,
           "Frame rate must not double from double startContinuousCapture()");
}

void TestMockCameraController::test_disconnectDevice_stopsCaptureTimer() {
  MockCameraController ctrl;
  ctrl.connectDevice();
  QTest::qWait(600);
  ctrl.startContinuousCapture();
  QVERIFY(ctrl.isCapturing());
  ctrl.disconnectDevice();
  // After disconnect the timer must be stopped.
  QVERIFY2(!ctrl.isCapturing(),
           "isCapturing() must be false after disconnectDevice()");
  QSignalSpy spy(
      &ctrl,
      &CameraControllerInterface::frameReady);
  QTest::qWait(150);
  QCOMPARE(spy.count(), 0);
}

// ---------------------------------------------------------------------------
QTEST_MAIN(TestMockCameraController)
#include "test_mock_camera_controller.moc"
