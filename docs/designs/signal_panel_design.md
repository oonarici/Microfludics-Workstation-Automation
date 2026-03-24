# Signal Generator / Network Analyzer Panel Design Specification

**Task:** MWA-03-D
**Requirement:** GUI-REQ-004
**Designer:** UX Agent
**Date:** 2026-03-23
**Status:** SUBMITTED for Lead review

---

## Overview

`SignalPanel` is a `QDockWidget` that provides a combined control interface for
the Signal Generator and Network Analyzer devices. Because in many microfluidics
setups a single vector-network-analyzer instrument acts as both signal source
and S-parameter measurer, the panel shares one Connection section at the top
and separates device-specific controls into two tabs of a `QTabWidget`.

The panel accepts a `SignalGeneratorControllerInterface*` and a
`NetworkAnalyzerControllerInterface*` independently; either pointer may be
`nullptr` when only one device is physically present (the corresponding tab
is then fully disabled).

Minimum panel width: 280 px. Maximum panel width: 400 px.

---

## 1. ASCII Layout

### Overall Panel Shell

```
┌─ Signal / Network Analyzer ────────────── [float][X] ─┐
│                                                        │
│  ┌─ Connection ─────────────────────────────────────┐  │
│  │  Port: [GPIB0::16  ▼]  [●]  [   Connect   ]    │  │
│  └─────────────────────────────────────────────────┘  │
│                                                        │
│  ┌─────────────────────────────────────────────────┐  │
│  │  Signal Generator  │  Network Analyzer          │  │
│  ├─────────────────────────────────────────────────┤  │
│  │  (Tab 1 or Tab 2 content — see below)           │  │
│  └─────────────────────────────────────────────────┘  │
│                                                        │
└────────────────────────────────────────────────────────┘
```

---

### Tab 1 — Signal Generator

```
┌─ Signal Generator  │  Network Analyzer ───────────────┐
│                                                        │
│  ┌─ Output Configuration ─────────────────────────┐   │
│  │                                                 │   │
│  │  Frequency:  [ 1000.000 ▲▼]  [MHz ▼]          │   │
│  │                                                 │   │
│  │  Amplitude:  [    1.000 ▲▼]  V                 │   │
│  │                                                 │   │
│  │  Waveform:   [ Sine           ▼]               │   │
│  │                                                 │   │
│  └─────────────────────────────────────────────────┘  │
│                                                        │
│  ┌─ Sweep Configuration ──────────────────────────┐   │
│  │                                                 │   │
│  │  Start Freq: [  100.000 ▲▼]  [MHz ▼]          │   │
│  │  Stop  Freq: [ 2000.000 ▲▼]  [MHz ▼]          │   │
│  │  Step  Freq: [   10.000 ▲▼]  [MHz ▼]          │   │
│  │                                                 │   │
│  │           [ Apply Sweep Config ]               │   │
│  └─────────────────────────────────────────────────┘  │
│                                                        │
│  ┌─ Output Status ────────────────────────────────┐   │
│  │                                                 │   │
│  │  Output:  [●  Output OFF  ]   (toggle button)  │   │
│  │                                                 │   │
│  │  Frequency:   [    1000.000 MHz    ]  (LCD)    │   │
│  │  Amplitude:   [       1.000 V      ]  (LCD)    │   │
│  │  Waveform:    Sine                              │   │
│  │                                                 │   │
│  └─────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────┘
```

---

### Tab 2 — Network Analyzer

```
┌─ Signal Generator  │  Network Analyzer ───────────────┐
│                                                        │
│  ┌─ Sweep Configuration ──────────────────────────┐   │
│  │                                                 │   │
│  │  Start Freq: [  100.000 ▲▼]  [MHz ▼]          │   │
│  │  Stop  Freq: [ 2000.000 ▲▼]  [MHz ▼]          │   │
│  │                                                 │   │
│  │  Points:     [     201  ▲▼]                    │   │
│  │                                                 │   │
│  └─────────────────────────────────────────────────┘  │
│                                                        │
│  ┌─ Measurement ──────────────────────────────────┐   │
│  │                                                 │   │
│  │     [ Measure S-Parameters ]   (Ctrl+M)        │   │
│  │                                                 │   │
│  │  ┌─────────────────────────────────────────┐   │   │
│  │  │                                         │   │   │
│  │  │  S-Parameter Display                    │   │   │
│  │  │  (placeholder — awaiting measurement)   │   │   │
│  │  │                                         │   │   │
│  │  │                                         │   │   │
│  │  └─────────────────────────────────────────┘   │   │
│  │                                                 │   │
│  │  [========================] Measuring... 42%   │   │
│  │  (QProgressBar — hidden when not measuring)    │   │
│  │                                                 │   │
│  └─────────────────────────────────────────────────┘  │
│                                                        │
│  ┌─ Measurement Status ───────────────────────────┐   │
│  │  State:  ● Idle                                │   │
│  │  Points: 201    Start: 100 MHz    Stop: 2 GHz  │   │
│  └─────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────┘
```

---

## 2. Widget Table

### Panel Root

| Widget | Type | objectName | Properties | Signals |
|--------|------|------------|------------|---------|
| Panel root | `QDockWidget` | `dockSignalPanel` | windowTitle: "Signal / Network Analyzer", features: DockWidgetMovable\|DockWidgetFloatable\|DockWidgetClosable, minWidth: 280, maxWidth: 400 | — |
| Inner container | `QWidget` | `wgtSignalPanelRoot` | Layout: `QVBoxLayout`, margin: 8, spacing: 12 | — |

---

### Connection Section

| Widget | Type | objectName | Properties | Signals |
|--------|------|------------|------------|---------|
| Connection group | `QGroupBox` | `grpSignalConnection` | Title: "Connection", layout: `QHBoxLayout`, margin: 8, spacing: 6 | — |
| Port selector | `QComboBox` | `cmbSignalPort` | editable: true, sizePolicy: Expanding, toolTip: "GPIB address, VISA resource string, or COM port" | `currentTextChanged(QString)` |
| Status indicator | `QLabel` | `lblSignalStatus` | Fixed 14x14 px, border-radius: 7px via stylesheet, background: `#95A5A6`, toolTip: "Connection state" | — |
| Connect button | `QPushButton` | `btnSignalConnect` | text: "Connect", checkable: false, minHeight: 32, shortcut: Ctrl+Shift+G | `clicked()` |

---

### Tab Widget

| Widget | Type | objectName | Properties | Signals |
|--------|------|------------|------------|---------|
| Tab container | `QTabWidget` | `tabSignalPanel` | tabs: ["Signal Generator", "Network Analyzer"], tabPosition: North, sizePolicy: Expanding | `currentChanged(int)` |

---

### Tab 1 — Signal Generator Widgets

**Output Configuration group**

| Widget | Type | objectName | Properties | Signals |
|--------|------|------------|------------|---------|
| Output Config group | `QGroupBox` | `grpSigOutputConfig` | Title: "Output Configuration", layout: `QFormLayout`, margin: 8, spacing: 6 | — |
| Frequency spinbox | `QDoubleSpinBox` | `spnSigFrequency` | min: 0.000001, max: 999999.999999, decimals: 6, value: 1000.0, sizePolicy: Expanding, toolTip: "Output frequency (value in currently selected unit)" | `valueChanged(double)` |
| Frequency unit selector | `QComboBox` | `cmbSigFreqUnit` | items: ["Hz", "kHz", "MHz"], currentIndex: 2 (MHz), fixedWidth: 56, toolTip: "Frequency unit" | `currentIndexChanged(int)` |
| Frequency row container | `QWidget` | `wgtSigFreqRow` | Layout: `QHBoxLayout`, spacing: 4, contains spnSigFrequency + cmbSigFreqUnit | — |
| Amplitude spinbox | `QDoubleSpinBox` | `spnSigAmplitude` | min: 0.001, max: 20.000, decimals: 3, value: 1.0, suffix: " V", sizePolicy: Expanding, toolTip: "Peak output amplitude in volts" | `valueChanged(double)` |
| Waveform selector | `QComboBox` | `cmbSigWaveform` | items: ["Sine", "Square", "Triangle"], currentIndex: 0, sizePolicy: Expanding, toolTip: "Output waveform shape" | `currentIndexChanged(int)` |

**Sweep Configuration group**

| Widget | Type | objectName | Properties | Signals |
|--------|------|------------|------------|---------|
| Sweep Config group | `QGroupBox` | `grpSigSweep` | Title: "Sweep Configuration", layout: `QFormLayout`, margin: 8, spacing: 6 | — |
| Sweep start spinbox | `QDoubleSpinBox` | `spnSigSweepStart` | min: 0.000001, max: 999999.999999, decimals: 6, value: 100.0, sizePolicy: Expanding, toolTip: "Sweep start frequency" | `valueChanged(double)` |
| Sweep start unit | `QComboBox` | `cmbSigSweepStartUnit` | items: ["Hz", "kHz", "MHz"], currentIndex: 2, fixedWidth: 56, toolTip: "Start frequency unit" | `currentIndexChanged(int)` |
| Sweep start row | `QWidget` | `wgtSigSweepStartRow` | Layout: `QHBoxLayout`, spacing: 4 | — |
| Sweep stop spinbox | `QDoubleSpinBox` | `spnSigSweepStop` | min: 0.000001, max: 999999.999999, decimals: 6, value: 2000.0, sizePolicy: Expanding, toolTip: "Sweep stop frequency" | `valueChanged(double)` |
| Sweep stop unit | `QComboBox` | `cmbSigSweepStopUnit` | items: ["Hz", "kHz", "MHz"], currentIndex: 2, fixedWidth: 56, toolTip: "Stop frequency unit" | `currentIndexChanged(int)` |
| Sweep stop row | `QWidget` | `wgtSigSweepStopRow` | Layout: `QHBoxLayout`, spacing: 4 | — |
| Sweep step spinbox | `QDoubleSpinBox` | `spnSigSweepStep` | min: 0.000001, max: 999999.999999, decimals: 6, value: 10.0, sizePolicy: Expanding, toolTip: "Sweep step size" | `valueChanged(double)` |
| Sweep step unit | `QComboBox` | `cmbSigSweepStepUnit` | items: ["Hz", "kHz", "MHz"], currentIndex: 2, fixedWidth: 56, toolTip: "Step frequency unit" | `currentIndexChanged(int)` |
| Sweep step row | `QWidget` | `wgtSigSweepStepRow` | Layout: `QHBoxLayout`, spacing: 4 | — |
| Apply Sweep button | `QPushButton` | `btnSigApplySweep` | text: "Apply Sweep Config", minHeight: 32, toolTip: "Send sweep start/stop/step to instrument (Ctrl+W)" | `clicked()` |

**Output Status group**

| Widget | Type | objectName | Properties | Signals |
|--------|------|------------|------------|---------|
| Output Status group | `QGroupBox` | `grpSigOutputStatus` | Title: "Output Status", layout: `QVBoxLayout`, margin: 8, spacing: 6 | — |
| Output toggle button | `QPushButton` | `btnSigOutputEnable` | checkable: true, checked: false, text (unchecked): "  Output OFF", text (checked): "  Output ON", minHeight: 36, toolTip: "Enable or disable signal output (Ctrl+E). Requires confirmation to disable." | `toggled(bool)` |
| Freq display label | `QLabel` | `lblSigFreqDisplay` | text: "— MHz", font: 13pt bold, alignment: AlignCenter, frame: QFrame::Panel\|QFrame::Sunken, minHeight: 28, toolTip: "Current output frequency (read-only — double-click to copy)" | — |
| Amplitude display label | `QLabel` | `lblSigAmpDisplay` | text: "— V", font: 13pt bold, alignment: AlignCenter, frame: QFrame::Panel\|QFrame::Sunken, minHeight: 28, toolTip: "Current output amplitude (read-only — double-click to copy)" | — |
| Waveform display label | `QLabel` | `lblSigWaveformDisplay` | text: "—", font: 12pt, alignment: AlignCenter | — |

---

### Tab 2 — Network Analyzer Widgets

**Sweep Configuration group**

| Widget | Type | objectName | Properties | Signals |
|--------|------|------------|------------|---------|
| NA Sweep group | `QGroupBox` | `grpNaSweepConfig` | Title: "Sweep Configuration", layout: `QFormLayout`, margin: 8, spacing: 6 | — |
| NA start freq spinbox | `QDoubleSpinBox` | `spnNaStartFreq` | min: 0.000001, max: 999999.999999, decimals: 6, value: 100.0, sizePolicy: Expanding, toolTip: "Sweep start frequency for measurement" | `valueChanged(double)` |
| NA start freq unit | `QComboBox` | `cmbNaStartFreqUnit` | items: ["Hz", "kHz", "MHz"], currentIndex: 2, fixedWidth: 56 | `currentIndexChanged(int)` |
| NA start row | `QWidget` | `wgtNaStartRow` | Layout: `QHBoxLayout`, spacing: 4 | — |
| NA stop freq spinbox | `QDoubleSpinBox` | `spnNaStopFreq` | min: 0.000001, max: 999999.999999, decimals: 6, value: 2000.0, sizePolicy: Expanding, toolTip: "Sweep stop frequency for measurement" | `valueChanged(double)` |
| NA stop freq unit | `QComboBox` | `cmbNaStopFreqUnit` | items: ["Hz", "kHz", "MHz"], currentIndex: 2, fixedWidth: 56 | `currentIndexChanged(int)` |
| NA stop row | `QWidget` | `wgtNaStopRow` | Layout: `QHBoxLayout`, spacing: 4 | — |
| Points spinbox | `QSpinBox` | `spnNaPoints` | min: 2, max: 16001, value: 201, singleStep: 50, sizePolicy: Expanding, toolTip: "Number of evenly-spaced frequency points in the sweep" | `valueChanged(int)` |

**Measurement group**

| Widget | Type | objectName | Properties | Signals |
|--------|------|------------|------------|---------|
| Measurement group | `QGroupBox` | `grpNaMeasurement` | Title: "Measurement", layout: `QVBoxLayout`, margin: 8, spacing: 6 | — |
| Measure button | `QPushButton` | `btnNaMeasure` | text: "Measure S-Parameters", minHeight: 36, toolTip: "Trigger S-parameter measurement (Ctrl+M)" | `clicked()` |
| S-param display | `QLabel` | `lblNaSParamDisplay` | text: "No measurement data.\nPress 'Measure S-Parameters' to begin.", alignment: AlignCenter, frameShape: QFrame::Panel, frameShadow: QFrame::Sunken, minHeight: 120, wordWrap: true, toolTip: "S-parameter trace result (double-click to copy data)" | — |
| Progress bar | `QProgressBar` | `prgNaMeasurement` | minimum: 0, maximum: 0 (indeterminate), visible: false, textVisible: true, format: "Measuring...", toolTip: "Measurement progress" | — |

**Measurement Status group**

| Widget | Type | objectName | Properties | Signals |
|--------|------|------------|------------|---------|
| Measurement Status group | `QGroupBox` | `grpNaMeasStatus` | Title: "Measurement Status", layout: `QGridLayout`, margin: 8, spacing: 4 | — |
| State dot | `QLabel` | `lblNaStateDot` | Fixed 12x12 px, border-radius: 6px, background: `#95A5A6` | — |
| State text | `QLabel` | `lblNaStateText` | text: "Idle", font: 12pt | — |
| Points summary label | `QLabel` | `lblNaPointsSummary` | text: "— pts", font: 11pt, alignment: AlignRight | — |
| Range summary label | `QLabel` | `lblNaRangeSummary` | text: "— MHz … — MHz", font: 11pt, alignment: AlignLeft | — |

---

## 3. State Table

### Connection States (affect the entire panel)

| State | Status Dot Color | Connect Button Text | All Controls | Notes |
|-------|-----------------|---------------------|--------------|-------|
| **Disconnected** | `#95A5A6` (gray) | "Connect" | disabled (grayed) | Only `cmbSignalPort` and `btnSignalConnect` are enabled |
| **Connecting** | `#F39C12` (orange) | "Connecting..." (disabled) | disabled | `btnSignalConnect` disabled; no spinner widget — button text conveys state |
| **Connected** | `#27AE60` (green) | "Disconnect" | enabled | Both tabs fully enabled |
| **Error** | `#E74C3C` (red) | "Connect" | disabled | Error message shown in status group label; user must reconnect |

### Tab 1 — Signal Generator Output States

| State | Output Toggle Appearance | Status Dot (inside button) | Display Labels |
|-------|--------------------------|----------------------------|----------------|
| **Output OFF** | text: "  Output OFF", background: default, unchecked | gray LED `#95A5A6` prefix | show "—" until device reports |
| **Output ON** | text: "  Output ON", background: `#27AE60` tint, checked | green LED `#27AE60` prefix | show live frequency / amplitude / waveform |
| **Disconnected** | disabled, unchecked | gray | show "—" |

### Tab 2 — Network Analyzer Measurement States

| State | Measure Button | Progress Bar | S-Param Display | State Dot | State Text |
|-------|---------------|--------------|-----------------|-----------|------------|
| **Idle / No Data** | enabled | hidden | placeholder text | `#95A5A6` | "Idle" |
| **Measuring** | disabled | visible, indeterminate | unchanged | `#3498DB` (blue) | "Measuring..." |
| **Measurement Complete** | enabled | hidden | "Last measurement: N points\nFreq: X MHz … Y MHz\nDisplay: see trace data" | `#27AE60` | "Complete" |
| **Measurement Error** | enabled | hidden | "Measurement failed. Check connection." in red text | `#E74C3C` | "Error" |
| **Disconnected** | disabled | hidden | placeholder text | `#95A5A6` | "Idle" |

---

## 4. Interaction List

### Connection Section

| User Action | UI Response | Signal / Controller Call |
|-------------|-------------|--------------------------|
| Edit port field | Updates `cmbSignalPort` text | none (deferred until Connect) |
| Click "Connect" (disconnected) | Button text → "Connecting...", disabled; dot → orange; all controls disabled | Calls `controller->connect()` on both controllers |
| `stateChanged(kConnected)` from either controller | Dot → green; button text → "Disconnect"; all controls enabled; status labels updated | — |
| Click "Disconnect" (connected) | `QMessageBox::question` — "Disconnect from instrument?" Yes/No. On Yes: dot → gray, controls disabled, button → "Connect" | Calls `controller->disconnect()` on both controllers |
| `stateChanged(kError)` | Dot → red; all controls disabled; error message shown at bottom of relevant group; button → "Connect" | — |

### Tab 1 — Signal Generator

| User Action | UI Response | Signal / Controller Call |
|-------------|-------------|--------------------------|
| Change frequency value or unit | Updates `spnSigFrequency` display; converts to Hz internally | Calls `setFrequency(hz)` on value commit (editingFinished or spinbox step) → `frequencyChanged(double)` emitted by controller |
| Change amplitude | Updates `spnSigAmplitude` | Calls `setAmplitude(volts)` → `amplitudeChanged(double)` |
| Change waveform selector | Updates `cmbSigWaveform` selection | Calls `setWaveform(Waveform)` → `waveformChanged(Waveform)` |
| Click "Apply Sweep Config" | Reads start/stop/step spinboxes and unit selectors; converts all to Hz | Calls `configureSweep(start_hz, stop_hz, step_hz)` |
| Click output toggle (OFF → ON) | Button background → green tint, text → "Output ON" | Calls `setOutputEnabled(true)` → `outputStateChanged(true)` |
| Click output toggle (ON → OFF) | `QMessageBox::question` — "Disable signal output?" Yes/No. On Yes: button → unchecked, background reset, text → "Output OFF" | Calls `setOutputEnabled(false)` → `outputStateChanged(false)` |
| `frequencyChanged(hz)` received | `lblSigFreqDisplay` updated with formatted frequency + unit | — |
| `amplitudeChanged(volts)` received | `lblSigAmpDisplay` updated | — |
| `waveformChanged(waveform)` received | `lblSigWaveformDisplay` updated | — |
| `outputStateChanged(bool)` received | Syncs toggle button and display color to match hardware state | — |
| Double-click `lblSigFreqDisplay` | Copies frequency text to clipboard; tooltip flash "Copied!" | — |
| Double-click `lblSigAmpDisplay` | Copies amplitude text to clipboard | — |
| Right-click anywhere on Tab 1 | Context menu: "Reset to Defaults" / "Copy Settings" / "Copy Log" | — |

### Tab 2 — Network Analyzer

| User Action | UI Response | Signal / Controller Call |
|-------------|-------------|--------------------------|
| Change NA start/stop freq or unit | Updates spinbox value | Calls `setFrequencyRange(start_hz, stop_hz)` on both commit → `frequencyRangeChanged(start, stop)` |
| Change points spinbox | Updates `spnNaPoints` | Calls `setNumPoints(points)` → `numPointsChanged(int)` |
| Click "Measure S-Parameters" | Button disabled; `prgNaMeasurement` shown (indeterminate); state dot → blue, text → "Measuring..." | Calls `measureSParameters()` → `measurementStarted()` |
| `measurementStarted()` received | Measure button disabled; progress bar visible; state updated | — |
| `measurementComplete()` received | Progress bar hidden; measure button re-enabled; state dot → green, text → "Complete"; `lblNaSParamDisplay` updated with trace summary; range/points summary labels updated | — |
| `frequencyRangeChanged(start, stop)` received | `lblNaRangeSummary` updated | — |
| `numPointsChanged(int)` received | `lblNaPointsSummary` updated | — |
| Double-click `lblNaSParamDisplay` | Copies trace data summary to clipboard | — |
| Right-click anywhere on Tab 2 | Context menu: "Reset to Defaults" / "Export S-Parameters..." / "Copy Log" | — |

---

## 5. Keyboard Shortcuts

| Shortcut | Scope | Action |
|----------|-------|--------|
| `Ctrl+Shift+G` | Panel | Connect / Disconnect instrument |
| `Ctrl+E` | Tab 1 | Toggle signal output enable/disable |
| `Ctrl+W` | Tab 1 | Apply sweep configuration |
| `Ctrl+M` | Tab 2 | Trigger S-parameter measurement |
| `Escape` | Dialog | Cancel any open `QMessageBox` (built-in Qt behavior) |
| `Tab` | Panel-wide | Advance focus top-to-bottom, left-to-right within each group |
| `Shift+Tab` | Panel-wide | Reverse focus order |
| `Alt+1` | Panel | Switch to Signal Generator tab |
| `Alt+2` | Panel | Switch to Network Analyzer tab |

All shortcuts are displayed in widget tooltips. Example tooltip for the output
toggle: "Enable or disable signal output (Ctrl+E). Requires confirmation to
disable."

---

## 6. Frequency Unit Conversion Rule

All frequency values are stored and sent to the controller exclusively in Hz.
The unit selector (`QComboBox`) adjusts the displayed scale only. Conversion:

- Hz selected  → controller value = spinbox value × 1.0
- kHz selected → controller value = spinbox value × 1 000.0
- MHz selected → controller value = spinbox value × 1 000 000.0

When the controller emits `frequencyChanged(hz)`, the display label
`lblSigFreqDisplay` formats the value using the most readable unit
(e.g., 1 000 000 Hz → "1.000000 MHz"). The input spinboxes retain the
user's last chosen unit — they are not auto-converted.

The same rule applies to all six frequency spinboxes (output frequency,
sweep start/stop/step, NA start/stop).

---

## 7. Layout Stack Summary

```
QDockWidget [dockSignalPanel]
  └── QWidget [wgtSignalPanelRoot]  (QVBoxLayout, margin:8, spacing:12)
        ├── QGroupBox [grpSignalConnection]  (QHBoxLayout)
        │     ├── QComboBox [cmbSignalPort]
        │     ├── QLabel [lblSignalStatus]       (14×14 px dot)
        │     └── QPushButton [btnSignalConnect]
        │
        └── QTabWidget [tabSignalPanel]
              │
              ├── Tab 0: "Signal Generator"
              │     QWidget (QVBoxLayout, margin:8, spacing:8)
              │       ├── QGroupBox [grpSigOutputConfig]  (QFormLayout)
              │       │     ├── row "Frequency":  QWidget[wgtSigFreqRow]
              │       │     │     ├── QDoubleSpinBox [spnSigFrequency]
              │       │     │     └── QComboBox [cmbSigFreqUnit]
              │       │     ├── row "Amplitude":  QDoubleSpinBox [spnSigAmplitude]
              │       │     └── row "Waveform":   QComboBox [cmbSigWaveform]
              │       │
              │       ├── QGroupBox [grpSigSweep]  (QFormLayout)
              │       │     ├── row "Start Freq": QWidget[wgtSigSweepStartRow]
              │       │     │     ├── QDoubleSpinBox [spnSigSweepStart]
              │       │     │     └── QComboBox [cmbSigSweepStartUnit]
              │       │     ├── row "Stop Freq":  QWidget[wgtSigSweepStopRow]
              │       │     │     ├── QDoubleSpinBox [spnSigSweepStop]
              │       │     │     └── QComboBox [cmbSigSweepStopUnit]
              │       │     ├── row "Step Freq":  QWidget[wgtSigSweepStepRow]
              │       │     │     ├── QDoubleSpinBox [spnSigSweepStep]
              │       │     │     └── QComboBox [cmbSigSweepStepUnit]
              │       │     └── QPushButton [btnSigApplySweep]
              │       │
              │       └── QGroupBox [grpSigOutputStatus]  (QVBoxLayout)
              │             ├── QPushButton [btnSigOutputEnable]  (checkable)
              │             ├── QLabel [lblSigFreqDisplay]
              │             ├── QLabel [lblSigAmpDisplay]
              │             └── QLabel [lblSigWaveformDisplay]
              │
              └── Tab 1: "Network Analyzer"
                    QWidget (QVBoxLayout, margin:8, spacing:8)
                      ├── QGroupBox [grpNaSweepConfig]  (QFormLayout)
                      │     ├── row "Start Freq": QWidget[wgtNaStartRow]
                      │     │     ├── QDoubleSpinBox [spnNaStartFreq]
                      │     │     └── QComboBox [cmbNaStartFreqUnit]
                      │     ├── row "Stop Freq":  QWidget[wgtNaStopRow]
                      │     │     ├── QDoubleSpinBox [spnNaStopFreq]
                      │     │     └── QComboBox [cmbNaStopFreqUnit]
                      │     └── row "Points":     QSpinBox [spnNaPoints]
                      │
                      ├── QGroupBox [grpNaMeasurement]  (QVBoxLayout)
                      │     ├── QPushButton [btnNaMeasure]
                      │     ├── QLabel [lblNaSParamDisplay]
                      │     └── QProgressBar [prgNaMeasurement]
                      │
                      └── QGroupBox [grpNaMeasStatus]  (QGridLayout, 2 cols)
                            ├── [0,0] QLabel [lblNaStateDot]   (12×12 dot)
                            ├── [0,1] QLabel [lblNaStateText]
                            ├── [1,0] QLabel [lblNaRangeSummary]
                            └── [1,1] QLabel [lblNaPointsSummary]
```

---

## 8. Public API (for SWE reference — not code, design only)

```
class SignalPanel : public QDockWidget {
  Q_OBJECT
public:
  explicit SignalPanel(QWidget* parent = nullptr);

  void setSignalGeneratorController(
      mwa::hardware::SignalGeneratorControllerInterface* controller);
  void setNetworkAnalyzerController(
      mwa::hardware::NetworkAnalyzerControllerInterface* controller);
};
```

Both setters accept `nullptr` (controller absent — corresponding tab fully
disabled). Controllers may be set before or after the panel is shown.

---

## 9. Design Decisions and Rationale

1. **Single shared Connection section.** In common lab setups (e.g., Keysight
   VNA or Rhode & Schwarz ZVA), the signal generator and network analyzer are
   a single instrument. A shared connect button reflects this reality and avoids
   duplicating port selectors. When separate instruments are used, SWE can wire
   the single connect button to both controllers.

2. **QTabWidget over stacked layout.** The combined set of controls (six
   frequency spinboxes, amplitude, waveform, sweep, measure, display area)
   would overflow a 400 px panel if shown vertically. QTabWidget cleanly
   separates the two functional modes without horizontal scrolling.

3. **Frequency spinbox + unit combobox pattern.** A plain Hz spinbox would
   require the researcher to type "2000000000" for 2 GHz — error-prone.
   Separate value and unit selectors allow natural entry (e.g., "2.0" + "GHz")
   while the panel converts to Hz internally before all controller calls.

4. **Destructive action confirmation.** Disabling signal output mid-experiment
   can disrupt active microfluidic processes. `QMessageBox::question` is
   mandatory for the Output OFF transition and for Disconnect, per UX standards.

5. **Indeterminate progress bar for measurement.** Network analyzer sweep
   duration is instrument- and point-count-dependent. An indeterminate bar
   (`maximum = 0`) conveys activity without implying knowledge of elapsed time.
   When the controller provides incremental progress signals in a future sprint,
   SWE can switch to a determinate bar.

6. **S-parameter display as QLabel placeholder.** The requirements state
   "placeholder QLabel". Full charting is outside GUI-REQ-004's scope.
   The label is sized to 120 px minimum height so it reads as a dedicated
   display region and can be swapped for a QChartView in a later sprint.

---

## Submission

**SUBMISSION: MWA-03-D**
**AGENT: UX**
**FILES CHANGED:**
- `docs/designs/signal_panel_design.md` (this file — new)

**REQUIREMENT:** GUI-REQ-004
**STATUS:** Ready for Lead review
**NOTES:**
- No code written. Design only.
- Frequency unit conversion rule is fully specified; SWE must implement
  the Hz↔unit conversion internally — controller always receives Hz.
- The S-parameter display is a QLabel placeholder per requirements;
  charting is out of scope for this task.
- Both controller pointers are nullable; panel must handle nullptr gracefully
  (full tab disabled, no crash).
- Right-click context menus on both tabs are specified per UX standards
  (Reset, Export/Copy Settings, Copy Log).
