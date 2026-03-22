# Log Panel Widget Design Specification

**Task:** MWA-02-E
**Requirement:** GUI-REQ-009
**Designer:** UX Agent
**Date:** 2026-03-22
**Status:** APPROVED by Lead

---

## Overview

The Log Panel is a `QWidget` placed as the inner content widget of the existing `dockLogPanel` `QDockWidget` in `MainWindow`. It replaces the `wgtLogPanelPlaceholder` widget. The panel provides researchers with real-time visibility into all system events — debug traces, informational messages, warnings, and errors — from every module during an experiment session. It connects to `Logger::instance()` and drives a custom `QAbstractTableModel` (`LogTableModel`) that feeds a `QTableView`.

---

## 1. ASCII Layout

```
┌─ Log ──────────────────────────────────────────────────────── [_][□][X] ─┐
│ (dockLogPanel title bar — provided by QDockWidget, not LogPanel)         │
├──────────────────────────────────────────────────────────────────────────┤
│ ┌─ Toolbar row ────────────────────────────────────────────────────────┐ │
│ │ [Show: All ▼]  [☐Debug] [☐Info] [☐Warning] [☐Error]               │ │
│ │                    [Filter: _______________]             [✕ Clear]  │ │
│ └──────────────────────────────────────────────────────────────────────┘ │
│ ┌─ Log Table (QTableView / LogTableModel) ─────────────────────────────┐│
│ │ Timestamp          │ Sev  │ Source          │ Message                 ││
│ ├────────────────────┼──────┼─────────────────┼─────────────────────────┤│
│ │ 14:01:00.000       │ INFO │ MainWindow      │ Application started     ││
│ │ 14:01:05.123       │ DBG  │ Logger          │ Qt handler installed    ││
│ │ 14:01:10.456       │ WARN │ SettingsManager │ Config not found        ││
│ │ 14:01:15.789       │ ERR  │ DeviceInterface │ Serial port COM3 fail   ││
│ └──────────────────────────────────────────────────────────────────────┘ │
│ 47 entries (3 hidden by filter)                       [Auto-scroll ☑]   │
└──────────────────────────────────────────────────────────────────────────┘
```

### Layout Notes

- `LogPanel` uses `QVBoxLayout` with 8px margins and 4px spacing.
- Three rows: toolbar, table view (stretch 1), status row.
- Column widths (initial, user-resizable): Timestamp 175px, Severity 60px, Source 160px, Message stretch.
- Row height: 22px minimum.
- Monospace font (system default, 11pt) for table readability.

---

## 2. Widget Table

### LogPanel (root widget)

| Widget | Type | objectName | Properties | Signals |
|--------|------|------------|------------|---------|
| Log Panel root | `QWidget` | `logPanel` | Layout: `QVBoxLayout`, margins: 8px, spacing: 4px | — |

### Toolbar Row

| Widget | Type | objectName | Properties | Signals |
|--------|------|------------|------------|---------|
| Toolbar container | `QWidget` | `wgtLogToolbar` | Layout: `QHBoxLayout`, margins: 0, spacing: 4px; fixed height: 36px | — |
| Show label | `QLabel` | `lblShowFilter` | Text: "Show:", fixed width: 38px | — |
| Severity preset combo | `QComboBox` | `cmbSeverityPreset` | Items: "All", "Info+", "Warnings+", "Errors only"; index: 0; min width: 120px; min height: 32px | `currentIndexChanged(int)` |
| Debug checkbox | `QCheckBox` | `chkDebug` | Text: "Debug", checked: true, min height: 32px | `toggled(bool)` |
| Info checkbox | `QCheckBox` | `chkInfo` | Text: "Info", checked: true, min height: 32px | `toggled(bool)` |
| Warning checkbox | `QCheckBox` | `chkWarning` | Text: "Warning", checked: true, min height: 32px | `toggled(bool)` |
| Error checkbox | `QCheckBox` | `chkError` | Text: "Error", checked: true, min height: 32px | `toggled(bool)` |
| Filter label | `QLabel` | `lblSearchIcon` | Text: "Filter:", fixed width: 42px | — |
| Text filter | `QLineEdit` | `leFilterText` | Placeholder: "Filter messages...", clearButtonEnabled: true, min width: 140px, min height: 32px | `textChanged(QString)` |
| Spacer | `QSpacerItem` | — | Expanding horizontal | — |
| Clear button | `QPushButton` | `btnClearLog` | Text: "Clear", min size: 64x32px, tooltip: "Clear log (Ctrl+K)" | `clicked()` |

### Log Table

| Widget | Type | objectName | Properties | Signals |
|--------|------|------------|------------|---------|
| Log table view | `QTableView` | `tblLogView` | Model: `QSortFilterProxyModel` over `LogTableModel`; selectionMode: SingleSelection; selectionBehavior: SelectRows; editTriggers: NoEditTriggers; alternatingRowColors: true; verticalHeader hidden; stretchLastSection: true; sortingEnabled: false; wordWrap: false; font: monospace 11pt | — |

### Status Row

| Widget | Type | objectName | Properties | Signals |
|--------|------|------------|------------|---------|
| Status container | `QWidget` | `wgtLogStatus` | Layout: `QHBoxLayout`, margins: 0, spacing: 8px; fixed height: 24px | — |
| Entry count label | `QLabel` | `lblEntryCount` | Text: "0 entries"; font: 11pt; color: `#7F8C8D` | — |
| Filter status label | `QLabel` | `lblFilterStatus` | Text: ""; italic; color: `#F39C12`; hidden when no filter active | — |
| Spacer | `QSpacerItem` | — | Expanding horizontal | — |
| Auto-scroll checkbox | `QCheckBox` | `chkAutoScroll` | Text: "Auto-scroll", checked: true, min height: 24px | `toggled(bool)` |

---

## 3. LogTableModel Specification

Separate files: `src/gui/widgets/log_table_model.h` / `log_table_model.cpp`

### Severity Color Mapping

| LogSeverity | Badge | Foreground | Row Background |
|---|---|---|---|
| `kDebug` | "DBG" | `#7F8C8D` (gray) | default alternating |
| `kInfo` | "INFO" | `#27AE60` (green) | default alternating |
| `kWarning` | "WARN" | `#E67E22` (dark orange) | `#FEF9E7` (light yellow) |
| `kError` | "ERR" | `#E74C3C` (red) | `#FDEDEC` (light red) |

### Filtering

`QSortFilterProxyModel` between `LogTableModel` and `QTableView`. Custom `filterAcceptsRow()` checks severity bitmask AND text filter (case-insensitive on Source + Message).

### Thread Safety

Connection from `Logger::newLogEntry` to `LogPanel` uses `Qt::QueuedConnection`.

---

## 4. State Table

| UI State | Trigger | Visual Change |
|----------|---------|---------------|
| **Initial / Empty** | Constructed | 0 rows; btnClearLog disabled |
| **Receiving Entries** | newLogEntry signal | Row appended; auto-scroll if enabled; count updated |
| **Filter Active** | Severity/text filter changed | Proxy re-filters; lblFilterStatus shows hidden count |
| **Auto-scroll On** | Default / checkbox checked | Scrolls to bottom on each new entry |
| **Auto-scroll Off** | User scrolls up / unchecks | No auto-scroll; user scroll preserved |
| **Log Cleared** | btnClearLog clicked | Model cleared; count reset; btnClearLog disabled |

---

## 5. Interaction List

| User Action | UI Response | Signal |
|---|---|---|
| New log entry | Row appended, auto-scroll if enabled | `Logger::newLogEntry` → `LogPanel::onNewLogEntry` |
| Change severity preset | Checkboxes updated to match; proxy re-filters | `cmbSeverityPreset::currentIndexChanged` |
| Toggle severity checkbox | Proxy re-filters; lblFilterStatus updated | `QCheckBox::toggled` |
| Type in filter | Proxy applies text filter on Source+Message | `leFilterText::textChanged` |
| Click Clear | Model cleared | `btnClearLog::clicked` |
| Toggle auto-scroll | Enable/disable auto-scroll behavior | `chkAutoScroll::toggled` |
| Scroll up manually | Auto-scroll disabled | `scrollBar::valueChanged` |
| Scroll to bottom | Auto-scroll re-enabled | `scrollBar::valueChanged` |
| Double-click row | Entry copied to clipboard | `tblLogView::doubleClicked` |

---

## 6. Keyboard Shortcuts

| Shortcut | Action |
|---|---|
| `Ctrl+F` | Focus text filter input |
| `Ctrl+K` | Clear log entries |
| `Escape` (in filter) | Clear filter, return focus to table |

---

## 7. MainWindow Integration

Replace placeholder in `MainWindow::createDocks()`:
```
dock_log_panel_->setWidget(new LogPanel(this));
```
Remove `wgtLogPanelPlaceholder` and `lblLogPanelPlaceholder`.
