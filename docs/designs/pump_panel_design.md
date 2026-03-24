# Syringe Pump Control Panel Design Specification

**Task:** MWA-03-C
**Requirement:** GUI-REQ-003
**Designer:** UX Agent
**Date:** 2026-03-23
**Status:** Submitted for Lead Review

---

## Overview

`PumpPanel` is a `QWidget` intended to be hosted inside a `QDockWidget`. It
controls the Cetoni Nemesys syringe pump by wrapping a
`PumpControllerInterface*` pointer. The panel provides three clearly separated
`QGroupBox` sections — Connection, Controls, Status — that match the mandatory
Device Panel Template.

Panel width: minimum 280 px, maximum 400 px.
All controls scale vertically; the panel does not have a fixed height.

---

## 1. ASCII Layout

```
┌─ Syringe Pump ─────────────────────────────────┐
│                                                 │
│ ┌─ Connection ──────────────────────────────┐  │
│ │ Port: [USB — Nemesys          ▼]          │  │
│ │        [● Disconnected      ] [Connect  ] │  │
│ └───────────────────────────────────────────┘  │
│                                                 │
│ ┌─ Controls ────────────────────────────────┐  │
│ │ Flow Rate:  [  1.00  ▲▼]  µL/min          │  │
│ │ Volume:     [ 10.00  ▲▼]  µL              │  │
│ │                                           │  │
│ │ [  Start Infusion  ]  [  Stop  ]          │  │
│ │ [        Refill         ]                 │  │
│ └───────────────────────────────────────────┘  │
│                                                 │
│ ┌─ Status ──────────────────────────────────┐  │
│ │ Position:  [=== 10.00 µL ===]             │  │
│ │ State:     ● Idle                         │  │
│ │ Progress:  [████████░░░░░░░░] 50%         │  │
│ │                                           │  │
│ │ (error label — hidden when no error)      │  │
│ └───────────────────────────────────────────┘  │
│                                                 │
└─────────────────────────────────────────────────┘
```

### Detail: Connection Group

```
┌─ Connection ──────────────────────────────────────┐
│  Port:  [QComboBox — device name/port   ▼]        │
│  [QLabel: ● Disconnected        ] [QPushButton]   │
└────────────────────────────────────────────────────┘
```

- The combo box row uses `QFormLayout` label "Port:".
- The indicator + button row uses a `QHBoxLayout`; the label expands, the
  button is fixed width 90 px, minimum height 32 px.
- The colored dot is a 12×12 px `QLabel` with `border-radius: 6px` stylesheet,
  colored per state (see State Table).

### Detail: Controls Group

```
┌─ Controls ────────────────────────────────────────┐
│  Flow Rate:  [QDoubleSpinBox▲▼]  µL/min           │
│  Volume:     [QDoubleSpinBox▲▼]  µL               │
│                                                   │
│  [  Start Infusion  ]  [  Stop  ]                 │
│  [          Refill          ]                     │
└────────────────────────────────────────────────────┘
```

- `QFormLayout` for the two numeric rows (label + spinbox + unit label).
- `QHBoxLayout` for Start / Stop row; both buttons equal width, minimum 32 px
  height.
- Refill button is full-width below the Start/Stop row, minimum 32 px height.

### Detail: Status Group

```
┌─ Status ──────────────────────────────────────────┐
│  Position:  [   10.00 µL   ]   ← QLCDNumber       │
│  State:     [● label]          ← dot + QLabel      │
│  Progress:  [QProgressBar   ]  ← 0–100%            │
│  [error QLabel — red, hidden by default]          │
└────────────────────────────────────────────────────┘
```

- Position uses `QLCDNumber` (7-segment style) with a trailing "µL" `QLabel`
  in a `QHBoxLayout`.
- State row: 12×12 dot `QLabel` + state text `QLabel` in a `QHBoxLayout`.
- Progress bar is always visible; set to 0 when idle/disconnected, animated
  (busy-style) during refill, percent-based during infusion.
- Error label is `QLabel` hidden by default; shown in red when
  `errorOccurred()` fires.

---

## 2. Widget Table

### PumpPanel (root widget)

| Widget | Type | objectName | Properties | Signals / Slots |
|--------|------|------------|------------|-----------------|
| Panel root | `QWidget` | `pumpPanel` | Layout: `QVBoxLayout`, margins: 8 px, spacing: 12 px; min-width: 280 px, max-width: 400 px | — |

### Connection Group

| Widget | Type | objectName | Properties | Signals / Slots |
|--------|------|------------|------------|-----------------|
| Connection group | `QGroupBox` | `grpConnection` | Title: "Connection"; layout: `QVBoxLayout`, inner margin: 8 px, spacing: 4 px | — |
| Port combo | `QComboBox` | `cmbPort` | Placeholder: "Select device…"; min-height: 32 px; populated on construction / refresh | `currentIndexChanged(int)` |
| Indicator dot | `QLabel` | `lblStatusDot` | Fixed 12×12 px; stylesheet: `border-radius: 6px; background-color: #95A5A6;` | — |
| Status text | `QLabel` | `lblStatusText` | Text: "Disconnected"; font: 12 pt; color: `#95A5A6`; sizePolicy: Expanding | — |
| Connect button | `QPushButton` | `btnConnect` | Text: "Connect"; fixed width: 90 px; min-height: 32 px; tooltip: "Connect to pump (Ctrl+Shift+P)" | `clicked()` |

### Controls Group

| Widget | Type | objectName | Properties | Signals / Slots |
|--------|------|------------|------------|-----------------|
| Controls group | `QGroupBox` | `grpControls` | Title: "Controls"; layout: `QVBoxLayout`, inner margin: 8 px, spacing: 4 px | — |
| Flow rate spinbox | `QDoubleSpinBox` | `spnFlowRate` | min: 0.01; max: 1000.00; value: 1.00; decimals: 2; suffix: " µL/min"; singleStep: 0.10; min-height: 32 px; tooltip: "Infusion flow rate in microlitres per minute" | `valueChanged(double)` |
| Flow rate unit label | `QLabel` | `lblFlowRateUnit` | Text: "µL/min"; font: 12 pt; alignment: AlignVCenter (rendered inside QFormLayout, after suffix — serves as accessible field label supplement) | — |
| Volume spinbox | `QDoubleSpinBox` | `spnVolume` | min: 0.01; max: 10000.00; value: 10.00; decimals: 2; suffix: " µL"; singleStep: 1.00; min-height: 32 px; tooltip: "Target infusion volume in microlitres" | `valueChanged(double)` |
| Volume unit label | `QLabel` | `lblVolumeUnit` | Text: "µL"; font: 12 pt; alignment: AlignVCenter | — |
| Start button | `QPushButton` | `btnStartInfusion` | Text: "Start Infusion"; sizePolicy: Expanding; min-height: 32 px; tooltip: "Start infusion (Ctrl+Return)" | `clicked()` |
| Stop button | `QPushButton` | `btnStop` | Text: "Stop"; sizePolicy: Expanding; min-height: 32 px; tooltip: "Stop infusion (Ctrl+.)" | `clicked()` |
| Refill button | `QPushButton` | `btnRefill` | Text: "Refill"; sizePolicy: Expanding (full-width row); min-height: 32 px; tooltip: "Retract plunger to refill syringe (Ctrl+R)" | `clicked()` |

### Status Group

| Widget | Type | objectName | Properties | Signals / Slots |
|--------|------|------------|------------|-----------------|
| Status group | `QGroupBox` | `grpStatus` | Title: "Status"; layout: `QVBoxLayout`, inner margin: 8 px, spacing: 4 px | — |
| Position LCD | `QLCDNumber` | `lcdPosition` | digitCount: 7; mode: Dec; segmentStyle: Flat; min-height: 40 px; toolTip: "Current syringe plunger position (remaining volume). Double-click to copy." | double-click → copy to clipboard |
| Position unit label | `QLabel` | `lblPositionUnit` | Text: "µL"; font: 12 pt; alignment: AlignVCenter | — |
| State dot | `QLabel` | `lblStateDot` | Fixed 12×12 px; stylesheet: `border-radius: 6px; background-color: #95A5A6;` | — |
| State text label | `QLabel` | `lblStateText` | Text: "Idle"; font: 12 pt; sizePolicy: Expanding | — |
| Progress bar | `QProgressBar` | `prgInfusion` | range: 0–100; value: 0; textVisible: true; format: "%p%"; min-height: 20 px | — |
| Error label | `QLabel` | `lblError` | Text: ""; font: 12 pt; color: `#E74C3C`; wordWrap: true; hidden by default (`setVisible(false)`); alignment: AlignTop|AlignLeft | — |

---

## 3. State Table

| UI State | Trigger | Status Dot Color | Status Text | Controls Enabled | Start | Stop | Refill | Progress | Error Label |
|----------|---------|-----------------|-------------|-----------------|-------|------|--------|----------|-------------|
| **Disconnected** | Initial / `stateChanged(kDisconnected)` | `#95A5A6` gray | "Disconnected" | grpControls disabled, grpStatus disabled | disabled | disabled | disabled | value: 0, disabled | hidden |
| **Connecting** | `stateChanged(kConnecting)` | `#F39C12` orange | "Connecting…" | grpControls disabled | disabled | disabled | disabled | indeterminate (busy) | hidden |
| **Connected — Idle** | `stateChanged(kConnected)` + not infusing | `#27AE60` green | "Connected" | grpControls enabled | enabled | disabled | enabled | value: 0 | hidden |
| **Infusing** | `infusionStarted()` | `#3498DB` blue | "Infusing" | spnFlowRate disabled, spnVolume disabled | disabled | enabled | disabled | animated 0→100% proportional to volume consumed | hidden |
| **Refilling** | `refill()` called | `#3498DB` blue | "Refilling" | spnFlowRate disabled, spnVolume disabled | disabled | disabled | disabled | indeterminate (busy) | hidden |
| **Error** | `errorOccurred(message)` | `#E74C3C` red | "Error" | grpControls disabled | disabled | disabled | disabled | value: 0 | visible, shows message |

### Connect Button Label Transitions

| State | btnConnect Text | btnConnect Enabled |
|-------|-----------------|--------------------|
| Disconnected | "Connect" | true |
| Connecting | "Connecting…" | false |
| Connected (any) | "Disconnect" | true |
| Error | "Reconnect" | true |

### State Dot — Status Group (lblStateDot / lblStateText)

| Pump Sub-state | Dot Color | Text |
|----------------|-----------|------|
| Idle (connected, not infusing) | `#27AE60` green | "Idle" |
| Infusing | `#3498DB` blue | "Infusing" |
| Refilling | `#3498DB` blue | "Refilling" |

---

## 4. Interaction List

| User Action | Precondition | UI Response | Signal / Call |
|-------------|-------------|-------------|---------------|
| Select port in `cmbPort` | Any | Updates selected device port string | `cmbPort.currentIndexChanged` |
| Click "Connect" | State: Disconnected / Error | btnConnect disabled, text → "Connecting…", dot → orange, status → "Connecting…", progress → busy | `controller->connectDevice()` |
| Click "Disconnect" | State: Connected | `QMessageBox::question` confirmation dialog; on Yes: grpControls disabled, dot → gray, status → "Disconnected" | `controller->disconnectDevice()` |
| Click "Reconnect" | State: Error | Same as Connect action | `controller->connectDevice()` |
| Edit flow rate spinbox | State: Connected-Idle | Value updates immediately; tooltip shows new value | `controller->setFlowRate(value)` via `valueChanged(double)` |
| Edit volume spinbox | State: Connected-Idle | Value updates immediately | `controller->setTargetVolume(value)` via `valueChanged(double)` |
| Click "Start Infusion" | State: Connected-Idle | `QMessageBox::question` if volume > syringe capacity warning; btnStart disabled, btnStop enabled, spinboxes disabled, dot → blue, state → "Infusing", progress animates | `controller->startInfusion()` |
| Click "Stop" | State: Infusing | `QMessageBox::question` confirmation ("Stop the current infusion?"); on Yes: btnStop disabled, btnStart enabled, spinboxes enabled, dot → green, state → "Idle", progress stops at current value | `controller->stopInfusion()` |
| Click "Refill" | State: Connected-Idle | `QMessageBox::question` confirmation ("Retract plunger to refill?"); on Yes: all controls disabled, dot → blue, state → "Refilling", progress → busy | `controller->refill()` |
| `positionChanged(uL)` received | Any | `lcdPosition` updates to new value; `prgInfusion` updates percentage relative to target volume | — (slot) |
| `infusionStopped()` received | State: Infusing | Transition to Connected-Idle state; progress stops | — (slot) |
| `errorOccurred(msg)` received | Any | Dot → red, status text → "Error", lblError visible with msg, grpControls disabled, btnConnect text → "Reconnect" | — (slot) |
| `stateChanged(kConnected)` received | Any | Dot → green, status → "Connected", btnConnect → "Disconnect", grpControls enabled (if not infusing/refilling) | — (slot) |
| Double-click `lcdPosition` | Any | Copy position value to clipboard via `QGuiApplication::clipboard()` | — |
| Right-click panel | Any | Context menu: "Reset to Defaults", "Export Settings…", "Copy Log" | respective slots |
| Press Escape | Dialog open | Close dialog without action | Qt default |

---

## 5. Keyboard Shortcuts

| Shortcut | Action | Widget / Scope |
|----------|--------|----------------|
| `Ctrl+Shift+P` | Connect / Disconnect | `btnConnect` — panel scope |
| `Ctrl+Return` | Start Infusion | `btnStartInfusion` — panel scope |
| `Ctrl+.` | Stop Infusion | `btnStop` — panel scope |
| `Ctrl+R` | Refill | `btnRefill` — panel scope |
| `Tab` | Move to next field | Standard Qt tab order |
| `Shift+Tab` | Move to previous field | Standard Qt tab order |
| `Escape` | Cancel / close dialog | Active `QMessageBox` |
| `Up` / `Down` | Increment / decrement spinbox | `spnFlowRate`, `spnVolume` when focused |

Tab order within panel (top-to-bottom, left-to-right):

1. `cmbPort`
2. `btnConnect`
3. `spnFlowRate`
4. `spnVolume`
5. `btnStartInfusion`
6. `btnStop`
7. `btnRefill`

---

## 6. Layout Hierarchy (Qt Layout Tree)

```
PumpPanel (QWidget)
└── QVBoxLayout [margins: 8, spacing: 12]
    ├── grpConnection (QGroupBox "Connection")
    │   └── QVBoxLayout [margins: 8, spacing: 4]
    │       ├── QFormLayout
    │       │   └── "Port:" → cmbPort
    │       └── QHBoxLayout
    │           ├── lblStatusDot
    │           ├── lblStatusText  [Expanding]
    │           └── btnConnect
    ├── grpControls (QGroupBox "Controls")
    │   └── QVBoxLayout [margins: 8, spacing: 4]
    │       ├── QFormLayout
    │       │   ├── "Flow Rate:" → spnFlowRate
    │       │   └── "Volume:"    → spnVolume
    │       ├── QHBoxLayout
    │       │   ├── btnStartInfusion [Expanding]
    │       │   └── btnStop          [Expanding]
    │       └── QHBoxLayout
    │           └── btnRefill [Expanding, full width]
    └── grpStatus (QGroupBox "Status")
        └── QVBoxLayout [margins: 8, spacing: 4]
            ├── QFormLayout
            │   └── "Position:" → QHBoxLayout
            │                      ├── lcdPosition [Expanding]
            │                      └── lblPositionUnit
            ├── QHBoxLayout
            │   ├── lblStateDot
            │   └── lblStateText [Expanding]
            ├── QFormLayout
            │   └── "Progress:" → prgInfusion
            └── lblError [hidden by default]
```

---

## 7. Public API for SWE

The following interface is inferred from the design. SWE must add full Doxygen
documentation on every method.

```
class PumpPanel : public QWidget {
  Q_OBJECT
public:
  explicit PumpPanel(QWidget* parent = nullptr);

  /// Attach the controller. Panel observes all signals. Must be called before
  /// the panel is shown. Pass nullptr to detach.
  void setController(mwa::hardware::PumpControllerInterface* controller);

signals:
  /// Emitted after the user confirms and initiates a connection request.
  void connectRequested();

  /// Emitted after the user confirms and initiates a disconnection request.
  void disconnectRequested();

private slots:
  void onConnectClicked();
  void onStartInfusionClicked();
  void onStopClicked();
  void onRefillClicked();
  void onFlowRateChanged(double uL_per_min);
  void onVolumeChanged(double uL);
  void onStateChanged(mwa::hardware::DeviceInterface::DeviceState state);
  void onErrorOccurred(const QString& message);
  void onPositionChanged(double uL);
  void onInfusionStarted();
  void onInfusionStopped();
};
```

---

## 8. Ambiguities / Open Questions for Lead

1. **Port combo population**: The design assumes the combo box is pre-populated
   with available serial/USB ports. Should the panel scan ports itself
   (QSerialPortInfo) or should port selection be handled by a separate
   connection dialog? Recommend: scan on construction + "Refresh" button.

2. **Progress bar calculation**: `positionChanged(uL)` gives absolute remaining
   volume. Progress percentage requires the initial syringe capacity. Should
   `PumpControllerInterface` expose a `syringeCapacity()` method, or should
   progress be computed from the ratio of dispensed volume to target volume?
   Recommend: use `dispensed / targetVolume * 100` for simplicity.

3. **Refill completion signal**: `PumpControllerInterface` emits `infusionStopped`
   for both stop-infusion and end-of-refill. Should refill completion be
   detected by a separate signal, or by observing position returning to maximum?
   This affects how the UI transitions out of "Refilling" state.

4. **Flow rate / volume spinbox maximums**: Max flow rate set to 1000.00 µL/min
   and max volume to 10 000.00 µL. Are these within Nemesys hardware limits or
   should they be controller-reported at connect time?

---

*Submitted to Lead for review.*
