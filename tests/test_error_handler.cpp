/**
 * @file test_error_handler.cpp
 * @brief Tests for the ErrorHandler recovery framework.
 */

#include <QCoreApplication>
#include <QSignalSpy>
#include <QTest>

#include "hardware/error_handler.h"
#include "hardware/led/mock_led_controller.h"

using mwa::hardware::DeviceInterface;
using mwa::hardware::ErrorHandler;
using mwa::hardware::MockLedController;
using mwa::hardware::RetryPolicy;

class TestErrorHandler : public QObject {
  Q_OBJECT

 private:
  void connectAndWait(MockLedController* dev) {
    dev->connectDevice();
    QSignalSpy spy(dev, &DeviceInterface::stateChanged);
    QVERIFY(spy.wait(2000));
    if (dev->state() != DeviceInterface::DeviceState::kConnected) {
      QVERIFY(spy.wait(2000));
    }
  }

 private slots:
  void testDefaultRetryPolicy() {
    ErrorHandler handler;
    auto policy = handler.retryPolicy();
    QCOMPARE(policy.max_attempts, 3);
    QCOMPARE(policy.base_delay_ms, 500);
    QCOMPARE(policy.backoff_factor, 2.0);
    QCOMPARE(policy.max_delay_ms, 10000);
  }

  void testSetRetryPolicy() {
    ErrorHandler handler;
    RetryPolicy custom{5, 100, 1.5, 5000};
    handler.setRetryPolicy(custom);

    auto policy = handler.retryPolicy();
    QCOMPARE(policy.max_attempts, 5);
    QCOMPARE(policy.base_delay_ms, 100);
    QCOMPARE(policy.backoff_factor, 1.5);
    QCOMPARE(policy.max_delay_ms, 5000);
  }

  void testImmediateSuccess() {
    ErrorHandler handler;
    QSignalSpy success_spy(&handler, &ErrorHandler::operationSucceeded);
    QSignalSpy fail_spy(&handler, &ErrorHandler::allRetriesFailed);

    handler.execute([]() { return true; });

    QCOMPARE(success_spy.count(), 1);
    QCOMPARE(success_spy.first().at(0).toInt(), 1);
    QCOMPARE(fail_spy.count(), 0);
    QVERIFY(!handler.isRetrying());
  }

  void testImmediateFailureExhaustsRetries() {
    ErrorHandler handler;
    RetryPolicy fast{3, 10, 1.0, 100};
    handler.setRetryPolicy(fast);

    QSignalSpy success_spy(&handler, &ErrorHandler::operationSucceeded);
    QSignalSpy retry_spy(&handler, &ErrorHandler::retrying);
    QSignalSpy fail_spy(&handler, &ErrorHandler::allRetriesFailed);

    int call_count = 0;
    handler.execute([&call_count]() {
      ++call_count;
      return false;
    });

    // First attempt happens synchronously and fails.
    QCOMPARE(call_count, 1);
    QVERIFY(handler.isRetrying());

    // Wait for all retries to complete.
    QVERIFY(QTest::qWaitFor(
        [&fail_spy]() { return fail_spy.count() == 1; }, 2000));

    QCOMPARE(call_count, 3);
    QCOMPARE(success_spy.count(), 0);
    QCOMPARE(retry_spy.count(), 2);  // 2 retries after first failure.
    QCOMPARE(fail_spy.first().at(0).toInt(), 3);
    QVERIFY(!handler.isRetrying());
  }

  void testSuccessOnSecondAttempt() {
    ErrorHandler handler;
    RetryPolicy fast{3, 10, 1.0, 100};
    handler.setRetryPolicy(fast);

    QSignalSpy success_spy(&handler, &ErrorHandler::operationSucceeded);

    int call_count = 0;
    handler.execute([&call_count]() {
      ++call_count;
      return call_count >= 2;
    });

    QVERIFY(QTest::qWaitFor(
        [&success_spy]() { return success_spy.count() == 1; }, 2000));

    QCOMPARE(call_count, 2);
    QCOMPARE(success_spy.first().at(0).toInt(), 2);
  }

  void testExponentialBackoffDelays() {
    ErrorHandler handler;
    RetryPolicy policy{4, 100, 2.0, 10000};
    handler.setRetryPolicy(policy);

    QSignalSpy retry_spy(&handler, &ErrorHandler::retrying);
    QSignalSpy fail_spy(&handler, &ErrorHandler::allRetriesFailed);

    handler.execute([]() { return false; });

    QVERIFY(QTest::qWaitFor(
        [&fail_spy]() { return fail_spy.count() == 1; }, 5000));

    // Should have 3 retrying signals (attempts 1, 2, 3 — then fail).
    QCOMPARE(retry_spy.count(), 3);
    // Verify delays: 100, 200, 400.
    QCOMPARE(retry_spy.at(0).at(1).toInt(), 100);
    QCOMPARE(retry_spy.at(1).at(1).toInt(), 200);
    QCOMPARE(retry_spy.at(2).at(1).toInt(), 400);
  }

  void testMaxDelayIsCapped() {
    ErrorHandler handler;
    RetryPolicy policy{3, 50, 10.0, 80};
    handler.setRetryPolicy(policy);

    QSignalSpy retry_spy(&handler, &ErrorHandler::retrying);
    QSignalSpy fail_spy(&handler, &ErrorHandler::allRetriesFailed);

    handler.execute([]() { return false; });

    QVERIFY(QTest::qWaitFor(
        [&fail_spy]() { return fail_spy.count() == 1; }, 2000));

    // Second retry delay would be 500 without cap (50 * 10.0).
    QVERIFY(retry_spy.count() >= 1);
    for (int i = 0; i < retry_spy.count(); ++i) {
      QVERIFY(retry_spy.at(i).at(1).toInt() <= 80);
    }
  }

  void testCancelStopsRetrying() {
    ErrorHandler handler;
    RetryPolicy slow{5, 1000, 2.0, 10000};
    handler.setRetryPolicy(slow);

    QSignalSpy fail_spy(&handler, &ErrorHandler::allRetriesFailed);

    handler.execute([]() { return false; });

    QVERIFY(handler.isRetrying());
    handler.cancel();
    QVERIFY(!handler.isRetrying());
    QCOMPARE(handler.currentAttempt(), 0);

    // Wait a bit to make sure no more signals come.
    QTest::qWait(200);
    QCOMPARE(fail_spy.count(), 0);
  }

  void testCurrentAttemptDuringRetry() {
    ErrorHandler handler;
    RetryPolicy fast{3, 10, 1.0, 100};
    handler.setRetryPolicy(fast);

    int captured_attempt = 0;
    handler.execute([&handler, &captured_attempt]() {
      captured_attempt = handler.currentAttempt();
      return false;
    });

    QCOMPARE(captured_attempt, 1);
  }

  void testNotRetryingInitially() {
    ErrorHandler handler;
    QVERIFY(!handler.isRetrying());
    QCOMPARE(handler.currentAttempt(), 0);
  }

  void testExceptionTreatedAsFailure() {
    ErrorHandler handler;
    RetryPolicy fast{2, 10, 1.0, 100};
    handler.setRetryPolicy(fast);

    QSignalSpy fail_spy(&handler, &ErrorHandler::allRetriesFailed);

    handler.execute([]() -> bool {
      throw std::runtime_error("test exception");
    });

    QVERIFY(QTest::qWaitFor(
        [&fail_spy]() { return fail_spy.count() == 1; }, 2000));
    QCOMPARE(fail_spy.first().at(0).toInt(), 2);
  }

  void testMonitorDeviceDetectsDisconnection() {
    ErrorHandler handler;
    auto* led = new MockLedController();

    QSignalSpy recon_spy(&handler, &ErrorHandler::reconnectStarted);

    handler.monitorDevice(led);
    QVERIFY(handler.isMonitoring());

    connectAndWait(led);
    led->disconnectDevice();

    QCOMPARE(recon_spy.count(), 1);
    QCOMPARE(recon_spy.first().at(0).toString(),
             led->deviceName());

    handler.stopMonitoring();
    delete led;
  }

  void testStopMonitoring() {
    ErrorHandler handler;
    auto* led = new MockLedController();

    handler.monitorDevice(led);
    QVERIFY(handler.isMonitoring());

    handler.stopMonitoring();
    QVERIFY(!handler.isMonitoring());

    delete led;
  }

  void testMonitorNullIgnored() {
    ErrorHandler handler;
    handler.monitorDevice(nullptr);
    QVERIFY(!handler.isMonitoring());
  }

  void testReconnectSucceeded() {
    ErrorHandler handler;
    auto* led = new MockLedController();

    QSignalSpy recon_ok_spy(&handler,
                            &ErrorHandler::reconnectSucceeded);

    handler.monitorDevice(led);

    connectAndWait(led);
    led->disconnectDevice();

    // Wait for reconnect to succeed (mock will auto-connect).
    QVERIFY(QTest::qWaitFor(
        [&recon_ok_spy]() { return recon_ok_spy.count() >= 1; },
        5000));

    QCOMPARE(recon_ok_spy.first().at(0).toString(),
             led->deviceName());

    handler.stopMonitoring();
    delete led;
  }

  void testSingleAttemptPolicy() {
    ErrorHandler handler;
    RetryPolicy single{1, 100, 2.0, 1000};
    handler.setRetryPolicy(single);

    QSignalSpy success_spy(&handler, &ErrorHandler::operationSucceeded);
    QSignalSpy retry_spy(&handler, &ErrorHandler::retrying);
    QSignalSpy fail_spy(&handler, &ErrorHandler::allRetriesFailed);

    handler.execute([]() { return false; });

    // With max_attempts=1, should fail immediately.
    QCOMPARE(fail_spy.count(), 1);
    QCOMPARE(retry_spy.count(), 0);
    QCOMPARE(success_spy.count(), 0);
  }

  void testExecuteResetsState() {
    ErrorHandler handler;
    RetryPolicy fast{2, 10, 1.0, 100};
    handler.setRetryPolicy(fast);

    QSignalSpy fail_spy(&handler, &ErrorHandler::allRetriesFailed);

    handler.execute([]() { return false; });
    QVERIFY(QTest::qWaitFor(
        [&fail_spy]() { return fail_spy.count() == 1; }, 2000));

    // Now execute again — should work fresh.
    QSignalSpy success_spy(&handler, &ErrorHandler::operationSucceeded);
    handler.execute([]() { return true; });
    QCOMPARE(success_spy.count(), 1);
  }
};

QTEST_MAIN(TestErrorHandler)
#include "test_error_handler.moc"
