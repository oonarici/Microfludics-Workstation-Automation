/**
 * @file test_device_status_dashboard.cpp
 * @brief Unit tests for the DeviceStatusDashboard widget.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Tests cover widget structure, initial state for all 6 device cards,
 * state updates via registerDevice/unregisterDevice, and signal
 * emission.
 *
 * @copyright LGPL-3.0-or-later
 */

#include <QApplication>
#include <QFrame>
#include <QGroupBox>
#include <QLabel>
#include <QSignalSpy>
#include <QTest>

#include "gui/widgets/device_status_dashboard.h"
#include "hardware/device_interface.h"

static int s_argc = 1;
static char s_app_name[] = "test_device_status_dashboard";
static char* s_argv[] = {s_app_name};

// ---------------------------------------------------------------------------
// Mock device for testing state changes.
// ---------------------------------------------------------------------------
class MockDevice : public mwa::hardware::DeviceInterface {
  Q_OBJECT

 public:
  explicit MockDevice(
      mwa::hardware::DeviceInterface::DeviceType type,
      QObject* parent = nullptr)
      : DeviceInterface(parent), type_(type) {}

  void connectDevice() override {
    state_ = DeviceState::kConnecting;
    emit stateChanged(state_);
    state_ = DeviceState::kConnected;
    emit stateChanged(state_);
  }

  void disconnectDevice() override {
    state_ = DeviceState::kDisconnected;
    emit stateChanged(state_);
  }

  QString deviceName() const override {
    return QStringLiteral("MockDevice");
  }

  DeviceType deviceType() const override { return type_; }
  DeviceState state() const override { return state_; }
  bool isConnected() const override {
    return state_ == DeviceState::kConnected;
  }

  void simulateState(DeviceState s) {
    state_ = s;
    emit stateChanged(state_);
  }

 private:
  DeviceType type_;
  DeviceState state_{DeviceState::kDisconnected};
};

class TestDeviceStatusDashboard : public QObject {
  Q_OBJECT

 private:
  QApplication* app_{nullptr};
  mwa::gui::DeviceStatusDashboard* dashboard_{nullptr};

 private slots:
  void initTestCase() {
    app_ = new QApplication(s_argc, s_argv);
  }

  void init() {
    dashboard_ = new mwa::gui::DeviceStatusDashboard();
    QApplication::processEvents();
  }

  void cleanup() {
    delete dashboard_;
    dashboard_ = nullptr;
  }

  void cleanupTestCase() {
    delete app_;
  }

  // --- Widget structure ---
  void test_objectName() {
    QCOMPARE(dashboard_->objectName(),
             QStringLiteral("deviceStatusDashboard"));
  }

  void test_groupBox_exists() {
    auto* grp = dashboard_->findChild<QGroupBox*>(
        QStringLiteral("grpDeviceStatus"));
    QVERIFY(grp != nullptr);
  }

  // --- Card existence (all 6 devices) ---
  void test_cardLed_exists() {
    QVERIFY(dashboard_->findChild<QFrame*>(
                QStringLiteral("cardFrameLed")) != nullptr);
  }

  void test_cardPump_exists() {
    QVERIFY(dashboard_->findChild<QFrame*>(
                QStringLiteral("cardFramePump")) != nullptr);
  }

  void test_cardSignalGenerator_exists() {
    QVERIFY(dashboard_->findChild<QFrame*>(
        QStringLiteral("cardFrameSignalGenerator")) != nullptr);
  }

  void test_cardNetworkAnalyzer_exists() {
    QVERIFY(dashboard_->findChild<QFrame*>(
        QStringLiteral("cardFrameNetworkAnalyzer")) != nullptr);
  }

  void test_cardCamera_exists() {
    QVERIFY(dashboard_->findChild<QFrame*>(
        QStringLiteral("cardFrameCamera")) != nullptr);
  }

  void test_cardStage_exists() {
    QVERIFY(dashboard_->findChild<QFrame*>(
        QStringLiteral("cardFrameStage")) != nullptr);
  }

  // --- Initial state (all disconnected) ---
  void test_allCards_initiallyDisconnected() {
    QStringList suffixes = {
        "Led", "Pump", "SignalGenerator",
        "NetworkAnalyzer", "Camera", "Stage"};
    for (const auto& suffix : suffixes) {
      auto* lbl = dashboard_->findChild<QLabel*>(
          QStringLiteral("lblState") + suffix);
      QVERIFY2(lbl != nullptr,
               qPrintable("Missing lblState" + suffix));
      QCOMPARE(lbl->text(), QStringLiteral("Disconnected"));
    }
  }

  // --- Name labels ---
  void test_nameLed_correct() {
    auto* lbl = dashboard_->findChild<QLabel*>(
        QStringLiteral("lblNameLed"));
    QVERIFY(lbl != nullptr);
    QCOMPARE(lbl->text(),
             QStringLiteral("LED Light Source"));
  }

  void test_nameCamera_correct() {
    auto* lbl = dashboard_->findChild<QLabel*>(
        QStringLiteral("lblNameCamera"));
    QVERIFY(lbl != nullptr);
    QCOMPARE(lbl->text(), QStringLiteral("Camera"));
  }

  void test_nameStage_correct() {
    auto* lbl = dashboard_->findChild<QLabel*>(
        QStringLiteral("lblNameStage"));
    QVERIFY(lbl != nullptr);
    QCOMPARE(lbl->text(), QStringLiteral("XYZ Stage"));
  }

  // --- Register device + state updates ---
  void test_registerDevice_updatesState() {
    using DT = mwa::hardware::DeviceInterface::DeviceType;
    auto* mock = new MockDevice(DT::kLed, dashboard_);

    dashboard_->registerDevice(mock);
    QApplication::processEvents();

    auto* lbl = dashboard_->findChild<QLabel*>(
        QStringLiteral("lblStateLed"));
    QCOMPARE(lbl->text(), QStringLiteral("Disconnected"));
  }

  void test_stateChange_connected_updatesCard() {
    using DT = mwa::hardware::DeviceInterface::DeviceType;
    using DS = mwa::hardware::DeviceInterface::DeviceState;
    auto* mock = new MockDevice(DT::kCamera, dashboard_);

    dashboard_->registerDevice(mock);
    mock->simulateState(DS::kConnected);
    QApplication::processEvents();

    auto* lbl = dashboard_->findChild<QLabel*>(
        QStringLiteral("lblStateCamera"));
    QCOMPARE(lbl->text(), QStringLiteral("Connected"));
  }

  void test_stateChange_connecting_updatesCard() {
    using DT = mwa::hardware::DeviceInterface::DeviceType;
    using DS = mwa::hardware::DeviceInterface::DeviceState;
    auto* mock = new MockDevice(DT::kPump, dashboard_);

    dashboard_->registerDevice(mock);
    mock->simulateState(DS::kConnecting);
    QApplication::processEvents();

    auto* lbl = dashboard_->findChild<QLabel*>(
        QStringLiteral("lblStatePump"));
    QCOMPARE(lbl->text(), QStringLiteral("Connecting..."));
  }

  void test_stateChange_error_updatesCard() {
    using DT = mwa::hardware::DeviceInterface::DeviceType;
    using DS = mwa::hardware::DeviceInterface::DeviceState;
    auto* mock = new MockDevice(DT::kStage, dashboard_);

    dashboard_->registerDevice(mock);
    mock->simulateState(DS::kError);
    QApplication::processEvents();

    auto* lbl = dashboard_->findChild<QLabel*>(
        QStringLiteral("lblStateStage"));
    QCOMPARE(lbl->text(), QStringLiteral("Error"));
  }

  // --- Unregister device ---
  void test_unregisterDevice_resetsToDisconnected() {
    using DT = mwa::hardware::DeviceInterface::DeviceType;
    using DS = mwa::hardware::DeviceInterface::DeviceState;
    auto* mock = new MockDevice(DT::kLed, dashboard_);

    dashboard_->registerDevice(mock);
    mock->simulateState(DS::kConnected);
    QApplication::processEvents();

    auto* lbl = dashboard_->findChild<QLabel*>(
        QStringLiteral("lblStateLed"));
    QCOMPARE(lbl->text(), QStringLiteral("Connected"));

    dashboard_->unregisterDevice(DT::kLed);
    QApplication::processEvents();

    QCOMPARE(lbl->text(), QStringLiteral("Disconnected"));
  }

  // --- Register null device ---
  void test_registerDevice_null_noCrash() {
    dashboard_->registerDevice(nullptr);
    QApplication::processEvents();
    QVERIFY(true);
  }

  // --- Replace device registration ---
  void test_registerDevice_replacesExisting() {
    using DT = mwa::hardware::DeviceInterface::DeviceType;
    using DS = mwa::hardware::DeviceInterface::DeviceState;

    auto* mock1 = new MockDevice(DT::kCamera, dashboard_);
    auto* mock2 = new MockDevice(DT::kCamera, dashboard_);

    dashboard_->registerDevice(mock1);
    mock1->simulateState(DS::kConnected);
    QApplication::processEvents();

    dashboard_->registerDevice(mock2);
    QApplication::processEvents();

    auto* lbl = dashboard_->findChild<QLabel*>(
        QStringLiteral("lblStateCamera"));
    QCOMPARE(lbl->text(), QStringLiteral("Disconnected"));

    mock2->simulateState(DS::kError);
    QApplication::processEvents();
    QCOMPARE(lbl->text(), QStringLiteral("Error"));

    // Old device signals should be disconnected.
    mock1->simulateState(DS::kConnected);
    QApplication::processEvents();
    QCOMPARE(lbl->text(), QStringLiteral("Error"));
  }

  // --- cardClicked signal ---
  void test_cardClicked_signal_notEmittedOnConstruction() {
    QSignalSpy spy(dashboard_,
                   &mwa::gui::DeviceStatusDashboard::cardClicked);
    QApplication::processEvents();
    QCOMPARE(spy.count(), 0);
  }
};

QTEST_APPLESS_MAIN(TestDeviceStatusDashboard)
#include "test_device_status_dashboard.moc"
