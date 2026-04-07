/**
 * @file new_session_dialog.h
 * @brief Dialog for creating a new experiment session.
 * @author MWA Team
 * @date 2026-04-07
 *
 * Declares NewSessionDialog, a modal QDialog that collects a session name
 * and optional description before a recording session begins.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QDialog>
#include <QString>

class QDialogButtonBox;
class QLineEdit;
class QPlainTextEdit;

namespace mwa::gui {

/**
 * @class NewSessionDialog
 * @brief Modal dialog for entering new experiment session metadata.
 *
 * Collects a required session name and an optional free-text description.
 * The OK ("Start") button is disabled until the name field is non-empty.
 * Use name() and description() after exec() returns QDialog::Accepted to
 * retrieve the entered values.
 *
 * @see mwa::analysis::ExperimentSession
 */
class NewSessionDialog : public QDialog {
  Q_OBJECT

 public:
  /**
   * @brief Construct the dialog.
   *
   * @param parent Optional parent widget; the dialog is modal to it.
   */
  explicit NewSessionDialog(QWidget* parent = nullptr);

  /**
   * @brief Return the session name entered by the user.
   *
   * @return Trimmed session name. Empty if rejected or left blank.
   */
  [[nodiscard]] QString name() const;

  /**
   * @brief Return the optional description entered by the user.
   *
   * @return Description string, or an empty string if not provided.
   */
  [[nodiscard]] QString description() const;

 private slots:
  /**
   * @brief Enable or disable OK based on the name field content.
   *
   * @param text The current text in the name field.
   */
  void onNameTextChanged(const QString& text);

 private:
  QLineEdit*        edt_name_{nullptr};
  QPlainTextEdit*   edt_description_{nullptr};
  QDialogButtonBox* button_box_{nullptr};
};

}  // namespace mwa::gui
