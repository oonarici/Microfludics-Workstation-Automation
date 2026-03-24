/**
 * @file pump_panel.cpp
 * @brief Implementation of the PumpPanel widget.
 * @author MWA Team
 * @date 2026-03-23
 *
 * @copyright LGPL-3.0-or-later
 */

#include "gui/panels/pump_panel.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>

namespace mwa::gui {

// ---- Construction ---------------------------------------------------------

PumpPanel::PumpPanel(QWidget* parent) : QWidget(parent) {
  setObjectName(QStringLiteral("pumpPanel"));

  auto* root_layout = new QVBoxLayout(this);
  root_layout->setContentsMargins(8, 8, 8, 8);
  root_layout->setSpacing(12);

  root_layout->addWidget(createConnectionGroup());
  root_layout->addWidget(createControlsGroup());
  root_layout->addWidget(createStatusGroup());
  root_layout->addStretch();
}

// ---- Public API -----------------------------------------------------------

void PumpPanel::setController(
    mwa::hardware::PumpControllerInterface* controller) {
  if (controller_ != nullptr) {
    disconnect(controller_, nullptr, this, nullptr);
  }

  controller_ = controller;

  if (controller_ == nullptr) {
    grp_controls_->setEnabled(false);
    return;
  }

  connect(controller_,
          &mwa::hardware::DeviceInterface::stateChanged,
          this, &PumpPanel::onStateChanged);

  connect(controller_,
          &mwa::hardware::PumpControllerInterface::positionChanged,
          this, &PumpPanel::onPositionChanged);

  connect(controller_,
          &mwa::hardware::PumpControllerInterface::flowRateChanged,
          this, &PumpPanel::onFlowRateChanged);

  connect(controller_,
          &mwa::hardware::PumpControllerInterface::infusionStarted,
          this, &PumpPanel::onInfusionStarted);

  connect(controller_,
          &mwa::hardware::PumpControllerInterface::infusionStopped,
          this, &PumpPanel::onInfusionStopped);

  connect(controller_,
          &mwa::hardware::DeviceInterface::errorOccurred,
          this, &PumpPanel::onErrorOccurred);

  // Sync UI to current controller state on attachment.
  onStateChanged(controller_->state());
  onPositionChanged(controller_->currentPosition());
  onFlowRateChanged(controller_->flowRate());

  if (controller_->isInfusing()) {
    onInfusionStarted();
  }
}

// ---- Private builders -----------------------------------------------------

QGroupBox* PumpPanel::createConnectionGroup() {
  grp_connection_ = new QGroupBox(
      QStringLiteral("Connection"), this);
  grp_connection_->setObjectName(
      QStringLiteral("grpConnection"));

  auto* layout = new QFormLayout(grp_connection_);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(8);

  cmb_port_ = new QComboBox(grp_connection_);
  cmb_port_->setObjectName(QStringLiteral("cmbPort"));
  cmb_port_->addItem(QStringLiteral("Mock Pump"));
  layout->addRow(QStringLiteral("Port:"), cmb_port_);

  auto* status_row = new QWidget(grp_connection_);
  auto* status_row_layout = new QHBoxLayout(status_row);
  status_row_layout->setContentsMargins(0, 0, 0, 0);
  status_row_layout->setSpacing(6);

  lbl_status_dot_ = new QLabel(status_row);
  lbl_status_dot_->setObjectName(
      QStringLiteral("lblStatusDot"));
  lbl_status_dot_->setFixedSize(kDotSize, kDotSize);
  applyStatusDotColor(
      QLatin1String(kColorDisconnected));
  status_row_layout->addWidget(lbl_status_dot_);

  lbl_status_text_ = new QLabel(
      QStringLiteral("Disconnected"), status_row);
  lbl_status_text_->setObjectName(
      QStringLiteral("lblStatusText"));
  status_row_layout->addWidget(lbl_status_text_);
  status_row_layout->addStretch();

  layout->addRow(QStringLiteral("Status:"), status_row);

  btn_connect_ = new QPushButton(
      QStringLiteral("Connect"), grp_connection_);
  btn_connect_->setObjectName(QStringLiteral("btnConnect"));
  btn_connect_->setMinimumHeight(32);
  layout->addRow(btn_connect_);

  connect(btn_connect_, &QPushButton::clicked,
          this, &PumpPanel::onConnectClicked);

  return grp_connection_;
}

QGroupBox* PumpPanel::createControlsGroup() {
  grp_controls_ = new QGroupBox(
      QStringLiteral("Controls"), this);
  grp_controls_->setObjectName(
      QStringLiteral("grpControls"));
  grp_controls_->setEnabled(false);

  auto* layout = new QFormLayout(grp_controls_);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(8);

  spn_flow_rate_ = new QDoubleSpinBox(grp_controls_);
  spn_flow_rate_->setObjectName(
      QStringLiteral("spnFlowRate"));
  spn_flow_rate_->setRange(0.01, 1000.00);
  spn_flow_rate_->setDecimals(2);
  spn_flow_rate_->setSuffix(
      QStringLiteral(" \u00B5L/min"));
  spn_flow_rate_->setValue(1.00);
  layout->addRow(QStringLiteral("Flow Rate:"),
                 spn_flow_rate_);

  spn_volume_ = new QDoubleSpinBox(grp_controls_);
  spn_volume_->setObjectName(QStringLiteral("spnVolume"));
  spn_volume_->setRange(0.01, 10000.00);
  spn_volume_->setDecimals(2);
  spn_volume_->setSuffix(
      QStringLiteral(" \u00B5L"));
  spn_volume_->setValue(10.00);
  layout->addRow(QStringLiteral("Target Volume:"),
                 spn_volume_);

  auto* btn_row = new QWidget(grp_controls_);
  auto* btn_row_layout = new QHBoxLayout(btn_row);
  btn_row_layout->setContentsMargins(0, 0, 0, 0);
  btn_row_layout->setSpacing(6);

  btn_start_ = new QPushButton(
      QStringLiteral("Start Infusion"), btn_row);
  btn_start_->setObjectName(QStringLiteral("btnStart"));
  btn_start_->setMinimumHeight(32);
  btn_row_layout->addWidget(btn_start_);

  btn_stop_ = new QPushButton(
      QStringLiteral("Stop"), btn_row);
  btn_stop_->setObjectName(QStringLiteral("btnStop"));
  btn_stop_->setMinimumHeight(32);
  btn_stop_->setEnabled(false);
  btn_row_layout->addWidget(btn_stop_);

  btn_refill_ = new QPushButton(
      QStringLiteral("Refill"), btn_row);
  btn_refill_->setObjectName(QStringLiteral("btnRefill"));
  btn_refill_->setMinimumHeight(32);
  btn_row_layout->addWidget(btn_refill_);

  layout->addRow(btn_row);

  connect(spn_flow_rate_,
          &QDoubleSpinBox::valueChanged,
          this, &PumpPanel::onFlowRateValueChanged);
  connect(spn_volume_,
          &QDoubleSpinBox::valueChanged,
          this, &PumpPanel::onVolumeValueChanged);
  connect(btn_start_, &QPushButton::clicked,
          this, &PumpPanel::onStartClicked);
  connect(btn_stop_, &QPushButton::clicked,
          this, &PumpPanel::onStopClicked);
  connect(btn_refill_, &QPushButton::clicked,
          this, &PumpPanel::onRefillClicked);

  return grp_controls_;
}

QGroupBox* PumpPanel::createStatusGroup() {
  grp_status_ = new QGroupBox(
      QStringLiteral("Status"), this);
  grp_status_->setObjectName(QStringLiteral("grpStatus"));

  auto* layout = new QFormLayout(grp_status_);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(8);

  lcd_position_ = new QLCDNumber(7, grp_status_);
  lcd_position_->setObjectName(
      QStringLiteral("lcdPosition"));
  lcd_position_->setSegmentStyle(QLCDNumber::Flat);
  lcd_position_->display(0.0);
  lcd_position_->setMinimumHeight(40);
  layout->addRow(QStringLiteral("Position (\u00B5L):"),
                 lcd_position_);

  auto* state_row = new QWidget(grp_status_);
  auto* state_row_layout = new QHBoxLayout(state_row);
  state_row_layout->setContentsMargins(0, 0, 0, 0);
  state_row_layout->setSpacing(6);

  lbl_state_dot_ = new QLabel(state_row);
  lbl_state_dot_->setObjectName(
      QStringLiteral("lblStateDot"));
  lbl_state_dot_->setFixedSize(kDotSize, kDotSize);
  lbl_state_dot_->setStyleSheet(
      QStringLiteral(
          "background-color: %1; border-radius: 6px;")
          .arg(QLatin1String(kColorDisconnected)));
  state_row_layout->addWidget(lbl_state_dot_);

  lbl_state_text_ = new QLabel(
      QStringLiteral("Idle"), state_row);
  lbl_state_text_->setObjectName(
      QStringLiteral("lblStateText"));
  state_row_layout->addWidget(lbl_state_text_);
  state_row_layout->addStretch();

  layout->addRow(QStringLiteral("State:"), state_row);

  prg_infusion_ = new QProgressBar(grp_status_);
  prg_infusion_->setObjectName(
      QStringLiteral("prgInfusion"));
  prg_infusion_->setRange(0, 100);
  prg_infusion_->setValue(0);
  prg_infusion_->setTextVisible(true);
  layout->addRow(QStringLiteral("Progress:"),
                 prg_infusion_);

  lbl_error_ = new QLabel(grp_status_);
  lbl_error_->setObjectName(QStringLiteral("lblError"));
  lbl_error_->setStyleSheet(
      QStringLiteral("color: %1; font-weight: bold;")
          .arg(QLatin1String(kColorError)));
  lbl_error_->setWordWrap(true);
  lbl_error_->setVisible(false);
  layout->addRow(lbl_error_);

  return grp_status_;
}

// ---- Private helpers ------------------------------------------------------

void PumpPanel::applyStatusDotColor(const QString& color) {
  lbl_status_dot_->setStyleSheet(
      QStringLiteral(
          "background-color: %1; border-radius: 6px;")
          .arg(color));
}

void PumpPanel::updateSubState(const QString& color,
                               const QString& text) {
  lbl_state_dot_->setStyleSheet(
      QStringLiteral(
          "background-color: %1; border-radius: 6px;")
          .arg(color));
  lbl_state_text_->setText(text);
}

void PumpPanel::updateButtonStates(bool is_connected,
                                   bool is_infusing,
                                   bool is_refilling) {
  const bool busy = is_infusing || is_refilling;
  btn_start_->setEnabled(is_connected && !busy);
  btn_stop_->setEnabled(is_connected && is_infusing);
  btn_refill_->setEnabled(is_connected && !busy);
  spn_flow_rate_->setEnabled(is_connected && !busy);
  spn_volume_->setEnabled(is_connected && !busy);
}

// ---- Slots ----------------------------------------------------------------

void PumpPanel::onStateChanged(
    mwa::hardware::DeviceInterface::DeviceState new_state) {
  using DeviceState =
      mwa::hardware::DeviceInterface::DeviceState;

  QString dot_color;
  QString status_text;
  bool is_connected =
      (new_state == DeviceState::kConnected);

  switch (new_state) {
    case DeviceState::kConnected:
      dot_color   = QLatin1String(kColorConnected);
      status_text = QStringLiteral("Connected");
      btn_connect_->setText(QStringLiteral("Disconnect"));
      btn_connect_->setEnabled(true);
      updateSubState(QLatin1String(kColorConnected),
                     QStringLiteral("Idle"));
      break;
    case DeviceState::kConnecting:
      dot_color   = QLatin1String(kColorConnecting);
      status_text = QStringLiteral("Connecting");
      btn_connect_->setText(QStringLiteral("Connect"));
      btn_connect_->setEnabled(false);
      break;
    case DeviceState::kError:
      dot_color   = QLatin1String(kColorError);
      status_text = QStringLiteral("Error");
      btn_connect_->setText(QStringLiteral("Connect"));
      btn_connect_->setEnabled(true);
      updateSubState(QLatin1String(kColorError),
                     QStringLiteral("Error"));
      break;
    case DeviceState::kDisconnected:
    default:
      dot_color   = QLatin1String(kColorDisconnected);
      status_text = QStringLiteral("Disconnected");
      btn_connect_->setText(QStringLiteral("Connect"));
      btn_connect_->setEnabled(true);
      updateSubState(QLatin1String(kColorDisconnected),
                     QStringLiteral("Idle"));
      break;
  }

  applyStatusDotColor(dot_color);
  lbl_status_text_->setText(status_text);
  grp_controls_->setEnabled(is_connected);

  if (!is_connected) {
    is_refilling_ = false;
    prg_infusion_->setValue(0);
    lbl_error_->setVisible(false);
    updateButtonStates(false, false, false);
  } else {
    updateButtonStates(true, false, false);
  }
}

void PumpPanel::onPositionChanged(double uL) {
  lcd_position_->display(uL);

  // Update progress bar relative to current target volume.
  if (controller_ != nullptr) {
    const double target = controller_->targetVolume();
    if (target > 0.0) {
      const int progress =
          static_cast<int>((uL / target) * 100.0);
      prg_infusion_->setValue(
          qBound(0, progress, 100));
    }
  }
}

void PumpPanel::onFlowRateChanged(double uL_per_min) {
  spn_flow_rate_->blockSignals(true);
  spn_flow_rate_->setValue(uL_per_min);
  spn_flow_rate_->blockSignals(false);
}

void PumpPanel::onInfusionStarted() {
  is_refilling_ = false;
  lbl_error_->setVisible(false);
  updateSubState(QLatin1String(kColorActive),
                 QStringLiteral("Infusing"));
  updateButtonStates(true, true, false);
}

void PumpPanel::onInfusionStopped() {
  is_refilling_ = false;
  updateSubState(QLatin1String(kColorConnected),
                 QStringLiteral("Idle"));
  prg_infusion_->setValue(0);
  updateButtonStates(true, false, false);
}

void PumpPanel::onErrorOccurred(const QString& message) {
  lbl_error_->setText(message);
  lbl_error_->setVisible(true);
  updateSubState(QLatin1String(kColorError),
                 QStringLiteral("Error"));
  is_refilling_ = false;
  using DevState =
      mwa::hardware::DeviceInterface::DeviceState;
  const bool connected =
      controller_ != nullptr &&
      controller_->state() == DevState::kConnected;
  updateButtonStates(connected, false, false);
}

void PumpPanel::onConnectClicked() {
  if (controller_ == nullptr) {
    return;
  }

  using DeviceState =
      mwa::hardware::DeviceInterface::DeviceState;

  if (controller_->state() == DeviceState::kConnected) {
    auto reply = QMessageBox::question(
        this,
        QStringLiteral("Disconnect Pump"),
        QStringLiteral(
            "Disconnect from the pump controller?"),
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

void PumpPanel::onFlowRateValueChanged(double value) {
  if (controller_ == nullptr) {
    return;
  }
  controller_->setFlowRate(value);
}

void PumpPanel::onVolumeValueChanged(double value) {
  if (controller_ == nullptr) {
    return;
  }
  controller_->setTargetVolume(value);
}

void PumpPanel::onStartClicked() {
  if (controller_ == nullptr) {
    return;
  }
  lbl_error_->setVisible(false);
  prg_infusion_->setValue(0);
  controller_->startInfusion();
}

void PumpPanel::onStopClicked() {
  if (controller_ == nullptr) {
    return;
  }
  auto reply = QMessageBox::question(
      this,
      QStringLiteral("Stop Infusion"),
      QStringLiteral("Stop the active infusion?"),
      QMessageBox::Yes | QMessageBox::No,
      QMessageBox::No);
  if (reply != QMessageBox::Yes) {
    return;
  }
  controller_->stopInfusion();
}

void PumpPanel::onRefillClicked() {
  if (controller_ == nullptr) {
    return;
  }
  auto reply = QMessageBox::question(
      this,
      QStringLiteral("Refill Syringe"),
      QStringLiteral(
          "Retract the plunger to refill the syringe?"),
      QMessageBox::Yes | QMessageBox::No,
      QMessageBox::No);
  if (reply != QMessageBox::Yes) {
    return;
  }
  is_refilling_ = true;
  lbl_error_->setVisible(false);
  updateSubState(QLatin1String(kColorActive),
                 QStringLiteral("Refilling"));
  updateButtonStates(true, false, true);
  controller_->refill();
}

}  // namespace mwa::gui
