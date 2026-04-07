/**
 * @file new_session_dialog.cpp
 * @brief Dialog for creating a new experiment session.
 * @author MWA Team
 * @date 2026-04-07
 *
 * @copyright LGPL-3.0-or-later
 */

#include "gui/dialogs/new_session_dialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace mwa::gui {

NewSessionDialog::NewSessionDialog(QWidget* parent) : QDialog(parent) {
  setWindowTitle(QStringLiteral("New Experiment Session"));
  setFixedWidth(420);
  setModal(true);

  edt_name_ = new QLineEdit(this);
  edt_name_->setObjectName(QStringLiteral("edtSessionName"));
  edt_name_->setPlaceholderText(QStringLiteral("e.g. Run 1"));

  edt_description_ = new QPlainTextEdit(this);
  edt_description_->setObjectName(
      QStringLiteral("edtSessionDescription"));
  edt_description_->setFixedHeight(72);
  edt_description_->setPlaceholderText(
      QStringLiteral("Optional free-text description"));

  button_box_ = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  button_box_->setObjectName(QStringLiteral("buttonBox"));
  button_box_->button(QDialogButtonBox::Ok)->setText(
      QStringLiteral("Start"));
  button_box_->button(QDialogButtonBox::Ok)->setEnabled(false);

  auto* form_layout = new QFormLayout;
  form_layout->addRow(QStringLiteral("Session Name *"), edt_name_);
  form_layout->addRow(QStringLiteral("Description"),   edt_description_);

  auto* layout = new QVBoxLayout(this);
  layout->addLayout(form_layout);
  layout->addWidget(button_box_);

  connect(edt_name_, &QLineEdit::textChanged,
          this,      &NewSessionDialog::onNameTextChanged);
  connect(button_box_, &QDialogButtonBox::accepted,
          this,        &QDialog::accept);
  connect(button_box_, &QDialogButtonBox::rejected,
          this,        &QDialog::reject);
}

QString NewSessionDialog::name() const {
  return edt_name_->text().trimmed();
}

QString NewSessionDialog::description() const {
  return edt_description_->toPlainText().trimmed();
}

void NewSessionDialog::onNameTextChanged(const QString& text) {
  button_box_->button(QDialogButtonBox::Ok)->setEnabled(
      !text.trimmed().isEmpty());
}

}  // namespace mwa::gui
