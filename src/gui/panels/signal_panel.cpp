/**
 * @file signal_panel.cpp
 * @brief Implementation of the SignalPanel widget.
 * @author MWA Team
 * @date 2026-03-23
 *
 * Implements the combined Signal Generator / Network Analyzer control
 * panel. The panel is built entirely from Qt widgets with no external
 * library dependencies beyond Qt 6.5+. All frequency arithmetic is
 * performed in double-precision Hz throughout the internal logic; unit
 * display is handled at the presentation layer only.
 *
 * @copyright LGPL-3.0-or-later
 */

#include "gui/panels/signal_panel.h"

#include <QApplication>
#include <QClipboard>
#include <QEvent>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSizePolicy>
#include <QVBoxLayout>

namespace mwa::gui {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

SignalPanel::SignalPanel(QWidget* parent) : QWidget(parent) {
  setObjectName(QStringLiteral("signalPanel"));

  auto* root = new QVBoxLayout(this);
  root->setContentsMargins(8, 8, 8, 8);
  root->setSpacing(12);

  root->addWidget(createConnectionGroup());

  tab_widget_ = new QTabWidget(this);
  tab_widget_->setObjectName(QStringLiteral("tabSignalNA"));
  tab_widget_->addTab(createSignalGeneratorTab(),
                      QStringLiteral("Signal Generator"));
  tab_widget_->addTab(createNetworkAnalyzerTab(),
                      QStringLiteral("Network Analyzer"));
  root->addWidget(tab_widget_);

  // Both tabs start disabled until a controller is attached.
  setSigTabEnabled(false);
  setNaTabEnabled(false);
  applyStatusStyle(
      mwa::hardware::DeviceInterface::DeviceState::kDisconnected);
}

// ---------------------------------------------------------------------------
// Public API — controller attachment
// ---------------------------------------------------------------------------

void SignalPanel::setSignalGeneratorController(
    mwa::hardware::SignalGeneratorControllerInterface* controller) {
  // Disconnect old controller.
  if (sig_controller_ != nullptr) {
    disconnect(sig_controller_, nullptr, this, nullptr);
  }

  sig_controller_ = controller;

  if (sig_controller_ == nullptr) {
    tab_widget_->setTabEnabled(0, false);
    return;
  }

  tab_widget_->setTabEnabled(0, true);

  // Wire controller → panel.
  connect(sig_controller_,
          &mwa::hardware::SignalGeneratorControllerInterface::stateChanged,
          this, &SignalPanel::onSigStateChanged);
  connect(sig_controller_,
          &mwa::hardware::SignalGeneratorControllerInterface::frequencyChanged,
          this, &SignalPanel::onSigFrequencyConfirmed);
  connect(sig_controller_,
          &mwa::hardware::SignalGeneratorControllerInterface::amplitudeChanged,
          this, &SignalPanel::onSigAmplitudeConfirmed);
  connect(sig_controller_,
          &mwa::hardware::SignalGeneratorControllerInterface::waveformChanged,
          this, &SignalPanel::onSigWaveformConfirmed);
  connect(
      sig_controller_,
      &mwa::hardware::SignalGeneratorControllerInterface::outputStateChanged,
      this, &SignalPanel::onSigOutputStateConfirmed);

  // Sync initial state.
  onSigStateChanged(sig_controller_->state());
}

void SignalPanel::setNetworkAnalyzerController(
    mwa::hardware::NetworkAnalyzerControllerInterface* controller) {
  // Disconnect old controller.
  if (na_controller_ != nullptr) {
    disconnect(na_controller_, nullptr, this, nullptr);
  }

  na_controller_ = controller;

  if (na_controller_ == nullptr) {
    tab_widget_->setTabEnabled(1, false);
    return;
  }

  tab_widget_->setTabEnabled(1, true);

  // Wire controller → panel.
  connect(na_controller_,
          &mwa::hardware::NetworkAnalyzerControllerInterface::stateChanged,
          this, &SignalPanel::onNaStateChanged);
  connect(
      na_controller_,
      &mwa::hardware::NetworkAnalyzerControllerInterface::measurementStarted,
      this, [this]() {
        btn_na_measure_->setEnabled(false);
        prg_na_measurement_->setVisible(true);
        setNaMeasurementState(
            QLatin1String(kColorActive),
            QStringLiteral("Measuring..."));
      });
  connect(
      na_controller_,
      &mwa::hardware::NetworkAnalyzerControllerInterface::measurementComplete,
      this, &SignalPanel::onNaMeasurementComplete);
  connect(
      na_controller_,
      &mwa::hardware::NetworkAnalyzerControllerInterface::frequencyRangeChanged,
      this, &SignalPanel::onNaFrequencyRangeChanged);
  connect(
      na_controller_,
      &mwa::hardware::NetworkAnalyzerControllerInterface::numPointsChanged,
      this, &SignalPanel::onNaNumPointsChanged);

  // Sync initial state.
  onNaStateChanged(na_controller_->state());
}

// ---------------------------------------------------------------------------
// Builder helpers — shared
// ---------------------------------------------------------------------------

QGroupBox* SignalPanel::createConnectionGroup() {
  grp_connection_ = new QGroupBox(QStringLiteral("Connection"), this);
  grp_connection_->setObjectName(QStringLiteral("grpConnection"));

  auto* layout = new QHBoxLayout(grp_connection_);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(8);

  cmb_port_ = new QComboBox(grp_connection_);
  cmb_port_->setObjectName(QStringLiteral("cmbPort"));
  cmb_port_->setEditable(true);
  cmb_port_->setPlaceholderText(QStringLiteral("GPIB0::16"));
  cmb_port_->setMinimumWidth(140);
  cmb_port_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  layout->addWidget(cmb_port_);

  lbl_status_ = new QLabel(grp_connection_);
  lbl_status_->setObjectName(QStringLiteral("lblStatus"));
  lbl_status_->setFixedSize(14, 14);
  lbl_status_->setToolTip(QStringLiteral("Device connection state"));
  layout->addWidget(lbl_status_);

  btn_connect_ = new QPushButton(QStringLiteral("Connect"),
                                 grp_connection_);
  btn_connect_->setObjectName(QStringLiteral("btnConnect"));
  btn_connect_->setMinimumWidth(100);
  layout->addWidget(btn_connect_);

  connect(btn_connect_, &QPushButton::clicked,
          this, &SignalPanel::onConnectClicked);

  return grp_connection_;
}

// ---------------------------------------------------------------------------
// Builder helpers — Signal Generator tab
// ---------------------------------------------------------------------------

QWidget* SignalPanel::createSignalGeneratorTab() {
  auto* tab = new QWidget();
  tab->setObjectName(QStringLiteral("tabSignalGenerator"));

  auto* layout = new QVBoxLayout(tab);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(8);

  layout->addWidget(createSigOutputConfigGroup(tab));
  layout->addWidget(createSigSweepGroup(tab));
  layout->addWidget(createSigOutputStatusGroup(tab));
  layout->addStretch();

  return tab;
}

QGroupBox* SignalPanel::createSigOutputConfigGroup(QWidget* parent) {
  grp_sig_output_config_ =
      new QGroupBox(QStringLiteral("Output Configuration"), parent);
  grp_sig_output_config_->setObjectName(
      QStringLiteral("grpSigOutputConfig"));

  auto* form = new QFormLayout(grp_sig_output_config_);
  form->setContentsMargins(8, 8, 8, 8);
  form->setSpacing(6);

  // --- Frequency row -------------------------------------------------------
  spn_sig_frequency_ = new QDoubleSpinBox(grp_sig_output_config_);
  spn_sig_frequency_->setObjectName(
      QStringLiteral("spnSigFrequency"));
  spn_sig_frequency_->setRange(kFreqMin, kFreqMax);
  spn_sig_frequency_->setDecimals(kFreqDecimals);
  spn_sig_frequency_->setValue(1.0);  // 1 MHz default
  spn_sig_frequency_->setSizePolicy(
      QSizePolicy::Expanding, QSizePolicy::Fixed);

  cmb_sig_freq_unit_ = createFreqUnitCombo(grp_sig_output_config_, 2);
  cmb_sig_freq_unit_->setObjectName(
      QStringLiteral("cmbSigFreqUnit"));

  auto* freq_row = new QWidget(grp_sig_output_config_);
  auto* freq_layout = new QHBoxLayout(freq_row);
  freq_layout->setContentsMargins(0, 0, 0, 0);
  freq_layout->setSpacing(4);
  freq_layout->addWidget(spn_sig_frequency_);
  freq_layout->addWidget(cmb_sig_freq_unit_);
  form->addRow(QStringLiteral("Frequency:"), freq_row);

  // --- Amplitude row -------------------------------------------------------
  spn_sig_amplitude_ = new QDoubleSpinBox(grp_sig_output_config_);
  spn_sig_amplitude_->setObjectName(
      QStringLiteral("spnSigAmplitude"));
  spn_sig_amplitude_->setRange(0.001, 20.000);
  spn_sig_amplitude_->setDecimals(3);
  spn_sig_amplitude_->setSingleStep(0.001);
  spn_sig_amplitude_->setSuffix(QStringLiteral(" V"));
  spn_sig_amplitude_->setValue(1.000);
  form->addRow(QStringLiteral("Amplitude:"), spn_sig_amplitude_);

  // --- Waveform row --------------------------------------------------------
  cmb_sig_waveform_ = new QComboBox(grp_sig_output_config_);
  cmb_sig_waveform_->setObjectName(QStringLiteral("cmbSigWaveform"));
  cmb_sig_waveform_->addItems({QStringLiteral("Sine"),
                                QStringLiteral("Square"),
                                QStringLiteral("Triangle")});
  form->addRow(QStringLiteral("Waveform:"), cmb_sig_waveform_);

  // Connections: user edits → controller
  connect(spn_sig_frequency_,
          qOverload<double>(&QDoubleSpinBox::valueChanged),
          this, &SignalPanel::onSigFrequencyChanged);
  connect(cmb_sig_freq_unit_,
          qOverload<int>(&QComboBox::currentIndexChanged),
          this, &SignalPanel::onSigFrequencyChanged);
  connect(spn_sig_amplitude_,
          qOverload<double>(&QDoubleSpinBox::valueChanged),
          this, &SignalPanel::onSigAmplitudeChanged);
  connect(cmb_sig_waveform_,
          qOverload<int>(&QComboBox::currentIndexChanged),
          this, &SignalPanel::onSigWaveformChanged);

  return grp_sig_output_config_;
}

QGroupBox* SignalPanel::createSigSweepGroup(QWidget* parent) {
  grp_sig_sweep_ =
      new QGroupBox(QStringLiteral("Sweep Configuration"), parent);
  grp_sig_sweep_->setObjectName(QStringLiteral("grpSigSweep"));

  auto* form = new QFormLayout(grp_sig_sweep_);
  form->setContentsMargins(8, 8, 8, 8);
  form->setSpacing(6);

  auto make_sweep_row =
      [this, &form](const QString& label,
                    QDoubleSpinBox*& spn,
                    QComboBox*& cmb,
                    const QString& spn_name,
                    const QString& cmb_name) {
        spn = new QDoubleSpinBox(grp_sig_sweep_);
        spn->setObjectName(spn_name);
        spn->setRange(kFreqMin, kFreqMax);
        spn->setDecimals(kFreqDecimals);
        spn->setValue(1.0);
        spn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        cmb = createFreqUnitCombo(grp_sig_sweep_, 2);
        cmb->setObjectName(cmb_name);

        auto* row = new QWidget(grp_sig_sweep_);
        auto* row_layout = new QHBoxLayout(row);
        row_layout->setContentsMargins(0, 0, 0, 0);
        row_layout->setSpacing(4);
        row_layout->addWidget(spn);
        row_layout->addWidget(cmb);
        form->addRow(label, row);
      };

  make_sweep_row(QStringLiteral("Start:"),
                 spn_sig_sweep_start_,
                 cmb_sig_sweep_start_unit_,
                 QStringLiteral("spnSigSweepStart"),
                 QStringLiteral("cmbSigSweepStartUnit"));

  make_sweep_row(QStringLiteral("Stop:"),
                 spn_sig_sweep_stop_,
                 cmb_sig_sweep_stop_unit_,
                 QStringLiteral("spnSigSweepStop"),
                 QStringLiteral("cmbSigSweepStopUnit"));

  make_sweep_row(QStringLiteral("Step:"),
                 spn_sig_sweep_step_,
                 cmb_sig_sweep_step_unit_,
                 QStringLiteral("spnSigSweepStep"),
                 QStringLiteral("cmbSigSweepStepUnit"));

  // Sensible defaults: start 1 MHz, stop 10 MHz, step 100 kHz
  spn_sig_sweep_start_->setValue(1.0);
  spn_sig_sweep_stop_->setValue(10.0);
  spn_sig_sweep_step_->setValue(100.0);
  cmb_sig_sweep_step_unit_->setCurrentIndex(1);  // kHz

  btn_sig_apply_sweep_ = new QPushButton(
      QStringLiteral("Apply Sweep Config"), grp_sig_sweep_);
  btn_sig_apply_sweep_->setObjectName(
      QStringLiteral("btnSigApplySweep"));
  form->addRow(QString(), btn_sig_apply_sweep_);

  connect(btn_sig_apply_sweep_, &QPushButton::clicked,
          this, &SignalPanel::onSigApplySweep);

  return grp_sig_sweep_;
}

QGroupBox* SignalPanel::createSigOutputStatusGroup(QWidget* parent) {
  grp_sig_output_status_ =
      new QGroupBox(QStringLiteral("Output Status"), parent);
  grp_sig_output_status_->setObjectName(
      QStringLiteral("grpSigOutputStatus"));

  auto* layout = new QVBoxLayout(grp_sig_output_status_);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(6);

  btn_sig_output_enable_ = new QPushButton(
      QStringLiteral("Output OFF"), grp_sig_output_status_);
  btn_sig_output_enable_->setObjectName(
      QStringLiteral("btnSigOutputEnable"));
  btn_sig_output_enable_->setCheckable(true);
  btn_sig_output_enable_->setChecked(false);
  layout->addWidget(btn_sig_output_enable_);

  // Frequency display — 13pt bold, sunken frame.
  lbl_sig_freq_display_ = new QLabel(
      QStringLiteral("— Hz"), grp_sig_output_status_);
  lbl_sig_freq_display_->setObjectName(
      QStringLiteral("lblSigFreqDisplay"));
  lbl_sig_freq_display_->setFrameStyle(
      QFrame::Panel | QFrame::Sunken);
  lbl_sig_freq_display_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  lbl_sig_freq_display_->setToolTip(
      QStringLiteral("Double-click to copy to clipboard"));
  QFont freq_font = lbl_sig_freq_display_->font();
  freq_font.setPointSize(13);
  freq_font.setBold(true);
  lbl_sig_freq_display_->setFont(freq_font);
  layout->addWidget(lbl_sig_freq_display_);

  // Amplitude display.
  lbl_sig_amp_display_ = new QLabel(
      QStringLiteral("— V"), grp_sig_output_status_);
  lbl_sig_amp_display_->setObjectName(
      QStringLiteral("lblSigAmpDisplay"));
  lbl_sig_amp_display_->setFrameStyle(
      QFrame::Panel | QFrame::Sunken);
  lbl_sig_amp_display_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  lbl_sig_amp_display_->setToolTip(
      QStringLiteral("Double-click to copy to clipboard"));
  layout->addWidget(lbl_sig_amp_display_);

  // Waveform display.
  lbl_sig_waveform_display_ = new QLabel(
      QStringLiteral("—"), grp_sig_output_status_);
  lbl_sig_waveform_display_->setObjectName(
      QStringLiteral("lblSigWaveformDisplay"));
  lbl_sig_waveform_display_->setFrameStyle(
      QFrame::Panel | QFrame::Sunken);
  lbl_sig_waveform_display_->setAlignment(
      Qt::AlignLeft | Qt::AlignVCenter);
  lbl_sig_waveform_display_->setToolTip(
      QStringLiteral("Double-click to copy to clipboard"));
  layout->addWidget(lbl_sig_waveform_display_);

  connect(btn_sig_output_enable_, &QPushButton::toggled,
          this, &SignalPanel::onSigOutputToggled);

  // Install this panel as event filter on display labels so that
  // double-click events can be intercepted and routed to clipboard slots.
  lbl_sig_freq_display_->installEventFilter(this);
  lbl_sig_amp_display_->installEventFilter(this);
  lbl_sig_waveform_display_->installEventFilter(this);

  return grp_sig_output_status_;
}

// ---------------------------------------------------------------------------
// Builder helpers — Network Analyzer tab
// ---------------------------------------------------------------------------

QWidget* SignalPanel::createNetworkAnalyzerTab() {
  auto* tab = new QWidget();
  tab->setObjectName(QStringLiteral("tabNetworkAnalyzer"));

  auto* layout = new QVBoxLayout(tab);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(8);

  layout->addWidget(createNaSweepConfigGroup(tab));
  layout->addWidget(createNaMeasurementGroup(tab));
  layout->addWidget(createNaMeasStatusGroup(tab));
  layout->addStretch();

  return tab;
}

QGroupBox* SignalPanel::createNaSweepConfigGroup(QWidget* parent) {
  grp_na_sweep_config_ =
      new QGroupBox(QStringLiteral("Sweep Configuration"), parent);
  grp_na_sweep_config_->setObjectName(
      QStringLiteral("grpNaSweepConfig"));

  auto* form = new QFormLayout(grp_na_sweep_config_);
  form->setContentsMargins(8, 8, 8, 8);
  form->setSpacing(6);

  // --- Start frequency row -------------------------------------------------
  spn_na_start_freq_ = new QDoubleSpinBox(grp_na_sweep_config_);
  spn_na_start_freq_->setObjectName(
      QStringLiteral("spnNaStartFreq"));
  spn_na_start_freq_->setRange(kFreqMin, kFreqMax);
  spn_na_start_freq_->setDecimals(kFreqDecimals);
  spn_na_start_freq_->setValue(1.0);
  spn_na_start_freq_->setSizePolicy(
      QSizePolicy::Expanding, QSizePolicy::Fixed);

  cmb_na_start_freq_unit_ =
      createFreqUnitCombo(grp_na_sweep_config_, 2);
  cmb_na_start_freq_unit_->setObjectName(
      QStringLiteral("cmbNaStartFreqUnit"));

  auto* start_row = new QWidget(grp_na_sweep_config_);
  auto* start_layout = new QHBoxLayout(start_row);
  start_layout->setContentsMargins(0, 0, 0, 0);
  start_layout->setSpacing(4);
  start_layout->addWidget(spn_na_start_freq_);
  start_layout->addWidget(cmb_na_start_freq_unit_);
  form->addRow(QStringLiteral("Start:"), start_row);

  // --- Stop frequency row --------------------------------------------------
  spn_na_stop_freq_ = new QDoubleSpinBox(grp_na_sweep_config_);
  spn_na_stop_freq_->setObjectName(
      QStringLiteral("spnNaStopFreq"));
  spn_na_stop_freq_->setRange(kFreqMin, kFreqMax);
  spn_na_stop_freq_->setDecimals(kFreqDecimals);
  spn_na_stop_freq_->setValue(100.0);
  spn_na_stop_freq_->setSizePolicy(
      QSizePolicy::Expanding, QSizePolicy::Fixed);

  cmb_na_stop_freq_unit_ =
      createFreqUnitCombo(grp_na_sweep_config_, 2);
  cmb_na_stop_freq_unit_->setObjectName(
      QStringLiteral("cmbNaStopFreqUnit"));

  auto* stop_row = new QWidget(grp_na_sweep_config_);
  auto* stop_layout = new QHBoxLayout(stop_row);
  stop_layout->setContentsMargins(0, 0, 0, 0);
  stop_layout->setSpacing(4);
  stop_layout->addWidget(spn_na_stop_freq_);
  stop_layout->addWidget(cmb_na_stop_freq_unit_);
  form->addRow(QStringLiteral("Stop:"), stop_row);

  // --- Points row ----------------------------------------------------------
  spn_na_points_ = new QSpinBox(grp_na_sweep_config_);
  spn_na_points_->setObjectName(QStringLiteral("spnNaPoints"));
  spn_na_points_->setRange(2, 16001);
  spn_na_points_->setSingleStep(50);
  spn_na_points_->setValue(201);
  form->addRow(QStringLiteral("Points:"), spn_na_points_);

  // NA sweep parameters are sent to controller on measure trigger, not
  // live, so no valueChanged connections here.

  return grp_na_sweep_config_;
}

QGroupBox* SignalPanel::createNaMeasurementGroup(QWidget* parent) {
  grp_na_measurement_ =
      new QGroupBox(QStringLiteral("Measurement"), parent);
  grp_na_measurement_->setObjectName(
      QStringLiteral("grpNaMeasurement"));

  auto* layout = new QVBoxLayout(grp_na_measurement_);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(6);

  btn_na_measure_ = new QPushButton(
      QStringLiteral("Measure S-Parameters"), grp_na_measurement_);
  btn_na_measure_->setObjectName(QStringLiteral("btnNaMeasure"));
  layout->addWidget(btn_na_measure_);

  lbl_na_sparam_display_ = new QLabel(
      QStringLiteral("No measurement data..."), grp_na_measurement_);
  lbl_na_sparam_display_->setObjectName(
      QStringLiteral("lblNaSparamDisplay"));
  lbl_na_sparam_display_->setFrameStyle(
      QFrame::Panel | QFrame::Sunken);
  lbl_na_sparam_display_->setAlignment(
      Qt::AlignLeft | Qt::AlignTop);
  lbl_na_sparam_display_->setMinimumHeight(120);
  lbl_na_sparam_display_->setWordWrap(true);
  lbl_na_sparam_display_->setToolTip(
      QStringLiteral("Double-click to copy to clipboard"));
  layout->addWidget(lbl_na_sparam_display_);

  prg_na_measurement_ = new QProgressBar(grp_na_measurement_);
  prg_na_measurement_->setObjectName(
      QStringLiteral("prgNaMeasurement"));
  prg_na_measurement_->setRange(0, 0);  // Indeterminate.
  prg_na_measurement_->setVisible(false);
  layout->addWidget(prg_na_measurement_);

  connect(btn_na_measure_, &QPushButton::clicked,
          this, &SignalPanel::onNaMeasureClicked);

  lbl_na_sparam_display_->installEventFilter(this);

  return grp_na_measurement_;
}

QGroupBox* SignalPanel::createNaMeasStatusGroup(QWidget* parent) {
  grp_na_meas_status_ =
      new QGroupBox(QStringLiteral("Measurement Status"), parent);
  grp_na_meas_status_->setObjectName(
      QStringLiteral("grpNaMeasStatus"));

  auto* grid = new QGridLayout(grp_na_meas_status_);
  grid->setContentsMargins(8, 8, 8, 8);
  grid->setSpacing(6);

  lbl_na_state_dot_ = new QLabel(grp_na_meas_status_);
  lbl_na_state_dot_->setObjectName(
      QStringLiteral("lblNaStateDot"));
  lbl_na_state_dot_->setFixedSize(12, 12);
  lbl_na_state_dot_->setStyleSheet(
      QStringLiteral("background-color: %1;"
                     "border-radius: 6px;")
          .arg(QLatin1String(kColorDisconnected)));
  grid->addWidget(lbl_na_state_dot_, 0, 0);

  lbl_na_state_text_ = new QLabel(
      QStringLiteral("Idle"), grp_na_meas_status_);
  lbl_na_state_text_->setObjectName(
      QStringLiteral("lblNaStateText"));
  grid->addWidget(lbl_na_state_text_, 0, 1);

  lbl_na_range_summary_ = new QLabel(
      QStringLiteral("—"), grp_na_meas_status_);
  lbl_na_range_summary_->setObjectName(
      QStringLiteral("lblNaRangeSummary"));
  grid->addWidget(lbl_na_range_summary_, 1, 0, 1, 2);

  lbl_na_points_summary_ = new QLabel(
      QStringLiteral("—"), grp_na_meas_status_);
  lbl_na_points_summary_->setObjectName(
      QStringLiteral("lblNaPointsSummary"));
  grid->addWidget(lbl_na_points_summary_, 2, 0, 1, 2);

  grid->setColumnStretch(1, 1);

  return grp_na_meas_status_;
}

// ---------------------------------------------------------------------------
// Utility helpers
// ---------------------------------------------------------------------------

QComboBox* SignalPanel::createFreqUnitCombo(QWidget* parent,
                                            int default_index) {
  auto* cmb = new QComboBox(parent);
  cmb->addItems({QStringLiteral("Hz"),
                 QStringLiteral("kHz"),
                 QStringLiteral("MHz")});
  cmb->setCurrentIndex(default_index);
  return cmb;
}

double SignalPanel::convertToHz(double value, int unit_index) const {
  if (unit_index < 0 || unit_index > 2) {
    return value;
  }
  return value * kUnitMultipliers[unit_index];
}

QString SignalPanel::formatFrequency(double hz) const {
  if (hz >= 1.0e6) {
    return QStringLiteral("%1 MHz")
        .arg(hz / 1.0e6, 0, 'f', kFreqDecimals);
  }
  if (hz >= 1.0e3) {
    return QStringLiteral("%1 kHz")
        .arg(hz / 1.0e3, 0, 'f', kFreqDecimals);
  }
  return QStringLiteral("%1 Hz").arg(hz, 0, 'f', kFreqDecimals);
}

void SignalPanel::applyStatusStyle(
    mwa::hardware::DeviceInterface::DeviceState new_state) {
  const char* color = kColorDisconnected;
  QString text = QStringLiteral("Disconnected");

  switch (new_state) {
    case mwa::hardware::DeviceInterface::DeviceState::kConnected:
      color = kColorConnected;
      text = QStringLiteral("Connected");
      break;
    case mwa::hardware::DeviceInterface::DeviceState::kConnecting:
      color = kColorConnecting;
      text = QStringLiteral("Connecting...");
      break;
    case mwa::hardware::DeviceInterface::DeviceState::kError:
      color = kColorError;
      text = QStringLiteral("Error");
      break;
    case mwa::hardware::DeviceInterface::DeviceState::kDisconnected:
    default:
      break;
  }

  lbl_status_->setStyleSheet(
      QStringLiteral("background-color: %1; border-radius: 7px;")
          .arg(QLatin1String(color)));
  lbl_status_->setToolTip(text);
}

void SignalPanel::setNaMeasurementState(const QString& color,
                                        const QString& text) {
  lbl_na_state_dot_->setStyleSheet(
      QStringLiteral("background-color: %1; border-radius: 6px;")
          .arg(color));
  lbl_na_state_text_->setText(text);
}

void SignalPanel::setSigTabEnabled(bool enabled) {
  if (grp_sig_output_config_ != nullptr) {
    grp_sig_output_config_->setEnabled(enabled);
  }
  if (grp_sig_sweep_ != nullptr) {
    grp_sig_sweep_->setEnabled(enabled);
  }
  if (grp_sig_output_status_ != nullptr) {
    grp_sig_output_status_->setEnabled(enabled);
  }
}

void SignalPanel::setNaTabEnabled(bool enabled) {
  if (grp_na_sweep_config_ != nullptr) {
    grp_na_sweep_config_->setEnabled(enabled);
  }
  if (grp_na_measurement_ != nullptr) {
    grp_na_measurement_->setEnabled(enabled);
  }
  if (grp_na_meas_status_ != nullptr) {
    grp_na_meas_status_->setEnabled(enabled);
  }
}

// ---------------------------------------------------------------------------
// Event filter — double-click on display labels → clipboard
// ---------------------------------------------------------------------------

bool SignalPanel::eventFilter(QObject* watched, QEvent* event) {
  if (event->type() == QEvent::MouseButtonDblClick) {
    if (watched == lbl_sig_freq_display_) {
      onSigFreqDisplayDoubleClicked();
      return true;
    }
    if (watched == lbl_sig_amp_display_) {
      onSigAmpDisplayDoubleClicked();
      return true;
    }
    if (watched == lbl_sig_waveform_display_) {
      onSigWaveformDisplayDoubleClicked();
      return true;
    }
    if (watched == lbl_na_sparam_display_) {
      onNaSparamDisplayDoubleClicked();
      return true;
    }
  }
  return QWidget::eventFilter(watched, event);
}

// ---------------------------------------------------------------------------
// Slots — shared / connection
// ---------------------------------------------------------------------------

void SignalPanel::onConnectClicked() {
  // Determine the active controller based on the current tab.
  int current_tab = tab_widget_->currentIndex();

  mwa::hardware::DeviceInterface* active_ctrl = nullptr;
  if (current_tab == 0 && sig_controller_ != nullptr) {
    active_ctrl = sig_controller_;
  } else if (current_tab == 1 && na_controller_ != nullptr) {
    active_ctrl = na_controller_;
  }

  if (active_ctrl == nullptr) {
    return;
  }

  using State = mwa::hardware::DeviceInterface::DeviceState;
  if (active_ctrl->state() == State::kConnected) {
    auto answer = QMessageBox::question(
        this,
        QStringLiteral("Disconnect"),
        QStringLiteral("Disconnect from %1?")
            .arg(active_ctrl->deviceName()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes) {
      return;
    }
    active_ctrl->disconnectDevice();
  } else {
    active_ctrl->connectDevice();
  }
}

// ---------------------------------------------------------------------------
// Slots — Signal Generator
// ---------------------------------------------------------------------------

void SignalPanel::onSigStateChanged(
    mwa::hardware::DeviceInterface::DeviceState new_state) {
  using State = mwa::hardware::DeviceInterface::DeviceState;
  const bool connected = (new_state == State::kConnected);

  applyStatusStyle(new_state);
  setSigTabEnabled(connected);

  btn_connect_->setText(
      connected ? QStringLiteral("Disconnect")
                : QStringLiteral("Connect"));
}

void SignalPanel::onSigFrequencyChanged() {
  if (sig_controller_ == nullptr) {
    return;
  }
  const double hz = convertToHz(
      spn_sig_frequency_->value(),
      cmb_sig_freq_unit_->currentIndex());
  sig_controller_->setFrequency(hz);
}

void SignalPanel::onSigAmplitudeChanged(double volts) {
  if (sig_controller_ == nullptr) {
    return;
  }
  sig_controller_->setAmplitude(volts);
}

void SignalPanel::onSigWaveformChanged(int index) {
  if (sig_controller_ == nullptr) {
    return;
  }
  using Waveform =
      mwa::hardware::SignalGeneratorControllerInterface::Waveform;
  Waveform wf = Waveform::kSine;
  switch (index) {
    case 1:
      wf = Waveform::kSquare;
      break;
    case 2:
      wf = Waveform::kTriangle;
      break;
    default:
      break;
  }
  sig_controller_->setWaveform(wf);
}

void SignalPanel::onSigApplySweep() {
  if (sig_controller_ == nullptr) {
    return;
  }
  const double start_hz = convertToHz(
      spn_sig_sweep_start_->value(),
      cmb_sig_sweep_start_unit_->currentIndex());
  const double stop_hz = convertToHz(
      spn_sig_sweep_stop_->value(),
      cmb_sig_sweep_stop_unit_->currentIndex());
  const double step_hz = convertToHz(
      spn_sig_sweep_step_->value(),
      cmb_sig_sweep_step_unit_->currentIndex());
  sig_controller_->configureSweep(start_hz, stop_hz, step_hz);
}

void SignalPanel::onSigOutputToggled(bool checked) {
  if (sig_controller_ == nullptr) {
    // Revert button state without triggering another signal.
    btn_sig_output_enable_->blockSignals(true);
    btn_sig_output_enable_->setChecked(!checked);
    btn_sig_output_enable_->blockSignals(false);
    return;
  }

  if (!checked) {
    // Turning OFF — ask for confirmation.
    auto answer = QMessageBox::question(
        this,
        QStringLiteral("Disable Output"),
        QStringLiteral("Disable signal output?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes) {
      // Revert toggle.
      btn_sig_output_enable_->blockSignals(true);
      btn_sig_output_enable_->setChecked(true);
      btn_sig_output_enable_->blockSignals(false);
      return;
    }
  }

  sig_controller_->setOutputEnabled(checked);
  btn_sig_output_enable_->setText(
      checked ? QStringLiteral("Output ON")
              : QStringLiteral("Output OFF"));
}

void SignalPanel::onSigFrequencyConfirmed(double hz) {
  lbl_sig_freq_display_->setText(formatFrequency(hz));

  // Update spinbox/unit without re-triggering onSigFrequencyChanged.
  spn_sig_frequency_->blockSignals(true);
  cmb_sig_freq_unit_->blockSignals(true);
  if (hz >= 1.0e6) {
    cmb_sig_freq_unit_->setCurrentIndex(2);
    spn_sig_frequency_->setValue(hz / 1.0e6);
  } else if (hz >= 1.0e3) {
    cmb_sig_freq_unit_->setCurrentIndex(1);
    spn_sig_frequency_->setValue(hz / 1.0e3);
  } else {
    cmb_sig_freq_unit_->setCurrentIndex(0);
    spn_sig_frequency_->setValue(hz);
  }
  spn_sig_frequency_->blockSignals(false);
  cmb_sig_freq_unit_->blockSignals(false);
}

void SignalPanel::onSigAmplitudeConfirmed(double volts) {
  spn_sig_amplitude_->blockSignals(true);
  spn_sig_amplitude_->setValue(volts);
  spn_sig_amplitude_->blockSignals(false);

  lbl_sig_amp_display_->setText(
      QStringLiteral("%1 V").arg(volts, 0, 'f', 3));
}

void SignalPanel::onSigWaveformConfirmed(
    mwa::hardware::SignalGeneratorControllerInterface::Waveform waveform) {
  using Waveform =
      mwa::hardware::SignalGeneratorControllerInterface::Waveform;

  int index = 0;
  QString text = QStringLiteral("Sine");
  switch (waveform) {
    case Waveform::kSquare:
      index = 1;
      text = QStringLiteral("Square");
      break;
    case Waveform::kTriangle:
      index = 2;
      text = QStringLiteral("Triangle");
      break;
    default:
      break;
  }

  cmb_sig_waveform_->blockSignals(true);
  cmb_sig_waveform_->setCurrentIndex(index);
  cmb_sig_waveform_->blockSignals(false);

  lbl_sig_waveform_display_->setText(text);
}

void SignalPanel::onSigOutputStateConfirmed(bool enabled) {
  btn_sig_output_enable_->blockSignals(true);
  btn_sig_output_enable_->setChecked(enabled);
  btn_sig_output_enable_->setText(
      enabled ? QStringLiteral("Output ON")
              : QStringLiteral("Output OFF"));
  btn_sig_output_enable_->blockSignals(false);
}

// ---------------------------------------------------------------------------
// Slots — Network Analyzer
// ---------------------------------------------------------------------------

void SignalPanel::onNaStateChanged(
    mwa::hardware::DeviceInterface::DeviceState new_state) {
  using State = mwa::hardware::DeviceInterface::DeviceState;
  const bool connected = (new_state == State::kConnected);

  applyStatusStyle(new_state);
  setNaTabEnabled(connected);

  btn_connect_->setText(
      connected ? QStringLiteral("Disconnect")
                : QStringLiteral("Connect"));

  if (connected) {
    setNaMeasurementState(
        QLatin1String(kColorConnected),
        QStringLiteral("Idle"));
  } else {
    setNaMeasurementState(
        QLatin1String(kColorDisconnected),
        QStringLiteral("Idle"));
  }
}

void SignalPanel::onNaMeasureClicked() {
  if (na_controller_ == nullptr) {
    return;
  }

  // Push sweep config before triggering measurement.
  const double start_hz = convertToHz(
      spn_na_start_freq_->value(),
      cmb_na_start_freq_unit_->currentIndex());
  const double stop_hz = convertToHz(
      spn_na_stop_freq_->value(),
      cmb_na_stop_freq_unit_->currentIndex());
  na_controller_->setFrequencyRange(start_hz, stop_hz);
  na_controller_->setNumPoints(spn_na_points_->value());

  btn_na_measure_->setEnabled(false);
  prg_na_measurement_->setVisible(true);
  setNaMeasurementState(
      QLatin1String(kColorActive),
      QStringLiteral("Measuring..."));

  na_controller_->measureSParameters();
}

void SignalPanel::onNaMeasurementComplete() {
  prg_na_measurement_->setVisible(false);
  btn_na_measure_->setEnabled(true);

  setNaMeasurementState(
      QLatin1String(kColorConnected),
      QStringLiteral("Complete"));

  if (na_controller_ == nullptr) {
    return;
  }

  const QVector<double> freqs = na_controller_->traceFrequencies();
  const QVector<double> mags  = na_controller_->traceMagnitudes();

  if (freqs.isEmpty() || mags.isEmpty()) {
    lbl_na_sparam_display_->setText(
        QStringLiteral("Measurement complete — no trace data."));
    return;
  }

  // Build a brief summary: range, point count, min/max magnitude.
  double mag_min = mags.front();
  double mag_max = mags.front();
  for (double m : mags) {
    if (m < mag_min) mag_min = m;
    if (m > mag_max) mag_max = m;
  }

  const QString summary =
      QStringLiteral(
          "Points: %1\n"
          "Range:  %2 – %3\n"
          "S21 min: %4 dB\n"
          "S21 max: %5 dB")
          .arg(freqs.size())
          .arg(formatFrequency(freqs.front()),
               formatFrequency(freqs.back()))
          .arg(mag_min, 0, 'f', 3)
          .arg(mag_max, 0, 'f', 3);

  lbl_na_sparam_display_->setText(summary);

  // Update status summary labels.
  lbl_na_range_summary_->setText(
      QStringLiteral("Range: %1 – %2")
          .arg(formatFrequency(na_controller_->startFrequency()),
               formatFrequency(na_controller_->stopFrequency())));
  lbl_na_points_summary_->setText(
      QStringLiteral("Points: %1")
          .arg(na_controller_->numPoints()));
}

void SignalPanel::onNaFrequencyRangeChanged(double start, double stop) {
  lbl_na_range_summary_->setText(
      QStringLiteral("Range: %1 – %2")
          .arg(formatFrequency(start), formatFrequency(stop)));
}

void SignalPanel::onNaNumPointsChanged(int points) {
  lbl_na_points_summary_->setText(
      QStringLiteral("Points: %1").arg(points));
}

// ---------------------------------------------------------------------------
// Slots — clipboard helpers
// ---------------------------------------------------------------------------

void SignalPanel::onSigFreqDisplayDoubleClicked() {
  QApplication::clipboard()->setText(
      lbl_sig_freq_display_->text());
}

void SignalPanel::onSigAmpDisplayDoubleClicked() {
  QApplication::clipboard()->setText(
      lbl_sig_amp_display_->text());
}

void SignalPanel::onSigWaveformDisplayDoubleClicked() {
  QApplication::clipboard()->setText(
      lbl_sig_waveform_display_->text());
}

void SignalPanel::onNaSparamDisplayDoubleClicked() {
  QApplication::clipboard()->setText(
      lbl_na_sparam_display_->text());
}

}  // namespace mwa::gui
