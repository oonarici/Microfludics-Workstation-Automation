/**
 * @file log_table_model.cpp
 * @brief Implementation of the LogTableModel class.
 * @author MWA Team
 * @date 2026-03-22
 *
 * @copyright LGPL-3.0-or-later
 */

#include "gui/widgets/log_table_model.h"

#include <QBrush>
#include <QColor>

namespace mwa::gui {

LogTableModel::LogTableModel(QObject* parent)
    : QAbstractTableModel(parent) {}

int LogTableModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return static_cast<int>(entries_.size());
}

int LogTableModel::columnCount(const QModelIndex& parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return kColumnCount;
}

QVariant LogTableModel::data(const QModelIndex& index,
                             int role) const {
  if (!index.isValid() || index.row() >= entries_.size()) {
    return {};
  }

  const auto& entry = entries_.at(index.row());

  if (role == Qt::DisplayRole) {
    switch (index.column()) {
      case kTimestamp:
        return entry.timestamp.toString(
            QStringLiteral("HH:mm:ss.zzz"));
      case kSeverity:
        switch (entry.severity) {
          case mwa::core::LogSeverity::kDebug:
            return QStringLiteral("DBG");
          case mwa::core::LogSeverity::kInfo:
            return QStringLiteral("INFO");
          case mwa::core::LogSeverity::kWarning:
            return QStringLiteral("WARN");
          case mwa::core::LogSeverity::kError:
            return QStringLiteral("ERR");
        }
        return {};
      case kSource:
        return entry.source;
      case kMessage:
        return entry.message;
      default:
        return {};
    }
  }

  if (role == Qt::ForegroundRole && index.column() == kSeverity) {
    switch (entry.severity) {
      case mwa::core::LogSeverity::kDebug:
        return QBrush(QColor(0x7F, 0x8C, 0x8D));
      case mwa::core::LogSeverity::kInfo:
        return QBrush(QColor(0x27, 0xAE, 0x60));
      case mwa::core::LogSeverity::kWarning:
        return QBrush(QColor(0xE6, 0x7E, 0x22));
      case mwa::core::LogSeverity::kError:
        return QBrush(QColor(0xE7, 0x4C, 0x3C));
    }
  }

  if (role == Qt::BackgroundRole) {
    switch (entry.severity) {
      case mwa::core::LogSeverity::kWarning:
        return QBrush(QColor(0xFE, 0xF9, 0xE7));
      case mwa::core::LogSeverity::kError:
        return QBrush(QColor(0xFD, 0xED, 0xEC));
      default:
        return {};
    }
  }

  if (role == Qt::ToolTipRole && index.column() == kTimestamp) {
    return entry.timestamp.toString(
        QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
  }

  if (role == Qt::UserRole) {
    return static_cast<int>(entry.severity);
  }

  return {};
}

QVariant LogTableModel::headerData(int section,
                                   Qt::Orientation orientation,
                                   int role) const {
  if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
    return {};
  }
  switch (section) {
    case kTimestamp:
      return QStringLiteral("Timestamp");
    case kSeverity:
      return QStringLiteral("Sev");
    case kSource:
      return QStringLiteral("Source");
    case kMessage:
      return QStringLiteral("Message");
    default:
      return {};
  }
}

const mwa::core::LogEntry& LogTableModel::entryAt(int row) const {
  return entries_.at(row);
}

int LogTableModel::entryCount() const {
  return static_cast<int>(entries_.size());
}

void LogTableModel::appendEntry(
    const mwa::core::LogEntry& entry) {
  int row = static_cast<int>(entries_.size());
  beginInsertRows(QModelIndex(), row, row);
  entries_.append(entry);
  endInsertRows();
}

void LogTableModel::clear() {
  if (entries_.isEmpty()) {
    return;
  }
  beginResetModel();
  entries_.clear();
  endResetModel();
}

}  // namespace mwa::gui
