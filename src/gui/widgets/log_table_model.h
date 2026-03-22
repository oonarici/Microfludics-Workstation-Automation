/**
 * @file log_table_model.h
 * @brief Table model for displaying log entries in a QTableView.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Provides LogTableModel, a QAbstractTableModel that stores LogEntry
 * objects and exposes them as rows with Timestamp, Severity, Source,
 * and Message columns. Severity-based color roles are provided for
 * visual differentiation of log levels.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QAbstractTableModel>
#include <QList>

#include "core/logger.h"

namespace mwa::gui {

/**
 * @class LogTableModel
 * @brief Table model backing the log panel's QTableView.
 *
 * Stores an ordered list of LogEntry objects and presents them as a
 * four-column table (Timestamp, Severity, Source, Message). Provides
 * custom foreground and background color roles based on severity.
 *
 * @see mwa::core::LogEntry
 * @see mwa::core::LogSeverity
 */
class LogTableModel : public QAbstractTableModel {
  Q_OBJECT

 public:
  /**
   * @enum Column
   * @brief Column indices for the log table.
   */
  enum Column {
    kTimestamp = 0,  ///< Timestamp column (HH:mm:ss.zzz).
    kSeverity = 1,  ///< Severity badge column (DBG/INFO/WARN/ERR).
    kSource = 2,    ///< Source module column.
    kMessage = 3,   ///< Message text column.
    kColumnCount = 4  ///< Total number of columns.
  };

  /**
   * @brief Construct the log table model.
   *
   * @param parent Optional parent QObject.
   */
  explicit LogTableModel(QObject* parent = nullptr);

  // ---- QAbstractTableModel overrides ----

  /**
   * @brief Return the number of log entries.
   *
   * @param parent Must be an invalid index (flat table).
   * @return Number of stored log entries.
   */
  int rowCount(
      const QModelIndex& parent = QModelIndex()) const override;

  /**
   * @brief Return the number of columns (always 4).
   *
   * @param parent Must be an invalid index (flat table).
   * @return kColumnCount (4).
   */
  int columnCount(
      const QModelIndex& parent = QModelIndex()) const override;

  /**
   * @brief Return data for a given cell and role.
   *
   * Supports Qt::DisplayRole (text), Qt::ForegroundRole (severity
   * color), Qt::BackgroundRole (row tint for warnings/errors),
   * Qt::ToolTipRole (full timestamp), and Qt::UserRole (raw
   * LogSeverity enum value).
   *
   * @param index Cell index.
   * @param role  Data role.
   * @return Cell data as QVariant.
   */
  QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const override;

  /**
   * @brief Return column header text.
   *
   * @param section    Column index.
   * @param orientation Must be Qt::Horizontal.
   * @param role       Data role (Qt::DisplayRole supported).
   * @return Header label string.
   */
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;

  // ---- Public API ----

  /**
   * @brief Return the LogEntry at the given row.
   *
   * @param row Row index (must be valid).
   * @return Const reference to the LogEntry.
   */
  const mwa::core::LogEntry& entryAt(int row) const;

  /**
   * @brief Return the total number of entries (alias for rowCount).
   *
   * @return Number of stored entries.
   */
  int entryCount() const;

 public slots:
  /**
   * @brief Append a new log entry to the model.
   *
   * Emits rowsInserted() for the new row. Thread-safe when called
   * via a queued signal connection.
   *
   * @param entry The log entry to append.
   */
  void appendEntry(const mwa::core::LogEntry& entry);

  /**
   * @brief Remove all log entries from the model.
   *
   * Emits modelReset().
   */
  void clear();

 private:
  QList<mwa::core::LogEntry> entries_;  ///< Stored log entries.
};

}  // namespace mwa::gui
