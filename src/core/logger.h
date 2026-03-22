/**
 * @file logger.h
 * @brief Singleton logger wrapping Qt message handling for the MWA system.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Provides a centralized, thread-safe logging facility that intercepts
 * Qt debug/warning/critical messages and emits them as structured
 * LogEntry objects via a Qt signal for GUI consumption.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QDateTime>
#include <QMutex>
#include <QObject>
#include <QString>

namespace mwa::core {

/**
 * @enum LogSeverity
 * @brief Severity level for a log entry.
 */
enum class LogSeverity {
  kDebug,    ///< Debug-level information.
  kInfo,     ///< Informational message.
  kWarning,  ///< Warning — potential issue.
  kError     ///< Error — something failed.
};

/**
 * @struct LogEntry
 * @brief A single structured log record.
 */
struct LogEntry {
  QDateTime timestamp;   ///< When the entry was created.
  LogSeverity severity;  ///< Severity level.
  QString source;        ///< Originating component or category.
  QString message;       ///< Log message text.
};

/**
 * @class Logger
 * @brief Thread-safe singleton logger with Qt signal notification.
 *
 * Logger installs a custom Qt message handler so that qDebug(),
 * qWarning(), and qCritical() are routed through it. Each log call
 * creates a LogEntry and emits the newLogEntry() signal so that GUI
 * components (e.g. a log panel) can subscribe.
 *
 * @note All public logging methods are thread-safe.
 *
 * @see LogEntry
 * @see LogSeverity
 */
class Logger : public QObject {
  Q_OBJECT
  Q_ENUM(LogSeverity)

 public:
  /**
   * @brief Access the singleton Logger instance.
   *
   * The instance is created on first call and lives until application
   * exit. The custom Qt message handler is installed on first call.
   *
   * @return Reference to the singleton Logger.
   */
  static Logger& instance();

  /**
   * @brief Log an informational message.
   *
   * @param message The message text.
   * @param source  Optional originating component name.
   */
  void logInfo(const QString& message,
               const QString& source = QString());

  /**
   * @brief Log a warning message.
   *
   * @param message The message text.
   * @param source  Optional originating component name.
   */
  void logWarning(const QString& message,
                  const QString& source = QString());

  /**
   * @brief Log an error message.
   *
   * @param message The message text.
   * @param source  Optional originating component name.
   */
  void logError(const QString& message,
                const QString& source = QString());

  /**
   * @brief Log a debug message.
   *
   * @param message The message text.
   * @param source  Optional originating component name.
   */
  void logDebug(const QString& message,
                const QString& source = QString());

  // Non-copyable, non-movable singleton.
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;
  Logger(Logger&&) = delete;
  Logger& operator=(Logger&&) = delete;

 signals:
  /**
   * @brief Emitted for every new log entry.
   *
   * @param entry The structured log record.
   */
  void newLogEntry(const mwa::core::LogEntry& entry);

 private:
  /**
   * @brief Private constructor — use instance() instead.
   */
  Logger();

  /**
   * @brief Create a LogEntry and emit newLogEntry().
   *
   * @param severity The severity level.
   * @param message  The message text.
   * @param source   The originating component.
   */
  void log(LogSeverity severity, const QString& message,
           const QString& source);

  /**
   * @brief Custom Qt message handler installed via qInstallMessageHandler.
   *
   * @param type    The Qt message type.
   * @param context The message context (file, line, function).
   * @param msg     The formatted message string.
   */
  static void qtMessageHandler(QtMsgType type,
                                const QMessageLogContext& context,
                                const QString& msg);

  QMutex mutex_;  ///< Guards log() for thread safety.
};

}  // namespace mwa::core

Q_DECLARE_METATYPE(mwa::core::LogEntry)
