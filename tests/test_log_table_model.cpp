/**
 * @file test_log_table_model.cpp
 * @brief Unit tests for the LogTableModel class.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Tests cover row/column counts, display data, severity colors,
 * background tints, tooltip roles, append, clear, and header data.
 *
 * @copyright LGPL-3.0-or-later
 */

#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QDateTime>
#include <QTest>

#include "gui/widgets/log_table_model.h"

static int s_argc = 1;
static char s_app_name[] = "test_log_table_model";
static char* s_argv[] = {s_app_name};

class TestLogTableModel : public QObject {
  Q_OBJECT

 private:
  QApplication* app_{nullptr};
  mwa::gui::LogTableModel* model_{nullptr};

  mwa::core::LogEntry makeEntry(
      mwa::core::LogSeverity sev,
      const QString& msg,
      const QString& src = QStringLiteral("Test")) {
    return {QDateTime::currentDateTime(), sev, src, msg};
  }

 private slots:
  void initTestCase() {
    app_ = new QApplication(s_argc, s_argv);
  }

  void init() {
    model_ = new mwa::gui::LogTableModel();
  }

  void cleanup() {
    delete model_;
    model_ = nullptr;
  }

  void cleanupTestCase() {
    delete app_;
  }

  // --- Structure ---
  void test_emptyModel_hasZeroRows() {
    QCOMPARE(model_->rowCount(), 0);
  }

  void test_emptyModel_hasFourColumns() {
    QCOMPARE(model_->columnCount(), 4);
  }

  void test_entryCount_matchesRowCount() {
    model_->appendEntry(
        makeEntry(mwa::core::LogSeverity::kInfo, "a"));
    model_->appendEntry(
        makeEntry(mwa::core::LogSeverity::kInfo, "b"));
    QCOMPARE(model_->entryCount(), 2);
    QCOMPARE(model_->rowCount(), 2);
  }

  // --- Append ---
  void test_appendEntry_incrementsRowCount() {
    model_->appendEntry(
        makeEntry(mwa::core::LogSeverity::kInfo, "msg"));
    QCOMPARE(model_->rowCount(), 1);
  }

  void test_appendEntry_dataRetrievable() {
    model_->appendEntry(
        makeEntry(mwa::core::LogSeverity::kWarning,
                  "test message", "TestSource"));
    QModelIndex msg_idx = model_->index(
        0, mwa::gui::LogTableModel::kMessage);
    QCOMPARE(model_->data(msg_idx).toString(),
             QStringLiteral("test message"));

    QModelIndex src_idx = model_->index(
        0, mwa::gui::LogTableModel::kSource);
    QCOMPARE(model_->data(src_idx).toString(),
             QStringLiteral("TestSource"));
  }

  // --- Severity display ---
  void test_severityDisplay_debug() {
    model_->appendEntry(
        makeEntry(mwa::core::LogSeverity::kDebug, "d"));
    QModelIndex idx = model_->index(
        0, mwa::gui::LogTableModel::kSeverity);
    QCOMPARE(model_->data(idx).toString(),
             QStringLiteral("DBG"));
  }

  void test_severityDisplay_info() {
    model_->appendEntry(
        makeEntry(mwa::core::LogSeverity::kInfo, "i"));
    QModelIndex idx = model_->index(
        0, mwa::gui::LogTableModel::kSeverity);
    QCOMPARE(model_->data(idx).toString(),
             QStringLiteral("INFO"));
  }

  void test_severityDisplay_warning() {
    model_->appendEntry(
        makeEntry(mwa::core::LogSeverity::kWarning, "w"));
    QModelIndex idx = model_->index(
        0, mwa::gui::LogTableModel::kSeverity);
    QCOMPARE(model_->data(idx).toString(),
             QStringLiteral("WARN"));
  }

  void test_severityDisplay_error() {
    model_->appendEntry(
        makeEntry(mwa::core::LogSeverity::kError, "e"));
    QModelIndex idx = model_->index(
        0, mwa::gui::LogTableModel::kSeverity);
    QCOMPARE(model_->data(idx).toString(),
             QStringLiteral("ERR"));
  }

  // --- Foreground color ---
  void test_foregroundRole_errorIsRed() {
    model_->appendEntry(
        makeEntry(mwa::core::LogSeverity::kError, "e"));
    QModelIndex idx = model_->index(
        0, mwa::gui::LogTableModel::kSeverity);
    auto brush = model_->data(idx, Qt::ForegroundRole)
                     .value<QBrush>();
    QCOMPARE(brush.color(), QColor(0xE7, 0x4C, 0x3C));
  }

  void test_foregroundRole_infoIsGreen() {
    model_->appendEntry(
        makeEntry(mwa::core::LogSeverity::kInfo, "i"));
    QModelIndex idx = model_->index(
        0, mwa::gui::LogTableModel::kSeverity);
    auto brush = model_->data(idx, Qt::ForegroundRole)
                     .value<QBrush>();
    QCOMPARE(brush.color(), QColor(0x27, 0xAE, 0x60));
  }

  // --- Background tint ---
  void test_backgroundRole_warningHasTint() {
    model_->appendEntry(
        makeEntry(mwa::core::LogSeverity::kWarning, "w"));
    QModelIndex idx = model_->index(
        0, mwa::gui::LogTableModel::kMessage);
    auto bg = model_->data(idx, Qt::BackgroundRole);
    QVERIFY(bg.isValid());
    QCOMPARE(bg.value<QBrush>().color(),
             QColor(0xFE, 0xF9, 0xE7));
  }

  void test_backgroundRole_errorHasTint() {
    model_->appendEntry(
        makeEntry(mwa::core::LogSeverity::kError, "e"));
    QModelIndex idx = model_->index(
        0, mwa::gui::LogTableModel::kMessage);
    auto bg = model_->data(idx, Qt::BackgroundRole);
    QVERIFY(bg.isValid());
    QCOMPARE(bg.value<QBrush>().color(),
             QColor(0xFD, 0xED, 0xEC));
  }

  void test_backgroundRole_infoHasNoTint() {
    model_->appendEntry(
        makeEntry(mwa::core::LogSeverity::kInfo, "i"));
    QModelIndex idx = model_->index(
        0, mwa::gui::LogTableModel::kMessage);
    auto bg = model_->data(idx, Qt::BackgroundRole);
    QVERIFY(!bg.isValid());
  }

  // --- Tooltip ---
  void test_tooltipRole_timestampShowsFullDate() {
    model_->appendEntry(
        makeEntry(mwa::core::LogSeverity::kInfo, "i"));
    QModelIndex idx = model_->index(
        0, mwa::gui::LogTableModel::kTimestamp);
    auto tooltip = model_->data(idx, Qt::ToolTipRole).toString();
    QVERIFY2(tooltip.contains(QStringLiteral("-")),
             "Tooltip must contain full date");
  }

  // --- UserRole ---
  void test_userRole_returnsSeverityInt() {
    model_->appendEntry(
        makeEntry(mwa::core::LogSeverity::kWarning, "w"));
    QModelIndex idx = model_->index(
        0, mwa::gui::LogTableModel::kSeverity);
    int sev = model_->data(idx, Qt::UserRole).toInt();
    QCOMPARE(sev,
             static_cast<int>(mwa::core::LogSeverity::kWarning));
  }

  // --- Header data ---
  void test_headerData_column0() {
    QCOMPARE(
        model_->headerData(0, Qt::Horizontal).toString(),
        QStringLiteral("Timestamp"));
  }

  void test_headerData_column1() {
    QCOMPARE(
        model_->headerData(1, Qt::Horizontal).toString(),
        QStringLiteral("Sev"));
  }

  void test_headerData_column2() {
    QCOMPARE(
        model_->headerData(2, Qt::Horizontal).toString(),
        QStringLiteral("Source"));
  }

  void test_headerData_column3() {
    QCOMPARE(
        model_->headerData(3, Qt::Horizontal).toString(),
        QStringLiteral("Message"));
  }

  // --- Clear ---
  void test_clear_removesAllEntries() {
    model_->appendEntry(
        makeEntry(mwa::core::LogSeverity::kInfo, "a"));
    model_->appendEntry(
        makeEntry(mwa::core::LogSeverity::kInfo, "b"));
    QCOMPARE(model_->rowCount(), 2);
    model_->clear();
    QCOMPARE(model_->rowCount(), 0);
  }

  void test_clear_emptyModel_noop() {
    model_->clear();
    QCOMPARE(model_->rowCount(), 0);
  }

  // --- entryAt ---
  void test_entryAt_returnsCorrectEntry() {
    model_->appendEntry(
        makeEntry(mwa::core::LogSeverity::kInfo, "first"));
    model_->appendEntry(
        makeEntry(mwa::core::LogSeverity::kError, "second"));
    QCOMPARE(model_->entryAt(0).message,
             QStringLiteral("first"));
    QCOMPARE(model_->entryAt(1).message,
             QStringLiteral("second"));
  }

  // --- Invalid index ---
  void test_data_invalidIndex_returnsInvalid() {
    auto result = model_->data(QModelIndex());
    QVERIFY(!result.isValid());
  }

  void test_data_outOfRange_returnsInvalid() {
    auto idx = model_->index(99, 0);
    auto result = model_->data(idx);
    QVERIFY(!result.isValid());
  }
};

QTEST_APPLESS_MAIN(TestLogTableModel)
#include "test_log_table_model.moc"
