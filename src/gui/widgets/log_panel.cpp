/**
 * @file log_panel.cpp
 * @brief Implementation of the LogPanel and LogFilterProxy classes.
 * @author MWA Team
 * @date 2026-03-22
 *
 * @copyright LGPL-3.0-or-later
 */

#include "gui/widgets/log_panel.h"

#include <QApplication>
#include <QClipboard>
#include <QFontDatabase>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QScrollBar>
#include <QShortcut>
#include <QVBoxLayout>

namespace mwa::gui {

// ---- LogFilterProxy -------------------------------------------------------

LogFilterProxy::LogFilterProxy(QObject* parent)
    : QSortFilterProxyModel(parent) {}

void LogFilterProxy::setSeverityFilter(bool debug, bool info,
                                       bool warning, bool error) {
  show_debug_ = debug;
  show_info_ = info;
  show_warning_ = warning;
  show_error_ = error;
  QT_WARNING_PUSH
  QT_WARNING_DISABLE_DEPRECATED
  invalidateFilter();
  QT_WARNING_POP
}

void LogFilterProxy::setTextFilter(const QString& text) {
  text_filter_ = text;
  QT_WARNING_PUSH
  QT_WARNING_DISABLE_DEPRECATED
  invalidateFilter();
  QT_WARNING_POP
}

bool LogFilterProxy::filterAcceptsRow(
    int source_row,
    const QModelIndex& source_parent) const {
  auto* model = sourceModel();
  if (model == nullptr) {
    return true;
  }

  // Check severity filter.
  QModelIndex sev_index =
      model->index(source_row, LogTableModel::kSeverity,
                   source_parent);
  int severity = model->data(sev_index, Qt::UserRole).toInt();
  auto sev = static_cast<mwa::core::LogSeverity>(severity);

  switch (sev) {
    case mwa::core::LogSeverity::kDebug:
      if (!show_debug_) return false;
      break;
    case mwa::core::LogSeverity::kInfo:
      if (!show_info_) return false;
      break;
    case mwa::core::LogSeverity::kWarning:
      if (!show_warning_) return false;
      break;
    case mwa::core::LogSeverity::kError:
      if (!show_error_) return false;
      break;
  }

  // Check text filter.
  if (!text_filter_.isEmpty()) {
    QModelIndex src_index =
        model->index(source_row, LogTableModel::kSource,
                     source_parent);
    QModelIndex msg_index =
        model->index(source_row, LogTableModel::kMessage,
                     source_parent);
    QString source = model->data(src_index).toString();
    QString message = model->data(msg_index).toString();
    if (!source.contains(text_filter_, Qt::CaseInsensitive) &&
        !message.contains(text_filter_, Qt::CaseInsensitive)) {
      return false;
    }
  }

  return true;
}

// ---- LogPanel -------------------------------------------------------------

LogPanel::LogPanel(QWidget* parent) : QWidget(parent) {
  setObjectName(QStringLiteral("logPanel"));

  auto* root_layout = new QVBoxLayout(this);
  root_layout->setContentsMargins(8, 8, 8, 8);
  root_layout->setSpacing(4);

  createToolbar();
  createTableView();
  createStatusRow();

  // Connect to Logger.
  connect(&mwa::core::Logger::instance(),
          &mwa::core::Logger::newLogEntry,
          this, &LogPanel::onNewLogEntry,
          Qt::QueuedConnection);

  // Keyboard shortcuts.
  auto* shortcut_focus_filter = new QShortcut(
      QKeySequence(QStringLiteral("Ctrl+F")), this);
  shortcut_focus_filter->setContext(
      Qt::WidgetWithChildrenShortcut);
  connect(shortcut_focus_filter, &QShortcut::activated,
          this, [this]() {
            le_filter_text_->setFocus();
            le_filter_text_->selectAll();
          });

  auto* shortcut_clear = new QShortcut(
      QKeySequence(QStringLiteral("Ctrl+K")), this);
  shortcut_clear->setContext(Qt::WidgetWithChildrenShortcut);
  connect(shortcut_clear, &QShortcut::activated,
          this, &LogPanel::onClearLog);

  updateStatusLabels();
}

void LogPanel::createToolbar() {
  auto* toolbar = new QWidget(this);
  toolbar->setObjectName(QStringLiteral("wgtLogToolbar"));
  toolbar->setFixedHeight(36);

  auto* toolbar_layout = new QHBoxLayout(toolbar);
  toolbar_layout->setContentsMargins(0, 0, 0, 0);
  toolbar_layout->setSpacing(4);

  auto* lbl_show = new QLabel(QStringLiteral("Show:"), toolbar);
  lbl_show->setObjectName(QStringLiteral("lblShowFilter"));
  lbl_show->setFixedWidth(38);
  toolbar_layout->addWidget(lbl_show);

  cmb_severity_preset_ = new QComboBox(toolbar);
  cmb_severity_preset_->setObjectName(
      QStringLiteral("cmbSeverityPreset"));
  cmb_severity_preset_->addItems(
      {QStringLiteral("All"),
       QStringLiteral("Info+"),
       QStringLiteral("Warnings+"),
       QStringLiteral("Errors only")});
  cmb_severity_preset_->setMinimumWidth(120);
  cmb_severity_preset_->setMinimumHeight(32);
  cmb_severity_preset_->setToolTip(
      QStringLiteral("Quick severity filter"));
  toolbar_layout->addWidget(cmb_severity_preset_);

  auto* sep1 = new QFrame(toolbar);
  sep1->setObjectName(QStringLiteral("frmSep1"));
  sep1->setFrameShape(QFrame::VLine);
  sep1->setFixedWidth(1);
  toolbar_layout->addWidget(sep1);

  chk_debug_ = new QCheckBox(QStringLiteral("Debug"), toolbar);
  chk_debug_->setObjectName(QStringLiteral("chkDebug"));
  chk_debug_->setChecked(true);
  chk_debug_->setMinimumHeight(32);
  toolbar_layout->addWidget(chk_debug_);

  chk_info_ = new QCheckBox(QStringLiteral("Info"), toolbar);
  chk_info_->setObjectName(QStringLiteral("chkInfo"));
  chk_info_->setChecked(true);
  chk_info_->setMinimumHeight(32);
  toolbar_layout->addWidget(chk_info_);

  chk_warning_ =
      new QCheckBox(QStringLiteral("Warning"), toolbar);
  chk_warning_->setObjectName(QStringLiteral("chkWarning"));
  chk_warning_->setChecked(true);
  chk_warning_->setMinimumHeight(32);
  toolbar_layout->addWidget(chk_warning_);

  chk_error_ = new QCheckBox(QStringLiteral("Error"), toolbar);
  chk_error_->setObjectName(QStringLiteral("chkError"));
  chk_error_->setChecked(true);
  chk_error_->setMinimumHeight(32);
  toolbar_layout->addWidget(chk_error_);

  auto* sep2 = new QFrame(toolbar);
  sep2->setObjectName(QStringLiteral("frmSep2"));
  sep2->setFrameShape(QFrame::VLine);
  sep2->setFixedWidth(1);
  toolbar_layout->addWidget(sep2);

  auto* lbl_filter =
      new QLabel(QStringLiteral("Filter:"), toolbar);
  lbl_filter->setObjectName(QStringLiteral("lblSearchIcon"));
  lbl_filter->setFixedWidth(42);
  toolbar_layout->addWidget(lbl_filter);

  le_filter_text_ = new QLineEdit(toolbar);
  le_filter_text_->setObjectName(QStringLiteral("leFilterText"));
  le_filter_text_->setPlaceholderText(
      QStringLiteral("Filter messages..."));
  le_filter_text_->setClearButtonEnabled(true);
  le_filter_text_->setMinimumWidth(140);
  le_filter_text_->setMinimumHeight(32);
  le_filter_text_->setToolTip(
      QStringLiteral("Filter by Source or Message (Ctrl+F)"));
  toolbar_layout->addWidget(le_filter_text_);

  toolbar_layout->addStretch();

  btn_clear_log_ = new QPushButton(QStringLiteral("Clear"),
                                   toolbar);
  btn_clear_log_->setObjectName(QStringLiteral("btnClearLog"));
  btn_clear_log_->setMinimumSize(64, 32);
  btn_clear_log_->setToolTip(
      QStringLiteral("Clear all log entries (Ctrl+K)"));
  btn_clear_log_->setEnabled(false);
  toolbar_layout->addWidget(btn_clear_log_);

  layout()->addWidget(toolbar);

  // Connections.
  connect(cmb_severity_preset_,
          qOverload<int>(&QComboBox::currentIndexChanged),
          this, &LogPanel::onSeverityPresetChanged);
  connect(chk_debug_, &QCheckBox::toggled,
          this, &LogPanel::onSeverityFilterChanged);
  connect(chk_info_, &QCheckBox::toggled,
          this, &LogPanel::onSeverityFilterChanged);
  connect(chk_warning_, &QCheckBox::toggled,
          this, &LogPanel::onSeverityFilterChanged);
  connect(chk_error_, &QCheckBox::toggled,
          this, &LogPanel::onSeverityFilterChanged);
  connect(le_filter_text_, &QLineEdit::textChanged,
          this, &LogPanel::onTextFilterChanged);
  connect(btn_clear_log_, &QPushButton::clicked,
          this, &LogPanel::onClearLog);
}

void LogPanel::createTableView() {
  log_model_ = new LogTableModel(this);

  log_proxy_ = new LogFilterProxy(this);
  log_proxy_->setSourceModel(log_model_);

  tbl_log_view_ = new QTableView(this);
  tbl_log_view_->setObjectName(QStringLiteral("tblLogView"));
  tbl_log_view_->setModel(log_proxy_);

  tbl_log_view_->setSelectionMode(
      QAbstractItemView::SingleSelection);
  tbl_log_view_->setSelectionBehavior(
      QAbstractItemView::SelectRows);
  tbl_log_view_->setEditTriggers(
      QAbstractItemView::NoEditTriggers);
  tbl_log_view_->setAlternatingRowColors(true);
  tbl_log_view_->verticalHeader()->hide();
  tbl_log_view_->horizontalHeader()->setStretchLastSection(true);
  tbl_log_view_->setSortingEnabled(false);
  tbl_log_view_->setWordWrap(false);
  tbl_log_view_->setVerticalScrollMode(
      QAbstractItemView::ScrollPerPixel);
  tbl_log_view_->setFont(
      QFontDatabase::systemFont(QFontDatabase::FixedFont));

  // Initial column widths.
  tbl_log_view_->setColumnWidth(LogTableModel::kTimestamp, 175);
  tbl_log_view_->setColumnWidth(LogTableModel::kSeverity, 60);
  tbl_log_view_->setColumnWidth(LogTableModel::kSource, 160);

  layout()->addWidget(tbl_log_view_);

  // Double-click copies entry to clipboard.
  connect(tbl_log_view_, &QTableView::doubleClicked,
          this, &LogPanel::onRowDoubleClicked);

  // Auto-scroll detection via scrollbar.
  connect(tbl_log_view_->verticalScrollBar(),
          &QScrollBar::valueChanged,
          this, [this](int value) {
            auto* sb = tbl_log_view_->verticalScrollBar();
            bool at_bottom = (value >= sb->maximum() - 4);
            if (chk_auto_scroll_ != nullptr &&
                chk_auto_scroll_->isChecked() != at_bottom) {
              chk_auto_scroll_->setChecked(at_bottom);
            }
          });
}

void LogPanel::createStatusRow() {
  auto* status_row = new QWidget(this);
  status_row->setObjectName(QStringLiteral("wgtLogStatus"));
  status_row->setFixedHeight(24);

  auto* status_layout = new QHBoxLayout(status_row);
  status_layout->setContentsMargins(0, 0, 0, 0);
  status_layout->setSpacing(8);

  lbl_entry_count_ = new QLabel(
      QStringLiteral("0 entries"), status_row);
  lbl_entry_count_->setObjectName(
      QStringLiteral("lblEntryCount"));
  lbl_entry_count_->setStyleSheet(
      QStringLiteral("color: #7F8C8D; font-size: 11pt;"));
  status_layout->addWidget(lbl_entry_count_);

  lbl_filter_status_ = new QLabel(status_row);
  lbl_filter_status_->setObjectName(
      QStringLiteral("lblFilterStatus"));
  lbl_filter_status_->setStyleSheet(QStringLiteral(
      "color: #F39C12; font-size: 11pt; font-style: italic;"));
  lbl_filter_status_->setVisible(false);
  status_layout->addWidget(lbl_filter_status_);

  status_layout->addStretch();

  chk_auto_scroll_ = new QCheckBox(
      QStringLiteral("Auto-scroll"), status_row);
  chk_auto_scroll_->setObjectName(
      QStringLiteral("chkAutoScroll"));
  chk_auto_scroll_->setChecked(true);
  chk_auto_scroll_->setMinimumHeight(24);
  status_layout->addWidget(chk_auto_scroll_);

  layout()->addWidget(status_row);

  connect(chk_auto_scroll_, &QCheckBox::toggled,
          this, &LogPanel::onAutoScrollToggled);
}

// ---- Slots ----------------------------------------------------------------

void LogPanel::onNewLogEntry(
    const mwa::core::LogEntry& entry) {
  log_model_->appendEntry(entry);
  btn_clear_log_->setEnabled(true);
  updateStatusLabels();

  if (chk_auto_scroll_->isChecked()) {
    tbl_log_view_->scrollToBottom();
  }
}

void LogPanel::onSeverityPresetChanged(int index) {
  if (updating_preset_) {
    return;
  }
  updating_preset_ = true;

  switch (index) {
    case 0:  // All
      chk_debug_->setChecked(true);
      chk_info_->setChecked(true);
      chk_warning_->setChecked(true);
      chk_error_->setChecked(true);
      break;
    case 1:  // Info+
      chk_debug_->setChecked(false);
      chk_info_->setChecked(true);
      chk_warning_->setChecked(true);
      chk_error_->setChecked(true);
      break;
    case 2:  // Warnings+
      chk_debug_->setChecked(false);
      chk_info_->setChecked(false);
      chk_warning_->setChecked(true);
      chk_error_->setChecked(true);
      break;
    case 3:  // Errors only
      chk_debug_->setChecked(false);
      chk_info_->setChecked(false);
      chk_warning_->setChecked(false);
      chk_error_->setChecked(true);
      break;
    default:
      break;
  }

  log_proxy_->setSeverityFilter(
      chk_debug_->isChecked(), chk_info_->isChecked(),
      chk_warning_->isChecked(), chk_error_->isChecked());
  updateStatusLabels();
  updating_preset_ = false;
}

void LogPanel::onSeverityFilterChanged() {
  if (updating_preset_) {
    return;
  }
  log_proxy_->setSeverityFilter(
      chk_debug_->isChecked(), chk_info_->isChecked(),
      chk_warning_->isChecked(), chk_error_->isChecked());
  updateStatusLabels();
}

void LogPanel::onTextFilterChanged(const QString& text) {
  log_proxy_->setTextFilter(text);
  updateStatusLabels();
}

void LogPanel::onClearLog() {
  log_model_->clear();
  btn_clear_log_->setEnabled(false);
  updateStatusLabels();
}

void LogPanel::onAutoScrollToggled(bool enabled) {
  if (enabled) {
    tbl_log_view_->scrollToBottom();
  }
}

void LogPanel::onRowDoubleClicked(const QModelIndex& index) {
  if (!index.isValid()) {
    return;
  }
  QModelIndex source_index = log_proxy_->mapToSource(index);
  int row = source_index.row();
  if (row < 0 || row >= log_model_->entryCount()) {
    return;
  }
  const auto& entry = log_model_->entryAt(row);
  QString text = QStringLiteral("[%1] [%2] [%3] %4")
      .arg(entry.timestamp.toString(
               QStringLiteral("HH:mm:ss.zzz")),
           log_model_->data(
               log_model_->index(row, LogTableModel::kSeverity))
               .toString(),
           entry.source, entry.message);
  QApplication::clipboard()->setText(text);
}

void LogPanel::updateStatusLabels() {
  int total = log_model_->entryCount();
  int visible = log_proxy_->rowCount();
  int hidden = total - visible;

  lbl_entry_count_->setText(
      QStringLiteral("%1 entries").arg(total));

  if (hidden > 0) {
    lbl_filter_status_->setText(
        QStringLiteral("(%1 hidden by filter)").arg(hidden));
    lbl_filter_status_->setVisible(true);
  } else {
    lbl_filter_status_->setVisible(false);
  }
}

}  // namespace mwa::gui
