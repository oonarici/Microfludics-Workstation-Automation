/**
 * @file test_command_queue.cpp
 * @brief Adversarial tests for mwa::hardware::CommandQueue.
 * @date 2026-03-24
 * @copyright LGPL-3.0-or-later
 */

#include <QAtomicInt>
#include <QElapsedTimer>
#include <QMutex>
#include <QObject>
#include <QSignalSpy>
#include <QString>
#include <QThread>
#include <QVector>
#include <QtTest>

#include <stdexcept>

#include "hardware/command_queue.h"

using mwa::hardware::CommandQueue;

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------
class TestCommandQueue : public QObject {
  Q_OBJECT

 private slots:
  void test_construction_startsWorkerThread();
  void test_enqueue_executesCommand();
  void test_enqueue_executesOnWorkerThread();
  void test_enqueue_fifoOrdering();
  void test_enqueue_nullCommand_returnsFalse();
  void test_enqueue_afterShutdown_returnsFalse();
  void test_commandStarted_signal_emitted();
  void test_commandFinished_signal_emitted();
  void test_commandFailed_onStdException();
  void test_commandFailed_onUnknownException();
  void test_commandTimedOut_signalEmitted();
  void test_commandTimedOut_notEmittedWhenFast();
  void test_clear_removesPendingCommands();
  void test_shutdown_basic();
  void test_shutdown_idempotent();
  void test_shutdown_waitsForRunningCommand();
  void test_isShutdown_falseBeforeShutdown();
  void test_isShutdown_trueAfterShutdown();
  void test_pendingCount_reflectsQueueSize();
  void test_parentOwnership();
  void test_multipleCommands_allExecute();
  void test_concurrentEnqueue_fromMultipleThreads();
};

// ---------------------------------------------------------------------------
void TestCommandQueue::test_construction_startsWorkerThread() {
  CommandQueue queue;
  // Enqueue a command that records the thread it runs on.
  QThread* command_thread = nullptr;
  QSignalSpy finished_spy(&queue, &CommandQueue::commandFinished);
  queue.enqueue([&command_thread]() {
    command_thread = QThread::currentThread();
  });
  QVERIFY(finished_spy.wait(2000));
  QVERIFY2(command_thread != nullptr,
           "Command must have executed");
  QVERIFY2(command_thread != QThread::currentThread(),
           "Worker thread must differ from test thread");
}

// ---------------------------------------------------------------------------
void TestCommandQueue::test_enqueue_executesCommand() {
  CommandQueue queue;
  QAtomicInt executed{0};
  QSignalSpy finished_spy(&queue, &CommandQueue::commandFinished);
  queue.enqueue([&executed]() {
    executed.storeRelaxed(1);
  });
  QVERIFY(finished_spy.wait(2000));
  QCOMPARE(executed.loadRelaxed(), 1);
}

// ---------------------------------------------------------------------------
void TestCommandQueue::test_enqueue_executesOnWorkerThread() {
  CommandQueue queue;
  QThread* captured = nullptr;
  QSignalSpy finished_spy(&queue, &CommandQueue::commandFinished);
  queue.enqueue([&captured]() {
    captured = QThread::currentThread();
  });
  QVERIFY(finished_spy.wait(2000));
  QVERIFY(captured != nullptr);
  QVERIFY2(captured != QThread::currentThread(),
           "Commands must execute on the worker thread, not the GUI thread");
}

// ---------------------------------------------------------------------------
void TestCommandQueue::test_enqueue_fifoOrdering() {
  CommandQueue queue;
  QMutex result_mutex;
  QVector<int> order;

  // Enqueue a blocking command first to hold the worker, then batch more.
  QSignalSpy finished_spy(&queue, &CommandQueue::commandFinished);
  QAtomicInt gate{0};

  // Gate command: holds the worker while we enqueue the rest.
  queue.enqueue([&gate]() {
    while (gate.loadRelaxed() == 0) {
      QThread::msleep(5);
    }
  });

  // Enqueue 5 commands while the gate is held.
  for (int i = 0; i < 5; ++i) {
    queue.enqueue([i, &order, &result_mutex]() {
      QMutexLocker lock(&result_mutex);
      order.append(i);
    });
  }

  // Release the gate.
  gate.storeRelaxed(1);

  // Wait for all 6 commands (gate + 5).
  QTRY_COMPARE_WITH_TIMEOUT(finished_spy.count(), 6, 5000);

  QMutexLocker lock(&result_mutex);
  QCOMPARE(order.size(), 5);
  for (int i = 0; i < 5; ++i) {
    QCOMPARE(order[i], i);
  }
}

// ---------------------------------------------------------------------------
void TestCommandQueue::test_enqueue_nullCommand_returnsFalse() {
  CommandQueue queue;
  bool result = queue.enqueue(nullptr);
  QVERIFY2(!result, "enqueue(nullptr) must return false");
}

// ---------------------------------------------------------------------------
void TestCommandQueue::test_enqueue_afterShutdown_returnsFalse() {
  CommandQueue queue;
  queue.shutdown();
  bool result = queue.enqueue([]() {});
  QVERIFY2(!result, "enqueue after shutdown must return false");
}

// ---------------------------------------------------------------------------
void TestCommandQueue::test_commandStarted_signal_emitted() {
  CommandQueue queue;
  QSignalSpy started_spy(&queue, &CommandQueue::commandStarted);
  QSignalSpy finished_spy(&queue, &CommandQueue::commandFinished);
  queue.enqueue([]() {});
  QVERIFY(finished_spy.wait(2000));
  QCOMPARE(started_spy.count(), 1);
}

// ---------------------------------------------------------------------------
void TestCommandQueue::test_commandFinished_signal_emitted() {
  CommandQueue queue;
  QSignalSpy finished_spy(&queue, &CommandQueue::commandFinished);
  queue.enqueue([]() {});
  QVERIFY(finished_spy.wait(2000));
  QCOMPARE(finished_spy.count(), 1);
}

// ---------------------------------------------------------------------------
void TestCommandQueue::test_commandFailed_onStdException() {
  CommandQueue queue;
  QSignalSpy failed_spy(&queue, &CommandQueue::commandFailed);
  queue.enqueue([]() {
    throw std::runtime_error("test error");
  });
  QVERIFY(failed_spy.wait(2000));
  QCOMPARE(failed_spy.count(), 1);
  QCOMPARE(failed_spy.at(0).at(0).toString(),
           QStringLiteral("test error"));
}

// ---------------------------------------------------------------------------
void TestCommandQueue::test_commandFailed_onUnknownException() {
  CommandQueue queue;
  QSignalSpy failed_spy(&queue, &CommandQueue::commandFailed);
  queue.enqueue([]() {
    throw 42;  // Non-std::exception throw.
  });
  QVERIFY(failed_spy.wait(2000));
  QCOMPARE(failed_spy.count(), 1);
  QCOMPARE(failed_spy.at(0).at(0).toString(),
           QStringLiteral("Unknown error"));
}

// ---------------------------------------------------------------------------
void TestCommandQueue::test_commandTimedOut_signalEmitted() {
  CommandQueue queue;
  QSignalSpy timeout_spy(&queue, &CommandQueue::commandTimedOut);
  QSignalSpy finished_spy(&queue, &CommandQueue::commandFinished);

  // Command sleeps longer than the timeout.
  queue.enqueue([]() {
    QThread::msleep(500);
  }, 50);  // 50ms timeout, command takes 500ms.

  QVERIFY(finished_spy.wait(3000));
  // The timeout signal should have fired.
  QVERIFY2(timeout_spy.count() >= 1,
           "commandTimedOut must fire for a slow command");
  QCOMPARE(timeout_spy.at(0).at(0).toInt(), 50);
}

// ---------------------------------------------------------------------------
void TestCommandQueue::test_commandTimedOut_notEmittedWhenFast() {
  CommandQueue queue;
  QSignalSpy timeout_spy(&queue, &CommandQueue::commandTimedOut);
  QSignalSpy finished_spy(&queue, &CommandQueue::commandFinished);

  queue.enqueue([]() {
    // Instant command.
  }, 5000);  // 5 second timeout — should never fire.

  QVERIFY(finished_spy.wait(2000));
  // Give a brief moment for any late signals.
  QTest::qWait(100);
  QCOMPARE(timeout_spy.count(), 0);
}

// ---------------------------------------------------------------------------
void TestCommandQueue::test_clear_removesPendingCommands() {
  CommandQueue queue;
  QAtomicInt gate{0};
  QAtomicInt count{0};
  QSignalSpy started_spy(&queue, &CommandQueue::commandStarted);
  QSignalSpy finished_spy(&queue, &CommandQueue::commandFinished);

  // Hold the worker with a gate command.
  queue.enqueue([&gate]() {
    while (gate.loadRelaxed() == 0) {
      QThread::msleep(5);
    }
  });

  // Wait for the gate command to actually start executing before
  // enqueuing more work — avoids a race where clear() removes the
  // gate command itself.
  QVERIFY(started_spy.wait(2000));

  // Enqueue 3 more commands that increment the counter.
  for (int i = 0; i < 3; ++i) {
    queue.enqueue([&count]() {
      count.fetchAndAddRelaxed(1);
    });
  }

  // Clear the pending commands (gate command is already running).
  queue.clear();

  // Release the gate.
  gate.storeRelaxed(1);

  // Wait for the gate command to finish.
  QVERIFY(finished_spy.wait(2000));

  // Give time for any mistakenly remaining commands.
  QTest::qWait(200);

  QCOMPARE(count.loadRelaxed(), 0);
}

// ---------------------------------------------------------------------------
void TestCommandQueue::test_shutdown_basic() {
  CommandQueue queue;
  queue.shutdown();
  QVERIFY(queue.isShutdown());
}

// ---------------------------------------------------------------------------
void TestCommandQueue::test_shutdown_idempotent() {
  CommandQueue queue;
  queue.shutdown();
  queue.shutdown();  // Must not crash or deadlock.
  queue.shutdown();
  QVERIFY(queue.isShutdown());
}

// ---------------------------------------------------------------------------
void TestCommandQueue::test_shutdown_waitsForRunningCommand() {
  CommandQueue queue;
  QAtomicInt completed{0};
  QSignalSpy started_spy(&queue, &CommandQueue::commandStarted);

  queue.enqueue([&completed]() {
    QThread::msleep(200);
    completed.storeRelaxed(1);
  });

  // Wait until the command starts.
  QVERIFY(started_spy.wait(2000));

  // Shutdown should block until the command finishes.
  queue.shutdown();
  QCOMPARE(completed.loadRelaxed(), 1);
}

// ---------------------------------------------------------------------------
void TestCommandQueue::test_isShutdown_falseBeforeShutdown() {
  CommandQueue queue;
  QVERIFY2(!queue.isShutdown(), "isShutdown must be false initially");
}

// ---------------------------------------------------------------------------
void TestCommandQueue::test_isShutdown_trueAfterShutdown() {
  CommandQueue queue;
  queue.shutdown();
  QVERIFY2(queue.isShutdown(), "isShutdown must be true after shutdown");
}

// ---------------------------------------------------------------------------
void TestCommandQueue::test_pendingCount_reflectsQueueSize() {
  CommandQueue queue;
  QAtomicInt gate{0};

  // Hold the worker.
  queue.enqueue([&gate]() {
    while (gate.loadRelaxed() == 0) {
      QThread::msleep(5);
    }
  });

  // Let worker pick up the gate command.
  QTest::qWait(50);

  queue.enqueue([]() {});
  queue.enqueue([]() {});
  queue.enqueue([]() {});

  QCOMPARE(queue.pendingCount(), 3);

  // Release the gate and let everything drain.
  gate.storeRelaxed(1);
  QSignalSpy finished_spy(&queue, &CommandQueue::commandFinished);
  QTRY_COMPARE_WITH_TIMEOUT(finished_spy.count(), 4, 5000);
  QCOMPARE(queue.pendingCount(), 0);
}

// ---------------------------------------------------------------------------
void TestCommandQueue::test_parentOwnership() {
  auto* parent = new QObject();
  auto* queue = new CommandQueue(parent);
  QVERIFY(queue->parent() == parent);
  delete parent;  // Must not crash.
  QVERIFY(true);
}

// ---------------------------------------------------------------------------
void TestCommandQueue::test_multipleCommands_allExecute() {
  CommandQueue queue;
  constexpr int kCount = 20;
  QAtomicInt counter{0};
  QSignalSpy finished_spy(&queue, &CommandQueue::commandFinished);

  for (int i = 0; i < kCount; ++i) {
    queue.enqueue([&counter]() {
      counter.fetchAndAddRelaxed(1);
    });
  }

  QTRY_COMPARE_WITH_TIMEOUT(finished_spy.count(), kCount, 10000);
  QCOMPARE(counter.loadRelaxed(), kCount);
}

// ---------------------------------------------------------------------------
void TestCommandQueue::test_concurrentEnqueue_fromMultipleThreads() {
  CommandQueue queue;
  constexpr int kThreads = 4;
  constexpr int kPerThread = 25;
  QAtomicInt counter{0};

  QVector<QThread*> threads;
  for (int t = 0; t < kThreads; ++t) {
    auto* thread = QThread::create([&queue, &counter]() {
      for (int i = 0; i < kPerThread; ++i) {
        queue.enqueue([&counter]() {
          counter.fetchAndAddRelaxed(1);
        });
      }
    });
    threads.append(thread);
    thread->start();
  }

  // Wait for enqueue threads to finish.
  for (auto* thread : threads) {
    thread->wait();
    delete thread;
  }

  // Wait for all commands to execute.
  QSignalSpy finished_spy(&queue, &CommandQueue::commandFinished);
  QTRY_VERIFY_WITH_TIMEOUT(
      counter.loadRelaxed() == kThreads * kPerThread, 15000);
  QCOMPARE(counter.loadRelaxed(), kThreads * kPerThread);
}

// ---------------------------------------------------------------------------
QTEST_MAIN(TestCommandQueue)
#include "test_command_queue.moc"
