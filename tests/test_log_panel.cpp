/**
 * @file test_log_panel.cpp
 * @brief Unit tests for the LogPanel widget.
 * @author MWA Team
 * @date 2026-03-22
 *
 * Tests cover widget structure, severity filtering, text filtering,
 * clear, auto-scroll checkbox, and Logger integration.
 *
 * @note Uses !isHidden() instead of isVisible() for headless CI.
 *
 * @copyright LGPL-3.0-or-later
 */

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QTableView>
#include <QTest>

#include "core/logger.h"
#include "gui/widgets/log_panel.h"

static int argc = 1;
static char app_name[] = "test_log_panel";
static char* argv[] = {app_name};

class TestLogPanel : public QObject {
  Q_OBJECT

 private:
  QApplication* app_{nullptr};
  mwa::gui::LogPanel* panel_{nullptr};

 private slots:
  void initTestCase() {
    app_ = new QApplication(argc, argv);
  }

  void init() {
    panel_ = new mwa::gui::LogPanel();
    QApplication::processEvents();
  }

  void cleanup() {
    delete panel_;
    panel_ = nullptr;
  }

  void cleanupTestCase() {
    delete app_;
  }

  // --- Widget existence ---
  void test_logPanel_objectName() {
    QCOMPARE(panel_->objectName(),
             QStringLiteral("logPanel"));
  }

  void test_tableView_exists() {
    auto* tv = panel_->findChild<QTableView*>(
        QStringLiteral("tblLogView"));
    QVERIFY(tv != nullptr);
  }

  void test_severityPreset_exists() {
    auto* cmb = panel_->findChild<QComboBox*>(
        QStringLiteral("cmbSeverityPreset"));
    QVERIFY(cmb != nullptr);
    QCOMPARE(cmb->count(), 4);
  }

  void test_severityCheckboxes_exist() {
    QVERIFY(panel_->findChild<QCheckBox*>(
                QStringLiteral("chkDebug")) != nullptr);
    QVERIFY(panel_->findChild<QCheckBox*>(
                QStringLiteral("chkInfo")) != nullptr);
    QVERIFY(panel_->findChild<QCheckBox*>(
                QStringLiteral("chkWarning")) != nullptr);
    QVERIFY(panel_->findChild<QCheckBox*>(
                QStringLiteral("chkError")) != nullptr);
  }

  void test_filterInput_exists() {
    auto* le = panel_->findChild<QLineEdit*>(
        QStringLiteral("leFilterText"));
    QVERIFY(le != nullptr);
  }

  void test_clearButton_exists() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnClearLog"));
    QVERIFY(btn != nullptr);
  }

  void test_clearButton_initiallyDisabled() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnClearLog"));
    QVERIFY(!btn->isEnabled());
  }

  void test_autoScrollCheckbox_exists() {
    auto* chk = panel_->findChild<QCheckBox*>(
        QStringLiteral("chkAutoScroll"));
    QVERIFY(chk != nullptr);
    QVERIFY(chk->isChecked());
  }

  void test_entryCountLabel_exists() {
    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblEntryCount"));
    QVERIFY(lbl != nullptr);
    QVERIFY(lbl->text().contains(QStringLiteral("0")));
  }

  // --- Logger integration ---
  void test_loggerSignal_addsRowToTable() {
    auto* tv = panel_->findChild<QTableView*>(
        QStringLiteral("tblLogView"));
    QVERIFY(tv != nullptr);

    int rows_before = tv->model()->rowCount();
    mwa::core::Logger::instance().logInfo(
        QStringLiteral("test_entry"),
        QStringLiteral("TestLogPanel"));
    QApplication::processEvents();

    QCOMPARE(tv->model()->rowCount(), rows_before + 1);
  }

  void test_loggerSignal_enablesClearButton() {
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnClearLog"));
    QVERIFY(!btn->isEnabled());

    mwa::core::Logger::instance().logInfo(
        QStringLiteral("enable_clear"),
        QStringLiteral("TestLogPanel"));
    QApplication::processEvents();

    QVERIFY(btn->isEnabled());
  }

  void test_loggerSignal_updatesEntryCount() {
    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblEntryCount"));

    mwa::core::Logger::instance().logInfo(
        QStringLiteral("count_test"),
        QStringLiteral("TestLogPanel"));
    QApplication::processEvents();

    QVERIFY(lbl->text().contains(QStringLiteral("entr")));
  }

  // --- Clear ---
  void test_clearButton_removesAllEntries() {
    auto* tv = panel_->findChild<QTableView*>(
        QStringLiteral("tblLogView"));
    auto* btn = panel_->findChild<QPushButton*>(
        QStringLiteral("btnClearLog"));

    mwa::core::Logger::instance().logInfo(
        QStringLiteral("to_clear"),
        QStringLiteral("TestLogPanel"));
    QApplication::processEvents();
    QVERIFY(tv->model()->rowCount() > 0);

    btn->click();
    QApplication::processEvents();

    QCOMPARE(tv->model()->rowCount(), 0);
    QVERIFY(!btn->isEnabled());
  }

  // --- Severity filter ---
  void test_severityPreset_errorsOnly_filtersNonErrors() {
    auto* tv = panel_->findChild<QTableView*>(
        QStringLiteral("tblLogView"));
    auto* cmb = panel_->findChild<QComboBox*>(
        QStringLiteral("cmbSeverityPreset"));

    mwa::core::Logger::instance().logInfo(
        QStringLiteral("info_msg"),
        QStringLiteral("TestLogPanel"));
    mwa::core::Logger::instance().logError(
        QStringLiteral("error_msg"),
        QStringLiteral("TestLogPanel"));
    QApplication::processEvents();

    int total = tv->model()->rowCount();
    QVERIFY(total >= 2);

    // Set to "Errors only" (index 3).
    cmb->setCurrentIndex(3);
    QApplication::processEvents();

    int filtered = tv->model()->rowCount();
    QVERIFY2(filtered < total,
             "Errors only filter must hide non-error entries");
    QVERIFY(filtered >= 1);
  }

  // --- Text filter ---
  void test_textFilter_hidesNonMatchingEntries() {
    auto* tv = panel_->findChild<QTableView*>(
        QStringLiteral("tblLogView"));
    auto* le = panel_->findChild<QLineEdit*>(
        QStringLiteral("leFilterText"));

    mwa::core::Logger::instance().logInfo(
        QStringLiteral("alpha_unique"),
        QStringLiteral("TestLogPanel"));
    mwa::core::Logger::instance().logInfo(
        QStringLiteral("beta_unique"),
        QStringLiteral("TestLogPanel"));
    QApplication::processEvents();

    int total = tv->model()->rowCount();
    QVERIFY(total >= 2);

    le->setText(QStringLiteral("alpha_unique"));
    QApplication::processEvents();

    int filtered = tv->model()->rowCount();
    QVERIFY2(filtered < total,
             "Text filter must reduce visible entries");
  }

  // --- Filter status label ---
  void test_filterStatusLabel_visibleWhenFiltering() {
    auto* lbl = panel_->findChild<QLabel*>(
        QStringLiteral("lblFilterStatus"));
    QVERIFY(lbl->isHidden());

    mwa::core::Logger::instance().logInfo(
        QStringLiteral("vis_test"),
        QStringLiteral("TestLogPanel"));
    QApplication::processEvents();

    auto* le = panel_->findChild<QLineEdit*>(
        QStringLiteral("leFilterText"));
    le->setText(QStringLiteral("NONEXISTENT_STRING_XYZ"));
    QApplication::processEvents();

    QVERIFY2(!lbl->isHidden(),
             "Filter status must not be hidden when entries "
             "are filtered out");
    QVERIFY(lbl->text().contains(QStringLiteral("hidden")));
  }
};

QTEST_APPLESS_MAIN(TestLogPanel)
#include "test_log_panel.moc"
