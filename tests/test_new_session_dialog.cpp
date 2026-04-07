/**
 * @file test_new_session_dialog.cpp
 * @brief Adversarial tests for mwa::gui::NewSessionDialog.
 * @date 2026-04-07
 * @copyright LGPL-3.0-or-later
 *
 * Tests cover construction, name field validation (OK button state),
 * accessor methods, and whitespace-only rejection.
 */

#include <QApplication>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QObject>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QtTest>

#include "gui/dialogs/new_session_dialog.h"

using mwa::gui::NewSessionDialog;

// ---------------------------------------------------------------------------
class TestNewSessionDialog : public QObject {
  Q_OBJECT

 private slots:
  void init();
  void cleanup();

  // --- Construction ---
  void test_constructs_withoutCrash();
  void test_windowTitle_isCorrect();

  // --- Name field / OK button state ---
  void test_okButton_isDisabled_initially();
  void test_okButton_enables_whenNameIsNonEmpty();
  void test_okButton_disables_whenNameIsCleared();
  void test_okButton_stays_disabled_forWhitespaceOnlyName();

  // --- Accessors ---
  void test_name_returnsEmptyString_initially();
  void test_name_returnsTrimmedText_afterInput();
  void test_description_returnsEmptyString_initially();
  void test_description_returnsText_afterInput();

 private:
  NewSessionDialog* dialog_{nullptr};
};

// ---------------------------------------------------------------------------
void TestNewSessionDialog::init() {
  dialog_ = new NewSessionDialog();
}

void TestNewSessionDialog::cleanup() {
  delete dialog_;
  dialog_ = nullptr;
}

// ===========================================================================
// Construction
// ===========================================================================

void TestNewSessionDialog::test_constructs_withoutCrash() {
  QVERIFY(dialog_ != nullptr);
}

void TestNewSessionDialog::test_windowTitle_isCorrect() {
  QCOMPARE(dialog_->windowTitle(),
           QStringLiteral("New Experiment Session"));
}

// ===========================================================================
// Name field / OK button state
// ===========================================================================

void TestNewSessionDialog::test_okButton_isDisabled_initially() {
  auto* box = dialog_->findChild<QDialogButtonBox*>("buttonBox");
  QVERIFY(box != nullptr);
  auto* ok = box->button(QDialogButtonBox::Ok);
  QVERIFY2(ok != nullptr, "OK button must exist in buttonBox");
  QVERIFY2(!ok->isEnabled(),
           "OK button must be disabled when name field is empty");
}

void TestNewSessionDialog::test_okButton_enables_whenNameIsNonEmpty() {
  auto* edt = dialog_->findChild<QLineEdit*>("edtSessionName");
  QVERIFY(edt != nullptr);
  edt->setText(QStringLiteral("Run 1"));
  QApplication::processEvents();

  auto* box = dialog_->findChild<QDialogButtonBox*>("buttonBox");
  QVERIFY(box != nullptr);
  auto* ok = box->button(QDialogButtonBox::Ok);
  QVERIFY2(ok->isEnabled(),
           "OK button must be enabled when name field has text");
}

void TestNewSessionDialog::test_okButton_disables_whenNameIsCleared() {
  auto* edt = dialog_->findChild<QLineEdit*>("edtSessionName");
  QVERIFY(edt != nullptr);
  edt->setText(QStringLiteral("Run 1"));
  QApplication::processEvents();
  edt->clear();
  QApplication::processEvents();

  auto* box = dialog_->findChild<QDialogButtonBox*>("buttonBox");
  QVERIFY(box != nullptr);
  auto* ok = box->button(QDialogButtonBox::Ok);
  QVERIFY2(!ok->isEnabled(),
           "OK button must re-disable when name field is cleared");
}

void TestNewSessionDialog::test_okButton_stays_disabled_forWhitespaceOnlyName() {
  auto* edt = dialog_->findChild<QLineEdit*>("edtSessionName");
  QVERIFY(edt != nullptr);
  // Spaces only — trimmed result is empty, so OK must stay disabled.
  edt->setText(QStringLiteral("   "));
  QApplication::processEvents();

  auto* box = dialog_->findChild<QDialogButtonBox*>("buttonBox");
  QVERIFY(box != nullptr);
  auto* ok = box->button(QDialogButtonBox::Ok);
  QVERIFY2(!ok->isEnabled(),
           "OK button must remain disabled for whitespace-only name");
}

// ===========================================================================
// Accessors
// ===========================================================================

void TestNewSessionDialog::test_name_returnsEmptyString_initially() {
  QVERIFY2(dialog_->name().isEmpty(),
           "name() must return empty string before any input");
}

void TestNewSessionDialog::test_name_returnsTrimmedText_afterInput() {
  auto* edt = dialog_->findChild<QLineEdit*>("edtSessionName");
  QVERIFY(edt != nullptr);
  edt->setText(QStringLiteral("  Run 42  "));
  QApplication::processEvents();

  QCOMPARE(dialog_->name(), QStringLiteral("Run 42"));
}

void TestNewSessionDialog::test_description_returnsEmptyString_initially() {
  QVERIFY2(dialog_->description().isEmpty(),
           "description() must return empty string before any input");
}

void TestNewSessionDialog::test_description_returnsText_afterInput() {
  auto* edt = dialog_->findChild<QPlainTextEdit*>("edtSessionDescription");
  QVERIFY(edt != nullptr);
  edt->setPlainText(QStringLiteral("Focused flow test at 1 MHz"));
  QApplication::processEvents();

  QCOMPARE(dialog_->description(),
           QStringLiteral("Focused flow test at 1 MHz"));
}

// ---------------------------------------------------------------------------
QTEST_MAIN(TestNewSessionDialog)
#include "test_new_session_dialog.moc"
