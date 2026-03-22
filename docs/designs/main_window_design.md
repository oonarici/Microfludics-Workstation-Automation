# Main Window Design Specification

**Task:** MWA-02-D
**Requirement:** GUI-REQ-001, NFR-005
**Designer:** UX Agent
**Date:** 2026-03-22
**Status:** Ready for Lead Review

---

## Overview

The Main Window is the application shell for the Microfluidics Workstation Automation (MWA) system. It hosts all device panels as dockable widgets, provides a central workspace for the camera live view, and exposes all primary application actions via menu bar and toolbar. The design targets researchers with basic computer skills who need fast, reliable access to all workstation controls from a single unified interface.

This document covers the shell layout for Sprint 2, Round 2. Dock areas are reserved placeholders — actual device panels (Log Panel: MWA-02-E, Device Status Dashboard: MWA-02-F) will be added in subsequent rounds.

---

## 1. ASCII Layout

```
┌──────────────────────────────────────────────────────────────────────────────────┐
│  MWA — Microfluidics Workstation Automation                              [_][□][X]│
├──────────────────────────────────────────────────────────────────────────────────┤
│  File   View   Devices   Experiment   Tools   Help                               │
├──────────────────────────────────────────────────────────────────────────────────┤
│  [New] [Open] [Save] | [Connect All] [Disconnect All] | [Start] [Stop] | [About] │
├───────────────────┬──────────────────────────────────────────────────────────────┤
│                   │                                                              │
│  LEFT DOCK AREA   │                CENTRAL WORKSPACE                            │
│  (reserved for    │                                                              │
│   device panels)  │         ┌─────────────────────────────────┐                │
│                   │         │                                 │                │
│  280–400px wide   │         │      Camera Live View /         │                │
│                   │         │      Image Viewer               │                │
│  [Future panels:] │         │      (placeholder label)        │                │
│  • LED            │         │                                 │                │
│  • Pump           │         │      "No camera connected"      │                │
│  • Sig. Generator │         │                                 │                │
│  • Camera         │         └─────────────────────────────────┘                │
│  • XYZ Stage      │                                                              │
│  • Status Dash    │                                                              │
│                   │                                                              │
├───────────────────┴──────────────────────────────────────────────────────────────┤
│  BOTTOM DOCK AREA (reserved for log panel — MWA-02-E)                           │
│  Height: ~150px collapsed / 250px expanded                                       │
├──────────────────────────────────────────────────────────────────────────────────┤
│  Devices: – – – – – – │  Last event: Application started    │  MWA v0.1.0      │
└──────────────────────────────────────────────────────────────────────────────────┘
```

### Layout Notes

- Minimum window size: 1024 x 768 px.
- Left dock area: `Qt::LeftDockWidgetArea`, initial width 300 px. Device panels stack vertically; can be tabbed by the user.
- Bottom dock area: `Qt::BottomDockWidgetArea`, initial height 150 px (collapsed). Log panel occupies this area.
- Central widget: `QStackedWidget` — index 0 is the placeholder; index 1 will be the live camera view widget (Phase 2).
- All dock widgets are floatable and closeable. Visibility is restored from `QSettings` on startup.
- Toolbar is a single `QToolBar` anchored to the top, not moveable by default.

---

## 2. Widget Table

### Menu Bar

| Widget | Type | objectName | Properties / Contents | Signals |
|--------|------|------------|----------------------|---------|
| Menu Bar | `QMenuBar` | `menuBar` | Native menu bar (macOS: merged into system menu bar) | — |
| File Menu | `QMenu` | `menuFile` | Title: "&File" | — |
| Action: Exit | `QAction` | `actionExit` | Text: "E&xit", shortcut: Alt+F4 (Win) / Cmd+Q (mac) | `triggered()` |
| View Menu | `QMenu` | `menuView` | Title: "&View" | — |
| Action: Toggle Left Dock | `QAction` | `actionToggleLeftDock` | Text: "&Device Panels", checkable: true, checked: true | `toggled(bool)` |
| Action: Toggle Bottom Dock | `QAction` | `actionToggleBottomDock` | Text: "&Log Panel", checkable: true, checked: true | `toggled(bool)` |
| Action: Toggle Toolbar | `QAction` | `actionToggleToolbar` | Text: "&Toolbar", checkable: true, checked: true | `toggled(bool)` |
| Action: Toggle Status Bar | `QAction` | `actionToggleStatusBar` | Text: "&Status Bar", checkable: true, checked: true | `toggled(bool)` |
| Devices Menu | `QMenu` | `menuDevices` | Title: "&Devices" | — |
| Action: Connect All | `QAction` | `actionConnectAll` | Text: "Connect &All Devices", icon: connect icon | `triggered()` |
| Action: Disconnect All | `QAction` | `actionDisconnectAll` | Text: "&Disconnect All Devices", icon: disconnect icon | `triggered()` |
| Action: Device Settings | `QAction` | `actionDeviceSettings` | Text: "Device &Settings..." | `triggered()` |
| Experiment Menu | `QMenu` | `menuExperiment` | Title: "&Experiment" | — |
| Action: New Experiment | `QAction` | `actionNewExperiment` | Text: "&New Experiment", icon: new icon | `triggered()` |
| Action: Open Experiment | `QAction` | `actionOpenExperiment` | Text: "&Open Experiment...", icon: open icon | `triggered()` |
| Action: Save Experiment | `QAction` | `actionSaveExperiment` | Text: "&Save Experiment", icon: save icon | `triggered()` |
| Action: Start Experiment | `QAction` | `actionStartExperiment` | Text: "&Start", icon: start icon, enabled: false initially | `triggered()` |
| Action: Stop Experiment | `QAction` | `actionStopExperiment` | Text: "S&top", icon: stop icon, enabled: false initially | `triggered()` |
| Tools Menu | `QMenu` | `menuTools` | Title: "&Tools" | — |
| Action: Preferences | `QAction` | `actionPreferences` | Text: "&Preferences..." | `triggered()` |
| Action: Export Log | `QAction` | `actionExportLog` | Text: "&Export Log..." | `triggered()` |
| Help Menu | `QMenu` | `menuHelp` | Title: "&Help" | — |
| Action: About | `QAction` | `actionAbout` | Text: "&About MWA..." | `triggered()` |
| Action: About Qt | `QAction` | `actionAboutQt` | Text: "About &Qt..." | `triggered()` |

### Toolbar

| Widget | Type | objectName | Properties | Signals |
|--------|------|------------|------------|---------|
| Main Toolbar | `QToolBar` | `mainToolbar` | Title: "Main Toolbar", moveable: false, icon size: 24x24 px | — |
| Button: New | `QToolButton` (via QAction) | — | Action: `actionNewExperiment`, tooltip: "New Experiment (Ctrl+N)" | — |
| Button: Open | `QToolButton` (via QAction) | — | Action: `actionOpenExperiment`, tooltip: "Open Experiment (Ctrl+O)" | — |
| Button: Save | `QToolButton` (via QAction) | — | Action: `actionSaveExperiment`, tooltip: "Save Experiment (Ctrl+S)" | — |
| Separator | `QAction` (separator) | — | — | — |
| Button: Connect All | `QToolButton` (via QAction) | — | Action: `actionConnectAll`, tooltip: "Connect All Devices (Ctrl+Shift+C)" | — |
| Button: Disconnect All | `QToolButton` (via QAction) | — | Action: `actionDisconnectAll`, tooltip: "Disconnect All Devices (Ctrl+Shift+D)" | — |
| Separator | `QAction` (separator) | — | — | — |
| Button: Start | `QToolButton` (via QAction) | — | Action: `actionStartExperiment`, tooltip: "Start Experiment (F5)", enabled: false initially | — |
| Button: Stop | `QToolButton` (via QAction) | — | Action: `actionStopExperiment`, tooltip: "Stop Experiment (F6)", enabled: false initially | — |
| Separator | `QAction` (separator) | — | — | — |
| Button: About | `QToolButton` (via QAction) | — | Action: `actionAbout`, tooltip: "About MWA" | — |

### Central Widget

| Widget | Type | objectName | Properties | Signals |
|--------|------|------------|------------|---------|
| Central Stack | `QStackedWidget` | `centralStack` | Index 0: placeholder; Index 1: camera view (Phase 2) | `currentChanged(int)` |
| Placeholder Widget | `QWidget` | `placeholderWidget` | Background: `#2C3E50` (dark), contains `lblNoCameraText` | — |
| No Camera Label | `QLabel` | `lblNoCameraText` | Text: "No camera connected\nConnect a camera to view live feed.", alignment: center, font size: 14pt, color: `#95A5A6` | — |

### Dock Areas (Placeholder)

| Widget | Type | objectName | Properties | Signals |
|--------|------|------------|------------|---------|
| Left Dock Container | `QDockWidget` | `dockDevicePanels` | Title: "Device Panels", area: `Qt::LeftDockWidgetArea`, min width: 280 px, max width: 400 px, features: DockWidgetClosable \| DockWidgetMovable \| DockWidgetFloatable | `visibilityChanged(bool)` |
| Left Dock Placeholder | `QWidget` | `wgtDevicePanelsPlaceholder` | Background: `#ECF0F1`, contains `lblDevicePanelsPlaceholder` | — |
| Left Dock Label | `QLabel` | `lblDevicePanelsPlaceholder` | Text: "Device panels will appear here.", alignment: center, font size: 12pt, color: `#95A5A6` | — |
| Bottom Dock Container | `QDockWidget` | `dockLogPanel` | Title: "Log", area: `Qt::BottomDockWidgetArea`, min height: 100 px, features: DockWidgetClosable \| DockWidgetMovable \| DockWidgetFloatable | `visibilityChanged(bool)` |
| Bottom Dock Placeholder | `QWidget` | `wgtLogPanelPlaceholder` | Background: `#ECF0F1`, contains `lblLogPanelPlaceholder` | — |
| Bottom Dock Label | `QLabel` | `lblLogPanelPlaceholder` | Text: "Log panel will appear here (MWA-02-E).", alignment: center, font size: 12pt, color: `#95A5A6` | — |

### Status Bar

| Widget | Type | objectName | Properties | Signals |
|--------|------|------------|------------|---------|
| Status Bar | `QStatusBar` | `statusBar` | Sizing grip: visible | — |
| Device Summary Label | `QLabel` | `lblDeviceSummary` | Text: "Devices: –", min width: 200 px, placed on left via `addWidget()` | — |
| Last Event Label | `QLabel` | `lblLastEvent` | Text: "Last event: Application started", stretch: 1 (center), placed via `addWidget()` with stretch | — |
| Version Label | `QLabel` | `lblVersion` | Text: "MWA v0.1.0", placed on right via `addPermanentWidget()` | — |

---

## 3. State Table

| UI State | Trigger | Affected Widgets | Visual Change |
|----------|---------|-----------------|---------------|
| **Startup / Idle** | Application launches | All | Window title set; status bar shows "Devices: –" and "Application started"; dock areas show placeholders; Start/Stop actions disabled |
| **No Devices Connected** | No devices connected | `lblDeviceSummary`, toolbar Start/Stop, `actionStartExperiment`, `actionStopExperiment` | Device summary: "Devices: 0 / 0 connected"; Start disabled; Connect All enabled |
| **Devices Connecting** | User clicks Connect All | `actionConnectAll` toolbar button, `lblDeviceSummary`, `lblLastEvent` | Connect All button shows busy state (tooltip: "Connecting..."); status bar: "Connecting to devices..." |
| **Devices Connected (all)** | All devices report connected | `lblDeviceSummary`, `actionStartExperiment` | Device summary: "Devices: N / N connected" in `#27AE60` green; Start action enabled |
| **Devices Connected (partial)** | Some devices connected, some not | `lblDeviceSummary` | Device summary: "Devices: M / N connected" in `#F39C12` orange |
| **Devices Error** | One or more devices in error state | `lblDeviceSummary`, `lblLastEvent` | Device summary: "Devices: error" in `#E74C3C` red; last event shows error message |
| **Experiment Running** | User clicks Start | `actionStartExperiment`, `actionStopExperiment`, `actionNewExperiment`, `actionOpenExperiment`, `actionConnectAll`, `actionDisconnectAll` | Start disabled; Stop enabled and highlighted; New/Open/Connect All/Disconnect All disabled; window title: "MWA — Running: [Experiment Name]" |
| **Experiment Stopped** | User clicks Stop or experiment ends | Same as above | Revert to Connected state; Stop disabled; Start enabled (if devices still connected); window title: "MWA — [Experiment Name]" |
| **Left Dock Hidden** | User hides Device Panels dock | `dockDevicePanels`, `actionToggleLeftDock` | Dock hidden; View menu item unchecked |
| **Bottom Dock Hidden** | User hides Log dock | `dockLogPanel`, `actionToggleBottomDock` | Dock hidden; View menu item unchecked |
| **Platform SDK Unavailable** | Device SDK not present on platform | Relevant future device panel | (Future: device panel shows "Not available on this platform" with `#95A5A6` gray indicator — handled by individual panels, not the shell) |

---

## 4. Interaction List

| User Action | UI Response | Signal / Slot |
|-------------|------------|---------------|
| Launch application | Window opens at minimum 1024x768, title set, dock placeholders visible, status bar shows "Ready" | `MainWindow::MainWindow()` constructor |
| Resize window below 1024x768 | Window enforces minimum size | `setMinimumSize(1024, 768)` |
| File > Exit | Application closes (no confirmation unless experiment is running) | `actionExit::triggered()` → `QApplication::quit()` |
| File > Exit (experiment running) | `QMessageBox::question` — "An experiment is running. Stop and exit?" | `actionExit::triggered()` → confirmation dialog |
| View > Device Panels (toggle) | Left dock shown/hidden | `actionToggleLeftDock::toggled(bool)` → `dockDevicePanels::setVisible(bool)` |
| View > Log Panel (toggle) | Bottom dock shown/hidden | `actionToggleBottomDock::toggled(bool)` → `dockLogPanel::setVisible(bool)` |
| View > Toolbar (toggle) | Toolbar shown/hidden | `actionToggleToolbar::toggled(bool)` → `mainToolbar::setVisible(bool)` |
| View > Status Bar (toggle) | Status bar shown/hidden | `actionToggleStatusBar::toggled(bool)` → `statusBar::setVisible(bool)` |
| Devices > Connect All | Attempts connection for all registered device panels | `actionConnectAll::triggered()` → `MainWindow::connectAllDevices()` |
| Devices > Disconnect All | `QMessageBox::question` — "Disconnect all devices?" → disconnect all | `actionDisconnectAll::triggered()` → confirmation → `MainWindow::disconnectAllDevices()` |
| Devices > Device Settings | Opens device settings dialog (placeholder — future sprint) | `actionDeviceSettings::triggered()` |
| Experiment > New Experiment | Resets experiment configuration (placeholder — future sprint) | `actionNewExperiment::triggered()` |
| Experiment > Open Experiment | Opens file dialog to load experiment file (placeholder — future sprint) | `actionOpenExperiment::triggered()` |
| Experiment > Save Experiment | Saves current experiment configuration (placeholder — future sprint) | `actionSaveExperiment::triggered()` |
| Experiment > Start | Starts experiment sequence; disables New/Open/Connect/Disconnect; enables Stop | `actionStartExperiment::triggered()` → `MainWindow::startExperiment()` |
| Experiment > Stop | `QMessageBox::question` — "Stop the running experiment?" → stops experiment | `actionStopExperiment::triggered()` → confirmation → `MainWindow::stopExperiment()` |
| Tools > Preferences | Opens preferences dialog (placeholder — future sprint) | `actionPreferences::triggered()` |
| Tools > Export Log | Opens save file dialog to export log as text file | `actionExportLog::triggered()` → `QFileDialog::getSaveFileName()` |
| Help > About MWA | Opens `QDialog` with application name, version, license, and description | `actionAbout::triggered()` → `AboutDialog::exec()` |
| Help > About Qt | Opens Qt's built-in About Qt dialog | `actionAboutQt::triggered()` → `QApplication::aboutQt()` |
| Close left dock via X button | Dock hidden; View > Device Panels unchecked | `dockDevicePanels::visibilityChanged(false)` → `actionToggleLeftDock::setChecked(false)` |
| Close bottom dock via X button | Dock hidden; View > Log Panel unchecked | `dockLogPanel::visibilityChanged(false)` → `actionToggleBottomDock::setChecked(false)` |
| Logger emits `newLogEntry` | Status bar last-event label updated with latest log message | `Logger::newLogEntry(...)` → `MainWindow::onNewLogEntry(...)` → `lblLastEvent::setText(...)` |
| Window close button (X) | Same as File > Exit — confirms if experiment running | `QMainWindow::closeEvent(QCloseEvent*)` override |
| Drag dock to float | Dock becomes floating window | Qt built-in `QDockWidget` float behavior |
| Drag dock to new position | Dock re-anchored to new area | Qt built-in `QDockWidget` re-dock behavior |
| Right-click on toolbar | Context menu: "Main Toolbar" (checkable, mirrors View > Toolbar) | Qt built-in `QMainWindow` toolbar context menu |

---

## 5. Keyboard Shortcuts

| Shortcut | Platform | Action |
|----------|----------|--------|
| `Ctrl+N` | Both | New Experiment (`actionNewExperiment`) |
| `Ctrl+O` | Both | Open Experiment (`actionOpenExperiment`) |
| `Ctrl+S` | Both | Save Experiment (`actionSaveExperiment`) |
| `Ctrl+Q` | macOS | Exit application (`actionExit`) |
| `Alt+F4` | Windows | Exit application (`actionExit`) |
| `Ctrl+Shift+C` | Both | Connect All Devices (`actionConnectAll`) |
| `Ctrl+Shift+D` | Both | Disconnect All Devices (`actionDisconnectAll`) |
| `F5` | Both | Start Experiment (`actionStartExperiment`) |
| `F6` | Both | Stop Experiment (`actionStopExperiment`) |
| `Ctrl+,` | Both | Preferences (`actionPreferences`) |
| `F1` | Both | About MWA (`actionAbout`) |
| `Escape` | Both | Cancel current dialog / close floating modal |

All shortcuts are displayed in menu items and as tooltip text on toolbar buttons (e.g., tooltip: "Start Experiment (F5)").

Tab order within the shell follows the standard Qt focus chain: menu bar → toolbar → left dock → central widget → bottom dock → status bar.

---

## Design Notes and Ambiguities

1. **About Dialog contents:** The task spec requires a Help > About action. This design specifies a `QDialog` with application name, version (from `CMakeLists.txt` / `QCoreApplication::applicationVersion()`), Qt version, and license summary. Exact content subject to Lead review.

2. **Placeholder vs. empty dock:** When no device panels are added, the left dock shows a centered placeholder label. SWE should implement the left dock such that actual device panels can be added as child `QDockWidget` instances without rearchitecting the shell.

3. **Status bar device summary format:** The format "Devices: M / N connected" requires the `MainWindow` to track registered device panels. For Round 2 (shell only), the label shows "Devices: –" until device panels are integrated. SWE should provide a `registerDevice()` slot or similar for Round 3+ panels to attach.

4. **`QStackedWidget` vs. `QSplitter` for central area:** The task spec lists both as options. This design chooses `QStackedWidget` because: (a) initially there is only one view (camera placeholder), and (b) switching between Acquisition view and Analysis view in later phases maps naturally to stacked pages. If a split camera + analysis layout is desired, this can be revisited in Phase 3 design.

5. **Logger integration:** `Logger` (already implemented in Round 1) emits `newLogEntry`. The `MainWindow` should connect to this signal to update `lblLastEvent` in the status bar. Full log display is deferred to MWA-02-E (Log Panel).

6. **Settings persistence:** `SettingsManager` (Round 1) should be used to save and restore dock widget geometry and toolbar visibility on close/open. This is a `MainWindow` responsibility.

---

## Requesting Lead Review

This design specification is complete and ready for Lead review. Please evaluate:

- Completeness against GUI-REQ-001 and NFR-005
- Alignment with the UX agent standards (panel template, widget selection rules, state/interaction coverage)
- Whether the `QStackedWidget` choice for central area is acceptable for the current sprint scope
- Whether the placeholder-based dock approach satisfies the "reserved for future panels" requirement without creating technical debt for SWE
