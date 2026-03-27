# MWA — Unified GUI Panel Design Specification

## Design Philosophy

**One pattern, six devices.** Every panel follows the exact same visual and structural template. A user who learns the LED panel instantly understands the Camera panel. Differences are only in device-specific controls — the shell, layout, and interaction patterns are identical.

---

## Universal Panel Template

Every device panel is a `QDockWidget` containing a single `QWidget` with this vertical layout:

```
┌─────────────────────────────────────────────────┐
│ ■ LED Light Source                    [×]        │  ← Dock title bar (Qt default)
├─────────────────────────────────────────────────┤
│                                                 │
│  ┌─ Connection ────────────────────────────┐    │  SECTION 1: Connection
│  │  Port: [COM3          ▾]  Baud: [115200]│    │  (always the same for serial devices)
│  │  [● Connected]          [ Disconnect ]  │    │
│  └─────────────────────────────────────────┘    │
│                                                 │
│  ┌─ Controls ──────────────────────────────┐    │  SECTION 2: Controls
│  │                                         │    │  (device-specific inputs)
│  │  < device-specific control widgets >    │    │
│  │                                         │    │
│  └─────────────────────────────────────────┘    │
│                                                 │
│  ┌─ Status ────────────────────────────────┐    │  SECTION 3: Status
│  │                                         │    │  (device-specific read-only feedback)
│  │  < device-specific status widgets >     │    │
│  │                                         │    │
│  └─────────────────────────────────────────┘    │
│                                                 │
└─────────────────────────────────────────────────┘
```

### Three Sections — Always In This Order

| # | Section | Purpose | Same Across All Panels? |
|---|---------|---------|------------------------|
| 1 | **Connection** | Port selection, connect/disconnect, status dot | YES — identical layout |
| 2 | **Controls** | User inputs (sliders, spinboxes, buttons) | NO — device-specific |
| 3 | **Status** | Read-only feedback from device | NO — device-specific |

Each section is a `QGroupBox` with a title. The groupbox titles are always: **"Connection"**, **"Controls"**, **"Status"**.

---

## Section 1: Connection (Identical for All Panels)

```
┌─ Connection ───────────────────────────────────────────┐
│                                                        │
│  Port:  [/dev/cu.usbserial-1420 ▾]   Baud: [115200 ▾] │
│                                                        │
│  ● Connected                          [ Disconnect ]   │
│                                                        │
└────────────────────────────────────────────────────────┘
```

### Widgets

| Widget | Type | Behavior |
|--------|------|----------|
| Port combo | `QComboBox` | Populated from `QSerialPortInfo::availablePorts()`. Refresh button (⟳) next to it. Disabled while connected. |
| Baud combo | `QComboBox` | Values: 9600, 19200, 38400, 57600, 115200. Disabled while connected. |
| Status dot + label | `QLabel` | ● Green "Connected" / ● Yellow "Connecting..." / ● Red "Error: ..." / ● Gray "Disconnected" |
| Connect/Disconnect button | `QPushButton` | Text toggles: "Connect" (when disconnected) / "Disconnect" (when connected). Disabled during connecting. |

### Status Dot Colors (consistent everywhere)

| State | Dot | Label |
|-------|-----|-------|
| `kDisconnected` | ⚫ Gray (#888888) | "Disconnected" |
| `kConnecting` | 🟡 Yellow (#F5A623) | "Connecting..." |
| `kConnected` | 🟢 Green (#4CAF50) | "Connected" |
| `kError` | 🔴 Red (#F44336) | "Error: {message}" |

### Camera Exception

Camera uses Pylon SDK (not serial), so its Connection section is slightly different:

```
┌─ Connection ───────────────────────────────────────────┐
│                                                        │
│  Camera:  [Basler acA2440-35um (22012345) ▾]           │
│                                                        │
│  ● Connected                          [ Disconnect ]   │
│                                                        │
└────────────────────────────────────────────────────────┘
```

The port/baud combos are replaced by a single camera selector combo populated from device enumeration. Same status dot, same connect/disconnect button.

---

## Section 2: Controls (Device-Specific)

All controls use the same widget patterns:

### Widget Pattern Library

**A. Labeled Spinbox** — for numeric inputs

```
  Flow Rate:  [ 5.000   ↕ ] µL/min
```

- `QLabel` (left-aligned) + `QDoubleSpinBox` + unit suffix
- Label width fixed at 100px for alignment
- Spinbox stretches to fill

**B. Labeled Slider + Spinbox** — for range controls

```
  Intensity:  ──────●────────  [ 75.0  ↕ ] %
```

- `QLabel` + `QSlider` (horizontal) + `QDoubleSpinBox` + unit suffix
- Slider and spinbox bidirectionally synced
- Used for: LED intensity

**C. Combo Selector** — for enum choices

```
  Waveform:   [ Sine         ▾ ]
```

- `QLabel` + `QComboBox`
- Used for: waveform, frequency unit

**D. Action Button** — for triggering operations

```
  [ ▶ Start Infusion ]    [ ⏹ Stop ]
```

- `QPushButton` with text
- States: enabled/disabled based on device state
- Danger buttons (Stop, Emergency Stop): red background `#F44336`, white text

**E. Toggle Button** — for on/off states

```
  [ ◉ Output ON  ]     ← green background when active
  [ ○ Output OFF ]     ← default background when inactive
```

- `QPushButton` (checkable)
- Checked = active state (green background `#4CAF50`, white text)
- Unchecked = inactive state (default styling)
- Used for: LED power, SigGen output, Camera continuous capture

**F. Coordinate Group** — for X/Y/Z inputs (Stage only)

```
  X: [ 10.000 ↕ ] mm    Y: [ 5.000 ↕ ] mm    Z: [ 1.000 ↕ ] mm
```

- Three `QDoubleSpinBox` in a horizontal row with axis labels
- Used for: Stage target position

**G. Jog Pad** — for directional movement (Stage only)

```
          [ +Y ]
  [ -X ]  [HOME]  [ +X ]
          [ -Y ]

  [ +Z ]  [STOP]  [ -Z ]

  Step: [ 0.100 ↕ ] mm
```

- Grid of `QPushButton` (3×2 for XY, 1×2 for Z)
- HOME button: calls `home()`
- STOP button: red, always enabled, calls `stopMotion()`
- Step spinbox: jog increment

---

## Section 3: Status (Device-Specific)

All status displays use the same widget patterns:

### Status Widget Patterns

**A. Key-Value Row** — for single readings

```
  Current Intensity:    75.0 %
  Power State:          ON
```

- `QLabel` (key, bold, fixed width) + `QLabel` (value, right-aligned or left-aligned)
- Value updates via signal connection

**B. Position Display** — for coordinates (Stage)

```
  Position:   X: 10.000 mm    Y: 5.000 mm    Z: 1.000 mm
```

- Three `QLabel` values in a horizontal row
- Updated from `positionChanged(x, y, z)`

**C. Progress Bar** — for ongoing operations

```
  Dispensing:  [████████░░░░░░░░░░░░]  50 / 100 µL
```

- `QProgressBar` + `QLabel` showing current/target
- Used for: Pump dispensing, Camera batch capture

**D. Chart View** — for data plots (VNA only)

```
  ┌──────────────────────────────────────┐
  │ S21 Magnitude (dB)                   │
  │                                      │
  │   -20 ─         ╱╲                   │
  │   -30 ─    ╱──╱    ╲──╲             │
  │   -40 ─ ╱╱              ╲╲──        │
  │   -50 ─╱                    ╲       │
  │        ├────┬────┬────┬────┤        │
  │       1.0  2.0  3.0  4.0  5.0 GHz  │
  └──────────────────────────────────────┘
```

- `QChartView` with `QLineSeries`
- X axis: frequency (auto-scaled with SI prefix)
- Y axis: magnitude in dB
- Updated after `measurementComplete()`

**E. Image Preview** — for camera frames

```
  ┌──────────────────────────────────────┐
  │                                      │
  │            (live image)              │
  │                                      │
  │                                      │
  └──────────────────────────────────────┘
```

- `QLabel` displaying scaled `QPixmap`
- Aspect ratio preserved (`Qt::KeepAspectRatio`)
- Black background
- Updated from `frameReady(QImage)`

---

## Per-Panel Layouts

### Panel 1: LED Light Source

```
┌─ Connection ───────────────────────────────────────┐
│  Port: [▾]  Baud: [115200 ▾]                       │
│  ● Connected                      [ Disconnect ]   │
└────────────────────────────────────────────────────┘

┌─ Controls ─────────────────────────────────────────┐
│                                                    │
│  Power:    [ ◉ ON  ]                               │  ← Toggle Button (E)
│                                                    │
│  Intensity: ──────●────────  [ 75.0  ↕ ] %         │  ← Slider+Spinbox (B)
│                                                    │
└────────────────────────────────────────────────────┘

┌─ Status ───────────────────────────────────────────┐
│  Current Intensity:    75.0 %                      │  ← Key-Value (A)
│  Power State:          ON                          │  ← Key-Value (A)
└────────────────────────────────────────────────────┘
```

### Panel 2: Syringe Pump

```
┌─ Connection ───────────────────────────────────────┐
│  Port: [▾]  Baud: [115200 ▾]                       │
│  ● Connected                      [ Disconnect ]   │
└────────────────────────────────────────────────────┘

┌─ Controls ─────────────────────────────────────────┐
│                                                    │
│  Flow Rate:      [ 5.000    ↕ ] µL/min             │  ← Labeled Spinbox (A)
│  Target Volume:  [ 100.000  ↕ ] µL                 │  ← Labeled Spinbox (A)
│                                                    │
│  [ ▶ Start ]    [ ⏹ Stop ]    [ ↻ Refill ]        │  ← Action Buttons (D)
│                                                    │
└────────────────────────────────────────────────────┘

┌─ Status ───────────────────────────────────────────┐
│  Flow Rate:    5.000 µL/min                        │  ← Key-Value (A)
│  Dispensed:    [████████░░░░░░]  50.0 / 100.0 µL   │  ← Progress Bar (C)
│  State:        Infusing                            │  ← Key-Value (A)
└────────────────────────────────────────────────────┘
```

### Panel 3: Signal Generator

```
┌─ Connection ───────────────────────────────────────┐
│  Port: [▾]  Baud: [115200 ▾]                       │
│  ● Connected                      [ Disconnect ]   │
└────────────────────────────────────────────────────┘

┌─ Controls ─────────────────────────────────────────┐
│                                                    │
│  Frequency:  [ 1.000    ↕ ] [MHz ▾]               │  ← Spinbox + Unit Combo
│  Amplitude:  [ 2.500    ↕ ] Vpp                   │  ← Labeled Spinbox (A)
│  Waveform:   [ Sine         ▾ ]                   │  ← Combo Selector (C)
│                                                    │
│  Output:     [ ◉ ON  ]                            │  ← Toggle Button (E)
│                                                    │
│  ── Sweep ──                                       │
│  Start:  [ 0.500 ↕ ] MHz   Stop: [ 2.000 ↕ ] MHz │  ← Labeled Spinbox (A)
│  Step:   [ 0.010 ↕ ] MHz                          │  ← Labeled Spinbox (A)
│  [ Apply Sweep ]                                   │  ← Action Button (D)
│                                                    │
└────────────────────────────────────────────────────┘

┌─ Status ───────────────────────────────────────────┐
│  Frequency:    1.000 MHz                           │  ← Key-Value (A)
│  Amplitude:    2.500 Vpp                           │  ← Key-Value (A)
│  Waveform:     Sine                                │  ← Key-Value (A)
│  Output:       ON                                  │  ← Key-Value (A)
└────────────────────────────────────────────────────┘
```

### Panel 4: Network Analyzer (VNA)

```
┌─ Connection ───────────────────────────────────────┐
│  Port: [▾]  Baud: [115200 ▾]                       │
│  ● Connected                      [ Disconnect ]   │
└────────────────────────────────────────────────────┘

┌─ Controls ─────────────────────────────────────────┐
│                                                    │
│  Start Freq: [ 1.000  ↕ ] [GHz ▾]                 │  ← Spinbox + Unit Combo
│  Stop Freq:  [ 5.000  ↕ ] [GHz ▾]                 │  ← Spinbox + Unit Combo
│  Points:     [ 201    ↕ ]                          │  ← Labeled Spinbox (A)
│                                                    │
│  [ ▶ Measure ]                                     │  ← Action Button (D)
│                                                    │
└────────────────────────────────────────────────────┘

┌─ Status ───────────────────────────────────────────┐
│  State:   Ready                                    │  ← Key-Value (A)
│                                                    │
│  ┌──────────────────────────────────────────┐      │
│  │ S21 Magnitude (dB) vs Frequency          │      │  ← Chart View (D)
│  │                                          │      │
│  │   (chart renders here after measurement) │      │
│  │                                          │      │
│  └──────────────────────────────────────────┘      │
│                                                    │
└────────────────────────────────────────────────────┘
```

### Panel 5: Camera

```
┌─ Connection ───────────────────────────────────────┐
│  Camera: [Basler acA2440-35um (22012345) ▾]        │
│  ● Connected                      [ Disconnect ]   │
└────────────────────────────────────────────────────┘

┌─ Controls ─────────────────────────────────────────┐
│                                                    │
│  Exposure:  [ 25.000  ↕ ] ms                       │  ← Labeled Spinbox (A)
│  Gain:      [ 6.000   ↕ ] dB                       │  ← Labeled Spinbox (A)
│                                                    │
│  ── ROI ──                                         │
│  X: [ 0  ↕ ]  Y: [ 0  ↕ ]  W: [ 640 ↕ ]  H: [ 480 ↕ ]  ← Coordinate Group (F)
│                                                    │
│  ── Capture ──                                     │
│  [ Grab Single ]  [ ◉ Continuous ]                 │  ← Action (D) + Toggle (E)
│                                                    │
│  Batch:  Count: [ 10 ↕ ]  Interval: [ 500 ↕ ] ms  │  ← Labeled Spinbox (A)
│  [ ▶ Start Batch ]                                 │  ← Action Button (D)
│                                                    │
└────────────────────────────────────────────────────┘

┌─ Status ───────────────────────────────────────────┐
│  ┌──────────────────────────────────────────┐      │
│  │                                          │      │
│  │           (live image preview)           │      │  ← Image Preview (E)
│  │                                          │      │
│  └──────────────────────────────────────────┘      │
│  Batch:  [████████░░░░░░]  5 / 10 frames           │  ← Progress Bar (C)
│  Exposure:  25.000 ms    Gain: 6.0 dB              │  ← Key-Value (A)
└────────────────────────────────────────────────────┘
```

### Panel 6: XYZ Stage

```
┌─ Connection ───────────────────────────────────────┐
│  Port: [▾]  Baud: [115200 ▾]                       │
│  ● Connected                      [ Disconnect ]   │
└────────────────────────────────────────────────────┘

┌─ Controls ─────────────────────────────────────────┐
│                                                    │
│  ── Move To ──                                     │
│  X: [ 10.000 ↕ ] mm  Y: [ 5.000 ↕ ] mm  Z: [ 1.000 ↕ ] mm  ← Coord Group (F)
│  [ ▶ Go ]                                          │  ← Action Button (D)
│                                                    │
│  ── Jog ──                                         │
│           [ +Y ]                                   │
│   [ -X ]  [HOME]  [ +X ]                          │  ← Jog Pad (G)
│           [ -Y ]                                   │
│                                                    │
│   [ +Z ]          [ -Z ]                           │
│                                                    │
│  Step: [ 0.100 ↕ ] mm                              │  ← Labeled Spinbox (A)
│  Speed: [ 1.000 ↕ ] mm/s                           │  ← Labeled Spinbox (A)
│                                                    │
│  [ ⏹ STOP ]                                       │  ← Danger Button (D, red)
│                                                    │
└────────────────────────────────────────────────────┘

┌─ Status ───────────────────────────────────────────┐
│  Position:  X: 10.000 mm  Y: 5.000 mm  Z: 1.000 mm│  ← Position Display (B)
│  Speed:     1.000 mm/s                             │  ← Key-Value (A)
│  Motion:    Idle                                   │  ← Key-Value (A)
└────────────────────────────────────────────────────┘
```

---

## Main Window Layout

```
┌─────────────────────────────────────────────────────────────────────┐
│  File   View   Devices   Help                                       │  ← Menu Bar
├─────────────────────────────────────────────────────────────────────┤
│  [Connect All]  [Disconnect All]  │  separator  │                   │  ← Toolbar
├──────────────────────┬──────────────────────────────────────────────┤
│                      │                                              │
│   Device Panels      │           Central Workspace                  │
│   (Left Dock Area)   │                                              │
│                      │   ┌─ Status Dashboard ──────────────────┐    │
│  ┌─ LED ──────────┐  │   │  Device      Type     Status        │    │
│  │ (collapsed or  │  │   │  LED         Serial   ● Connected   │    │
│  │  expanded)     │  │   │  Pump        Serial   ● Connected   │    │
│  └────────────────┘  │   │  SigGen      Serial   ○ Disconnected│    │
│                      │   │  VNA         Serial   ○ Disconnected│    │
│  ┌─ Pump ─────────┐  │   │  Camera      USB3     ● Connected   │    │
│  │                │  │   │  Stage       Serial   ● Connected   │    │
│  └────────────────┘  │   │                                     │    │
│                      │   │  [Connect All]    [Disconnect All]  │    │
│  ┌─ SigGen ───────┐  │   └─────────────────────────────────────┘    │
│  │                │  │                                              │
│  └────────────────┘  │   ┌─ Camera Preview ────────────────────┐    │
│                      │   │                                     │    │
│  ┌─ VNA ──────────┐  │   │         (live camera image)         │    │
│  │                │  │   │                                     │    │
│  └────────────────┘  │   └─────────────────────────────────────┘    │
│                      │                                              │
│  ┌─ Camera ───────┐  │   ┌─ VNA Chart ────────────────────────┐    │
│  │                │  │   │   (S-parameter plot)               │    │
│  └────────────────┘  │   └─────────────────────────────────────┘    │
│                      │                                              │
│  ┌─ Stage ────────┐  │                                              │
│  │                │  │                                              │
│  └────────────────┘  │                                              │
│                      │                                              │
├──────────────────────┴──────────────────────────────────────────────┤
│                                                                     │
│  ┌─ Log ───────────────────────────────────────────────────────┐    │
│  │  Severity: [All ▾]  Source: [All ▾]  [Clear]  [Export]      │    │  ← Bottom Dock
│  │  ☐ Auto-scroll                                              │    │
│  │                                                             │    │
│  │  12:34:56.789  INFO     LED      Connected on /dev/cu.usb.. │    │
│  │  12:34:57.001  INFO     Pump     Connected on COM3          │    │
│  │  12:35:01.234  WARNING  Camera   Grab timeout, retrying     │    │
│  │  12:35:02.567  ERROR    Stage    Axis X limit reached       │    │
│  └─────────────────────────────────────────────────────────────┘    │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### Dock Layout Rules

| Dock Area | Panels | Default State |
|-----------|--------|---------------|
| Left | LED, Pump, SigGen, VNA, Camera, Stage (stacked/tabbed) | All visible, scrollable |
| Center | Status Dashboard, Camera Preview, VNA Chart | Tabbed or stacked |
| Bottom | Log Panel | Visible, height ~150px |

### Menu Structure

| Menu | Items |
|------|-------|
| **File** | Settings..., Separator, Exit |
| **View** | Toggle LED Panel, Toggle Pump Panel, Toggle SigGen Panel, Toggle VNA Panel, Toggle Camera Panel, Toggle Stage Panel, Separator, Toggle Log Panel, Toggle Status Dashboard |
| **Devices** | Connect All, Disconnect All, Separator, Refresh Ports |
| **Help** | About MWA, About Qt |

---

## Styling Constants

All panels use these shared constants (defined in a single header or QSS):

```
// Colors
kColorConnected    = "#4CAF50"   // Green
kColorConnecting   = "#F5A623"   // Yellow/Amber
kColorError        = "#F44336"   // Red
kColorDisconnected = "#888888"   // Gray
kColorDangerButton = "#F44336"   // Red (Stop buttons)
kColorActiveToggle = "#4CAF50"   // Green (ON state)

// Layout
kLabelWidth        = 100         // Fixed label width for alignment
kGroupBoxMargin    = 8           // Margin inside group boxes
kWidgetSpacing     = 6           // Vertical spacing between rows
kSpinBoxWidth      = 120         // Minimum spinbox width

// Fonts
kStatusDotSize     = 12          // Status dot diameter (px)
```

---

## Reusable Base Class: DevicePanel

To enforce the uniform pattern, all panels inherit from a single `DevicePanel` base class:

```cpp
class DevicePanel : public QDockWidget {
  Q_OBJECT
public:
  explicit DevicePanel(const QString& title,
                       DeviceInterface* device,
                       QWidget* parent = nullptr);

protected:
  // Subclasses override these to provide device-specific content
  virtual QWidget* createControlsWidget() = 0;
  virtual QWidget* createStatusWidget() = 0;

  // Shared helpers available to all panels
  QHBoxLayout* createLabeledSpinbox(const QString& label,
                                     QDoubleSpinBox** out_spinbox,
                                     double min, double max,
                                     double step, int decimals,
                                     const QString& suffix);

  QHBoxLayout* createLabeledCombo(const QString& label,
                                   QComboBox** out_combo,
                                   const QStringList& items);

  QPushButton* createToggleButton(const QString& on_text,
                                   const QString& off_text);

  QPushButton* createActionButton(const QString& text,
                                   bool is_danger = false);

  QHBoxLayout* createKeyValueRow(const QString& key,
                                  QLabel** out_value);

  // Connection section is built automatically by the base class
  QComboBox* portCombo();
  QComboBox* baudCombo();
  QPushButton* connectButton();
  QLabel* statusLabel();

  // Access the device interface
  DeviceInterface* device() const;

private:
  void buildConnectionSection();  // Identical for all panels
  void updateConnectionState(DeviceInterface::DeviceState state);

  DeviceInterface* device_;
  QComboBox* port_combo_;
  QComboBox* baud_combo_;
  QPushButton* connect_button_;
  QLabel* status_label_;
  QLabel* status_dot_;
};
```

### Why This Matters

1. **Connection section** is built once in the base class — no duplication across 6 panels
2. **Helper methods** ensure every spinbox, combo, button looks identical
3. **Adding a new device** = subclass DevicePanel, implement `createControlsWidget()` + `createStatusWidget()`
4. **GUI never changes for hardware** — panels bind to interface pointers, not concrete classes

---

## Interaction Rules (Same for All Panels)

| Rule | Behavior |
|------|----------|
| **Connect flow** | User selects port → clicks Connect → button disables, shows "Connecting..." → on success: controls enable, status dot green → on error: status dot red with message |
| **Disconnected state** | All controls in Controls section are **disabled**. Only Connection section is active. |
| **Connected state** | All controls enabled. Connection section port/baud combos disabled (can't change while connected). |
| **Error display** | Status dot turns red. Error message shown in status label. Controls remain enabled (user can retry). Log entry auto-generated. |
| **Value echo** | When user changes a control (e.g., intensity slider), the GUI updates immediately. The Status section updates only when the device confirms via signal (e.g., `intensityChanged`). This means Controls show the *desired* value, Status shows the *confirmed* value. |
| **Settings restore** | On panel construction, load last values from SettingsManager. Apply to controls but do NOT send to device (device not connected yet). |
| **Settings save** | On every control change, save to SettingsManager. On window close, save window geometry/dock state. |
