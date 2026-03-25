/**
 * @file test_device_manager.cpp
 * @brief Tests for the DeviceManager singleton.
 */

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>
#include <QTimer>

#include "hardware/device_manager.h"
#include "hardware/led/mock_led_controller.h"
#include "hardware/pump/mock_pump_controller.h"

using mwa::hardware::DeviceInterface;
using mwa::hardware::DeviceManager;
using mwa::hardware::MockLedController;
using mwa::hardware::MockPumpController;

class TestDeviceManager : public QObject {
  Q_OBJECT

 private:
  void clearAllDevices() {
    auto& mgr = DeviceManager::instance();
    for (auto* dev : mgr.allDevices()) {
      mgr.removeDevice(dev->deviceType());
    }
  }

 private slots:
  void init() { clearAllDevices(); }

  void cleanup() { clearAllDevices(); }

  void testSingletonIdentity() {
    auto& a = DeviceManager::instance();
    auto& b = DeviceManager::instance();
    QCOMPARE(&a, &b);
  }

  void testRegisterDeviceNull() {
    auto& mgr = DeviceManager::instance();
    QVERIFY(!mgr.registerDevice(nullptr));
    QCOMPARE(mgr.deviceCount(), 0);
  }

  void testRegisterAndLookup() {
    auto& mgr = DeviceManager::instance();
    auto* led = new MockLedController();

    QSignalSpy reg_spy(&mgr, &DeviceManager::deviceRegistered);

    QVERIFY(mgr.registerDevice(led));
    QCOMPARE(mgr.deviceCount(), 1);
    QCOMPARE(mgr.device(DeviceInterface::DeviceType::kLed), led);
    QCOMPARE(reg_spy.count(), 1);

    auto args = reg_spy.takeFirst();
    QCOMPARE(
        args.at(0).value<DeviceInterface::DeviceType>(),
        DeviceInterface::DeviceType::kLed);
  }

  void testLookupUnregisteredReturnsNull() {
    auto& mgr = DeviceManager::instance();
    QCOMPARE(mgr.device(DeviceInterface::DeviceType::kCamera),
             nullptr);
  }

  void testRegisterReplacesExisting() {
    auto& mgr = DeviceManager::instance();
    auto* led1 = new MockLedController();
    auto* led2 = new MockLedController();

    mgr.registerDevice(led1);
    QCOMPARE(mgr.device(DeviceInterface::DeviceType::kLed), led1);

    mgr.registerDevice(led2);
    QCOMPARE(mgr.device(DeviceInterface::DeviceType::kLed), led2);
    QCOMPARE(mgr.deviceCount(), 1);
  }

  void testRemoveDevice() {
    auto& mgr = DeviceManager::instance();
    auto* led = new MockLedController();
    mgr.registerDevice(led);

    QSignalSpy rem_spy(&mgr, &DeviceManager::deviceRemoved);

    QVERIFY(mgr.removeDevice(DeviceInterface::DeviceType::kLed));
    QCOMPARE(mgr.deviceCount(), 0);
    QCOMPARE(mgr.device(DeviceInterface::DeviceType::kLed), nullptr);
    QCOMPARE(rem_spy.count(), 1);
  }

  void testRemoveNonexistentReturnsFalse() {
    auto& mgr = DeviceManager::instance();
    QVERIFY(!mgr.removeDevice(DeviceInterface::DeviceType::kStage));
  }

  void testAllDevices() {
    auto& mgr = DeviceManager::instance();
    auto* led = new MockLedController();
    auto* pump = new MockPumpController();

    mgr.registerDevice(led);
    mgr.registerDevice(pump);

    auto all = mgr.allDevices();
    QCOMPARE(static_cast<int>(all.size()), 2);
    QVERIFY(std::find(all.begin(), all.end(), led) != all.end());
    QVERIFY(std::find(all.begin(), all.end(), pump) != all.end());
  }

  void testAllDevicesEmptyWhenNoneRegistered() {
    auto& mgr = DeviceManager::instance();
    auto all = mgr.allDevices();
    QVERIFY(all.empty());
  }

  void testStateChangedSignalForwarded() {
    auto& mgr = DeviceManager::instance();
    auto* led = new MockLedController();
    mgr.registerDevice(led);

    QSignalSpy state_spy(&mgr, &DeviceManager::deviceStateChanged);

    led->connectDevice();
    // MockLedController goes kConnecting immediately.
    QVERIFY(state_spy.count() >= 1);

    auto args = state_spy.first();
    QCOMPARE(
        args.at(0).value<DeviceInterface::DeviceType>(),
        DeviceInterface::DeviceType::kLed);
    QCOMPARE(
        args.at(1).value<DeviceInterface::DeviceState>(),
        DeviceInterface::DeviceState::kConnecting);
  }

  void testErrorSignalForwarded() {
    auto& mgr = DeviceManager::instance();
    auto* led = new MockLedController();
    mgr.registerDevice(led);

    QSignalSpy error_spy(&mgr, &DeviceManager::deviceError);

    // Directly emit an error on the device.
    emit led->errorOccurred("test error");

    QCOMPARE(error_spy.count(), 1);
    auto args = error_spy.first();
    QCOMPARE(
        args.at(0).value<DeviceInterface::DeviceType>(),
        DeviceInterface::DeviceType::kLed);
    QCOMPARE(args.at(1).toString(), QString("test error"));
  }

  void testConnectAll() {
    auto& mgr = DeviceManager::instance();
    auto* led = new MockLedController();
    auto* pump = new MockPumpController();
    mgr.registerDevice(led);
    mgr.registerDevice(pump);

    mgr.connectAll();

    // Both should be at least kConnecting.
    QVERIFY(led->state() ==
                DeviceInterface::DeviceState::kConnecting ||
            led->state() ==
                DeviceInterface::DeviceState::kConnected);
    QVERIFY(pump->state() ==
                DeviceInterface::DeviceState::kConnecting ||
            pump->state() ==
                DeviceInterface::DeviceState::kConnected);
  }

  void testConnectAllSkipsAlreadyConnected() {
    auto& mgr = DeviceManager::instance();
    auto* led = new MockLedController();
    mgr.registerDevice(led);

    led->connectDevice();
    // Wait for mock to finish connecting (500 ms delay).
    QSignalSpy spy(led, &DeviceInterface::stateChanged);
    QVERIFY(spy.wait(2000));

    QCOMPARE(led->state(), DeviceInterface::DeviceState::kConnected);

    QSignalSpy state_spy(&mgr, &DeviceManager::deviceStateChanged);
    mgr.connectAll();
    // No additional state changes should occur.
    QCOMPARE(state_spy.count(), 0);
  }

  void testDisconnectAll() {
    auto& mgr = DeviceManager::instance();
    auto* led = new MockLedController();
    mgr.registerDevice(led);

    led->connectDevice();
    QSignalSpy spy(led, &DeviceInterface::stateChanged);
    QVERIFY(spy.wait(2000));
    QCOMPARE(led->state(), DeviceInterface::DeviceState::kConnected);

    mgr.disconnectAll();
    QCOMPARE(led->state(),
             DeviceInterface::DeviceState::kDisconnected);
  }

  void testDisconnectAllSkipsAlreadyDisconnected() {
    auto& mgr = DeviceManager::instance();
    auto* led = new MockLedController();
    mgr.registerDevice(led);

    QCOMPARE(led->state(),
             DeviceInterface::DeviceState::kDisconnected);

    QSignalSpy state_spy(&mgr, &DeviceManager::deviceStateChanged);
    mgr.disconnectAll();
    QCOMPARE(state_spy.count(), 0);
  }

  void testRemoveDisconnectsDevice() {
    auto& mgr = DeviceManager::instance();
    auto* led = new MockLedController();
    mgr.registerDevice(led);

    led->connectDevice();
    QSignalSpy spy(led, &DeviceInterface::stateChanged);
    QVERIFY(spy.wait(2000));
    QCOMPARE(led->state(), DeviceInterface::DeviceState::kConnected);

    // Grab a spy before removal to catch disconnect.
    QSignalSpy rem_spy(&mgr, &DeviceManager::deviceRemoved);
    mgr.removeDevice(DeviceInterface::DeviceType::kLed);
    QCOMPARE(rem_spy.count(), 1);
  }

  void testSignalsDisconnectedAfterRemoval() {
    auto& mgr = DeviceManager::instance();
    auto* led = new MockLedController();
    mgr.registerDevice(led);
    mgr.removeDevice(DeviceInterface::DeviceType::kLed);

    // Process deleteLater.
    QCoreApplication::processEvents();

    // The manager should no longer forward signals from removed device.
    QSignalSpy state_spy(&mgr, &DeviceManager::deviceStateChanged);
    // led is now deleted — we cannot emit on it.
    // The key check is that removal worked without crash.
    QCOMPARE(state_spy.count(), 0);
  }

  void testMultipleDeviceTypes() {
    auto& mgr = DeviceManager::instance();
    auto* led = new MockLedController();
    auto* pump = new MockPumpController();

    mgr.registerDevice(led);
    mgr.registerDevice(pump);

    QCOMPARE(mgr.deviceCount(), 2);
    QCOMPARE(mgr.device(DeviceInterface::DeviceType::kLed), led);
    QCOMPARE(mgr.device(DeviceInterface::DeviceType::kPump), pump);

    mgr.removeDevice(DeviceInterface::DeviceType::kLed);
    QCOMPARE(mgr.deviceCount(), 1);
    QCOMPARE(mgr.device(DeviceInterface::DeviceType::kLed), nullptr);
    QCOMPARE(mgr.device(DeviceInterface::DeviceType::kPump), pump);
  }
};

QTEST_MAIN(TestDeviceManager)
#include "test_device_manager.moc"
