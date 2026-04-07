/**
 * @file analysis_panel.h
 * @brief Analysis panel widget for reviewing experiment session data.
 * @author MWA Team
 * @date 2026-04-06
 *
 * Declares AnalysisPanel, a dockable QWidget that lets researchers load an
 * ExperimentSession from disk, browse captured camera frames via a scrolling
 * thumbnail strip, inspect VNA sweep metadata, and export data to disk using
 * CameraFrameExporter and VnaCsvExporter.
 *
 * @copyright LGPL-3.0-or-later
 */

#pragma once

#include <QComboBox>
#include <QFutureWatcher>
#include <QGroupBox>
#include <QKeyEvent>
#include <QLabel>
#include <QMap>
#include <QPixmap>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QTableWidget>
#include <QToolButton>
#include <QWidget>

#include "analysis/camera_frame_exporter.h"
#include "analysis/experiment_session.h"

namespace mwa::gui {

/**
 * @class AnalysisPanel
 * @brief Panel for reviewing a loaded ExperimentSession and exporting data.
 *
 * AnalysisPanel is a QWidget intended to be placed inside a QDockWidget.
 * It contains five sections:
 *  - **Session**       — load a session JSON file, view metadata.
 *  - **Image Browser** — scrolling 80×60 thumbnail strip with page navigation.
 *  - **Frame Detail**  — 320×240 preview of the selected frame.
 *  - **VNA Data**      — sweep metadata table (index, point count, freq range).
 *  - **Export**        — export frames as PNG/TIFF or VNA data as CSV.
 *
 * Image export is performed asynchronously (via QFutureWatcher) to keep the
 * UI responsive.  CSV export is synchronous (typically < 1 ms).
 *
 * @note Not wired to live hardware — operates on persisted session data only.
 *
 * @see mwa::analysis::ExperimentSession
 * @see mwa::analysis::CameraFrameExporter
 * @see mwa::analysis::VnaCsvExporter
 * @see mwa::analysis::SessionSerializer
 */
class AnalysisPanel : public QWidget {
  Q_OBJECT

 public:
  /**
   * @brief Construct the analysis panel with all sub-widgets.
   *
   * All groups except Session are disabled until a session is loaded.
   *
   * @param parent Optional parent widget for Qt ownership.
   */
  explicit AnalysisPanel(QWidget* parent = nullptr);

 protected:
  /**
   * @brief Handle key presses for frame navigation and shortcut keys.
   *
   * Left/Right navigate between frames; Escape clears the selection.
   * Ctrl+O, Ctrl+E, Ctrl+Shift+E are handled via QShortcut instead.
   *
   * @param event The key event.
   */
  void keyPressEvent(QKeyEvent* event) override;

 private slots:
  /**
   * @brief Open a file dialog and load the chosen session JSON.
   */
  void onLoadSessionClicked();

  /**
   * @brief Reload the session from the file path at @p index in the combo.
   *
   * @param index Index of the selected item in cmbSession.
   */
  void onSessionComboChanged(int index);

  /**
   * @brief Select the frame at @p thumb_index and update the detail view.
   *
   * @param thumb_index Zero-based index into the session's frame list.
   */
  void onThumbnailClicked(int thumb_index);

  /**
   * @brief Navigate the thumbnail strip one page backward.
   */
  void onPrevPage();

  /**
   * @brief Navigate the thumbnail strip one page forward.
   */
  void onNextPage();

  /**
   * @brief Open a directory dialog and export all frames asynchronously.
   */
  void onExportFramesClicked();

  /**
   * @brief Open a file dialog and export VNA data to a CSV file.
   */
  void onExportCsvClicked();

  /**
   * @brief Called when the asynchronous frame export future completes.
   */
  void onExportFinished();

 private:
  /**
   * @brief Build and return the Session group box.
   * @return Pointer to the created QGroupBox (owned by this widget).
   */
  QGroupBox* createSessionGroup();

  /**
   * @brief Build and return the Image Browser group box.
   * @return Pointer to the created QGroupBox (owned by this widget).
   */
  QGroupBox* createImageBrowserGroup();

  /**
   * @brief Build and return the Frame Detail group box.
   * @return Pointer to the created QGroupBox (owned by this widget).
   */
  QGroupBox* createFrameDetailGroup();

  /**
   * @brief Build and return the VNA Data group box.
   * @return Pointer to the created QGroupBox (owned by this widget).
   */
  QGroupBox* createVnaDataGroup();

  /**
   * @brief Build and return the Export group box.
   * @return Pointer to the created QGroupBox (owned by this widget).
   */
  QGroupBox* createExportGroup();

  /**
   * @brief Populate all groups from the currently loaded session.
   *
   * Resets thumbnail page and selection, rebuilds thumbnail strip,
   * fills VNA table, updates session info label, and enables groups.
   */
  void populateFromSession();

  /**
   * @brief Rebuild the thumbnail strip for the current page.
   *
   * Clears all QToolButton children of wgt_thumbnail_strip_ and
   * re-populates them with thumbnails for the current page.
   * Updates the enabled state of prev/next page buttons.
   */
  void rebuildThumbnailPage();

  /**
   * @brief Select the frame at @p index (-1 to clear selection).
   *
   * Updates the Frame Detail preview and info label.  If the frame is
   * not on the current page, navigates to the correct page first.
   * Does nothing if @p index is out of range.
   *
   * @param index Zero-based frame index, or -1 to clear.
   */
  void selectFrame(int index);

  /**
   * @brief Enable or disable the data groups (Image Browser, Frame Detail,
   *        VNA Data, Export).
   *
   * @param enabled true to enable all data groups.
   */
  void setDataGroupsEnabled(bool enabled);

  /**
   * @brief Enter or leave the "exporting" UI state.
   *
   * Disables the load button and export buttons; shows/hides and
   * configures the progress bar.
   *
   * @param exporting true when export is in progress.
   */
  Q_INVOKABLE void setExportingState(bool exporting);

  /**
   * @brief Update the session info label from the loaded session metadata.
   */
  void updateSessionInfo();

  /**
   * @brief Reset the Frame Detail group to the "no frame selected" state.
   */
  void clearFrameDetail();

  /**
   * @brief Show @p msg in the parent window's status bar for @p msec ms.
   *
   * Does nothing if the parent window has no QStatusBar.
   *
   * @param msg  Message to display.
   * @param msec Duration in milliseconds (default 3000).
   */
  void showStatusMessage(const QString& msg, int msec = 3000);

  // ---- Session group -------------------------------------------------------
  QGroupBox*   grp_session_{nullptr};       ///< Session section.
  QComboBox*   cmb_session_{nullptr};       ///< Recent sessions selector.
  QPushButton* btn_load_{nullptr};          ///< Open file dialog button.
  QLabel*      lbl_session_info_{nullptr};  ///< Frames / sweeps / saved text.

  // ---- Image Browser group -------------------------------------------------
  QGroupBox*   grp_image_browser_{nullptr};     ///< Image Browser section.
  QScrollArea* scroll_thumbnails_{nullptr};     ///< Horizontal thumbnail scroll.
  QWidget*     wgt_thumbnail_strip_{nullptr};   ///< Container inside scroll area.
  QPushButton* btn_prev_page_{nullptr};         ///< Previous page button.
  QPushButton* btn_next_page_{nullptr};         ///< Next page button.

  // ---- Frame Detail group --------------------------------------------------
  QGroupBox* grp_frame_detail_{nullptr};    ///< Frame Detail section.
  QLabel*    lbl_frame_preview_{nullptr};   ///< 320×240 frame preview.
  QLabel*    lbl_frame_info_{nullptr};      ///< "Frame NNNN / TTTT | W × H".

  // ---- VNA Data group ------------------------------------------------------
  QGroupBox*    grp_vna_data_{nullptr};     ///< VNA Data section.
  QLabel*       lbl_vna_summary_{nullptr};  ///< "Sweeps: N | Freq range: …".
  QTableWidget* tbl_vna_sweeps_{nullptr};   ///< Sweep metadata table.

  // ---- Export group --------------------------------------------------------
  QGroupBox*    grp_export_{nullptr};         ///< Export section.
  QComboBox*    cmb_image_format_{nullptr};   ///< PNG / TIFF selector.
  QPushButton*  btn_export_frames_{nullptr};  ///< Export Frames… button.
  QPushButton*  btn_export_csv_{nullptr};     ///< Export CSV… button.
  QProgressBar* prg_export_{nullptr};         ///< Export progress (hidden idle).

  // ---- State ---------------------------------------------------------------
  /// The loaded session (owned by this widget via Qt parent).
  mwa::analysis::ExperimentSession* session_{nullptr};

  /// Current thumbnail page (zero-based; 4 thumbs per page).
  int current_page_{0};

  /// Currently selected frame index, or -1 if none.
  int selected_frame_{-1};

  /// File paths parallel to cmb_session_ items (for re-loading).
  QStringList session_file_paths_;

  /// Cached scaled thumbnails keyed by frame index; populated on session load.
  QMap<int, QPixmap> thumbnail_cache_;

  /// Watcher for the asynchronous frame export future.
  QFutureWatcher<bool>* export_watcher_{nullptr};

  // ---- Constants -----------------------------------------------------------
  static constexpr int kThumbWidth    = 80;   ///< Thumbnail width in pixels.
  static constexpr int kThumbHeight   = 60;   ///< Thumbnail height in pixels.
  static constexpr int kPreviewWidth  = 320;  ///< Detail preview width.
  static constexpr int kPreviewHeight = 240;  ///< Detail preview height.
  static constexpr int kThumbsPerPage = 4;    ///< Thumbnails shown per page.
};

}  // namespace mwa::gui
