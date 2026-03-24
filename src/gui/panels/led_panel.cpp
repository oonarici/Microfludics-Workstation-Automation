/**
 * @file led_panel.cpp
 * @brief Implementation of the LedPanel widget.
 * @author MWA Team
 * @date 2026-03-23
 *
 * @copyright LGPL-3.0-or-later
 */

#include "gui/panels/led_panel.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QVBoxLayout>

namespace mwa::gui {

// ---- Construction ---------------------------------------------------------

LedPanel::LedPanel(QWidget* parent) : QWidget(parent) {
  setObjectName(QStringLiteral("ledPanel"));

  auto* root_layout = new QVBoxLayout(this);
  root_layout->setContentsMargins(8, 8, 8, 8);
  root_layout->setSpacing(12);

  root_layout->addWidget(createConnectionGroup());
  root_layout->addWidget(createControlsGroup());
  root_layout->addWidget(createStatusGroup());
  root_layout->addStretch();
}

// ---- Public API -----------------------------------------------------------

void LedPanel::setController(
    mwa::hardware::LedControllerInterface* controller) {
  // Disconnect old controller signals if any.
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
          this, &LedPanel::onStateChanged);

  connect(controller_,
          &mwa::hardware::LedControllerInterface::intensityChanged,
          this, &LedPanel::onIntensityChanged);

  connect(controller_,
          &mwa::hardware::LedControllerInterface::powerStateChanged,
          this, &LedPanel::onPowerStateChanged);

  // Sync UI to current controller state on attachment.
  onStateChanged(controller_->state());
  onIntensityChanged(controller_->intensity());
  onPowerStateChanged(controller_->isPowerOn());
}

// ---- Private builders -----------------------------------------------------

QGroupBox* LedPanel::createConnectionGroup() {
  grp_connection_ = new QGroupBox(
      QStringLiteral("Connection"), this);
  grp_connection_->setObjectName(
      QStringLiteral("grpConnection"));

  auto* layout = new QFormLayout(grp_connection_);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(8);

  cmb_device_ = new QComboBox(grp_connection_);
  cmb_device_->setObjectName(QStringLiteral("cmbDevice"));
  cmb_device_->addItem(QStringLiteral("Mock LED"));
  layout->addRow(QStringLiteral("Device:"), cmb_device_);

  lbl_status_ = new QLabel(
      QStringLiteral("\u25CF Disconnected"), grp_connection_);
  lbl_status_->setObjectName(QStringLiteral("lblStatus"));
  applyStatusStyle(
      mwa::hardware::DeviceInterface::DeviceState::kDisconnected);
  layout->addRow(QStringLiteral("Status:"), lbl_status_);

  btn_connect_ = new QPushButton(
      QStringLiteral("Connect"), grp_connection_);
  btn_connect_->setObjectName(QStringLiteral("btnConnect"));
  btn_connect_->setMinimumHeight(32);
  layout->addRow(btn_connect_);

  connect(btn_connect_, &QPushButton::clicked,
          this, &LedPanel::onConnectClicked);

  return grp_connection_;
}

QGroupBox* LedPanel::createControlsGroup() {
  grp_controls_ = new QGroupBox(
      QStringLiteral("Controls"), this);
  grp_controls_->setObjectName(QStringLiteral("grpControls"));
  grp_controls_->setEnabled(false);

  auto* layout = new QVBoxLayout(grp_controls_);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(8);

  // Power toggle button.
  btn_power_ = new QPushButton(
      QStringLiteral("LED OFF"), grp_controls_);
  btn_power_->setObjectName(QStringLiteral("btnPower"));
  btn_power_->setCheckable(true);
  btn_power_->setChecked(false);
  btn_power_->setMinimumHeight(36);
  layout->addWidget(btn_power_);

  // Intensity spinbox row.
  auto* intensity_row = new QWidget(grp_controls_);
  auto* intensity_row_layout = new QHBoxLayout(intensity_row);
  intensity_row_layout->setContentsMargins(0, 0, 0, 0);
  intensity_row_layout->setSpacing(8);

  auto* lbl_intensity = new QLabel(
      QStringLiteral("Intensity:"), intensity_row);
  lbl_intensity->setObjectName(QStringLiteral("lblIntensity"));
  intensity_row_layout->addWidget(lbl_intensity);

  spn_intensity_ = new QDoubleSpinBox(intensity_row);
  spn_intensity_->setObjectName(
      QStringLiteral("spnIntensity"));
  spn_intensity_->setRange(0.0, 100.0);
  spn_intensity_->setDecimals(1);
  spn_intensity_->setSingleStep(1.0);
  spn_intensity_->setSuffix(QStringLiteral(" %"));
  spn_intensity_->setMinimumWidth(100);
  intensity_row_layout->addWidget(spn_intensity_);
  intensity_row_layout->addStretch();

  layout->addWidget(intensity_row);

  // Intensity slider.
  sld_intensity_ = new QSlider(Qt::Horizontal, grp_controls_);
  sld_intensity_->setObjectName(
      QStringLiteral("sldIntensity"));
  sld_intensity_->setRange(0, 1000);
  sld_intensity_->setValue(0);
  layout->addWidget(sld_intensity_);

  // Internal connections: slider ↔ spinbox display sync.
  connect(sld_intensity_, &QSlider::valueChanged,
          this, &LedPanel::onSliderValueChanged);
  connect(sld_intensity_, &QSlider::sliderReleased,
          this, &LedPanel::onSliderReleased);
  connect(spn_intensity_,
          &QDoubleSpinBox::editingFinished,
          this, &LedPanel::onSpinboxEditingFinished);
  connect(btn_power_, &QPushButton::toggled,
          this, &LedPanel::onPowerToggled);

  return grp_controls_;
}

QGroupBox* LedPanel::createStatusGroup() {
  grp_status_ = new QGroupBox(
      QStringLiteral("Status"), this);
  grp_status_->setObjectName(QStringLiteral("grpStatus"));

  auto* layout = new QFormLayout(grp_status_);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(8);

  // Power state label.
  lbl_power_value_ = new QLabel(
      QStringLiteral("\u25CF OFF"), grp_status_);
  lbl_power_value_->setObjectName(
      QStringLiteral("lblPowerValue"));
  lbl_power_value_->setStyleSheet(
      QStringLiteral("color: %1; font-weight: bold;")
          .arg(QLatin1String(kColorDisconnected)));
  layout->addRow(QStringLiteral("Power:"),
                 lbl_power_value_);

  // LCD intensity readout.
  lcd_intensity_ = new QLCDNumber(5, grp_status_);
  lcd_intensity_->setObjectName(
      QStringLiteral("lcdIntensity"));
  lcd_intensity_->setSegmentStyle(QLCDNumber::Flat);
  lcd_intensity_->display(0.0);
  lcd_intensity_->setMinimumHeight(40);
  layout->addRow(QStringLiteral("Intensity:"),
                 lcd_intensity_);

  // Device state text.
  lbl_device_state_ = new QLabel(
      QStringLiteral("Disconnected"), grp_status_);
  lbl_device_state_->setObjectName(
      QStringLiteral("lblDeviceState"));
  layout->addRow(QStringLiteral("Device:"),
                 lbl_device_state_);

  return grp_status_;
}

// ---- Private helpers ------------------------------------------------------

void LedPanel::applyStatusStyle(
    mwa::hardware::DeviceInterface::DeviceState new_state) {
  using DeviceState =
      mwa::hardware::DeviceInterface::DeviceState;

  const char* color = kColorDisconnected;
  switch (new_state) {
    case DeviceState::kConnected:
      color = kColorConnected;
      break;
    case DeviceState::kConnecting:
      color = kColorConnecting;
      break;
    case DeviceState::kError:
      color = kColorError;
      break;
    case DeviceState::kDisconnected:
    default:
      color = kColorDisconnected;
      break;
  }

  if (lbl_status_ != nullptr) {
    lbl_status_->setStyleSheet(
        QStringLiteral("color: %1; font-weight: bold;")
            .arg(QLatin1String(color)));
  }
}

// ---- Slots ----------------------------------------------------------------

void LedPanel::onStateChanged(
    mwa::hardware::DeviceInterface::DeviceState new_state) {
  using DeviceState =
      mwa::hardware::DeviceInterface::DeviceState;

  applyStatusStyle(new_state);

  QString status_text;
  QString state_text;
  bool is_connected = (new_state == DeviceState::kConnected);

  switch (new_state) {
    case DeviceState::kConnected:
      status_text = QStringLiteral("\u25CF Connected");
      state_text  = QStringLiteral("Connected");
      btn_connect_->setText(QStringLiteral("Disconnect"));
      break;
    case DeviceState::kConnecting:
      status_text = QStringLiteral("\u25CF Connecting...");
      state_text  = QStringLiteral("Connecting");
      btn_connect_->setText(QStringLiteral("Connect"));
      btn_connect_->setEnabled(false);
      break;
    case DeviceState::kError:
      status_text = QStringLiteral("\u25CF Error");
      state_text  = QStringLiteral("Error");
      btn_connect_->setText(QStringLiteral("Connect"));
      btn_connect_->setEnabled(true);
      break;
    case DeviceState::kDisconnected:
    default:
      status_text = QStringLiteral("\u25CF Disconnected");
      state_text  = QStringLiteral("Disconnected");
      btn_connect_->setText(QStringLiteral("Connect"));
      btn_connect_->setEnabled(true);
      break;
  }

  lbl_status_->setText(status_text);
  lbl_device_state_->setText(state_text);
  grp_controls_->setEnabled(is_connected);

  // Re-enable connect button after connecting state resolves.
  if (new_state != DeviceState::kConnecting) {
    btn_connect_->setEnabled(true);
  }
}

void LedPanel::onIntensityChanged(double percent) {
  lcd_intensity_->display(percent);

  // Sync spinbox without triggering editingFinished.
  spn_intensity_->blockSignals(true);
  spn_intensity_->setValue(percent);
  spn_intensity_->blockSignals(false);

  // Sync slider without triggering sliderReleased.
  sld_intensity_->blockSignals(true);
  sld_intensity_->setValue(
      static_cast<int>(percent * kSliderScale));
  sld_intensity_->blockSignals(false);
}

void LedPanel::onPowerStateChanged(bool on) {
  // Sync button without re-triggering toggled().
  btn_power_->blockSignals(true);
  btn_power_->setChecked(on);
  btn_power_->setText(on ? QStringLiteral("LED ON")
                         : QStringLiteral("LED OFF"));
  btn_power_->blockSignals(false);

  const char* color =
      on ? kColorConnected : kColorDisconnected;
  lbl_power_value_->setStyleSheet(
      QStringLiteral("color: %1; font-weight: bold;")
          .arg(QLatin1String(color)));
  lbl_power_value_->setText(
      on ? QStringLiteral("\u25CF ON")
         : QStringLiteral("\u25CF OFF"));
}

void LedPanel::onConnectClicked() {
  if (controller_ == nullptr) {
    return;
  }

  using DeviceState =
      mwa::hardware::DeviceInterface::DeviceState;

  if (controller_->state() == DeviceState::kConnected) {
    auto reply = QMessageBox::question(
        this,
        QStringLiteral("Disconnect LED"),
        QStringLiteral(
            "Disconnect from the LED controller?"),
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

void LedPanel::onPowerToggled(bool checked) {
  if (controller_ == nullptr) {
    return;
  }
  controller_->setPowerOn(checked);
}

void LedPanel::onSpinboxEditingFinished() {
  if (controller_ == nullptr) {
    return;
  }
  const double value = spn_intensity_->value();

  // Keep slider display in sync (no sliderReleased triggered).
  sld_intensity_->blockSignals(true);
  sld_intensity_->setValue(
      static_cast<int>(value * kSliderScale));
  sld_intensity_->blockSignals(false);

  controller_->setIntensity(value);
}

void LedPanel::onSliderReleased() {
  if (controller_ == nullptr) {
    return;
  }
  const double percent =
      static_cast<double>(sld_intensity_->value()) /
      kSliderScale;
  controller_->setIntensity(percent);
}

void LedPanel::onSliderValueChanged(int value) {
  // Live display update only — no hardware call.
  const double percent =
      static_cast<double>(value) / kSliderScale;
  spn_intensity_->blockSignals(true);
  spn_intensity_->setValue(percent);
  spn_intensity_->blockSignals(false);
}

}  // namespace mwa::gui
