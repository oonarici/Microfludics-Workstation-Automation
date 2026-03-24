/**
 * @file test_camera_panel.cpp
 * @brief Adversarial unit tests for the CameraPanel widget.
 * @author MWA Team
 * @date 2026-03-23
 *
 * Tests cover widget structure, initial disabled state, setController()
 * signal wiring, all DeviceState transitions (kDisconnected, kConnecting,
 * kConnected, kError), capture button enable/disable logic, debounce timers,
 * ROI reset, frame counter increments, preview label, stop-button visibility,
 * and null-controller edge cases.
 *
 * @note Uses !isHidden() instead of isVisible() for headless CI.
 * @note QMessageBox dialogs spawned by onConnectClicked() and
 *       onHomeClicked() require confirmation — tests bypass by calling
 *       the controller directly or by operating below the dialog layer.
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
#include <QSpinBox>
#include <QTest>
#include <QTimer>

#include "gui/panels/camera_panel.h"
#include "hardware/camera/mock_camera_controller.h"

using mwa::gui::CameraPanel;
using mwa::hardware::MockCameraController;
using mwa::hardware::DeviceInterface;
using DeviceState = DeviceInterface::DeviceState;

static int s_argc = 1;
static char s_app_name[] = "test_camera_panel";
static char* s_argv[]    = {s_app_name};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/** Pump the event loop briefly without a QTest::qWait delay penalty. */
static void processEvents() {
  QApplication::processEvents();
}

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------

class TestCameraPanel : public QObject {
  Q_OBJECT

 private:
  QApplication*        app_{nullptr};
  CameraPanel*         panel_{nullptr};
  MockCameraController ctrl_;

 private slots:
  // ---- Lifecycle -----------------------------------------------------------
  void initTestCase() {
    app_ = new QApplication(s_argc, s_argv);
  }

  void init() {
    // Reset controller state from any previous test.
    ctrl_.disconnectDevice();
    QApplication::processEvents();
    panel_ = new CameraPanel();
    processEvents();
  }

  void cleanup() {
    // Detach controller first to avoid dangling connection during destruction.
    panel_->setController(nullptr);
    delete panel_;
    panel_ = nullptr;
  }

  void cleanupTestCase() {
    delete app_;
  }

  // =========================================================================
  // A. Widget existence
  // =========================================================================

  void test_objectName_isCameraPanel() {
    QCOMPARE(panel_->objectName(), QStringLiteral("cameraPanel"));
  }

  void test_grpConnection_exists() {
    QVERIFY2(panel_->findChild<QGroupBox*>(
                 QStringLiteral("grpConnection")) != nullptr,
             "Connection group box must exist");
  }

  void test_grpAcquisition_exists() {
    QVERIFY2(panel_->findChild<QGroupBox*>(
                 QStringLiteral("grpAcquisition")) != nullptr,
             "Acquisition Settings group box must exist");
  }

  void test_grpCapture_exists() {
    QVERIFY2(panel_->findChild<QGroupBox*>(
                 QStringLiteral("grpCapture")) != nullptr,
             "Capture group box must exist");
  }

  void test_grpStatus_exists() {
    QVERIFY2(panel_->findChild<QGroupBox*>(
                 QStringLiteral("grpStatus")) != nullptr,
             "Status group box must exist");
  }

  void test_cmbCamera_exists() {
    QVERIFY(panel_->findChild<QComboBox*>(
                QStringLiteral("cmbCamera")) != nullptr);
  }

  void test_lblStatus_exists() {
    QVERIFY(panel_->findChild<QLabel*>(
                QStringLiteral("lblStatus")) != nullptr);
  }

  void test_btnConnect_exists() {
    QVERIFY(panel_->findChild<QPushButton*>(
                QStringLiteral("btnConnect")) != nullptr);
  }

  void test_spnExposure_exists() {
    QVERIFY(panel_->findChild<QDoubleSpinBox*>(
                QStringLiteral("spnExposure")) != nullptr);
  }

  void test_spnGain_exists() {
    QVERIFY(panel_->findChild<QDoubleSpinBox*>(
                QStringLiteral("spnGain")) != nullptr);
  }

  void test_spnRoiX_exists() {
    QVERIFY(panel_->findChild<QSpinBox*>(
                QStringLiteral("spnRoiX")) != nullptr);
  }

  void test_spnRoiY_exists() {
    QVERIFY(panel_->findChild<QSpinBox*>(
                QStringLiteral("spnRoiY")) != nullptr);
  }

  void test_spnRoiW_exists() {
    QVERIFY(panel_->findChild<QSpinBox*>(
                QStringLiteral("spnRoiW")) != nullptr);
  }

  void test_spnRoiH_exists() {
    QVERIFY(panel_->findChild<QSpinBox*>(
                QStringLiteral("spnRoiH")) != nullptr);
  }

  void test_btnResetRoi_exists() {
    QVERIFY(panel_->findChild<QPushButton*>(
                QStringLiteral("btnResetRoi")) != nullptr);
  }

  void test_btnGrabSingle_exists() {
    QVERIFY(panel_->findChild<QPushButton*>(
                QStringLiteral("btnGrabSingle")) != nullptr);
  }

  void test_btnContinuous_exists() {
    QVERIFY(panel_->findChild<QPushButton*>(
                QStringLiteral("btnContinuous")) != nullptr);
  }

  void test_spnBatchCount_exists() {
    QVERIFY(panel_->findChild<QSpinBox*>(
                QStringLiteral("spnBatchCount")) != nullptr);
  }

  void test_spnBatchInterval_exists() {
    QVERIFY(panel_->findChild<QSpinBox*>(
                QStringLiteral("spnBatchInterval")) != nullptr);
  }

  void test_btnStartBatch_exists() {
    QVERIFY(panel_->findChild<QPushButton*>(
                QStringLiteral("btnStartBatch")) != nullptr);
  }

  void test_btnStop_exists() {
    QVERIFY(panel_->findChild<QPushButton*>(
                QStringLiteral("btnStop")) != nullptr);
  }

  void test_lblCaptureState_exists() {
    QVERIFY(panel_->findChild<QLabel*>(
                QStringLiteral("lblCaptureState")) != nullptr);
  }

  void test_lblFrameCount_exists() {
    QVERIFY(panel_->findChild<QLabel*>(
                QStringLiteral("lblFrameCount")) != nullptr);
  }

  void test_lblPreview_exists() {
    QVERIFY(panel_->findChild<QLabel*>(
                QStringLiteral("lblPreview")) != nullptr);
  }

  // =========================================================================
  // B. Initial state — no controller attached
  // =========================================================================

  void test_initialState_acquisitionGroupDisabled() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpAcquisition"));
    QVERIFY2(!grp->isEnabled(),
             "Acquisition group must be disabled before a controller is set");
  }

  void test_initialState_captureGroupDisabled() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpCapture"));
    QVERIFY2(!grp->isEnabled(),
             "Capture group must be disabled before a controller is set");
  }

  void test_initialState_statusLabelShowsDisconnected() {
    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblStatus"));
    QVERIFY2(lbl->text().contains(QStringLiteral("Disconnected")),
             "Status label must contain 'Disconnected' on construction");
  }

  void test_initialState_btnConnectText_isConnect() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnConnect"));
    QCOMPARE(btn->text(), QStringLiteral("Connect"));
  }

  void test_initialState_roiX_isZero() {
    auto* spn = panel_->findChild<QSpinBox*>(
        QStringLiteral("spnRoiX"));
    QCOMPARE(spn->value(), 0);
  }

  void test_initialState_roiY_isZero() {
    auto* spn = panel_->findChild<QSpinBox*>(
        QStringLiteral("spnRoiY"));
    QCOMPARE(spn->value(), 0);
  }

  void test_initialState_roiW_is1920() {
    auto* spn = panel_->findChild<QSpinBox*>(
        QStringLiteral("spnRoiW"));
    QCOMPARE(spn->value(), 1920);
  }

  void test_initialState_roiH_is1080() {
    auto* spn = panel_->findChild<QSpinBox*>(
        QStringLiteral("spnRoiH"));
    QCOMPARE(spn->value(), 1080);
  }

  void test_initialState_frameCountLabel_isZero() {
    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblFrameCount"));
    QVERIFY2(lbl->text().contains(QStringLiteral("0")),
             "Frame counter label must show '0' on construction");
  }

  void test_initialState_captureStateLabel_isIdle() {
    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblCaptureState"));
    QCOMPARE(lbl->text(), QStringLiteral("Idle"));
  }

  void test_initialState_previewLabel_showsNoImage() {
    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblPreview"));
    // Preview must display "No Image" or be empty — not a real frame.
    const bool no_image =
        lbl->text().contains(QStringLiteral("No Image")) ||
        lbl->pixmap().isNull();
    QVERIFY2(no_image,
             "Preview label must show 'No Image' or null pixmap before capture");
  }

  void test_initialState_stopButton_isHidden() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnStop"));
    QVERIFY2(btn->isHidden(),
             "Stop button must be hidden when no capture is active");
  }

  void test_initialState_continuousButton_isCheckable() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnContinuous"));
    QVERIFY2(btn->isCheckable(),
             "Continuous button must be checkable (toggle)");
  }

  void test_initialState_continuousButton_notChecked() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnContinuous"));
    QVERIFY2(!btn->isChecked(),
             "Continuous button must start unchecked");
  }

  void test_initialState_debounceTimers_exist() {
    // Debounce timers are children of the panel.
    const auto timers = panel_->findChildren<QTimer*>();
    QVERIFY2(timers.count() >= 3,
             "At least 3 QTimer children (exposure/gain/roi) must exist");
  }

  // =========================================================================
  // C. setController(nullptr) — must not crash and must disable controls
  // =========================================================================

  void test_setControllerNull_doesNotCrash() {
    panel_->setController(nullptr);
    processEvents();
    QVERIFY(true);  // Reaching here means no crash.
  }

  void test_setControllerNull_disablesAcquisitionGroup() {
    // First attach a connected controller so groups become enabled.
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    // Now detach.
    panel_->setController(nullptr);
    processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpAcquisition"));
    QVERIFY2(!grp->isEnabled(),
             "Acquisition group must be disabled after setController(nullptr)");
  }

  void test_setControllerNull_disablesCaptureGroup() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    panel_->setController(nullptr);
    processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpCapture"));
    QVERIFY2(!grp->isEnabled(),
             "Capture group must be disabled after setController(nullptr)");
  }

  void test_setControllerNull_twiceDoesNotCrash() {
    panel_->setController(nullptr);
    panel_->setController(nullptr);
    processEvents();
    QVERIFY(true);
  }

  // =========================================================================
  // D. setController — signals connected and initial state reflected
  // =========================================================================

  void test_setController_syncToDisconnectedState() {
    // ctrl_ is freshly constructed — state is kDisconnected.
    panel_->setController(&ctrl_);
    processEvents();

    auto* grp_acq = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpAcquisition"));
    QVERIFY2(!grp_acq->isEnabled(),
             "Acquisition group must stay disabled for disconnected controller");
  }

  void test_setController_syncToConnectedState() {
    ctrl_.connectDevice();
    QTest::qWait(600);

    panel_->setController(&ctrl_);
    processEvents();

    auto* grp_acq = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpAcquisition"));
    QVERIFY2(grp_acq->isEnabled(),
             "Acquisition group must be enabled when controller is connected");
  }

  void test_setController_syncsPositionLabels_fromConnectedController() {
    ctrl_.connectDevice();
    QTest::qWait(600);

    panel_->setController(&ctrl_);
    processEvents();

    // Button text must switch to "Disconnect" for a connected controller.
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnConnect"));
    QCOMPARE(btn->text(), QStringLiteral("Disconnect"));
  }

  // =========================================================================
  // E. State transitions update UI
  // =========================================================================

  void test_stateConnecting_disablesControls() {
    panel_->setController(&ctrl_);

    // Spy on the state just before kConnecting fires.
    ctrl_.connectDevice();
    // kConnecting fires synchronously — process events to let the slot run.
    processEvents();

    auto* grp_acq = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpAcquisition"));
    QVERIFY2(!grp_acq->isEnabled(),
             "Acquisition group must stay disabled while connecting");
  }

  void test_stateConnected_enablesAcquisitionGroup() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpAcquisition"));
    QVERIFY2(grp->isEnabled(),
             "Acquisition group must be enabled after kConnected");
  }

  void test_stateConnected_enablesCaptureGroup() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpCapture"));
    QVERIFY2(grp->isEnabled(),
             "Capture group must be enabled after kConnected");
  }

  void test_stateConnected_statusLabelShowsConnected() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblStatus"));
    QVERIFY2(lbl->text().contains(QStringLiteral("Connected")),
             "Status label must contain 'Connected' after kConnected");
  }

  void test_stateConnected_connectButtonTextIsDisconnect() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnConnect"));
    QCOMPARE(btn->text(), QStringLiteral("Disconnect"));
  }

  void test_stateDisconnected_afterConnect_disablesControls() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    ctrl_.disconnectDevice();
    processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpAcquisition"));
    QVERIFY2(!grp->isEnabled(),
             "Acquisition group must be disabled after kDisconnected");
  }

  void test_stateDisconnected_connectButtonTextIsConnect() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    ctrl_.disconnectDevice();
    processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnConnect"));
    QCOMPARE(btn->text(), QStringLiteral("Connect"));
  }

  void test_stateError_disablesAcquisitionGroup() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    // Simulate error by emitting stateChanged directly.
    emit ctrl_.stateChanged(DeviceState::kError);
    processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpAcquisition"));
    QVERIFY2(!grp->isEnabled(),
             "Acquisition group must be disabled in kError state");
  }

  void test_stateError_statusLabelShowsError() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);

    emit ctrl_.stateChanged(DeviceState::kError);
    processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblStatus"));
    QVERIFY2(lbl->text().contains(QStringLiteral("Error")),
             "Status label must contain 'Error' in kError state");
  }

  void test_stateConnecting_statusLabelShowsConnecting() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblStatus"));
    QVERIFY2(lbl->text().contains(QStringLiteral("Connecting")),
             "Status label must contain 'Connecting' during kConnecting");
  }

  // =========================================================================
  // F. Disconnection resets in-progress capture state
  // =========================================================================

  void test_disconnect_resetsContinuousButton() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    // Simulate continuous running by checking the button.
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnContinuous"));
    btn->setChecked(true);
    processEvents();

    // Now disconnect.
    ctrl_.disconnectDevice();
    processEvents();

    QVERIFY2(!btn->isChecked(),
             "Continuous button must be unchecked after disconnect");
  }

  void test_disconnect_hidesBtnStop() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    ctrl_.disconnectDevice();
    processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnStop"));
    QVERIFY2(btn->isHidden(),
             "Stop button must be hidden after disconnect");
  }

  void test_disconnect_resetsCaptureStateToIdle() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    ctrl_.disconnectDevice();
    processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblCaptureState"));
    QCOMPARE(lbl->text(), QStringLiteral("Idle"));
  }

  // =========================================================================
  // G. frameReady signal handling
  // =========================================================================

  void test_frameReady_incrementsFrameCount() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblFrameCount"));
    QCOMPARE(lbl->text(), QStringLiteral("0"));

    // Emit a test frame.
    QImage test_frame(64, 64, QImage::Format_RGB32);
    test_frame.fill(Qt::red);
    emit ctrl_.frameReady(test_frame);
    processEvents();

    QCOMPARE(lbl->text(), QStringLiteral("1"));
  }

  void test_frameReady_multipleFrames_accumulatesCount() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    QImage frame(64, 64, QImage::Format_RGB32);
    frame.fill(Qt::blue);

    for (int i = 0; i < 5; ++i) {
      emit ctrl_.frameReady(frame);
    }
    processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblFrameCount"));
    QCOMPARE(lbl->text(), QStringLiteral("5"));
  }

  void test_frameReady_updatesPreviewLabel() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    // Throttle timer is invalid on construction — first frame always updates.
    QImage frame(640, 480, QImage::Format_RGB32);
    frame.fill(Qt::green);
    emit ctrl_.frameReady(frame);
    processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblPreview"));
    QVERIFY2(!lbl->pixmap().isNull(),
             "Preview label must display a pixmap after frameReady");
  }

  void test_frameReady_nullFrame_doesNotUpdatePreview() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    // Emit a null QImage — preview must not be set.
    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblPreview"));
    const QPixmap before = lbl->pixmap();

    emit ctrl_.frameReady(QImage());
    processEvents();

    // The pixmap must not change to a valid image from a null frame.
    QVERIFY2(lbl->pixmap().isNull() || lbl->pixmap().cacheKey() == before.cacheKey(),
             "Null QImage must not update the preview pixmap");
  }

  // =========================================================================
  // H. batchComplete signal handling
  // =========================================================================

  void test_batchComplete_resetsCaptureStateToIdle() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    // Put panel in a batch-capturing state.
    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblCaptureState"));
    lbl->setText(QStringLiteral("Batch 1/3"));

    emit ctrl_.batchComplete();
    processEvents();

    QCOMPARE(lbl->text(), QStringLiteral("Idle"));
  }

  void test_batchComplete_hidesBtnStop() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    emit ctrl_.batchComplete();
    processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnStop"));
    QVERIFY2(btn->isHidden(),
             "Stop button must be hidden after batchComplete");
  }

  // =========================================================================
  // I. ROI reset button
  // =========================================================================

  void test_resetRoi_setsXToZero() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    auto* spn_x = panel_->findChild<QSpinBox*>(
        QStringLiteral("spnRoiX"));
    spn_x->setValue(500);

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnResetRoi"));
    btn->click();
    processEvents();

    QCOMPARE(spn_x->value(), 0);
  }

  void test_resetRoi_setsYToZero() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    auto* spn_y = panel_->findChild<QSpinBox*>(
        QStringLiteral("spnRoiY"));
    spn_y->setValue(300);

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnResetRoi"));
    btn->click();
    processEvents();

    QCOMPARE(spn_y->value(), 0);
  }

  void test_resetRoi_setsWidthTo1920() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    auto* spn_w = panel_->findChild<QSpinBox*>(
        QStringLiteral("spnRoiW"));
    spn_w->setValue(320);

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnResetRoi"));
    btn->click();
    processEvents();

    QCOMPARE(spn_w->value(), 1920);
  }

  void test_resetRoi_setsHeightTo1080() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    auto* spn_h = panel_->findChild<QSpinBox*>(
        QStringLiteral("spnRoiH"));
    spn_h->setValue(240);

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnResetRoi"));
    btn->click();
    processEvents();

    QCOMPARE(spn_h->value(), 1080);
  }

  // =========================================================================
  // J. Debounce timer — exposure forwarded to controller after 300 ms
  // =========================================================================

  void test_exposureDebounce_forwardsToController() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    auto* spn = panel_->findChild<QDoubleSpinBox*>(
        QStringLiteral("spnExposure"));
    QVERIFY(spn != nullptr);

    QSignalSpy spy(&ctrl_,
                   &mwa::hardware::CameraControllerInterface::exposureChanged);

    spn->setValue(25.0);
    // Debounce fires after 300 ms.
    QTest::qWait(400);

    QVERIFY2(spy.count() >= 1,
             "exposureChanged must be emitted after exposure debounce fires");
    QCOMPARE(spy.last().at(0).toDouble(), 25.0);
  }

  void test_gainDebounce_forwardsToController() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    auto* spn = panel_->findChild<QDoubleSpinBox*>(
        QStringLiteral("spnGain"));
    QVERIFY(spn != nullptr);

    QSignalSpy spy(&ctrl_,
                   &mwa::hardware::CameraControllerInterface::gainChanged);

    spn->setValue(4.0);
    QTest::qWait(400);

    QVERIFY2(spy.count() >= 1,
             "gainChanged must be emitted after gain debounce fires");
    QCOMPARE(spy.last().at(0).toDouble(), 4.0);
  }

  void test_roiDebounce_forwardsToController() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    auto* spn_w = panel_->findChild<QSpinBox*>(
        QStringLiteral("spnRoiW"));
    spn_w->setValue(640);
    QTest::qWait(400);

    // ROI debounce should have fired; the mock tracks the ROI width.
    QCOMPARE(ctrl_.roi().width(), 640);
  }

  void test_exposureDebounce_noControllerNoForward_noCrash() {
    // Debounce fires with no controller — must not crash.
    auto* spn = panel_->findChild<QDoubleSpinBox*>(
        QStringLiteral("spnExposure"));
    // Panel has no controller (never set in this test).
    spn->setValue(50.0);
    QTest::qWait(400);
    QVERIFY(true);
  }

  // =========================================================================
  // K. grabSingle — UI state transitions
  // =========================================================================

  void test_grabSingle_setsCapturingState() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnGrabSingle"));

    btn->click();
    processEvents();

    // grabSingle triggers the controller; mock emits frameReady synchronously.
    // After frame, single-grab re-enables — verify via state label.
    // The critical assertion: pressing the button called grabSingle on ctrl_.
    QVERIFY2(!ctrl_.lastFrame().isNull(),
             "grabSingle() must have been called on the controller");
  }

  void test_grabSingle_resetsCaptureStateToIdleAfterFrame() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnGrabSingle"));
    btn->click();
    processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblCaptureState"));
    // After a single grab completes (frameReady from synchronous mock),
    // state should return to Idle.
    QCOMPARE(lbl->text(), QStringLiteral("Idle"));
  }

  // =========================================================================
  // L. Continuous capture — toggle behaviour
  // =========================================================================

  void test_continuousButton_whenChecked_callsStartContinuous() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnContinuous"));

    // Click to check — mock should now be capturing.
    btn->click();
    processEvents();

    QVERIFY2(ctrl_.isCapturing(),
             "startContinuousCapture() must have been called when toggled on");
    ctrl_.stopCapture();
  }

  void test_continuousButton_whenUnchecked_callsStopCapture() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnContinuous"));

    // Start continuous.
    btn->click();
    processEvents();
    QVERIFY(ctrl_.isCapturing());

    // Stop continuous.
    btn->click();
    processEvents();

    QVERIFY2(!ctrl_.isCapturing(),
             "stopCapture() must have been called when continuous toggled off");
  }

  void test_continuousButton_textChangesWhenChecked() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnContinuous"));
    btn->click();
    processEvents();

    QVERIFY2(btn->text().contains(QStringLiteral("Stop")),
             "Continuous button text must contain 'Stop' while active");
    ctrl_.stopCapture();
  }

  // =========================================================================
  // M. Stop button visibility during capture
  // =========================================================================

  void test_stopButton_visibleDuringBatchCapture() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    auto* btn_batch = panel_->findChild<QPushButton*>(
        QStringLiteral("btnStartBatch"));
    auto* btn_stop  = panel_->findChild<QPushButton*>(
        QStringLiteral("btnStop"));

    // Set a large batch so it doesn't complete immediately.
    auto* spn_count = panel_->findChild<QSpinBox*>(
        QStringLiteral("spnBatchCount"));
    spn_count->setValue(100);
    auto* spn_interval = panel_->findChild<QSpinBox*>(
        QStringLiteral("spnBatchInterval"));
    spn_interval->setValue(1000);

    btn_batch->click();
    processEvents();

    QVERIFY2(!btn_stop->isHidden(),
             "Stop button must be visible while a batch capture is in progress");

    // Clean up.
    ctrl_.stopCapture();
  }

  // =========================================================================
  // N. Replace controller — old signals disconnected
  // =========================================================================

  void test_replaceController_oldSignalsDisconnected() {
    MockCameraController ctrl2;
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    // Replace with ctrl2 (disconnected).
    panel_->setController(&ctrl2);
    processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpAcquisition"));
    QVERIFY2(!grp->isEnabled(),
             "Acquisition group must be disabled after replacing with disconnected controller");

    // Signals from old controller must not affect the panel.
    emit ctrl_.stateChanged(DeviceState::kConnected);
    processEvents();

    // Still disabled — old controller's signal was disconnected.
    QVERIFY2(!grp->isEnabled(),
             "Old controller's stateChanged must be disconnected after replace");

    // Detach ctrl2 before it goes out of scope to avoid dangling pointer
    // in cleanup() which calls setController(nullptr).
    panel_->setController(nullptr);
  }

  // =========================================================================
  // O. Edge cases
  // =========================================================================

  void test_connectButton_withNoController_doesNotCrash() {
    // No setController() called — clicking Connect must not crash.
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnConnect"));
    btn->click();
    processEvents();
    QVERIFY(true);
  }

  void test_grabSingle_withNoController_doesNotCrash() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnGrabSingle"));
    // Direct click while disabled should not crash; forcibly call via
    // setting enabled for coverage.
    btn->setEnabled(true);
    btn->click();
    processEvents();
    QVERIFY(true);
  }

  void test_stopButton_withNoController_doesNotCrash() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnStop"));
    btn->setVisible(true);
    btn->click();
    processEvents();
    QVERIFY(true);
  }

  void test_frameCountResets_onGrabSingle() {
    panel_->setController(&ctrl_);
    ctrl_.connectDevice();
    QTest::qWait(600);
    processEvents();

    // Seed the counter by emitting frames directly.
    QImage frame(64, 64, QImage::Format_RGB32);
    frame.fill(Qt::cyan);
    for (int i = 0; i < 3; ++i) {
      emit ctrl_.frameReady(frame);
    }
    processEvents();

    auto* lbl_count = panel_->findChild<QLabel*>(
        QStringLiteral("lblFrameCount"));
    QCOMPARE(lbl_count->text(), QStringLiteral("3"));

    // Clicking Grab Single resets the counter to 0 before capturing.
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnGrabSingle"));
    btn->click();
    processEvents();

    // After the single grab the mock fires frameReady (count = 1), not 4.
    QCOMPARE(lbl_count->text(), QStringLiteral("1"));
  }
};

QTEST_APPLESS_MAIN(TestCameraPanel)
#include "test_camera_panel.moc"
