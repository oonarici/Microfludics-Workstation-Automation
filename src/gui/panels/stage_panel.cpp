/**
 * @file stage_panel.cpp
 * @brief Implementation of the StagePanel widget.
 * @author MWA Team
 * @date 2026-03-23
 *
 * @copyright LGPL-3.0-or-later
 */

#include "gui/panels/stage_panel.h"

#include <QFontDatabase>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>

namespace mwa::gui {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

StagePanel::StagePanel(QWidget* parent) : QWidget(parent) {
  setObjectName(QStringLiteral("stagePanel"));

  auto* root_layout = new QVBoxLayout(this);
  root_layout->setContentsMargins(8, 8, 8, 8);
  root_layout->setSpacing(12);

  root_layout->addWidget(createConnectionGroup());
  root_layout->addWidget(createJogGroup());
  root_layout->addWidget(createAbsPositionGroup());
  root_layout->addWidget(createSpeedGroup());
  root_layout->addWidget(createStatusGroup());
  root_layout->addStretch();
  root_layout->addWidget(createEmergencyStopButton());

  timer_speed_ = new QTimer(this);
  timer_speed_->setSingleShot(true);
  timer_speed_->setInterval(kDebounceMs);

  connect(timer_speed_, &QTimer::timeout,
          this, &StagePanel::onSpeedDebounced);

  setMotionControlsEnabled(false);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void StagePanel::setController(
    mwa::hardware::StageControllerInterface* controller) {
  if (controller_ != nullptr) {
    disconnect(controller_, nullptr, this, nullptr);
  }

  controller_ = controller;

  if (controller_ == nullptr) {
    setMotionControlsEnabled(false);
    return;
  }

  connect(controller_,
          &mwa::hardware::StageControllerInterface::stateChanged,
          this, &StagePanel::onStateChanged);
  connect(controller_,
          &mwa::hardware::StageControllerInterface::positionChanged,
          this, &StagePanel::onPositionChanged);
  connect(controller_,
          &mwa::hardware::StageControllerInterface::moveComplete,
          this, &StagePanel::onMoveComplete);
  connect(controller_,
          &mwa::hardware::StageControllerInterface::homeComplete,
          this, &StagePanel::onHomeComplete);

  // Sync UI to current controller state.
  onStateChanged(controller_->state());
  onPositionChanged(controller_->positionX(),
                    controller_->positionY(),
                    controller_->positionZ());
}

// ---------------------------------------------------------------------------
// Group-box builders
// ---------------------------------------------------------------------------

QGroupBox* StagePanel::createConnectionGroup() {
  grp_connection_ = new QGroupBox(
      QStringLiteral("Connection"), this);
  grp_connection_->setObjectName(
      QStringLiteral("grpConnection"));

  auto* layout = new QHBoxLayout(grp_connection_);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(8);

  cmb_port_ = new QComboBox(grp_connection_);
  cmb_port_->setObjectName(QStringLiteral("cmbPort"));
  cmb_port_->addItem(QStringLiteral("Mock Stage"));
  layout->addWidget(cmb_port_);

  lbl_status_ = new QLabel(
      QStringLiteral("\u25CF Disconnected"), grp_connection_);
  lbl_status_->setObjectName(QStringLiteral("lblStatus"));
  applyStatusStyle(
      mwa::hardware::DeviceInterface::DeviceState::kDisconnected);
  layout->addWidget(lbl_status_);

  layout->addStretch();

  btn_connect_ = new QPushButton(
      QStringLiteral("Connect"), grp_connection_);
  btn_connect_->setObjectName(QStringLiteral("btnConnect"));
  btn_connect_->setMinimumSize(96, 32);
  layout->addWidget(btn_connect_);

  connect(btn_connect_, &QPushButton::clicked,
          this, &StagePanel::onConnectClicked);

  return grp_connection_;
}

QGroupBox* StagePanel::createJogGroup() {
  grp_jog_ = new QGroupBox(QStringLiteral("Jog"), this);
  grp_jog_->setObjectName(QStringLiteral("grpJog"));

  auto* outer = new QVBoxLayout(grp_jog_);
  outer->setContentsMargins(8, 8, 8, 8);
  outer->setSpacing(8);

  // Step size selector.
  auto* step_row = new QHBoxLayout();
  step_row->setSpacing(6);
  step_row->addWidget(
      new QLabel(QStringLiteral("Step:"), grp_jog_));

  cmb_step_size_ = new QComboBox(grp_jog_);
  cmb_step_size_->setObjectName(
      QStringLiteral("cmbStepSize"));
  cmb_step_size_->addItem(QStringLiteral("0.01 mm"));
  cmb_step_size_->addItem(QStringLiteral("0.10 mm"));
  cmb_step_size_->addItem(QStringLiteral("1.00 mm"));
  cmb_step_size_->addItem(QStringLiteral("10.0 mm"));
  cmb_step_size_->setCurrentIndex(1);
  step_row->addWidget(cmb_step_size_);
  step_row->addStretch();
  outer->addLayout(step_row);

  // XY jog grid (3x3).
  auto* xy_grid = new QGridLayout();
  xy_grid->setSpacing(4);

  auto makeJogBtn = [this](const QString& label) {
    auto* btn = new QPushButton(label, grp_jog_);
    btn->setMinimumSize(kJogButtonMinW, kJogButtonMinH);
    return btn;
  };

  btn_jog_y_plus_ = makeJogBtn(QStringLiteral("Y+"));
  btn_jog_y_plus_->setObjectName(
      QStringLiteral("btnJogYPlus"));
  xy_grid->addWidget(btn_jog_y_plus_, 0, 1);

  btn_jog_x_minus_ = makeJogBtn(QStringLiteral("X\u2212"));
  btn_jog_x_minus_->setObjectName(
      QStringLiteral("btnJogXMinus"));
  xy_grid->addWidget(btn_jog_x_minus_, 1, 0);

  btn_home_ = makeJogBtn(QStringLiteral("Home"));
  btn_home_->setObjectName(QStringLiteral("btnHome"));
  QFont home_font = btn_home_->font();
  home_font.setBold(true);
  btn_home_->setFont(home_font);
  xy_grid->addWidget(btn_home_, 1, 1);

  btn_jog_x_plus_ = makeJogBtn(QStringLiteral("X+"));
  btn_jog_x_plus_->setObjectName(
      QStringLiteral("btnJogXPlus"));
  xy_grid->addWidget(btn_jog_x_plus_, 1, 2);

  btn_jog_y_minus_ = makeJogBtn(QStringLiteral("Y\u2212"));
  btn_jog_y_minus_->setObjectName(
      QStringLiteral("btnJogYMinus"));
  xy_grid->addWidget(btn_jog_y_minus_, 2, 1);

  outer->addLayout(xy_grid);

  // Z axis buttons (centred below XY grid).
  auto* z_row = new QHBoxLayout();
  z_row->setSpacing(4);
  z_row->addStretch();

  btn_jog_z_plus_ = makeJogBtn(QStringLiteral("Z+"));
  btn_jog_z_plus_->setObjectName(
      QStringLiteral("btnJogZPlus"));
  z_row->addWidget(btn_jog_z_plus_);

  btn_jog_z_minus_ = makeJogBtn(QStringLiteral("Z\u2212"));
  btn_jog_z_minus_->setObjectName(
      QStringLiteral("btnJogZMinus"));
  z_row->addWidget(btn_jog_z_minus_);

  z_row->addStretch();
  outer->addLayout(z_row);

  connect(btn_jog_x_plus_,  &QPushButton::clicked,
          this, &StagePanel::onJogXPlus);
  connect(btn_jog_x_minus_, &QPushButton::clicked,
          this, &StagePanel::onJogXMinus);
  connect(btn_jog_y_plus_,  &QPushButton::clicked,
          this, &StagePanel::onJogYPlus);
  connect(btn_jog_y_minus_, &QPushButton::clicked,
          this, &StagePanel::onJogYMinus);
  connect(btn_jog_z_plus_,  &QPushButton::clicked,
          this, &StagePanel::onJogZPlus);
  connect(btn_jog_z_minus_, &QPushButton::clicked,
          this, &StagePanel::onJogZMinus);
  connect(btn_home_, &QPushButton::clicked,
          this, &StagePanel::onHomeClicked);

  return grp_jog_;
}

QGroupBox* StagePanel::createAbsPositionGroup() {
  grp_abs_position_ = new QGroupBox(
      QStringLiteral("Absolute Position"), this);
  grp_abs_position_->setObjectName(
      QStringLiteral("grpAbsPosition"));

  auto* layout = new QVBoxLayout(grp_abs_position_);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(6);

  auto* form = new QFormLayout();
  form->setSpacing(4);

  auto makeAbsSpinBox = [this]() {
    auto* spn = new QDoubleSpinBox(grp_abs_position_);
    spn->setRange(-9999.999, 9999.999);
    spn->setDecimals(3);
    spn->setSuffix(QStringLiteral(" mm"));
    spn->setValue(0.0);
    return spn;
  };

  spn_abs_x_ = makeAbsSpinBox();
  spn_abs_x_->setObjectName(QStringLiteral("spnAbsX"));
  form->addRow(QStringLiteral("X:"), spn_abs_x_);

  spn_abs_y_ = makeAbsSpinBox();
  spn_abs_y_->setObjectName(QStringLiteral("spnAbsY"));
  form->addRow(QStringLiteral("Y:"), spn_abs_y_);

  spn_abs_z_ = makeAbsSpinBox();
  spn_abs_z_->setObjectName(QStringLiteral("spnAbsZ"));
  form->addRow(QStringLiteral("Z:"), spn_abs_z_);

  layout->addLayout(form);

  btn_go_to_ = new QPushButton(
      QStringLiteral("Go To"), grp_abs_position_);
  btn_go_to_->setObjectName(QStringLiteral("btnGoTo"));
  layout->addWidget(btn_go_to_);

  connect(btn_go_to_, &QPushButton::clicked,
          this, &StagePanel::onGoToClicked);

  return grp_abs_position_;
}

QGroupBox* StagePanel::createSpeedGroup() {
  grp_speed_ = new QGroupBox(
      QStringLiteral("Speed"), this);
  grp_speed_->setObjectName(QStringLiteral("grpSpeed"));

  auto* layout = new QFormLayout(grp_speed_);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(6);

  spn_speed_ = new QDoubleSpinBox(grp_speed_);
  spn_speed_->setObjectName(QStringLiteral("spnSpeed"));
  spn_speed_->setRange(0.01, 100.0);
  spn_speed_->setDecimals(2);
  spn_speed_->setSuffix(QStringLiteral(" mm/s"));
  spn_speed_->setValue(1.0);
  layout->addRow(QStringLiteral("Speed:"), spn_speed_);

  connect(spn_speed_,
          qOverload<double>(&QDoubleSpinBox::valueChanged),
          this, [this]() { timer_speed_->start(); });

  return grp_speed_;
}

QGroupBox* StagePanel::createStatusGroup() {
  grp_status_ = new QGroupBox(
      QStringLiteral("Status"), this);
  grp_status_->setObjectName(QStringLiteral("grpStatus"));

  auto* layout = new QFormLayout(grp_status_);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(6);

  const QFont mono_font =
      QFontDatabase::systemFont(QFontDatabase::FixedFont);

  auto makePosLabel = [&](QWidget* parent) {
    auto* lbl = new QLabel(QStringLiteral("0.000 mm"), parent);
    lbl->setFont(mono_font);
    return lbl;
  };

  lbl_pos_x_ = makePosLabel(grp_status_);
  lbl_pos_x_->setObjectName(QStringLiteral("lblPosX"));
  layout->addRow(QStringLiteral("X:"), lbl_pos_x_);

  lbl_pos_y_ = makePosLabel(grp_status_);
  lbl_pos_y_->setObjectName(QStringLiteral("lblPosY"));
  layout->addRow(QStringLiteral("Y:"), lbl_pos_y_);

  lbl_pos_z_ = makePosLabel(grp_status_);
  lbl_pos_z_->setObjectName(QStringLiteral("lblPosZ"));
  layout->addRow(QStringLiteral("Z:"), lbl_pos_z_);

  lbl_state_text_ = new QLabel(
      QStringLiteral("Idle"), grp_status_);
  lbl_state_text_->setObjectName(
      QStringLiteral("lblStateText"));
  layout->addRow(QStringLiteral("State:"), lbl_state_text_);

  return grp_status_;
}

QPushButton* StagePanel::createEmergencyStopButton() {
  btn_emergency_stop_ = new QPushButton(
      QStringLiteral("EMERGENCY STOP"), this);
  btn_emergency_stop_->setObjectName(
      QStringLiteral("btnEmergencyStop"));
  btn_emergency_stop_->setMinimumHeight(kEmergencyStopHeight);
  btn_emergency_stop_->setSizePolicy(
      QSizePolicy::Expanding, QSizePolicy::Fixed);

  QFont stop_font = btn_emergency_stop_->font();
  stop_font.setBold(true);
  stop_font.setPointSize(14);
  btn_emergency_stop_->setFont(stop_font);

  btn_emergency_stop_->setStyleSheet(
      QStringLiteral(
          "QPushButton {"
          "  background-color: #E74C3C;"
          "  color: white;"
          "  font-weight: bold;"
          "}"
          "QPushButton:pressed {"
          "  background-color: #C0392B;"
          "}"));

  // Emergency stop is ALWAYS enabled — never disable it.
  btn_emergency_stop_->setEnabled(true);

  connect(btn_emergency_stop_, &QPushButton::clicked,
          this, &StagePanel::onEmergencyStop);

  return btn_emergency_stop_;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void StagePanel::applyStatusStyle(
    mwa::hardware::DeviceInterface::DeviceState new_state) {
  const char* color = kColorDisconnected;
  QString text;

  switch (new_state) {
    case mwa::hardware::DeviceInterface::DeviceState::kDisconnected:
      color = kColorDisconnected;
      text = QStringLiteral("\u25CF Disconnected");
      break;
    case mwa::hardware::DeviceInterface::DeviceState::kConnecting:
      color = kColorConnecting;
      text = QStringLiteral("\u25CF Connecting");
      break;
    case mwa::hardware::DeviceInterface::DeviceState::kConnected:
      color = kColorConnected;
      text = QStringLiteral("\u25CF Connected");
      break;
    case mwa::hardware::DeviceInterface::DeviceState::kError:
      color = kColorError;
      text = QStringLiteral("\u25CF Error");
      break;
  }

  lbl_status_->setText(text);
  lbl_status_->setStyleSheet(
      QStringLiteral("color: %1; font-weight: bold;")
          .arg(QString::fromLatin1(color)));
}

void StagePanel::setMotionControlsEnabled(bool enabled) {
  grp_jog_->setEnabled(enabled);
  grp_abs_position_->setEnabled(enabled);
  grp_speed_->setEnabled(enabled);
}

void StagePanel::setMoveButtonsEnabled(bool enabled) {
  btn_jog_x_plus_->setEnabled(enabled);
  btn_jog_x_minus_->setEnabled(enabled);
  btn_jog_y_plus_->setEnabled(enabled);
  btn_jog_y_minus_->setEnabled(enabled);
  btn_jog_z_plus_->setEnabled(enabled);
  btn_jog_z_minus_->setEnabled(enabled);
  btn_home_->setEnabled(enabled);
  btn_go_to_->setEnabled(enabled);
}

double StagePanel::currentStepSize() const {
  // Step values correspond to combo index order.
  static constexpr double kStepSizes[] = {
      0.01, 0.10, 1.00, 10.0};
  const int idx = cmb_step_size_->currentIndex();
  if (idx >= 0 &&
      idx < static_cast<int>(sizeof(kStepSizes) /
                              sizeof(kStepSizes[0]))) {
    return kStepSizes[idx];
  }
  return 1.0;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void StagePanel::onStateChanged(
    mwa::hardware::DeviceInterface::DeviceState new_state) {
  applyStatusStyle(new_state);

  const bool connected =
      (new_state ==
       mwa::hardware::DeviceInterface::DeviceState::kConnected);

  btn_connect_->setText(connected ? QStringLiteral("Disconnect")
                                  : QStringLiteral("Connect"));
  setMotionControlsEnabled(connected);

  if (!connected) {
    lbl_state_text_->setText(QStringLiteral("Idle"));
  }
}

void StagePanel::onPositionChanged(double x, double y,
                                   double z) {
  lbl_pos_x_->setText(
      QStringLiteral("%1 mm").arg(x, 0, 'f', 3));
  lbl_pos_y_->setText(
      QStringLiteral("%1 mm").arg(y, 0, 'f', 3));
  lbl_pos_z_->setText(
      QStringLiteral("%1 mm").arg(z, 0, 'f', 3));
}

void StagePanel::onMoveComplete() {
  setMoveButtonsEnabled(true);
  lbl_state_text_->setText(QStringLiteral("Idle"));
}

void StagePanel::onHomeComplete() {
  setMoveButtonsEnabled(true);
  lbl_state_text_->setText(QStringLiteral("Idle"));
}

void StagePanel::onConnectClicked() {
  if (controller_ == nullptr) {
    return;
  }

  const bool connected =
      controller_->state() ==
      mwa::hardware::DeviceInterface::DeviceState::kConnected;

  if (connected) {
    auto reply = QMessageBox::question(
        this,
        QStringLiteral("Disconnect Stage"),
        QStringLiteral(
            "Are you sure you want to disconnect the stage?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (reply != QMessageBox::Yes) {
      return;
    }
    controller_->disconnectDevice();
  } else {
    controller_->connectDevice();
  }
}

void StagePanel::onJogXPlus() {
  if (controller_ == nullptr) {
    return;
  }
  const double step = currentStepSize();
  setMoveButtonsEnabled(false);
  lbl_state_text_->setText(QStringLiteral("Moving"));
  controller_->moveRelative(step, 0.0, 0.0);
}

void StagePanel::onJogXMinus() {
  if (controller_ == nullptr) {
    return;
  }
  const double step = currentStepSize();
  setMoveButtonsEnabled(false);
  lbl_state_text_->setText(QStringLiteral("Moving"));
  controller_->moveRelative(-step, 0.0, 0.0);
}

void StagePanel::onJogYPlus() {
  if (controller_ == nullptr) {
    return;
  }
  const double step = currentStepSize();
  setMoveButtonsEnabled(false);
  lbl_state_text_->setText(QStringLiteral("Moving"));
  controller_->moveRelative(0.0, step, 0.0);
}

void StagePanel::onJogYMinus() {
  if (controller_ == nullptr) {
    return;
  }
  const double step = currentStepSize();
  setMoveButtonsEnabled(false);
  lbl_state_text_->setText(QStringLiteral("Moving"));
  controller_->moveRelative(0.0, -step, 0.0);
}

void StagePanel::onJogZPlus() {
  if (controller_ == nullptr) {
    return;
  }
  const double step = currentStepSize();
  setMoveButtonsEnabled(false);
  lbl_state_text_->setText(QStringLiteral("Moving"));
  controller_->moveRelative(0.0, 0.0, step);
}

void StagePanel::onJogZMinus() {
  if (controller_ == nullptr) {
    return;
  }
  const double step = currentStepSize();
  setMoveButtonsEnabled(false);
  lbl_state_text_->setText(QStringLiteral("Moving"));
  controller_->moveRelative(0.0, 0.0, -step);
}

void StagePanel::onHomeClicked() {
  if (controller_ == nullptr) {
    return;
  }
  auto reply = QMessageBox::question(
      this,
      QStringLiteral("Home Stage"),
      QStringLiteral(
          "Home all axes? The stage will move to its "
          "home position."),
      QMessageBox::Yes | QMessageBox::No,
      QMessageBox::No);
  if (reply != QMessageBox::Yes) {
    return;
  }
  setMoveButtonsEnabled(false);
  lbl_state_text_->setText(QStringLiteral("Homing"));
  controller_->home();
}

void StagePanel::onGoToClicked() {
  if (controller_ == nullptr) {
    return;
  }
  const double tx = spn_abs_x_->value();
  const double ty = spn_abs_y_->value();
  const double tz = spn_abs_z_->value();

  auto reply = QMessageBox::question(
      this,
      QStringLiteral("Move to Absolute Position"),
      QStringLiteral(
          "Move stage to:\n"
          "  X = %1 mm\n"
          "  Y = %2 mm\n"
          "  Z = %3 mm\n\n"
          "Proceed?")
          .arg(tx, 0, 'f', 3)
          .arg(ty, 0, 'f', 3)
          .arg(tz, 0, 'f', 3),
      QMessageBox::Yes | QMessageBox::No,
      QMessageBox::No);
  if (reply != QMessageBox::Yes) {
    return;
  }
  setMoveButtonsEnabled(false);
  lbl_state_text_->setText(QStringLiteral("Moving"));
  controller_->moveAbsolute(tx, ty, tz);
}

void StagePanel::onEmergencyStop() {
  if (controller_ == nullptr) {
    return;
  }
  controller_->stopMotion();
  setMoveButtonsEnabled(true);
  lbl_state_text_->setText(QStringLiteral("Idle"));
}

void StagePanel::onSpeedDebounced() {
  if (controller_ == nullptr) {
    return;
  }
  controller_->setSpeed(spn_speed_->value());
}

}  // namespace mwa::gui
