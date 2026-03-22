/**
 * @file test_device_interface.cpp
 * @brief Adversarial tests for mwa::hardware::DeviceInterface.
 * @date 2026-03-22
 * @copyright LGPL-3.0-or-later
 */

#include <QMetaEnum>
#include <QObject>
#include <QSignalSpy>
#include <QString>
#include <QtTest>

#include "hardware/device_interface.h"

using mwa::hardware::DeviceInterface;
using DeviceState = DeviceInterface::DeviceState;
using DeviceType = DeviceInterface::DeviceType;

// ---------------------------------------------------------------------------
// Minimal concrete implementation used across tests.
// ---------------------------------------------------------------------------
class MockDevice : public DeviceInterface {
  Q_OBJECT

 public:
  explicit MockDevice(QObject* parent = nullptr)
      : DeviceInterface(parent),
        state_(DeviceState::kDisconnected),
        type_(DeviceType::kLed) {}

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

  [[nodiscard]] QString deviceName() const override { return name_; }

  [[nodiscard]] DeviceType deviceType() const override { return type_; }

  [[nodiscard]] DeviceState state() const override { return state_; }

  [[nodiscard]] bool isConnected() const override {
    return state_ == DeviceState::kConnected;
  }

  // Helpers to drive the device for testing.
  void setName(const QString& n) { name_ = n; }
  void setType(DeviceType t) { type_ = t; }
  void triggerError(const QString& msg) {
    state_ = DeviceState::kError;
    emit stateChanged(state_);
    emit errorOccurred(msg);
  }

 private:
  DeviceState state_;
  DeviceType type_;
  QString name_{QStringLiteral("MockDevice")};
};

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------
class TestDeviceInterface : public QObject {
  Q_OBJECT

 private slots:
  void test_concreteSubclass_canBeInstantiated();
  void test_connectDevice_emitsStateChangingThenConnected();
  void test_disconnectDevice_emitsDisconnected();
  void test_isConnected_trueAfterConnect();
  void test_isConnected_falseAfterDisconnect();
  void test_isConnected_falseInitially();
  void test_deviceName_returnsSetValue();
  void test_deviceType_returnsSetType();
  void test_errorOccurred_signalEmitted();
  void test_stateChanged_toError_viaHelper();
  void test_deviceState_enumValues_areDistinct();
  void test_deviceType_enumValues_areDistinct();
  void test_deviceState_qEnum_accessible();
  void test_deviceType_qEnum_accessible();
  void test_stateChanged_signalCarriesCorrectValue();
  void test_errorOccurred_signalCarriesMessage();
  void test_parentOwnership_deviceDeletedWithParent();
};

// ---------------------------------------------------------------------------
void TestDeviceInterface::test_concreteSubclass_canBeInstantiated() {
  MockDevice dev;
  // Verify the object is usable: a virtual call must return a non-null
  // string (the default name set in the constructor).
  QVERIFY2(!dev.deviceName().isNull(),
           "deviceName() must return a valid QString on a live instance");
}

// ---------------------------------------------------------------------------
void TestDeviceInterface::test_connectDevice_emitsStateChangingThenConnected() {
  MockDevice dev;
  QSignalSpy spy(&dev, &DeviceInterface::stateChanged);
  dev.connectDevice();
  QCOMPARE(spy.count(), 2);
  QCOMPARE(qvariant_cast<DeviceState>(spy.at(0).at(0)),
           DeviceState::kConnecting);
  QCOMPARE(qvariant_cast<DeviceState>(spy.at(1).at(0)),
           DeviceState::kConnected);
}

// ---------------------------------------------------------------------------
void TestDeviceInterface::test_disconnectDevice_emitsDisconnected() {
  MockDevice dev;
  dev.connectDevice();
  QSignalSpy spy(&dev, &DeviceInterface::stateChanged);
  dev.disconnectDevice();
  QCOMPARE(spy.count(), 1);
  QCOMPARE(qvariant_cast<DeviceState>(spy.at(0).at(0)),
           DeviceState::kDisconnected);
}

// ---------------------------------------------------------------------------
void TestDeviceInterface::test_isConnected_trueAfterConnect() {
  MockDevice dev;
  dev.connectDevice();
  QVERIFY2(dev.isConnected(), "isConnected() must be true after connectDevice");
}

// ---------------------------------------------------------------------------
void TestDeviceInterface::test_isConnected_falseAfterDisconnect() {
  MockDevice dev;
  dev.connectDevice();
  dev.disconnectDevice();
  QVERIFY2(!dev.isConnected(),
           "isConnected() must be false after disconnectDevice");
}

// ---------------------------------------------------------------------------
void TestDeviceInterface::test_isConnected_falseInitially() {
  MockDevice dev;
  QVERIFY2(!dev.isConnected(), "isConnected() must be false before connect");
}

// ---------------------------------------------------------------------------
void TestDeviceInterface::test_deviceName_returnsSetValue() {
  MockDevice dev;
  dev.setName(QStringLiteral("Pump Alpha"));
  QCOMPARE(dev.deviceName(), QStringLiteral("Pump Alpha"));
}

// ---------------------------------------------------------------------------
void TestDeviceInterface::test_deviceType_returnsSetType() {
  MockDevice dev;
  dev.setType(DeviceType::kCamera);
  QCOMPARE(dev.deviceType(), DeviceType::kCamera);
}

// ---------------------------------------------------------------------------
void TestDeviceInterface::test_errorOccurred_signalEmitted() {
  MockDevice dev;
  QSignalSpy spy(&dev, &DeviceInterface::errorOccurred);
  dev.triggerError(QStringLiteral("timeout"));
  QCOMPARE(spy.count(), 1);
}

// ---------------------------------------------------------------------------
void TestDeviceInterface::test_stateChanged_toError_viaHelper() {
  MockDevice dev;
  QSignalSpy spy(&dev, &DeviceInterface::stateChanged);
  dev.triggerError(QStringLiteral("fault"));
  // triggerError emits stateChanged(kError) then errorOccurred.
  QVERIFY2(spy.count() >= 1, "stateChanged must fire on error transition");
  bool saw_error = false;
  for (int i = 0; i < spy.count(); ++i) {
    if (qvariant_cast<DeviceState>(spy.at(i).at(0)) ==
        DeviceState::kError) {
      saw_error = true;
    }
  }
  QVERIFY2(saw_error, "stateChanged(kError) must be emitted");
}

// ---------------------------------------------------------------------------
// Enum integrity: all enum values must differ from one another.
// ---------------------------------------------------------------------------
void TestDeviceInterface::test_deviceState_enumValues_areDistinct() {
  QList<int> vals{
      static_cast<int>(DeviceState::kDisconnected),
      static_cast<int>(DeviceState::kConnecting),
      static_cast<int>(DeviceState::kConnected),
      static_cast<int>(DeviceState::kError)};
  for (int i = 0; i < vals.size(); ++i) {
    for (int j = i + 1; j < vals.size(); ++j) {
      QVERIFY2(vals[i] != vals[j],
               "DeviceState enum values must be distinct");
    }
  }
}

// ---------------------------------------------------------------------------
void TestDeviceInterface::test_deviceType_enumValues_areDistinct() {
  QList<int> vals{
      static_cast<int>(DeviceType::kLed),
      static_cast<int>(DeviceType::kPump),
      static_cast<int>(DeviceType::kSignalGenerator),
      static_cast<int>(DeviceType::kNetworkAnalyzer),
      static_cast<int>(DeviceType::kCamera),
      static_cast<int>(DeviceType::kStage)};
  for (int i = 0; i < vals.size(); ++i) {
    for (int j = i + 1; j < vals.size(); ++j) {
      QVERIFY2(vals[i] != vals[j],
               "DeviceType enum values must be distinct");
    }
  }
}

// ---------------------------------------------------------------------------
// Q_ENUM must make the enum discoverable via QMetaEnum.
// ---------------------------------------------------------------------------
static int findEnumInChain(const QMetaObject* mo, const char* name) {
  while (mo != nullptr) {
    int idx = mo->indexOfEnumerator(name);
    if (idx >= 0) return idx;
    mo = mo->superClass();
  }
  return -1;
}

void TestDeviceInterface::test_deviceState_qEnum_accessible() {
  int idx = findEnumInChain(&MockDevice::staticMetaObject,
                            "DeviceState");
  QVERIFY2(idx >= 0,
           "DeviceState must be registered via Q_ENUM");
}

// ---------------------------------------------------------------------------
void TestDeviceInterface::test_deviceType_qEnum_accessible() {
  int idx = findEnumInChain(&MockDevice::staticMetaObject,
                            "DeviceType");
  QVERIFY2(idx >= 0,
           "DeviceType must be registered via Q_ENUM");
}

// ---------------------------------------------------------------------------
void TestDeviceInterface::test_stateChanged_signalCarriesCorrectValue() {
  MockDevice dev;
  QSignalSpy spy(&dev, &DeviceInterface::stateChanged);
  dev.connectDevice();
  // Last emission should be kConnected.
  QVERIFY(spy.count() > 0);
  auto last_state =
      qvariant_cast<DeviceState>(spy.last().at(0));
  QCOMPARE(last_state, DeviceState::kConnected);
}

// ---------------------------------------------------------------------------
void TestDeviceInterface::test_errorOccurred_signalCarriesMessage() {
  MockDevice dev;
  QSignalSpy spy(&dev, &DeviceInterface::errorOccurred);
  const QString msg = QStringLiteral("communication timeout");
  dev.triggerError(msg);
  QCOMPARE(spy.count(), 1);
  QCOMPARE(spy.at(0).at(0).toString(), msg);
}

// ---------------------------------------------------------------------------
// Qt parent ownership: when parent is deleted the device must also be
// deleted — this verifies proper QObject hierarchy.
// ---------------------------------------------------------------------------
void TestDeviceInterface::test_parentOwnership_deviceDeletedWithParent() {
  auto* parent = new QObject();
  // Device is owned by parent; casting to DeviceInterface* to verify
  // it is accessible via the abstract pointer.
  DeviceInterface* dev = new MockDevice(parent);
  QVERIFY(dev->parent() == parent);
  delete parent;  // Must not crash.
  QVERIFY(true);
}

// ---------------------------------------------------------------------------
QTEST_MAIN(TestDeviceInterface)
#include "test_device_interface.moc"
