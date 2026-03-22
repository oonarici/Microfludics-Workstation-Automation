/**
 * @file logger.cpp
 * @brief Implementation of the singleton Logger class.
 * @author MWA Team
 * @date 2026-03-22
 *
 * @copyright LGPL-3.0-or-later
 */

#include "logger.h"

#include <QMutexLocker>

namespace mwa::core {

Logger::Logger() {
  qRegisterMetaType<LogEntry>("mwa::core::LogEntry");
  qInstallMessageHandler(qtMessageHandler);
}

Logger& Logger::instance() {
  static Logger logger;
  return logger;
}

void Logger::logInfo(const QString& message, const QString& source) {
  log(LogSeverity::kInfo, message, source);
}

void Logger::logWarning(const QString& message,
                        const QString& source) {
  log(LogSeverity::kWarning, message, source);
}

void Logger::logError(const QString& message,
                      const QString& source) {
  log(LogSeverity::kError, message, source);
}

void Logger::logDebug(const QString& message,
                      const QString& source) {
  log(LogSeverity::kDebug, message, source);
}

void Logger::log(LogSeverity severity, const QString& message,
                 const QString& source) {
  QMutexLocker locker(&mutex_);
  LogEntry entry{QDateTime::currentDateTime(), severity, source,
                 message};
  emit newLogEntry(entry);
}

void Logger::qtMessageHandler(QtMsgType type,
                              const QMessageLogContext& context,
                              const QString& msg) {
  LogSeverity severity = LogSeverity::kDebug;
  switch (type) {
    case QtDebugMsg:
      severity = LogSeverity::kDebug;
      break;
    case QtInfoMsg:
      severity = LogSeverity::kInfo;
      break;
    case QtWarningMsg:
      severity = LogSeverity::kWarning;
      break;
    case QtCriticalMsg:
    case QtFatalMsg:
      severity = LogSeverity::kError;
      break;
  }

  QString source;
  if (context.category != nullptr) {
    source = QString::fromUtf8(context.category);
  }

  instance().log(severity, msg, source);
}

}  // namespace mwa::core
