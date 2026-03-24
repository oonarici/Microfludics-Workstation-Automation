/**
 * @file camera_panel.cpp
 * @brief Implementation of the CameraPanel widget.
 * @author MWA Team
 * @date 2026-03-23
 *
 * @copyright LGPL-3.0-or-later
 */

#include "gui/panels/camera_panel.h"

#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPixmap>
#include <QVBoxLayout>

namespace mwa::gui {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

CameraPanel::CameraPanel(QWidget* parent) : QWidget(parent) {
  setObjectName(QStringLiteral("cameraPanel"));

  auto* root_layout = new QVBoxLayout(this);
  root_layout->setContentsMargins(8, 8, 8, 8);
  root_layout->setSpacing(12);

  root_layout->addWidget(createConnectionGroup());
  root_layout->addWidget(createAcquisitionGroup());
  root_layout->addWidget(createCaptureGroup());
  root_layout->addWidget(createStatusGroup());
  root_layout->addStretch();

  // Debounce timers — single-shot, fire once after the last edit.
  timer_exposure_ = new QTimer(this);
  timer_exposure_->setSingleShot(true);
  timer_exposure_->setInterval(kDebounceMs);

  timer_gain_ = new QTimer(this);
  timer_gain_->setSingleShot(true);
  timer_gain_->setInterval(kDebounceMs);

  timer_roi_ = new QTimer(this);
  timer_roi_->setSingleShot(true);
  timer_roi_->setInterval(kDebounceMs);

  connect(timer_exposure_, &QTimer::timeout,
          this, &CameraPanel::onExposureDebounced);
  connect(timer_gain_, &QTimer::timeout,
          this, &CameraPanel::onGainDebounced);
  connect(timer_roi_, &QTimer::timeout,
          this, &CameraPanel::onRoiDebounced);

  // Start the preview throttle timer in an invalidated state.
  preview_throttle_.invalidate();

  setControlsEnabled(false);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void CameraPanel::setController(
    mwa::hardware::CameraControllerInterface* controller) {
  if (controller_ != nullptr) {
    disconnect(controller_, nullptr, this, nullptr);
  }

  controller_ = controller;

  if (controller_ == nullptr) {
    setControlsEnabled(false);
    return;
  }

  connect(controller_,
          &mwa::hardware::CameraControllerInterface::stateChanged,
          this, &CameraPanel::onStateChanged);
  connect(controller_,
          &mwa::hardware::CameraControllerInterface::frameReady,
          this, &CameraPanel::onFrameReady);
  connect(controller_,
          &mwa::hardware::CameraControllerInterface::batchComplete,
          this, &CameraPanel::onBatchComplete);

  // Sync UI to current controller state.
  onStateChanged(controller_->state());
}

// ---------------------------------------------------------------------------
// Group-box builders
// ---------------------------------------------------------------------------

QGroupBox* CameraPanel::createConnectionGroup() {
  grp_connection_ = new QGroupBox(
      QStringLiteral("Connection"), this);
  grp_connection_->setObjectName(
      QStringLiteral("grpConnection"));

  auto* layout = new QHBoxLayout(grp_connection_);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(8);

  cmb_camera_ = new QComboBox(grp_connection_);
  cmb_camera_->setObjectName(QStringLiteral("cmbCamera"));
  cmb_camera_->addItem(QStringLiteral("Mock Camera"));
  layout->addWidget(cmb_camera_);

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
          this, &CameraPanel::onConnectClicked);

  return grp_connection_;
}

QGroupBox* CameraPanel::createAcquisitionGroup() {
  grp_acquisition_ = new QGroupBox(
      QStringLiteral("Acquisition Settings"), this);
  grp_acquisition_->setObjectName(
      QStringLiteral("grpAcquisition"));

  auto* outer = new QVBoxLayout(grp_acquisition_);
  outer->setContentsMargins(8, 8, 8, 8);
  outer->setSpacing(8);

  // Exposure and gain form.
  auto* form = new QFormLayout();
  form->setSpacing(6);

  spn_exposure_ = new QDoubleSpinBox(grp_acquisition_);
  spn_exposure_->setObjectName(QStringLiteral("spnExposure"));
  spn_exposure_->setRange(0.001, 10000.0);
  spn_exposure_->setDecimals(3);
  spn_exposure_->setSuffix(QStringLiteral(" ms"));
  spn_exposure_->setValue(10.0);
  form->addRow(QStringLiteral("Exposure:"), spn_exposure_);

  spn_gain_ = new QDoubleSpinBox(grp_acquisition_);
  spn_gain_->setObjectName(QStringLiteral("spnGain"));
  spn_gain_->setRange(1.0, 48.0);
  spn_gain_->setDecimals(3);
  spn_gain_->setSuffix(QStringLiteral(" \u00D7"));
  spn_gain_->setValue(1.0);
  form->addRow(QStringLiteral("Gain:"), spn_gain_);

  outer->addLayout(form);

  // ROI nested group box.
  auto* roi_group = new QGroupBox(
      QStringLiteral("Region of Interest"), grp_acquisition_);
  roi_group->setObjectName(QStringLiteral("grpRoi"));

  auto* roi_layout = new QGridLayout(roi_group);
  roi_layout->setContentsMargins(8, 8, 8, 8);
  roi_layout->setSpacing(6);

  auto makeSpinBox = [](QWidget* parent, int min,
                         int max) -> QSpinBox* {
    auto* spn = new QSpinBox(parent);
    spn->setRange(min, max);
    spn->setValue(0);
    return spn;
  };

  spn_roi_x_ = makeSpinBox(roi_group, 0, 9999);
  spn_roi_x_->setObjectName(QStringLiteral("spnRoiX"));
  spn_roi_y_ = makeSpinBox(roi_group, 0, 9999);
  spn_roi_y_->setObjectName(QStringLiteral("spnRoiY"));
  spn_roi_w_ = makeSpinBox(roi_group, 1, 9999);
  spn_roi_w_->setObjectName(QStringLiteral("spnRoiW"));
  spn_roi_w_->setValue(kDefaultRoiWidth);
  spn_roi_h_ = makeSpinBox(roi_group, 1, 9999);
  spn_roi_h_->setObjectName(QStringLiteral("spnRoiH"));
  spn_roi_h_->setValue(kDefaultRoiHeight);

  roi_layout->addWidget(new QLabel(QStringLiteral("X:"),
                                   roi_group), 0, 0);
  roi_layout->addWidget(spn_roi_x_, 0, 1);
  roi_layout->addWidget(new QLabel(QStringLiteral("Y:"),
                                   roi_group), 0, 2);
  roi_layout->addWidget(spn_roi_y_, 0, 3);
  roi_layout->addWidget(new QLabel(QStringLiteral("W:"),
                                   roi_group), 1, 0);
  roi_layout->addWidget(spn_roi_w_, 1, 1);
  roi_layout->addWidget(new QLabel(QStringLiteral("H:"),
                                   roi_group), 1, 2);
  roi_layout->addWidget(spn_roi_h_, 1, 3);

  btn_reset_roi_ = new QPushButton(
      QStringLiteral("Reset ROI"), roi_group);
  btn_reset_roi_->setObjectName(QStringLiteral("btnResetRoi"));
  roi_layout->addWidget(btn_reset_roi_, 2, 0, 1, 4);

  outer->addWidget(roi_group);

  // Wire debounce triggers.
  connect(spn_exposure_,
          qOverload<double>(&QDoubleSpinBox::valueChanged),
          this, [this]() { timer_exposure_->start(); });
  connect(spn_gain_,
          qOverload<double>(&QDoubleSpinBox::valueChanged),
          this, [this]() { timer_gain_->start(); });

  auto roi_trigger = [this]() { timer_roi_->start(); };
  connect(spn_roi_x_, qOverload<int>(&QSpinBox::valueChanged),
          this, roi_trigger);
  connect(spn_roi_y_, qOverload<int>(&QSpinBox::valueChanged),
          this, roi_trigger);
  connect(spn_roi_w_, qOverload<int>(&QSpinBox::valueChanged),
          this, roi_trigger);
  connect(spn_roi_h_, qOverload<int>(&QSpinBox::valueChanged),
          this, roi_trigger);
  connect(btn_reset_roi_, &QPushButton::clicked,
          this, &CameraPanel::onResetRoiClicked);

  return grp_acquisition_;
}

QGroupBox* CameraPanel::createCaptureGroup() {
  grp_capture_ = new QGroupBox(
      QStringLiteral("Capture"), this);
  grp_capture_->setObjectName(QStringLiteral("grpCapture"));

  auto* layout = new QVBoxLayout(grp_capture_);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(6);

  btn_grab_single_ = new QPushButton(
      QStringLiteral("Grab Single Frame"), grp_capture_);
  btn_grab_single_->setObjectName(
      QStringLiteral("btnGrabSingle"));
  layout->addWidget(btn_grab_single_);

  auto* sep1 = new QFrame(grp_capture_);
  sep1->setFrameShape(QFrame::HLine);
  sep1->setFrameShadow(QFrame::Sunken);
  layout->addWidget(sep1);

  btn_continuous_ = new QPushButton(
      QStringLiteral("Start Continuous"), grp_capture_);
  btn_continuous_->setObjectName(
      QStringLiteral("btnContinuous"));
  btn_continuous_->setCheckable(true);
  layout->addWidget(btn_continuous_);

  auto* sep2 = new QFrame(grp_capture_);
  sep2->setFrameShape(QFrame::HLine);
  sep2->setFrameShadow(QFrame::Sunken);
  layout->addWidget(sep2);

  // Batch controls form.
  auto* batch_form = new QFormLayout();
  batch_form->setSpacing(4);

  spn_batch_count_ = new QSpinBox(grp_capture_);
  spn_batch_count_->setObjectName(
      QStringLiteral("spnBatchCount"));
  spn_batch_count_->setRange(1, 9999);
  spn_batch_count_->setValue(10);
  spn_batch_count_->setSuffix(QStringLiteral(" frames"));
  batch_form->addRow(QStringLiteral("Count:"),
                     spn_batch_count_);

  spn_batch_interval_ = new QSpinBox(grp_capture_);
  spn_batch_interval_->setObjectName(
      QStringLiteral("spnBatchInterval"));
  spn_batch_interval_->setRange(1, 60000);
  spn_batch_interval_->setValue(100);
  spn_batch_interval_->setSuffix(QStringLiteral(" ms"));
  batch_form->addRow(QStringLiteral("Interval:"),
                     spn_batch_interval_);

  layout->addLayout(batch_form);

  btn_start_batch_ = new QPushButton(
      QStringLiteral("Start Batch Capture"), grp_capture_);
  btn_start_batch_->setObjectName(
      QStringLiteral("btnStartBatch"));
  layout->addWidget(btn_start_batch_);

  btn_stop_ = new QPushButton(
      QStringLiteral("Stop"), grp_capture_);
  btn_stop_->setObjectName(QStringLiteral("btnStop"));
  btn_stop_->setStyleSheet(
      QStringLiteral(
          "QPushButton { background-color: #E74C3C;"
          " color: white; font-weight: bold; }"));
  btn_stop_->setVisible(false);
  layout->addWidget(btn_stop_);

  connect(btn_grab_single_, &QPushButton::clicked,
          this, &CameraPanel::onGrabSingleClicked);
  connect(btn_continuous_, &QPushButton::toggled,
          this, &CameraPanel::onContinuousToggled);
  connect(btn_start_batch_, &QPushButton::clicked,
          this, &CameraPanel::onStartBatchClicked);
  connect(btn_stop_, &QPushButton::clicked,
          this, &CameraPanel::onStopClicked);

  return grp_capture_;
}

QGroupBox* CameraPanel::createStatusGroup() {
  grp_status_ = new QGroupBox(
      QStringLiteral("Status"), this);
  grp_status_->setObjectName(QStringLiteral("grpStatus"));

  auto* layout = new QFormLayout(grp_status_);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(6);

  lbl_capture_state_ = new QLabel(
      QStringLiteral("Idle"), grp_status_);
  lbl_capture_state_->setObjectName(
      QStringLiteral("lblCaptureState"));
  layout->addRow(QStringLiteral("State:"),
                 lbl_capture_state_);

  lbl_frame_count_ = new QLabel(
      QStringLiteral("0"), grp_status_);
  lbl_frame_count_->setObjectName(
      QStringLiteral("lblFrameCount"));
  layout->addRow(QStringLiteral("Frames:"),
                 lbl_frame_count_);

  lbl_preview_ = new QLabel(grp_status_);
  lbl_preview_->setObjectName(QStringLiteral("lblPreview"));
  lbl_preview_->setFixedSize(kPreviewWidth, kPreviewHeight);
  lbl_preview_->setAlignment(Qt::AlignCenter);
  lbl_preview_->setText(QStringLiteral("No Image"));
  lbl_preview_->setStyleSheet(
      QStringLiteral("border: 1px solid #95A5A6;"
                     " background-color: #1A1A1A;"
                     " color: #7F8C8D;"));
  layout->addRow(QStringLiteral("Preview:"), lbl_preview_);

  return grp_status_;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void CameraPanel::applyStatusStyle(
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

void CameraPanel::setControlsEnabled(bool connected) {
  grp_acquisition_->setEnabled(connected);
  grp_capture_->setEnabled(connected);
}

void CameraPanel::setCaptureActive(bool capturing) {
  grp_acquisition_->setEnabled(!capturing);
  btn_grab_single_->setEnabled(!capturing);
  btn_start_batch_->setEnabled(!capturing);

  // Only show the continuous button when not in a non-continuous capture.
  // The continuous button manages itself via its checkable state.
  if (!btn_continuous_->isChecked()) {
    btn_continuous_->setEnabled(!capturing);
  }

  btn_stop_->setVisible(capturing);
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void CameraPanel::onStateChanged(
    mwa::hardware::DeviceInterface::DeviceState new_state) {
  applyStatusStyle(new_state);

  const bool connected =
      (new_state ==
       mwa::hardware::DeviceInterface::DeviceState::kConnected);

  btn_connect_->setText(connected ? QStringLiteral("Disconnect")
                                  : QStringLiteral("Connect"));

  if (!connected) {
    // Reset any in-progress capture state before disabling controls,
    // because setCaptureActive(false) re-enables acquisition widgets.
    setCaptureActive(false);
    lbl_capture_state_->setText(QStringLiteral("Idle"));

    // Reset continuous button without triggering the toggled slot.
    btn_continuous_->blockSignals(true);
    btn_continuous_->setChecked(false);
    btn_continuous_->setText(
        QStringLiteral("Start Continuous"));
    btn_continuous_->blockSignals(false);
  }

  // setControlsEnabled AFTER setCaptureActive so disconnected state
  // correctly disables all groups.
  setControlsEnabled(connected);
}

void CameraPanel::onFrameReady(const QImage& frame) {
  frame_count_++;
  lbl_frame_count_->setText(QString::number(frame_count_));

  if (batch_total_ > 0) {
    batch_received_++;
    lbl_capture_state_->setText(
        QStringLiteral("Batch %1/%2")
            .arg(batch_received_)
            .arg(batch_total_));
  }

  // Throttle preview updates to kPreviewThrottleMs.
  const bool should_update =
      !preview_throttle_.isValid() ||
      preview_throttle_.elapsed() >= kPreviewThrottleMs;

  if (should_update && !frame.isNull()) {
    QPixmap pix = QPixmap::fromImage(
        frame.scaled(kPreviewWidth, kPreviewHeight,
                     Qt::KeepAspectRatio,
                     Qt::SmoothTransformation));
    lbl_preview_->setPixmap(pix);
    preview_throttle_.restart();
  }

  // Re-enable capture controls after single grab completes.
  if (!btn_continuous_->isChecked() && batch_total_ == 0) {
    setCaptureActive(false);
    lbl_capture_state_->setText(QStringLiteral("Idle"));
  }
}

void CameraPanel::onBatchComplete() {
  batch_total_ = 0;
  batch_received_ = 0;
  setCaptureActive(false);
  lbl_capture_state_->setText(QStringLiteral("Idle"));
}

void CameraPanel::onConnectClicked() {
  if (controller_ == nullptr) {
    return;
  }

  const bool connected =
      controller_->state() ==
      mwa::hardware::DeviceInterface::DeviceState::kConnected;

  if (connected) {
    auto reply = QMessageBox::question(
        this,
        QStringLiteral("Disconnect Camera"),
        QStringLiteral(
            "Are you sure you want to disconnect the camera?"),
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

void CameraPanel::onGrabSingleClicked() {
  if (controller_ == nullptr) {
    return;
  }
  frame_count_ = 0;
  batch_total_ = 0;
  batch_received_ = 0;
  lbl_frame_count_->setText(QStringLiteral("0"));
  lbl_capture_state_->setText(QStringLiteral("Capturing"));
  setCaptureActive(true);
  controller_->grabSingle();
}

void CameraPanel::onContinuousToggled(bool checked) {
  if (controller_ == nullptr) {
    return;
  }

  if (checked) {
    frame_count_ = 0;
    batch_total_ = 0;
    batch_received_ = 0;
    lbl_frame_count_->setText(QStringLiteral("0"));
    lbl_capture_state_->setText(QStringLiteral("Live"));
    btn_continuous_->setText(
        QStringLiteral("Stop Continuous"));
    setCaptureActive(true);
    // Re-enable the continuous button itself so it can be toggled off.
    btn_continuous_->setEnabled(true);
    controller_->startContinuousCapture();
  } else {
    controller_->stopCapture();
    btn_continuous_->setText(
        QStringLiteral("Start Continuous"));
    setCaptureActive(false);
    lbl_capture_state_->setText(QStringLiteral("Idle"));
  }
}

void CameraPanel::onStartBatchClicked() {
  if (controller_ == nullptr) {
    return;
  }
  const int count = spn_batch_count_->value();
  const int interval_ms = spn_batch_interval_->value();

  frame_count_ = 0;
  batch_total_ = count;
  batch_received_ = 0;
  lbl_frame_count_->setText(QStringLiteral("0"));
  lbl_capture_state_->setText(
      QStringLiteral("Batch 0/%1").arg(count));
  setCaptureActive(true);
  controller_->startBatchCapture(count, interval_ms);
}

void CameraPanel::onStopClicked() {
  if (controller_ == nullptr) {
    return;
  }
  controller_->stopCapture();

  // Reset continuous button state without re-triggering the toggled slot.
  btn_continuous_->blockSignals(true);
  btn_continuous_->setChecked(false);
  btn_continuous_->setText(QStringLiteral("Start Continuous"));
  btn_continuous_->blockSignals(false);

  batch_total_ = 0;
  batch_received_ = 0;
  setCaptureActive(false);
  lbl_capture_state_->setText(QStringLiteral("Idle"));
}

void CameraPanel::onResetRoiClicked() {
  spn_roi_x_->setValue(0);
  spn_roi_y_->setValue(0);
  spn_roi_w_->setValue(kDefaultRoiWidth);
  spn_roi_h_->setValue(kDefaultRoiHeight);
  timer_roi_->start();
}

void CameraPanel::onExposureDebounced() {
  if (controller_ == nullptr) {
    return;
  }
  controller_->setExposure(spn_exposure_->value());
}

void CameraPanel::onGainDebounced() {
  if (controller_ == nullptr) {
    return;
  }
  controller_->setGain(spn_gain_->value());
}

void CameraPanel::onRoiDebounced() {
  if (controller_ == nullptr) {
    return;
  }
  QRect roi(spn_roi_x_->value(), spn_roi_y_->value(),
            spn_roi_w_->value(), spn_roi_h_->value());
  controller_->setRoi(roi);
}

}  // namespace mwa::gui
