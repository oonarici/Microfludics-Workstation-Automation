/**
 * @file test_logger.cpp
 * @brief Adversarial tests for mwa::core::Logger.
 * @date 2026-03-22
 * @copyright LGPL-3.0-or-later
 */

#include <QDateTime>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QSignalSpy>
#include <QString>
#include <QThread>
#include <QtTest>

#include "core/logger.h"

using mwa::core::LogEntry;
using mwa::core::Logger;
using mwa::core::LogSeverity;

// ---------------------------------------------------------------------------
// Helper: captures every newLogEntry emission into a list.
// ---------------------------------------------------------------------------
class LogCapture : public QObject {
  Q_OBJECT
 public:
  explicit LogCapture(QObject* parent = nullptr) : QObject(parent) {
    connect(&Logger::instance(), &Logger::newLogEntry, this,
            &LogCapture::onEntry, Qt::DirectConnection);
  }

  void clear() {
    QMutexLocker lock(&mutex_);
    entries_.clear();
  }

  QList<LogEntry> entries() {
    QMutexLocker lock(&mutex_);
    return entries_;
  }

 private slots:
  void onEntry(const mwa::core::LogEntry& e) {
    QMutexLocker lock(&mutex_);
    entries_.append(e);
  }

 private:
  QMutex mutex_;
  QList<LogEntry> entries_;
};

// ---------------------------------------------------------------------------
// Test class
// ---------------------------------------------------------------------------
class TestLogger : public QObject {
  Q_OBJECT

 private slots:
  void initTestCase();
  void cleanup();

  void test_instance_returnsSameObject();
  void test_logInfo_emitsSeverityInfo();
  void test_logWarning_emitsSeverityWarning();
  void test_logError_emitsSeverityError();
  void test_logDebug_emitsSeverityDebug();
  void test_logEntry_carriesCorrectMessage();
  void test_logEntry_carriesCorrectSource();
  void test_logEntry_emptySourceIsPreserved();
  void test_logEntry_timestampIsRecent();
  void test_qtMessageHandler_qDebugRoutedAsDebug();
  void test_qtMessageHandler_qWarningRoutedAsWarning();
  void test_qtMessageHandler_qCriticalRoutedAsError();
  void test_threadSafety_concurrentLogsAllDelivered();

 private:
  LogCapture* capture_{nullptr};
};

// ---------------------------------------------------------------------------
void TestLogger::initTestCase() {
  // Touch the singleton once so the message handler is installed.
  Logger::instance();
  qRegisterMetaType<LogEntry>("mwa::core::LogEntry");
}

void TestLogger::cleanup() {
  // Reset capture between tests.
  if (capture_) {
    capture_->clear();
  }
}

// ---------------------------------------------------------------------------
void TestLogger::test_instance_returnsSameObject() {
  Logger* a = &Logger::instance();
  Logger* b = &Logger::instance();
  QVERIFY2(a == b, "instance() must return the same address on every call");
}

// ---------------------------------------------------------------------------
void TestLogger::test_logInfo_emitsSeverityInfo() {
  QSignalSpy spy(&Logger::instance(), &Logger::newLogEntry);
  Logger::instance().logInfo(QStringLiteral("info msg"));
  QCOMPARE(spy.count(), 1);
  auto entry = qvariant_cast<LogEntry>(spy.at(0).at(0));
  QCOMPARE(entry.severity, LogSeverity::kInfo);
}

// ---------------------------------------------------------------------------
void TestLogger::test_logWarning_emitsSeverityWarning() {
  QSignalSpy spy(&Logger::instance(), &Logger::newLogEntry);
  Logger::instance().logWarning(QStringLiteral("warn msg"));
  QCOMPARE(spy.count(), 1);
  auto entry = qvariant_cast<LogEntry>(spy.at(0).at(0));
  QCOMPARE(entry.severity, LogSeverity::kWarning);
}

// ---------------------------------------------------------------------------
void TestLogger::test_logError_emitsSeverityError() {
  QSignalSpy spy(&Logger::instance(), &Logger::newLogEntry);
  Logger::instance().logError(QStringLiteral("err msg"));
  QCOMPARE(spy.count(), 1);
  auto entry = qvariant_cast<LogEntry>(spy.at(0).at(0));
  QCOMPARE(entry.severity, LogSeverity::kError);
}

// ---------------------------------------------------------------------------
void TestLogger::test_logDebug_emitsSeverityDebug() {
  QSignalSpy spy(&Logger::instance(), &Logger::newLogEntry);
  Logger::instance().logDebug(QStringLiteral("dbg msg"));
  QCOMPARE(spy.count(), 1);
  auto entry = qvariant_cast<LogEntry>(spy.at(0).at(0));
  QCOMPARE(entry.severity, LogSeverity::kDebug);
}

// ---------------------------------------------------------------------------
void TestLogger::test_logEntry_carriesCorrectMessage() {
  QSignalSpy spy(&Logger::instance(), &Logger::newLogEntry);
  const QString expected = QStringLiteral("Hello MWA");
  Logger::instance().logInfo(expected, QStringLiteral("src"));
  QCOMPARE(spy.count(), 1);
  auto entry = qvariant_cast<LogEntry>(spy.at(0).at(0));
  QCOMPARE(entry.message, expected);
}

// ---------------------------------------------------------------------------
void TestLogger::test_logEntry_carriesCorrectSource() {
  QSignalSpy spy(&Logger::instance(), &Logger::newLogEntry);
  const QString src = QStringLiteral("LedController");
  Logger::instance().logInfo(QStringLiteral("msg"), src);
  QCOMPARE(spy.count(), 1);
  auto entry = qvariant_cast<LogEntry>(spy.at(0).at(0));
  QCOMPARE(entry.source, src);
}

// ---------------------------------------------------------------------------
void TestLogger::test_logEntry_emptySourceIsPreserved() {
  QSignalSpy spy(&Logger::instance(), &Logger::newLogEntry);
  // Call with default (empty) source.
  Logger::instance().logInfo(QStringLiteral("no source"));
  QCOMPARE(spy.count(), 1);
  auto entry = qvariant_cast<LogEntry>(spy.at(0).at(0));
  QVERIFY2(entry.source.isEmpty(),
           "source must be empty when none is provided");
}

// ---------------------------------------------------------------------------
void TestLogger::test_logEntry_timestampIsRecent() {
  const QDateTime before = QDateTime::currentDateTime();
  QSignalSpy spy(&Logger::instance(), &Logger::newLogEntry);
  Logger::instance().logInfo(QStringLiteral("ts test"));
  const QDateTime after = QDateTime::currentDateTime();
  QCOMPARE(spy.count(), 1);
  auto entry = qvariant_cast<LogEntry>(spy.at(0).at(0));
  QVERIFY2(entry.timestamp >= before && entry.timestamp <= after,
           "LogEntry timestamp must fall between before and after the call");
}

// ---------------------------------------------------------------------------
void TestLogger::test_qtMessageHandler_qDebugRoutedAsDebug() {
  QSignalSpy spy(&Logger::instance(), &Logger::newLogEntry);
  qDebug("qt-debug-route");
  // qDebug may emit from the default category; we only care about severity.
  QVERIFY2(spy.count() >= 1, "qDebug must reach the Logger");
  auto entry = qvariant_cast<LogEntry>(spy.last().at(0));
  QCOMPARE(entry.severity, LogSeverity::kDebug);
}

// ---------------------------------------------------------------------------
void TestLogger::test_qtMessageHandler_qWarningRoutedAsWarning() {
  QSignalSpy spy(&Logger::instance(), &Logger::newLogEntry);
  qWarning("qt-warning-route");
  QVERIFY2(spy.count() >= 1, "qWarning must reach the Logger");
  auto entry = qvariant_cast<LogEntry>(spy.last().at(0));
  QCOMPARE(entry.severity, LogSeverity::kWarning);
}

// ---------------------------------------------------------------------------
void TestLogger::test_qtMessageHandler_qCriticalRoutedAsError() {
  QSignalSpy spy(&Logger::instance(), &Logger::newLogEntry);
  qCritical("qt-critical-route");
  QVERIFY2(spy.count() >= 1, "qCritical must reach the Logger");
  auto entry = qvariant_cast<LogEntry>(spy.last().at(0));
  QCOMPARE(entry.severity, LogSeverity::kError);
}

// ---------------------------------------------------------------------------
// Thread-safety: spin up several threads, each logs kIterations messages,
// and verify all are delivered without crash or message loss.
// ---------------------------------------------------------------------------
void TestLogger::test_threadSafety_concurrentLogsAllDelivered() {
  constexpr int kThreads = 8;
  constexpr int kIterations = 50;
  constexpr int kExpected = kThreads * kIterations;

  capture_ = new LogCapture(this);
  capture_->clear();

  QList<QThread*> threads;
  for (int t = 0; t < kThreads; ++t) {
    auto* thread = QThread::create([t]() {
      for (int i = 0; i < kIterations; ++i) {
        Logger::instance().logInfo(
            QStringLiteral("thread %1 msg %2").arg(t).arg(i),
            QStringLiteral("ThreadTest"));
      }
    });
    threads.append(thread);
  }

  for (auto* thread : threads) thread->start();
  for (auto* thread : threads) thread->wait(5000);
  for (auto* thread : threads) delete thread;

  // Process any pending queued signals.
  QCoreApplication::processEvents();

  int count = capture_->entries().size();
  QVERIFY2(count == kExpected,
           qPrintable(
               QStringLiteral("Expected %1 log entries, got %2")
                   .arg(kExpected)
                   .arg(count)));

  delete capture_;
  capture_ = nullptr;
}

// ---------------------------------------------------------------------------
QTEST_MAIN(TestLogger)
#include "test_logger.moc"
