/**
 * @file test_signal_panel.cpp
 * @brief Adversarial unit tests for the SignalPanel widget.
 * @author MWA Team
 * @date 2026-03-24
 *
 * Tests cover: initial widget state, controller attachment/detachment for
 * both Signal Generator (Tab 0) and Network Analyzer (Tab 1), connection
 * state transitions, frequency/amplitude/waveform forwarding, output
 * enable toggle, NA measurement lifecycle, and display label updates.
 *
 * @note Uses !isHidden() / isEnabled() rather than isVisible() for
 *       headless CI compatibility.  QMessageBox dialogs are tested
 *       indirectly via button-state checks.
 *
 * @copyright LGPL-3.0-or-later
 */

#include <QApplication>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalSpy>
#include <QSpinBox>
#include <QTabWidget>
#include <QTest>

#include "gui/panels/signal_panel.h"
#include "hardware/network_analyzer/mock_network_analyzer_controller.h"
#include "hardware/signal_generator/mock_signal_generator_controller.h"

using mwa::gui::SignalPanel;
using mwa::hardware::MockSignalGeneratorController;
using mwa::hardware::MockNetworkAnalyzerController;
using mwa::hardware::DeviceInterface;
using DeviceState = DeviceInterface::DeviceState;
using Waveform =
    mwa::hardware::SignalGeneratorControllerInterface::Waveform;

static int   s_argc        = 1;
static char  s_app_name[]  = "test_signal_panel";
static char* s_argv[]      = {s_app_name};

// ---------------------------------------------------------------------------
// Helper: drive mock controller to kConnected synchronously.
// ---------------------------------------------------------------------------
static void connectSync(MockSignalGeneratorController& ctrl) {
  ctrl.connectDevice();
  QTest::qWait(600);
}

static void connectSync(MockNetworkAnalyzerController& ctrl) {
  ctrl.connectDevice();
  QTest::qWait(600);
}

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------

/**
 * @class TestSignalPanel
 * @brief Qt Test class exercising mwa::gui::SignalPanel.
 */
class TestSignalPanel : public QObject {
  Q_OBJECT

 private:
  QApplication*                  app_{nullptr};
  SignalPanel*                   panel_{nullptr};
  MockSignalGeneratorController* sig_ctrl_{nullptr};
  MockNetworkAnalyzerController* na_ctrl_{nullptr};

 private slots:
  void initTestCase() {
    app_ = new QApplication(s_argc, s_argv);
  }

  void init() {
    sig_ctrl_ = new MockSignalGeneratorController();
    na_ctrl_  = new MockNetworkAnalyzerController();
    panel_    = new SignalPanel();
    QApplication::processEvents();
  }

  void cleanup() {
    delete panel_;
    panel_ = nullptr;
    delete na_ctrl_;
    na_ctrl_ = nullptr;
    delete sig_ctrl_;
    sig_ctrl_ = nullptr;
  }

  void cleanupTestCase() {
    delete app_;
    app_ = nullptr;
  }

  // =========================================================================
  // A. Widget structure
  // =========================================================================

  void test_objectName_isSignalPanel() {
    QCOMPARE(panel_->objectName(),
             QStringLiteral("signalPanel"));
  }

  void test_connectionGroupBox_exists() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpConnection"));
    QVERIFY2(grp != nullptr,
             "grpConnection QGroupBox must exist");
  }

  void test_tabWidget_exists() {
    auto* tab = panel_->findChild<QTabWidget*>(
        QStringLiteral("tabSignalNA"));
    QVERIFY2(tab != nullptr,
             "tabSignalNA QTabWidget must exist");
  }

  void test_tabWidget_hasTwoTabs() {
    auto* tab = panel_->findChild<QTabWidget*>(
        QStringLiteral("tabSignalNA"));
    QCOMPARE(tab->count(), 2);
  }

  void test_connectButton_exists() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnConnect"));
    QVERIFY2(btn != nullptr,
             "btnConnect QPushButton must exist");
  }

  void test_sigOutputConfigGroup_exists() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpSigOutputConfig"));
    QVERIFY2(grp != nullptr,
             "grpSigOutputConfig QGroupBox must exist");
  }

  void test_sigSweepGroup_exists() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpSigSweep"));
    QVERIFY2(grp != nullptr,
             "grpSigSweep QGroupBox must exist");
  }

  void test_sigOutputStatusGroup_exists() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpSigOutputStatus"));
    QVERIFY2(grp != nullptr,
             "grpSigOutputStatus QGroupBox must exist");
  }

  void test_sigOutputEnableButton_exists() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnSigOutputEnable"));
    QVERIFY2(btn != nullptr,
             "btnSigOutputEnable QPushButton must exist");
  }

  void test_sigFreqDisplay_exists() {
    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblSigFreqDisplay"));
    QVERIFY2(lbl != nullptr,
             "lblSigFreqDisplay QLabel must exist");
  }

  void test_sigAmpDisplay_exists() {
    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblSigAmpDisplay"));
    QVERIFY2(lbl != nullptr,
             "lblSigAmpDisplay QLabel must exist");
  }

  void test_sigWaveformDisplay_exists() {
    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblSigWaveformDisplay"));
    QVERIFY2(lbl != nullptr,
             "lblSigWaveformDisplay QLabel must exist");
  }

  void test_naSweepConfigGroup_exists() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpNaSweepConfig"));
    QVERIFY2(grp != nullptr,
             "grpNaSweepConfig QGroupBox must exist");
  }

  void test_naMeasurementGroup_exists() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpNaMeasurement"));
    QVERIFY2(grp != nullptr,
             "grpNaMeasurement QGroupBox must exist");
  }

  void test_naMeasureButton_exists() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnNaMeasure"));
    QVERIFY2(btn != nullptr,
             "btnNaMeasure QPushButton must exist");
  }

  void test_naSparamDisplay_exists() {
    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblNaSparamDisplay"));
    QVERIFY2(lbl != nullptr,
             "lblNaSparamDisplay QLabel must exist");
  }

  void test_naProgressBar_exists() {
    auto* prg = panel_->findChild<QProgressBar*>(
        QStringLiteral("prgNaMeasurement"));
    QVERIFY2(prg != nullptr,
             "prgNaMeasurement QProgressBar must exist");
  }

  void test_naMeasStatusGroup_exists() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpNaMeasStatus"));
    QVERIFY2(grp != nullptr,
             "grpNaMeasStatus QGroupBox must exist");
  }

  void test_naStateText_exists() {
    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblNaStateText"));
    QVERIFY2(lbl != nullptr,
             "lblNaStateText QLabel must exist");
  }

  // =========================================================================
  // B. Initial state (no controllers)
  // =========================================================================

  void test_initialState_sigOutputConfigDisabled() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpSigOutputConfig"));
    QVERIFY2(!grp->isEnabled(),
             "Sig output config must be disabled when no controller");
  }

  void test_initialState_sigSweepDisabled() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpSigSweep"));
    QVERIFY2(!grp->isEnabled(),
             "Sig sweep group must be disabled when no controller");
  }

  void test_initialState_sigOutputStatusDisabled() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpSigOutputStatus"));
    QVERIFY2(!grp->isEnabled(),
             "Sig output status must be disabled when no controller");
  }

  void test_initialState_naSweepConfigDisabled() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpNaSweepConfig"));
    QVERIFY2(!grp->isEnabled(),
             "NA sweep config must be disabled when no controller");
  }

  void test_initialState_naMeasurementDisabled() {
    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpNaMeasurement"));
    QVERIFY2(!grp->isEnabled(),
             "NA measurement group must be disabled when no controller");
  }

  void test_initialState_outputButtonUnchecked() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnSigOutputEnable"));
    QVERIFY2(!btn->isChecked(),
             "Output button must start unchecked");
    QCOMPARE(btn->text(), QStringLiteral("Output OFF"));
  }

  void test_initialState_connectButtonTextIsConnect() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnConnect"));
    QCOMPARE(btn->text(), QStringLiteral("Connect"));
  }

  void test_initialState_naProgressBarHidden() {
    auto* prg = panel_->findChild<QProgressBar*>(
        QStringLiteral("prgNaMeasurement"));
    QVERIFY2(prg->isHidden(),
             "NA progress bar must be hidden initially");
  }

  void test_initialState_naStateTextIdle() {
    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblNaStateText"));
    QCOMPARE(lbl->text(), QStringLiteral("Idle"));
  }

  // =========================================================================
  // C. Signal Generator controller attachment
  // =========================================================================

  void test_setSigController_nullptr_noCrash() {
    panel_->setSignalGeneratorController(nullptr);
    QApplication::processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpSigOutputConfig"));
    QVERIFY2(!grp->isEnabled(),
             "Sig controls must remain disabled after nullptr");
  }

  void test_setSigController_disconnected_controlsDisabled() {
    panel_->setSignalGeneratorController(sig_ctrl_);
    QApplication::processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpSigOutputConfig"));
    QVERIFY2(!grp->isEnabled(),
             "Sig controls must remain disabled when disconnected");
  }

  void test_setSigController_connected_controlsEnabled() {
    panel_->setSignalGeneratorController(sig_ctrl_);
    connectSync(*sig_ctrl_);
    QApplication::processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpSigOutputConfig"));
    QVERIFY2(grp->isEnabled(),
             "Sig controls must be enabled after kConnected");
  }

  void test_setSigController_tab0Enabled() {
    panel_->setSignalGeneratorController(sig_ctrl_);
    QApplication::processEvents();

    auto* tab = panel_->findChild<QTabWidget*>(
        QStringLiteral("tabSignalNA"));
    QVERIFY2(tab->isTabEnabled(0),
             "Tab 0 must be enabled after attaching sig controller");
  }

  void test_setSigController_nullptr_disablesTab0() {
    panel_->setSignalGeneratorController(sig_ctrl_);
    QApplication::processEvents();

    panel_->setSignalGeneratorController(nullptr);
    QApplication::processEvents();

    auto* tab = panel_->findChild<QTabWidget*>(
        QStringLiteral("tabSignalNA"));
    QVERIFY2(!tab->isTabEnabled(0),
             "Tab 0 must be disabled after detaching sig controller");
  }

  // =========================================================================
  // D. Signal Generator — connection state transitions
  // =========================================================================

  void test_sigState_kConnected_buttonTextIsDisconnect() {
    panel_->setSignalGeneratorController(sig_ctrl_);
    connectSync(*sig_ctrl_);
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnConnect"));
    QCOMPARE(btn->text(), QStringLiteral("Disconnect"));
  }

  void test_sigState_kDisconnected_controlsDisabled() {
    panel_->setSignalGeneratorController(sig_ctrl_);
    connectSync(*sig_ctrl_);
    sig_ctrl_->disconnectDevice();
    QApplication::processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpSigOutputConfig"));
    QVERIFY2(!grp->isEnabled(),
             "Sig controls must be disabled after disconnect");
  }

  void test_sigState_kDisconnected_buttonTextIsConnect() {
    panel_->setSignalGeneratorController(sig_ctrl_);
    connectSync(*sig_ctrl_);
    sig_ctrl_->disconnectDevice();
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnConnect"));
    QCOMPARE(btn->text(), QStringLiteral("Connect"));
  }

  // =========================================================================
  // E. Signal Generator — frequency forwarding
  // =========================================================================

  void test_sigFrequency_setOnController_updatesFreqDisplay() {
    panel_->setSignalGeneratorController(sig_ctrl_);
    connectSync(*sig_ctrl_);
    QApplication::processEvents();

    sig_ctrl_->setFrequency(5000000.0);  // 5 MHz
    QTest::qWait(100);  // Wait for 20ms command latency.
    QApplication::processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblSigFreqDisplay"));
    QVERIFY2(lbl->text().contains(QStringLiteral("MHz")),
             "Freq display must show MHz for 5 MHz");
    QVERIFY2(lbl->text().contains(QStringLiteral("5")),
             "Freq display must contain 5");
  }

  void test_sigFrequency_spinboxChange_reachesController() {
    panel_->setSignalGeneratorController(sig_ctrl_);
    connectSync(*sig_ctrl_);
    QApplication::processEvents();

    QSignalSpy spy(
        sig_ctrl_,
        &mwa::hardware::SignalGeneratorControllerInterface::frequencyChanged);

    auto* spn = panel_->findChild<QDoubleSpinBox*>(
        QStringLiteral("spnSigFrequency"));
    auto* cmb = panel_->findChild<QComboBox*>(
        QStringLiteral("cmbSigFreqUnit"));

    // Set to kHz and value 500 = 500000 Hz
    cmb->blockSignals(true);
    cmb->setCurrentIndex(1);  // kHz
    cmb->blockSignals(false);

    spn->setValue(500.0);
    QTest::qWait(100);  // Wait for 20ms command latency.
    QApplication::processEvents();

    QVERIFY2(spy.count() >= 1,
             "frequencyChanged must be emitted on spinbox change");
  }

  // =========================================================================
  // F. Signal Generator — amplitude forwarding
  // =========================================================================

  void test_sigAmplitude_setOnController_updatesAmpDisplay() {
    panel_->setSignalGeneratorController(sig_ctrl_);
    connectSync(*sig_ctrl_);
    QApplication::processEvents();

    sig_ctrl_->setAmplitude(2.5);
    QTest::qWait(100);  // Wait for 20ms command latency.
    QApplication::processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblSigAmpDisplay"));
    QVERIFY2(lbl->text().contains(QStringLiteral("2.500")),
             "Amp display must show 2.500 V");
  }

  void test_sigAmplitude_spinboxChange_reachesController() {
    panel_->setSignalGeneratorController(sig_ctrl_);
    connectSync(*sig_ctrl_);
    QApplication::processEvents();

    QSignalSpy spy(
        sig_ctrl_,
        &mwa::hardware::SignalGeneratorControllerInterface::amplitudeChanged);

    auto* spn = panel_->findChild<QDoubleSpinBox*>(
        QStringLiteral("spnSigAmplitude"));
    spn->setValue(3.0);
    QTest::qWait(100);  // Wait for 20ms command latency.
    QApplication::processEvents();

    QVERIFY2(spy.count() >= 1,
             "amplitudeChanged must be emitted on spinbox change");
  }

  // =========================================================================
  // G. Signal Generator — waveform forwarding
  // =========================================================================

  void test_sigWaveform_setOnController_updatesDisplay() {
    panel_->setSignalGeneratorController(sig_ctrl_);
    connectSync(*sig_ctrl_);
    QApplication::processEvents();

    sig_ctrl_->setWaveform(Waveform::kSquare);
    QTest::qWait(100);  // Wait for 20ms command latency.
    QApplication::processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblSigWaveformDisplay"));
    QCOMPARE(lbl->text(), QStringLiteral("Square"));
  }

  void test_sigWaveform_comboChange_reachesController() {
    panel_->setSignalGeneratorController(sig_ctrl_);
    connectSync(*sig_ctrl_);
    QApplication::processEvents();

    QSignalSpy spy(
        sig_ctrl_,
        &mwa::hardware::SignalGeneratorControllerInterface::waveformChanged);

    auto* cmb = panel_->findChild<QComboBox*>(
        QStringLiteral("cmbSigWaveform"));
    cmb->setCurrentIndex(2);  // Triangle
    QTest::qWait(100);  // Wait for 20ms command latency.
    QApplication::processEvents();

    QVERIFY2(spy.count() >= 1,
             "waveformChanged must be emitted on combo change");
  }

  void test_sigWaveform_triangle_displaysTriangle() {
    panel_->setSignalGeneratorController(sig_ctrl_);
    connectSync(*sig_ctrl_);
    QApplication::processEvents();

    sig_ctrl_->setWaveform(Waveform::kTriangle);
    QTest::qWait(100);  // Wait for 20ms command latency.
    QApplication::processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblSigWaveformDisplay"));
    QCOMPARE(lbl->text(), QStringLiteral("Triangle"));
  }

  // =========================================================================
  // H. Signal Generator — output enable toggle
  // =========================================================================

  void test_sigOutput_setEnabled_buttonShowsON() {
    panel_->setSignalGeneratorController(sig_ctrl_);
    connectSync(*sig_ctrl_);
    QApplication::processEvents();

    sig_ctrl_->setOutputEnabled(true);
    QTest::qWait(100);  // Wait for 20ms command latency.
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnSigOutputEnable"));
    QVERIFY2(btn->isChecked(),
             "Output button must be checked when output is enabled");
    QCOMPARE(btn->text(), QStringLiteral("Output ON"));
  }

  void test_sigOutput_setDisabled_buttonShowsOFF() {
    panel_->setSignalGeneratorController(sig_ctrl_);
    connectSync(*sig_ctrl_);
    sig_ctrl_->setOutputEnabled(true);
    QTest::qWait(100);
    sig_ctrl_->setOutputEnabled(false);
    QTest::qWait(100);  // Wait for 20ms command latency.
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnSigOutputEnable"));
    QVERIFY2(!btn->isChecked(),
             "Output button must be unchecked when output is disabled");
    QCOMPARE(btn->text(), QStringLiteral("Output OFF"));
  }

  // =========================================================================
  // I. Network Analyzer controller attachment
  // =========================================================================

  void test_setNaController_nullptr_noCrash() {
    panel_->setNetworkAnalyzerController(nullptr);
    QApplication::processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpNaSweepConfig"));
    QVERIFY2(!grp->isEnabled(),
             "NA controls must remain disabled after nullptr");
  }

  void test_setNaController_tab1Enabled() {
    panel_->setNetworkAnalyzerController(na_ctrl_);
    QApplication::processEvents();

    auto* tab = panel_->findChild<QTabWidget*>(
        QStringLiteral("tabSignalNA"));
    QVERIFY2(tab->isTabEnabled(1),
             "Tab 1 must be enabled after attaching NA controller");
  }

  void test_setNaController_nullptr_disablesTab1() {
    panel_->setNetworkAnalyzerController(na_ctrl_);
    panel_->setNetworkAnalyzerController(nullptr);
    QApplication::processEvents();

    auto* tab = panel_->findChild<QTabWidget*>(
        QStringLiteral("tabSignalNA"));
    QVERIFY2(!tab->isTabEnabled(1),
             "Tab 1 must be disabled after detaching NA controller");
  }

  void test_setNaController_connected_controlsEnabled() {
    panel_->setNetworkAnalyzerController(na_ctrl_);
    connectSync(*na_ctrl_);
    QApplication::processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpNaSweepConfig"));
    QVERIFY2(grp->isEnabled(),
             "NA sweep config must be enabled when connected");
  }

  // =========================================================================
  // J. Network Analyzer — connection state transitions
  // =========================================================================

  void test_naState_kConnected_buttonTextIsDisconnect() {
    panel_->setNetworkAnalyzerController(na_ctrl_);
    connectSync(*na_ctrl_);
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnConnect"));
    QCOMPARE(btn->text(), QStringLiteral("Disconnect"));
  }

  void test_naState_kConnected_stateTextIdle() {
    panel_->setNetworkAnalyzerController(na_ctrl_);
    connectSync(*na_ctrl_);
    QApplication::processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblNaStateText"));
    QCOMPARE(lbl->text(), QStringLiteral("Idle"));
  }

  void test_naState_kDisconnected_controlsDisabled() {
    panel_->setNetworkAnalyzerController(na_ctrl_);
    connectSync(*na_ctrl_);
    na_ctrl_->disconnectDevice();
    QApplication::processEvents();

    auto* grp = panel_->findChild<QGroupBox*>(
        QStringLiteral("grpNaSweepConfig"));
    QVERIFY2(!grp->isEnabled(),
             "NA controls must be disabled after disconnect");
  }

  // =========================================================================
  // K. Network Analyzer — measurement lifecycle
  // =========================================================================

  void test_naMeasure_click_disablesButton() {
    panel_->setNetworkAnalyzerController(na_ctrl_);
    connectSync(*na_ctrl_);
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnNaMeasure"));
    btn->click();
    QApplication::processEvents();

    QVERIFY2(!btn->isEnabled(),
             "Measure button must be disabled during measurement");
  }

  void test_naMeasure_click_showsProgressBar() {
    panel_->setNetworkAnalyzerController(na_ctrl_);
    connectSync(*na_ctrl_);
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnNaMeasure"));
    btn->click();
    QApplication::processEvents();

    auto* prg = panel_->findChild<QProgressBar*>(
        QStringLiteral("prgNaMeasurement"));
    QVERIFY2(!prg->isHidden(),
             "Progress bar must be visible during measurement");
  }

  void test_naMeasure_click_stateTextMeasuring() {
    panel_->setNetworkAnalyzerController(na_ctrl_);
    connectSync(*na_ctrl_);
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnNaMeasure"));
    btn->click();
    QApplication::processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblNaStateText"));
    QCOMPARE(lbl->text(), QStringLiteral("Measuring..."));
  }

  void test_naMeasure_complete_reEnablesButton() {
    panel_->setNetworkAnalyzerController(na_ctrl_);
    connectSync(*na_ctrl_);
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnNaMeasure"));
    btn->click();
    // Wait for 1s measurement delay + margin
    QTest::qWait(1500);
    QApplication::processEvents();

    QVERIFY2(btn->isEnabled(),
             "Measure button must be re-enabled after completion");
  }

  void test_naMeasure_complete_hidesProgressBar() {
    panel_->setNetworkAnalyzerController(na_ctrl_);
    connectSync(*na_ctrl_);
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnNaMeasure"));
    btn->click();
    QTest::qWait(1500);
    QApplication::processEvents();

    auto* prg = panel_->findChild<QProgressBar*>(
        QStringLiteral("prgNaMeasurement"));
    QVERIFY2(prg->isHidden(),
             "Progress bar must be hidden after measurement completes");
  }

  void test_naMeasure_complete_stateTextComplete() {
    panel_->setNetworkAnalyzerController(na_ctrl_);
    connectSync(*na_ctrl_);
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnNaMeasure"));
    btn->click();
    QTest::qWait(1500);
    QApplication::processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblNaStateText"));
    QCOMPARE(lbl->text(), QStringLiteral("Complete"));
  }

  void test_naMeasure_complete_sparamDisplayUpdated() {
    panel_->setNetworkAnalyzerController(na_ctrl_);
    connectSync(*na_ctrl_);
    QApplication::processEvents();

    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnNaMeasure"));
    btn->click();
    QTest::qWait(1500);
    QApplication::processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblNaSparamDisplay"));
    // After measurement, the label should show point count and
    // frequency range info, not the initial placeholder text.
    QVERIFY2(!lbl->text().contains(
                 QStringLiteral("No measurement data")),
             "S-param display must update after measurement");
    QVERIFY2(lbl->text().contains(QStringLiteral("Points")),
             "S-param display must contain point count");
  }

  // =========================================================================
  // L. NA — frequency range and point changes from controller
  // =========================================================================

  void test_naFrequencyRange_updated_summaryChanges() {
    panel_->setNetworkAnalyzerController(na_ctrl_);
    connectSync(*na_ctrl_);
    QApplication::processEvents();

    na_ctrl_->setFrequencyRange(2000000.0, 50000000.0);
    QApplication::processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblNaRangeSummary"));
    QVERIFY2(lbl->text().contains(QStringLiteral("Range")),
             "Range summary must be updated");
  }

  void test_naNumPoints_updated_summaryChanges() {
    panel_->setNetworkAnalyzerController(na_ctrl_);
    connectSync(*na_ctrl_);
    QApplication::processEvents();

    na_ctrl_->setNumPoints(401);
    QApplication::processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblNaPointsSummary"));
    QVERIFY2(lbl->text().contains(QStringLiteral("401")),
             "Points summary must show 401");
  }

  // =========================================================================
  // M. Independent controller attachment — both tabs
  // =========================================================================

  void test_bothControllers_attached_bothTabsEnabled() {
    panel_->setSignalGeneratorController(sig_ctrl_);
    panel_->setNetworkAnalyzerController(na_ctrl_);
    QApplication::processEvents();

    auto* tab = panel_->findChild<QTabWidget*>(
        QStringLiteral("tabSignalNA"));
    QVERIFY(tab->isTabEnabled(0));
    QVERIFY(tab->isTabEnabled(1));
  }

  void test_detachSig_naTabStillEnabled() {
    panel_->setSignalGeneratorController(sig_ctrl_);
    panel_->setNetworkAnalyzerController(na_ctrl_);
    QApplication::processEvents();

    panel_->setSignalGeneratorController(nullptr);
    QApplication::processEvents();

    auto* tab = panel_->findChild<QTabWidget*>(
        QStringLiteral("tabSignalNA"));
    QVERIFY2(!tab->isTabEnabled(0),
             "Tab 0 must be disabled");
    QVERIFY2(tab->isTabEnabled(1),
             "Tab 1 must remain enabled");
  }

  void test_detachNa_sigTabStillEnabled() {
    panel_->setSignalGeneratorController(sig_ctrl_);
    panel_->setNetworkAnalyzerController(na_ctrl_);
    QApplication::processEvents();

    panel_->setNetworkAnalyzerController(nullptr);
    QApplication::processEvents();

    auto* tab = panel_->findChild<QTabWidget*>(
        QStringLiteral("tabSignalNA"));
    QVERIFY2(tab->isTabEnabled(0),
             "Tab 0 must remain enabled");
    QVERIFY2(!tab->isTabEnabled(1),
             "Tab 1 must be disabled");
  }

  // =========================================================================
  // N. Rapid operations (stress boundary)
  // =========================================================================

  void test_sigFrequency_rapidChanges_noCrash() {
    panel_->setSignalGeneratorController(sig_ctrl_);
    connectSync(*sig_ctrl_);
    QApplication::processEvents();

    for (int i = 0; i < 20; ++i) {
      sig_ctrl_->setFrequency(1000.0 * (i + 1));
    }
    QTest::qWait(100);  // Wait for all deferred signals.
    QApplication::processEvents();

    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblSigFreqDisplay"));
    // Should show the last value (20 kHz)
    QVERIFY2(lbl->text().contains(QStringLiteral("kHz")),
             "Freq display must show kHz after rapid changes");
  }

  void test_controllerSwap_noCrash() {
    panel_->setSignalGeneratorController(sig_ctrl_);
    connectSync(*sig_ctrl_);
    QApplication::processEvents();

    // Swap to nullptr and back rapidly.
    for (int i = 0; i < 5; ++i) {
      panel_->setSignalGeneratorController(nullptr);
      QApplication::processEvents();
      panel_->setSignalGeneratorController(sig_ctrl_);
      QApplication::processEvents();
    }

    auto* tab = panel_->findChild<QTabWidget*>(
        QStringLiteral("tabSignalNA"));
    QVERIFY(tab->isTabEnabled(0));
  }
};

QTEST_APPLESS_MAIN(TestSignalPanel)
#include "test_signal_panel.moc"
