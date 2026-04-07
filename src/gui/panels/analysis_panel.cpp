/**
 * @file analysis_panel.cpp
 * @brief Implementation of the AnalysisPanel widget.
 * @author MWA Team
 * @date 2026-04-06
 *
 * @copyright LGPL-3.0-or-later
 */

#include "gui/panels/analysis_panel.h"

#include <QtConcurrent/QtConcurrent>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QScrollBar>
#include <QShortcut>
#include <QStatusBar>
#include <QVBoxLayout>

#include "analysis/session_serializer.h"
#include "analysis/vna_csv_exporter.h"

namespace mwa::gui {

// ---- Construction ---------------------------------------------------------

AnalysisPanel::AnalysisPanel(QWidget* parent) : QWidget(parent) {
  setObjectName(QStringLiteral("analysisPanel"));
  setFocusPolicy(Qt::StrongFocus);

  session_ = new mwa::analysis::ExperimentSession(this);

  auto* root_layout = new QVBoxLayout(this);
  root_layout->setContentsMargins(8, 8, 8, 8);
  root_layout->setSpacing(8);

  root_layout->addWidget(createSessionGroup());
  root_layout->addWidget(createImageBrowserGroup());
  root_layout->addWidget(createFrameDetailGroup());
  root_layout->addWidget(createVnaDataGroup());
  root_layout->addWidget(createExportGroup());
  root_layout->addStretch();

  setDataGroupsEnabled(false);

  export_watcher_ = new QFutureWatcher<bool>(this);
  connect(export_watcher_, &QFutureWatcher<bool>::finished,
          this, &AnalysisPanel::onExportFinished);

  auto* sc_load = new QShortcut(
      QKeySequence(QStringLiteral("Ctrl+O")), this,
      nullptr, nullptr, Qt::WidgetWithChildrenShortcut);
  connect(sc_load, &QShortcut::activated,
          this, &AnalysisPanel::onLoadSessionClicked);

  auto* sc_export_frames = new QShortcut(
      QKeySequence(QStringLiteral("Ctrl+E")), this,
      nullptr, nullptr, Qt::WidgetWithChildrenShortcut);
  connect(sc_export_frames, &QShortcut::activated,
          this, &AnalysisPanel::onExportFramesClicked);

  auto* sc_export_csv = new QShortcut(
      QKeySequence(QStringLiteral("Ctrl+Shift+E")), this,
      nullptr, nullptr, Qt::WidgetWithChildrenShortcut);
  connect(sc_export_csv, &QShortcut::activated,
          this, &AnalysisPanel::onExportCsvClicked);
}

// ---- keyPressEvent --------------------------------------------------------

void AnalysisPanel::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Left) {
    selectFrame(selected_frame_ - 1);
    return;
  }
  if (event->key() == Qt::Key_Right) {
    selectFrame(selected_frame_ + 1);
    return;
  }
  if (event->key() == Qt::Key_Escape) {
    selectFrame(-1);
    return;
  }
  QWidget::keyPressEvent(event);
}

// ---- Private slots --------------------------------------------------------

void AnalysisPanel::onLoadSessionClicked() {
  const QString file_path = QFileDialog::getOpenFileName(
      this,
      QStringLiteral("Load Experiment Session"),
      QString{},
      QStringLiteral("Session files (*.json);;All files (*)"));

  if (file_path.isEmpty()) {
    return;
  }

  if (!mwa::analysis::SessionSerializer::load(file_path, *session_)) {
    QMessageBox::warning(
        this,
        QStringLiteral("Load Failed"),
        QStringLiteral("Could not load session:\n") +
            mwa::analysis::SessionSerializer::lastError());
    return;
  }

  {
    QSignalBlocker blocker(cmb_session_);
    if (!session_file_paths_.contains(file_path)) {
      session_file_paths_.prepend(file_path);
      cmb_session_->insertItem(0, QFileInfo(file_path).fileName());
    }
    cmb_session_->setCurrentIndex(session_file_paths_.indexOf(file_path));
  }

  populateFromSession();
}

void AnalysisPanel::onSessionComboChanged(int index) {
  if (index < 0 || index >= session_file_paths_.size()) {
    return;
  }
  const QString file_path = session_file_paths_.at(index);
  if (!mwa::analysis::SessionSerializer::load(file_path, *session_)) {
    QMessageBox::warning(
        this,
        QStringLiteral("Load Failed"),
        QStringLiteral("Could not load session:\n") +
            mwa::analysis::SessionSerializer::lastError());
    return;
  }
  populateFromSession();
}

void AnalysisPanel::onThumbnailClicked(int thumb_index) {
  selectFrame(thumb_index);
}

void AnalysisPanel::onPrevPage() {
  if (current_page_ > 0) {
    --current_page_;
    rebuildThumbnailPage();
  }
}

void AnalysisPanel::onNextPage() {
  const int frame_count = session_->cameraFrameCount();
  const int last_page   = (frame_count - 1) / kThumbsPerPage;
  if (current_page_ < last_page) {
    ++current_page_;
    rebuildThumbnailPage();
  }
}

void AnalysisPanel::onExportFramesClicked() {
  if (session_->cameraFrameCount() == 0) {
    QMessageBox::information(
        this,
        QStringLiteral("No Frames"),
        QStringLiteral("The loaded session contains no camera frames."));
    return;
  }

  const QString dir = QFileDialog::getExistingDirectory(
      this, QStringLiteral("Export Frames To..."));
  if (dir.isEmpty()) {
    return;
  }

  const auto fmt = static_cast<mwa::analysis::ImageFormat>(
      cmb_image_format_->currentData().toInt());

  setExportingState(true);

  // session_ outlives the future: setExportingState(true) disables Load,
  // preventing session replacement while export runs.
  const mwa::analysis::ExperimentSession* session_ptr = session_;
  export_watcher_->setFuture(
      QtConcurrent::run([session_ptr, dir, fmt]() {
        return mwa::analysis::CameraFrameExporter::exportToDir(
            *session_ptr, dir, fmt);
      }));
}

void AnalysisPanel::onExportCsvClicked() {
  if (session_->vnaMeasurementCount() == 0) {
    QMessageBox::information(
        this,
        QStringLiteral("No VNA Data"),
        QStringLiteral("The loaded session contains no VNA measurements."));
    return;
  }

  const QString file_path = QFileDialog::getSaveFileName(
      this,
      QStringLiteral("Export VNA Data As CSV"),
      session_->name() + QStringLiteral(".csv"),
      QStringLiteral("CSV files (*.csv);;All files (*)"));
  if (file_path.isEmpty()) {
    return;
  }

  if (!mwa::analysis::VnaCsvExporter::exportToFile(*session_, file_path)) {
    QMessageBox::warning(
        this,
        QStringLiteral("Export Failed"),
        QStringLiteral("Could not write CSV:\n") +
            mwa::analysis::VnaCsvExporter::lastError());
    return;
  }

  showStatusMessage(QStringLiteral("CSV export complete."));
}

void AnalysisPanel::onExportFinished() {
  const bool ok = export_watcher_->result();
  setExportingState(false);

  if (!ok) {
    QMessageBox::warning(
        this,
        QStringLiteral("Export Failed"),
        QStringLiteral("Could not export frames:\n") +
            mwa::analysis::CameraFrameExporter::lastError());
  } else {
    showStatusMessage(QStringLiteral("Frame export complete."));
  }
}

// ---- Group creation -------------------------------------------------------

QGroupBox* AnalysisPanel::createSessionGroup() {
  grp_session_ = new QGroupBox(QStringLiteral("Session"), this);
  grp_session_->setObjectName(QStringLiteral("grpSession"));

  cmb_session_ = new QComboBox(grp_session_);
  cmb_session_->setObjectName(QStringLiteral("cmbSession"));
  cmb_session_->setMinimumWidth(180);
  cmb_session_->setPlaceholderText(QStringLiteral("No session loaded"));
  cmb_session_->setToolTip(QStringLiteral("Recently loaded sessions"));

  btn_load_ = new QPushButton(QStringLiteral("Load"), grp_session_);
  btn_load_->setObjectName(QStringLiteral("btnLoadSession"));
  btn_load_->setMinimumSize(60, 32);
  btn_load_->setToolTip(QStringLiteral("Load session from file (Ctrl+O)"));

  lbl_session_info_ = new QLabel(
      QStringLiteral("Frames: \u2013 | VNA Sweeps: \u2013 | Saved: \u2013"),
      grp_session_);
  lbl_session_info_->setObjectName(QStringLiteral("lblSessionInfo"));

  auto* row = new QHBoxLayout;
  row->addWidget(cmb_session_, 1);
  row->addWidget(btn_load_);

  auto* layout = new QVBoxLayout(grp_session_);
  layout->addLayout(row);
  layout->addWidget(lbl_session_info_);

  connect(btn_load_, &QPushButton::clicked,
          this, &AnalysisPanel::onLoadSessionClicked);
  connect(cmb_session_,
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &AnalysisPanel::onSessionComboChanged);

  return grp_session_;
}

QGroupBox* AnalysisPanel::createImageBrowserGroup() {
  grp_image_browser_ = new QGroupBox(QStringLiteral("Image Browser"), this);
  grp_image_browser_->setObjectName(QStringLiteral("grpImageBrowser"));

  wgt_thumbnail_strip_ = new QWidget;
  wgt_thumbnail_strip_->setObjectName(QStringLiteral("wgtThumbnailStrip"));
  auto* strip_layout = new QHBoxLayout(wgt_thumbnail_strip_);
  strip_layout->setContentsMargins(4, 4, 4, 4);
  strip_layout->setSpacing(4);
  strip_layout->addStretch();

  scroll_thumbnails_ = new QScrollArea(grp_image_browser_);
  scroll_thumbnails_->setObjectName(QStringLiteral("scrollThumbnails"));
  scroll_thumbnails_->setWidget(wgt_thumbnail_strip_);
  scroll_thumbnails_->setWidgetResizable(true);
  scroll_thumbnails_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
  scroll_thumbnails_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  scroll_thumbnails_->setMinimumHeight(kThumbHeight + 44);

  btn_prev_page_ = new QPushButton(QStringLiteral("\u25C4"), grp_image_browser_);
  btn_prev_page_->setObjectName(QStringLiteral("btnPrevPage"));
  btn_prev_page_->setFlat(true);
  btn_prev_page_->setMinimumSize(32, 32);
  btn_prev_page_->setToolTip(QStringLiteral("Previous page"));
  btn_prev_page_->setEnabled(false);

  btn_next_page_ = new QPushButton(QStringLiteral("\u25BA"), grp_image_browser_);
  btn_next_page_->setObjectName(QStringLiteral("btnNextPage"));
  btn_next_page_->setFlat(true);
  btn_next_page_->setMinimumSize(32, 32);
  btn_next_page_->setToolTip(QStringLiteral("Next page"));
  btn_next_page_->setEnabled(false);

  auto* nav_row = new QHBoxLayout;
  nav_row->addStretch();
  nav_row->addWidget(btn_prev_page_);
  nav_row->addWidget(btn_next_page_);

  auto* layout = new QVBoxLayout(grp_image_browser_);
  layout->addWidget(scroll_thumbnails_);
  layout->addLayout(nav_row);

  connect(btn_prev_page_, &QPushButton::clicked,
          this, &AnalysisPanel::onPrevPage);
  connect(btn_next_page_, &QPushButton::clicked,
          this, &AnalysisPanel::onNextPage);

  return grp_image_browser_;
}

QGroupBox* AnalysisPanel::createFrameDetailGroup() {
  grp_frame_detail_ = new QGroupBox(QStringLiteral("Frame Detail"), this);
  grp_frame_detail_->setObjectName(QStringLiteral("grpFrameDetail"));

  lbl_frame_preview_ = new QLabel(grp_frame_detail_);
  lbl_frame_preview_->setObjectName(QStringLiteral("lblFramePreview"));
  lbl_frame_preview_->setFixedSize(kPreviewWidth, kPreviewHeight);
  lbl_frame_preview_->setAlignment(Qt::AlignCenter);
  lbl_frame_preview_->setFrameShape(QFrame::Box);
  lbl_frame_preview_->setText(QStringLiteral("(no frame selected)"));

  lbl_frame_info_ = new QLabel(
      QStringLiteral("Frame: \u2013 / \u2013 | Size: \u2013 \u00D7 \u2013"),
      grp_frame_detail_);
  lbl_frame_info_->setObjectName(QStringLiteral("lblFrameInfo"));
  lbl_frame_info_->setAlignment(Qt::AlignCenter);

  auto* layout = new QVBoxLayout(grp_frame_detail_);
  layout->addWidget(lbl_frame_preview_, 0, Qt::AlignHCenter);
  layout->addWidget(lbl_frame_info_);

  return grp_frame_detail_;
}

QGroupBox* AnalysisPanel::createVnaDataGroup() {
  grp_vna_data_ = new QGroupBox(QStringLiteral("VNA Data"), this);
  grp_vna_data_->setObjectName(QStringLiteral("grpVnaData"));

  lbl_vna_summary_ = new QLabel(
      QStringLiteral("Sweeps: 0 | Freq range: \u2013"),
      grp_vna_data_);
  lbl_vna_summary_->setObjectName(QStringLiteral("lblVnaSummary"));

  tbl_vna_sweeps_ = new QTableWidget(0, 4, grp_vna_data_);
  tbl_vna_sweeps_->setObjectName(QStringLiteral("tblVnaSweeps"));
  tbl_vna_sweeps_->setHorizontalHeaderLabels(
      {QStringLiteral("Sweep"),
       QStringLiteral("Points"),
       QStringLiteral("Start Hz"),
       QStringLiteral("Stop Hz")});
  tbl_vna_sweeps_->setSelectionMode(QAbstractItemView::SingleSelection);
  tbl_vna_sweeps_->setSelectionBehavior(QAbstractItemView::SelectRows);
  tbl_vna_sweeps_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  tbl_vna_sweeps_->verticalHeader()->setVisible(false);
  tbl_vna_sweeps_->horizontalHeader()->setStretchLastSection(true);
  tbl_vna_sweeps_->setMinimumHeight(80);

  auto* layout = new QVBoxLayout(grp_vna_data_);
  layout->addWidget(lbl_vna_summary_);
  layout->addWidget(tbl_vna_sweeps_);

  return grp_vna_data_;
}

QGroupBox* AnalysisPanel::createExportGroup() {
  grp_export_ = new QGroupBox(QStringLiteral("Export"), this);
  grp_export_->setObjectName(QStringLiteral("grpExport"));

  cmb_image_format_ = new QComboBox(grp_export_);
  cmb_image_format_->setObjectName(QStringLiteral("cmbImageFormat"));
  cmb_image_format_->addItem(QStringLiteral("PNG"),
      static_cast<int>(mwa::analysis::ImageFormat::kPng));
  cmb_image_format_->addItem(QStringLiteral("TIFF"),
      static_cast<int>(mwa::analysis::ImageFormat::kTiff));
  cmb_image_format_->setMinimumSize(80, 32);

  btn_export_frames_ = new QPushButton(
      QStringLiteral("Export Frames\u2026"), grp_export_);
  btn_export_frames_->setObjectName(QStringLiteral("btnExportFrames"));
  btn_export_frames_->setMinimumSize(140, 32);
  btn_export_frames_->setToolTip(
      QStringLiteral("Export all frames to a directory (Ctrl+E)"));

  btn_export_csv_ = new QPushButton(
      QStringLiteral("Export CSV\u2026"), grp_export_);
  btn_export_csv_->setObjectName(QStringLiteral("btnExportCsv"));
  btn_export_csv_->setMinimumSize(140, 32);
  btn_export_csv_->setToolTip(
      QStringLiteral("Export VNA data to CSV (Ctrl+Shift+E)"));

  prg_export_ = new QProgressBar(grp_export_);
  prg_export_->setObjectName(QStringLiteral("prgExport"));
  prg_export_->setRange(0, 0);  // indeterminate by default
  prg_export_->setTextVisible(false);
  prg_export_->setVisible(false);

  auto* form = new QFormLayout;
  auto* images_row = new QHBoxLayout;
  images_row->addWidget(cmb_image_format_);
  images_row->addWidget(btn_export_frames_);
  form->addRow(QStringLiteral("Images:"), images_row);
  form->addRow(QStringLiteral("VNA:"), btn_export_csv_);

  auto* layout = new QVBoxLayout(grp_export_);
  layout->addLayout(form);
  layout->addWidget(prg_export_);

  connect(btn_export_frames_, &QPushButton::clicked,
          this, &AnalysisPanel::onExportFramesClicked);
  connect(btn_export_csv_, &QPushButton::clicked,
          this, &AnalysisPanel::onExportCsvClicked);

  return grp_export_;
}

// ---- Population helpers ---------------------------------------------------

void AnalysisPanel::populateFromSession() {
  current_page_    = 0;
  selected_frame_  = -1;

  updateSessionInfo();
  clearFrameDetail();

  // Pre-scale all thumbnails once so rebuildThumbnailPage() never re-scales.
  thumbnail_cache_.clear();
  const int frame_count = session_->cameraFrameCount();
  for (int i = 0; i < frame_count; ++i) {
    const QImage& img = session_->cameraFrames().at(i).image;
    thumbnail_cache_[i] = QPixmap::fromImage(
        img.scaled(kThumbWidth, kThumbHeight,
                   Qt::KeepAspectRatio,
                   Qt::SmoothTransformation));
  }

  rebuildThumbnailPage();

  const auto& sweeps = session_->vnaMeasurements();
  {
    QSignalBlocker blocker(tbl_vna_sweeps_);
    tbl_vna_sweeps_->setRowCount(0);
    tbl_vna_sweeps_->setRowCount(sweeps.size());
    for (int i = 0; i < sweeps.size(); ++i) {
      const auto& s = sweeps[i];
      tbl_vna_sweeps_->setItem(
          i, 0, new QTableWidgetItem(QString::number(i)));
      tbl_vna_sweeps_->setItem(
          i, 1, new QTableWidgetItem(
              QString::number(s.frequencies.size())));
      tbl_vna_sweeps_->setItem(
          i, 2, new QTableWidgetItem(
              QString::number(s.start_frequency, 'f', 0)));
      tbl_vna_sweeps_->setItem(
          i, 3, new QTableWidgetItem(
              QString::number(s.stop_frequency, 'f', 0)));
    }
  }

  if (sweeps.isEmpty()) {
    lbl_vna_summary_->setText(
        QStringLiteral("Sweeps: 0 | Freq range: \u2013"));
  } else {
    const double min_freq = sweeps.first().start_frequency;
    const double max_freq = sweeps.last().stop_frequency;
    lbl_vna_summary_->setText(
        QStringLiteral("Sweeps: %1 | Freq range: %2 \u2013 %3 Hz")
            .arg(sweeps.size())
            .arg(min_freq, 0, 'f', 0)
            .arg(max_freq, 0, 'f', 0));
  }

  setDataGroupsEnabled(true);
}

void AnalysisPanel::rebuildThumbnailPage() {
  auto* strip_layout =
      qobject_cast<QHBoxLayout*>(wgt_thumbnail_strip_->layout());
  while (strip_layout->count() > 1) {
    auto* item = strip_layout->takeAt(0);
    if (auto* widget = item->widget()) {
      widget->deleteLater();
    }
    delete item;
  }

  const int frame_count = session_->cameraFrameCount();
  const int page_start  = current_page_ * kThumbsPerPage;
  const int page_end    = qMin(page_start + kThumbsPerPage, frame_count);

  for (int i = page_start; i < page_end; ++i) {
    auto* btn = new QToolButton(wgt_thumbnail_strip_);
    btn->setObjectName(QStringLiteral("thumb%1").arg(i));
    btn->setFixedSize(kThumbWidth + 4, kThumbHeight + 20);
    btn->setIcon(QIcon(thumbnail_cache_.value(i)));
    btn->setIconSize(QSize(kThumbWidth, kThumbHeight));
    btn->setText(QStringLiteral("frame_%1").arg(i, 4, 10, QLatin1Char('0')));
    btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    if (i == selected_frame_) {
      btn->setStyleSheet(
          QStringLiteral("QToolButton { border: 2px solid #3498DB; }"));
    }

    const int index = i;
    connect(btn, &QToolButton::clicked,
            this, [this, index]() { onThumbnailClicked(index); });

    strip_layout->insertWidget(strip_layout->count() - 1, btn);
  }

  btn_prev_page_->setEnabled(current_page_ > 0);
  const int last_page =
      frame_count > 0 ? (frame_count - 1) / kThumbsPerPage : 0;
  btn_next_page_->setEnabled(current_page_ < last_page);
}

void AnalysisPanel::selectFrame(int index) {
  const int frame_count = session_->cameraFrameCount();

  if (index < 0 || frame_count == 0) {
    index = -1;
  } else if (index >= frame_count) {
    index = frame_count - 1;
  }

  selected_frame_ = index;

  if (index == -1) {
    clearFrameDetail();
  } else {
    const QImage& img = session_->cameraFrames().at(index).image;
    lbl_frame_preview_->setText(QString{});
    lbl_frame_preview_->setPixmap(
        QPixmap::fromImage(img.scaled(kPreviewWidth, kPreviewHeight,
                                      Qt::KeepAspectRatio,
                                      Qt::SmoothTransformation)));
    lbl_frame_info_->setText(
        QStringLiteral("Frame: %1 / %2 | Size: %3 \u00D7 %4")
            .arg(index, 4, 10, QLatin1Char('0'))
            .arg(frame_count - 1, 4, 10, QLatin1Char('0'))
            .arg(img.width())
            .arg(img.height()));

    const int target_page = index / kThumbsPerPage;
    if (target_page != current_page_) {
      current_page_ = target_page;
    }
  }

  rebuildThumbnailPage();
}

void AnalysisPanel::setDataGroupsEnabled(bool enabled) {
  grp_image_browser_->setEnabled(enabled);
  grp_frame_detail_->setEnabled(enabled);
  grp_vna_data_->setEnabled(enabled);
  grp_export_->setEnabled(enabled);
}

void AnalysisPanel::setExportingState(bool exporting) {
  btn_load_->setEnabled(!exporting);
  cmb_session_->setEnabled(!exporting);
  btn_export_frames_->setEnabled(!exporting);
  btn_export_csv_->setEnabled(!exporting);
  cmb_image_format_->setEnabled(!exporting);
  prg_export_->setVisible(exporting);
}

void AnalysisPanel::updateSessionInfo() {
  const QString saved_str = session_->endTime().isValid()
      ? session_->endTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"))
      : QStringLiteral("\u2013");

  lbl_session_info_->setText(
      QStringLiteral("Frames: %1 | VNA Sweeps: %2 | Saved: %3")
          .arg(session_->cameraFrameCount())
          .arg(session_->vnaMeasurementCount())
          .arg(saved_str));
}

void AnalysisPanel::clearFrameDetail() {
  lbl_frame_preview_->setPixmap(QPixmap{});
  lbl_frame_preview_->setText(QStringLiteral("(click a thumbnail to preview)"));
  lbl_frame_info_->setText(
      QStringLiteral("Frame: \u2013 / \u2013 | Size: \u2013 \u00D7 \u2013"));
}

void AnalysisPanel::showStatusMessage(const QString& msg, int msec) {
  if (auto* status = window()->findChild<QStatusBar*>()) {
    status->showMessage(msg, msec);
  }
}

}  // namespace mwa::gui
