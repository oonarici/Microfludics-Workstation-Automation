/**
 * @file log_panel.h
 * @brief Log panel widget for displaying real-time log entries.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Declares the LogPanel widget that replaces the bottom dock
 * placeholder in MainWindow. It displays log entries from the
 * Logger singleton in a filterable, scrollable table view.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QWidget>

#include "core/logger.h"
#include "gui/widgets/log_table_model.h"

namespace mwa::gui {

class LogFilterProxy;

/**
 * @class LogPanel
 * @brief Widget displaying log entries with severity and text filtering.
 *
 * LogPanel connects to Logger::newLogEntry and appends each entry to
 * a LogTableModel. A QSortFilterProxyModel provides severity-based
 * and text-based filtering. The panel supports auto-scroll, clear,
 * and clipboard copy on double-click.
 *
 * @see LogTableModel
 * @see mwa::core::Logger
 */
class LogPanel : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Construct the log panel.
   *
   * Creates the toolbar (severity filters, text filter, clear
   * button), the table view backed by LogTableModel, and the status
   * row with entry count and auto-scroll checkbox.
   *
   * @param parent Optional parent widget.
   */
  explicit LogPanel(QWidget* parent = nullptr);

 private slots:
  /**
   * @brief Append a new log entry from the Logger.
   *
   * @param entry The log entry to display.
   */
  void onNewLogEntry(const mwa::core::LogEntry& entry);

  /**
   * @brief Handle severity preset combo box change.
   *
   * @param index The selected preset index.
   */
  void onSeverityPresetChanged(int index);

  /**
   * @brief Reapply severity filter when any checkbox toggles.
   */
  void onSeverityFilterChanged();

  /**
   * @brief Handle text filter input change.
   *
   * @param text The current filter text.
   */
  void onTextFilterChanged(const QString& text);

  /**
   * @brief Clear all log entries.
   */
  void onClearLog();

  /**
   * @brief Handle auto-scroll checkbox toggle.
   *
   * @param enabled True to enable auto-scroll.
   */
  void onAutoScrollToggled(bool enabled);

  /**
   * @brief Copy a double-clicked row to the clipboard.
   *
   * @param index The clicked model index.
   */
  void onRowDoubleClicked(const QModelIndex& index);

 private:
  /**
   * @brief Build the toolbar row with filter controls.
   */
  void createToolbar();

  /**
   * @brief Build the table view and model.
   */
  void createTableView();

  /**
   * @brief Build the status row with entry count and auto-scroll.
   */
  void createStatusRow();

  /**
   * @brief Update the entry count and filter status labels.
   */
  void updateStatusLabels();

  // Toolbar widgets
  QComboBox* cmb_severity_preset_{nullptr};
  QCheckBox* chk_debug_{nullptr};
  QCheckBox* chk_info_{nullptr};
  QCheckBox* chk_warning_{nullptr};
  QCheckBox* chk_error_{nullptr};
  QLineEdit* le_filter_text_{nullptr};
  QPushButton* btn_clear_log_{nullptr};

  // Table
  QTableView* tbl_log_view_{nullptr};
  LogTableModel* log_model_{nullptr};
  LogFilterProxy* log_proxy_{nullptr};

  // Status row
  QLabel* lbl_entry_count_{nullptr};
  QLabel* lbl_filter_status_{nullptr};
  QCheckBox* chk_auto_scroll_{nullptr};

  bool updating_preset_{false};  ///< Guard for preset/checkbox sync.
};

/**
 * @class LogFilterProxy
 * @brief Proxy model filtering log entries by severity and text.
 *
 * Filters rows based on a severity bitmask (which log levels to
 * show) and a case-insensitive text match on the Source and Message
 * columns.
 *
 * @see LogTableModel
 */
class LogFilterProxy : public QSortFilterProxyModel {
  Q_OBJECT

 public:
  /**
   * @brief Construct the filter proxy.
   *
   * @param parent Optional parent QObject.
   */
  explicit LogFilterProxy(QObject* parent = nullptr);

  /**
   * @brief Set which severity levels are visible.
   *
   * @param debug   Show debug entries.
   * @param info    Show info entries.
   * @param warning Show warning entries.
   * @param error   Show error entries.
   */
  void setSeverityFilter(bool debug, bool info,
                         bool warning, bool error);

  /**
   * @brief Set the text filter string.
   *
   * @param text Case-insensitive text to match against Source
   *             and Message columns. Empty string matches all.
   */
  void setTextFilter(const QString& text);

 protected:
  /**
   * @brief Return true if the row passes all active filters.
   *
   * @param source_row   Row index in the source model.
   * @param source_parent Parent index (unused, flat model).
   * @return True if the row should be shown.
   */
  bool filterAcceptsRow(
      int source_row,
      const QModelIndex& source_parent) const override;

 private:
  bool show_debug_{true};    ///< Show kDebug entries.
  bool show_info_{true};     ///< Show kInfo entries.
  bool show_warning_{true};  ///< Show kWarning entries.
  bool show_error_{true};    ///< Show kError entries.
  QString text_filter_;      ///< Text filter string.
};

}  // namespace mwa::gui
