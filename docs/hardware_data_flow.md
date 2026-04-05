# Hardware Data Flow Architecture

## Purpose

This document defines the **exact data types, signal/slot contracts, and value ranges** for every device in the MWA system. The GUI panels are built against these contracts. When real hardware drivers replace mock controllers, **zero GUI changes** are needed.

## Architecture Overview

```
┌──────────────────────────────────────────────────────────────┐
│                         GUI Layer                            │
│  ┌───────────┐ ┌───────────┐ ┌───────────┐ ┌──────────────┐ │
│  │ LED Panel │ │Pump Panel │ │ SigGen    │ │ VNA Panel    │ │
│  │           │ │           │ │ Panel     │ │              │ │
│  └─────┬─────┘ └─────┬─────┘ └─────┬─────┘ └──────┬───────┘ │
│  ┌─────┴─────┐ ┌─────┴──────────────┴─────┐ ┌─────┴───────┐ │
│  │Camera Pan.│ │    Stage Panel            │ │Status Dashb.│ │
│  └─────┬─────┘ └─────┬───────────────────┘ └──────┬───────┘ │
│        │              │                            │         │
└────────┼──────────────┼────────────────────────────┼─────────┘
         │   Qt Signals/Slots (thread-safe)          │
┌────────┼──────────────┼────────────────────────────┼─────────┐
│        │      Hardware Abstraction Layer            │         │
│  ┌─────▼─────┐ ┌─────▼─────┐ ┌───────────┐ ┌──────▼───────┐ │
│  │LedCtrl    │ │StageCtrl  │ │PumpCtrl   │ │DeviceManager │ │
│  │Interface  │ │Interface  │ │Interface  │ │  (singleton) │ │
│  └─────┬─────┘ └─────┬─────┘ └─────┬─────┘ └──────────────┘ │
│  ┌─────┴─────┐ ┌─────┴─────┐ ┌─────┴─────┐                  │
│  │SigGenCtrl │ │VNACtrl    │ │CameraCtrl │                  │
│  │Interface  │ │Interface  │ │Interface  │                  │
│  └───────────┘ └───────────┘ └───────────┘                  │
└──────────────────────────────────────────────────────────────┘
```

**Rule:** GUI panels hold a pointer to the *interface* (e.g., `LedControllerInterface*`), never to a concrete implementation. Panels connect to interface signals and call interface slots. This is what makes the GUI hardware-agnostic.

---

## Interface Type Contract (Standard I/O Principle)

The interfaces are the **type firewall** between vendor SDKs and the rest of the application. This guarantee must be maintained by every driver implementation and by every consumer of hardware data (GUI, Analysis tool, session recording).

### The Rule

**No vendor-specific type may cross the interface boundary.**

Every method parameter and return type on every `*Interface` class must be a Qt primitive or Qt class. Vendor SDK types (Pylon, QmixSDK, SCPI transport objects, etc.) are permitted only inside the concrete driver `.cpp` files.

### Type Table

| Device | Input types (caller → driver) | Output types (driver → caller) |
|--------|-------------------------------|-------------------------------|
| LED | `bool`, `double` | `bool`, `double` |
| Pump | `double` (µL, µL/min) | `double`, `bool` |
| Signal Generator | `double` (Hz, V), `Waveform` enum | `double`, `bool` |
| Network Analyzer | `double` (Hz), `int` | `QVector<double>`, `bool` |
| Camera | `double` (ms, gain), `QRect` | `QImage`, `double`, `bool` |
| Stage | `double` (mm, mm/s) | `double`, `bool` |

All enums (`Waveform`, `DeviceState`, `DeviceType`) are defined in the interface headers — never in vendor headers.

### Unit Conversion Responsibility

All unit conversions between MWA units and vendor SDK units are the **driver's responsibility**, performed inside the concrete `.cpp` file before any cross-boundary call.

| Driver | MWA unit | SDK unit | Conversion |
|--------|----------|----------|------------|
| BaslerCameraController | ms (exposure) | µs | `us = ms × 1000` |
| BaslerCameraController | gain multiplier | dB | `dB = 20 × log10(x)` |
| PumpController | µL, µL/min | SDK counts | per syringe calibration |
| NetworkAnalyzerController | Hz | Hz (SCPI) | none |

Callers (GUI, Analysis, session model) always work in MWA units and never need to know the SDK unit.

### Analysis Tool and Session Recording

The Analysis tool and `ExperimentSession` model must follow the same rule — they consume data in Qt types, never in vendor types.

**Correct — session stores Qt types:**
```cpp
struct ExperimentSession {
  QVector<QImage>   camera_frames;    // from frameReady(QImage)
  QVector<double>   vna_frequencies;  // from traceFrequencies()
  QVector<double>   vna_magnitudes;   // from traceMagnitudes()
  QVector<double>   pump_positions;   // from positionChanged(double)
};
```

**Wrong — vendor type escapes the driver:**
```cpp
struct ExperimentSession {
  Pylon::CGrabResultPtr last_grab;  // NEVER — Pylon type outside driver
};
```

This ensures that swapping one hardware vendor for another (e.g., Basler → Teledyne camera) requires changes only inside the concrete driver class. The GUI, Analysis tool, and session model are unaffected.

---

## 1. LED Light Source (DEV-LED)

### Real Hardware
- **Device:** Thorlabs DC2200 / DC4100
- **Protocol:** SCPI over USB-TMC (NI-VISA) or virtual serial port
- **Key commands:** `OUTP ON/OFF`, `SOUR:BRIG <0-100>`, `SOUR:BRIG?`, `*IDN?`

### Data Contract (LedControllerInterface)

#### GUI → Hardware (User Actions)

| Action | Method | Parameter Type | Range | Unit |
|--------|--------|---------------|-------|------|
| Turn on/off | `setPowerOn(bool)` | `bool` | true/false | — |
| Set brightness | `setIntensity(double)` | `double` | 0.0 – 100.0 | percent |
| Connect | `connectDevice()` | — | — | — |
| Disconnect | `disconnectDevice()` | — | — | — |

#### Hardware → GUI (Device Feedback)

| Event | Signal | Parameter Type | Example Value |
|-------|--------|---------------|---------------|
| Power changed | `powerStateChanged(bool)` | `bool` | `true` |
| Intensity changed | `intensityChanged(double)` | `double` | `75.0` |
| State changed | `stateChanged(DeviceState)` | `DeviceState` enum | `kConnected` |
| Error | `errorOccurred(QString)` | `QString` | `"Timeout on intensity set"` |

#### GUI Panel Widgets

| Widget | Type | Bound To | Notes |
|--------|------|----------|-------|
| Power toggle | `QPushButton` (checkable) | `setPowerOn()` / `powerStateChanged()` | Green=on, gray=off |
| Intensity slider | `QSlider` (0–100) | `setIntensity()` / `intensityChanged()` | Horizontal, + spinbox |
| Intensity spinbox | `QDoubleSpinBox` | Linked to slider | Range 0.0–100.0, step 0.1 |
| Status indicator | `QLabel` with colored dot | `stateChanged()` | Green/yellow/red/gray |
| Port selector | `QComboBox` | Serial port list | Populated from QSerialPortInfo |

#### Mock Test Data

```
Connect → stateChanged(kConnecting) → stateChanged(kConnected)
setPowerOn(true) → powerStateChanged(true)
setIntensity(75.0) → intensityChanged(75.0)
setPowerOn(false) → powerStateChanged(false)
Disconnect → stateChanged(kDisconnected)
```

---

## 2. Syringe Pump (DEV-PUMP)

### Real Hardware
- **Device:** Cetoni Nemesys
- **Protocol:** Cetoni QmixSDK C API (compiled, not text-based)
- **Key functions:** `LCP_Dispense()`, `LCP_Aspirate()`, `LCP_GenerateFlow()`, `LCP_GetFillLevel()`
- **Units (SDK):** Configurable — typically µL for volume, µL/min for flow rate

### Data Contract (PumpControllerInterface)

#### GUI → Hardware (User Actions)

| Action | Method | Parameter Type | Range | Unit |
|--------|--------|---------------|-------|------|
| Set flow rate | `setFlowRate(double)` | `double` | 0.001 – 100.0 | µL/min |
| Set target volume | `setTargetVolume(double)` | `double` | 0.0 – 50000.0 | µL |
| Start infusion | `startInfusion()` | — | — | — |
| Stop infusion | `stopInfusion()` | — | — | — |
| Refill syringe | `refill()` | — | — | — |
| Connect | `connectDevice()` | — | — | — |
| Disconnect | `disconnectDevice()` | — | — | — |

#### Hardware → GUI (Device Feedback)

| Event | Signal | Parameter Type | Example Value |
|-------|--------|---------------|---------------|
| Flow rate confirmed | `flowRateChanged(double)` | `double` | `5.0` |
| Syringe position | `positionChanged(double)` | `double` | `123.4` (µL dispensed) |
| Infusion started | `infusionStarted()` | — | — |
| Infusion stopped | `infusionStopped()` | — | — |
| State changed | `stateChanged(DeviceState)` | `DeviceState` | `kConnected` |
| Error | `errorOccurred(QString)` | `QString` | `"Syringe empty"` |

#### GUI Panel Widgets

| Widget | Type | Bound To | Notes |
|--------|------|----------|-------|
| Flow rate input | `QDoubleSpinBox` | `setFlowRate()` / `flowRateChanged()` | Range 0.001–100.0, suffix " µL/min" |
| Volume input | `QDoubleSpinBox` | `setTargetVolume()` | Range 0.0–50000.0, suffix " µL" |
| Start button | `QPushButton` | `startInfusion()` | Disabled while infusing |
| Stop button | `QPushButton` | `stopInfusion()` | Disabled while idle |
| Refill button | `QPushButton` | `refill()` | Disabled while infusing |
| Progress bar | `QProgressBar` | `positionChanged()` | Shows dispensed/target ratio |
| Status indicator | `QLabel` | `stateChanged()` | Connected/Infusing/Idle/Error |
| Flow rate display | `QLabel` | `flowRateChanged()` | Read-only current rate |

#### Mock Test Data

```
Connect → stateChanged(kConnected)
setFlowRate(5.0) → flowRateChanged(5.0)
setTargetVolume(100.0)
startInfusion() → infusionStarted()
  → positionChanged(0.0)   (t=0s)
  → positionChanged(25.0)  (t=5min)
  → positionChanged(50.0)  (t=10min)
  → positionChanged(75.0)  (t=15min)
  → positionChanged(100.0) (t=20min)
  → infusionStopped()      (target reached)
```

---

## 3. Frequency / Signal Generator (DEV-SIGGEN)

### Real Hardware
- **Devices:** Rigol DG1062Z, Keysight 33500B, Tektronix AFG1022, etc.
- **Protocol:** SCPI over USB-TMC, LAN (TCP:5025), or serial
- **Key commands:** `SOUR:FREQ`, `SOUR:VOLT:AMPL`, `SOUR:FUNC`, `OUTP ON/OFF`

### Data Contract (SignalGeneratorControllerInterface)

#### GUI → Hardware (User Actions)

| Action | Method | Parameter Type | Range | Unit |
|--------|--------|---------------|-------|------|
| Set frequency | `setFrequency(double)` | `double` | 0.1 – 120,000,000.0 | Hz |
| Set amplitude | `setAmplitude(double)` | `double` | 0.01 – 20.0 | Vpp |
| Set waveform | `setWaveform(Waveform)` | `Waveform` enum | kSine, kSquare, kTriangle | — |
| Enable output | `setOutputEnabled(bool)` | `bool` | true/false | — |
| Configure sweep | `configureSweep(double, double, double)` | `double` × 3 | start_hz, stop_hz, step_hz | Hz |
| Connect | `connectDevice()` | — | — | — |
| Disconnect | `disconnectDevice()` | — | — | — |

#### Hardware → GUI (Device Feedback)

| Event | Signal | Parameter Type | Example Value |
|-------|--------|---------------|---------------|
| Frequency confirmed | `frequencyChanged(double)` | `double` | `1000000.0` |
| Amplitude confirmed | `amplitudeChanged(double)` | `double` | `2.5` |
| Waveform confirmed | `waveformChanged(Waveform)` | `Waveform` | `kSine` |
| Output state | `outputStateChanged(bool)` | `bool` | `true` |
| State changed | `stateChanged(DeviceState)` | `DeviceState` | `kConnected` |
| Error | `errorOccurred(QString)` | `QString` | `"Frequency out of range"` |

#### GUI Panel Widgets

| Widget | Type | Bound To | Notes |
|--------|------|----------|-------|
| Frequency input | `QDoubleSpinBox` | `setFrequency()` / `frequencyChanged()` | Suffix " Hz", adaptive step (1/10/100/1k/10k) |
| Frequency unit selector | `QComboBox` | Multiplier for spinbox | Hz, kHz, MHz |
| Amplitude input | `QDoubleSpinBox` | `setAmplitude()` / `amplitudeChanged()` | Range 0.01–20.0, suffix " Vpp" |
| Waveform selector | `QComboBox` | `setWaveform()` / `waveformChanged()` | Sine, Square, Triangle |
| Output toggle | `QPushButton` (checkable) | `setOutputEnabled()` / `outputStateChanged()` | Red=active, gray=off |
| Sweep start | `QDoubleSpinBox` | `configureSweep()` | Hz |
| Sweep stop | `QDoubleSpinBox` | `configureSweep()` | Hz |
| Sweep step | `QDoubleSpinBox` | `configureSweep()` | Hz |
| Status indicator | `QLabel` | `stateChanged()` | Connection state |

#### Mock Test Data

```
Connect → stateChanged(kConnected)
setFrequency(1000000.0) → frequencyChanged(1000000.0)
setAmplitude(2.5) → amplitudeChanged(2.5)
setWaveform(kSine) → waveformChanged(kSine)
setOutputEnabled(true) → outputStateChanged(true)
configureSweep(500000, 2000000, 10000)
```

---

## 4. Vector Network Analyzer (DEV-VNA)

### Real Hardware
- **Devices:** Keysight E5080A, R&S ZNB/ZVA, miniVNA
- **Protocol:** SCPI over USB-TMC, LAN (TCP:5025), or GPIB
- **Key commands:** `SENS:FREQ:STAR/STOP`, `SENS:SWE:POIN`, `CALC:DATA? FDATA/SDATA`

### Data Contract (NetworkAnalyzerControllerInterface)

#### GUI → Hardware (User Actions)

| Action | Method | Parameter Type | Range | Unit |
|--------|--------|---------------|-------|------|
| Set freq range | `setFrequencyRange(double, double)` | `double` × 2 | 100,000 – 8,500,000,000 | Hz |
| Set num points | `setNumPoints(int)` | `int` | 2 – 32001 | — |
| Start measurement | `measureSParameters()` | — | — | — |
| Connect | `connectDevice()` | — | — | — |
| Disconnect | `disconnectDevice()` | — | — | — |

#### Hardware → GUI (Device Feedback)

| Event | Signal | Parameter Type | Example Value |
|-------|--------|---------------|---------------|
| Measurement started | `measurementStarted()` | — | — |
| Measurement complete | `measurementComplete()` | — | — |
| Freq range changed | `frequencyRangeChanged(double, double)` | `double` × 2 | `1e9, 5e9` |
| Num points changed | `numPointsChanged(int)` | `int` | `201` |
| State changed | `stateChanged(DeviceState)` | `DeviceState` | `kConnected` |
| Error | `errorOccurred(QString)` | `QString` | `"Measurement timeout"` |

#### Data Retrieval (after measurementComplete)

| Method | Return Type | Description |
|--------|-------------|-------------|
| `traceFrequencies()` | `QVector<double>` | Frequency values for each sweep point (Hz) |
| `traceMagnitudes()` | `QVector<double>` | S-parameter magnitude at each point (dB) |

#### GUI Panel Widgets

| Widget | Type | Bound To | Notes |
|--------|------|----------|-------|
| Start freq | `QDoubleSpinBox` | `setFrequencyRange()` | Suffix " Hz", with unit selector |
| Stop freq | `QDoubleSpinBox` | `setFrequencyRange()` | Suffix " Hz", with unit selector |
| Freq unit selector | `QComboBox` | Multiplier | kHz, MHz, GHz |
| Num points | `QSpinBox` | `setNumPoints()` | Range 2–32001, default 201 |
| Measure button | `QPushButton` | `measureSParameters()` | Disabled while measuring |
| S-param plot | `QChartView` (QtCharts) | `traceFrequencies()` + `traceMagnitudes()` | X=freq, Y=dB |
| Status indicator | `QLabel` | `stateChanged()` | Connection + measurement state |
| Progress | `QLabel` | `measurementStarted/Complete()` | "Measuring..." / "Ready" |

#### Mock Test Data

```
setFrequencyRange(1e9, 5e9) → frequencyRangeChanged(1e9, 5e9)
setNumPoints(201) → numPointsChanged(201)
measureSParameters() → measurementStarted()
  (simulate 2-second sweep)
  → measurementComplete()

traceFrequencies() → [1.0e9, 1.02e9, 1.04e9, ..., 5.0e9]  (201 values)
traceMagnitudes()  → [-45.2, -42.1, -39.8, ..., -38.5]      (201 values, dB)

// Mock generates a realistic S21 curve: a resonance dip near 2 MHz
// resembling PZT transducer response
```

---

## 5. Camera (DEV-CAM)

### Real Hardware
- **Device:** Basler area-scan camera (ace/ace2 series)
- **Protocol:** Basler Pylon C++ SDK (GenICam, not serial)
- **Key classes:** `CInstantCamera`, `CGrabResultPtr`
- **Unit conversions:** MWA exposure in ms → Pylon in µs (×1000); MWA gain linear → Pylon gain in dB

### Data Contract (CameraControllerInterface)

#### GUI → Hardware (User Actions)

| Action | Method | Parameter Type | Range | Unit |
|--------|--------|---------------|-------|------|
| Set exposure | `setExposure(double)` | `double` | 0.01 – 10000.0 | ms |
| Set gain | `setGain(double)` | `double` | 0.0 – 24.0 | dB |
| Set ROI | `setRoi(QRect)` | `QRect` | x,y: 0–max; w,h: 1–max | pixels |
| Grab single | `grabSingle()` | — | — | — |
| Start continuous | `startContinuousCapture()` | — | — | — |
| Stop capture | `stopCapture()` | — | — | — |
| Start batch | `startBatchCapture(int, int)` | `int` × 2 | count: 1–10000, interval: 0–60000 | count, ms |
| Connect | `connectDevice()` | — | — | — |
| Disconnect | `disconnectDevice()` | — | — | — |

#### Hardware → GUI (Device Feedback)

| Event | Signal | Parameter Type | Example Value |
|-------|--------|---------------|---------------|
| Frame ready | `frameReady(QImage)` | `QImage` | 640×480 grayscale image |
| Batch complete | `batchComplete()` | — | — |
| Exposure changed | `exposureChanged(double)` | `double` | `25.0` (ms) |
| Gain changed | `gainChanged(double)` | `double` | `6.0` (dB) |
| State changed | `stateChanged(DeviceState)` | `DeviceState` | `kConnected` |
| Error | `errorOccurred(QString)` | `QString` | `"Grab timeout"` |

#### GUI Panel Widgets

| Widget | Type | Bound To | Notes |
|--------|------|----------|-------|
| Live preview | `QLabel` (with QPixmap) | `frameReady()` | Scaled to fit, aspect ratio preserved |
| Exposure input | `QDoubleSpinBox` | `setExposure()` / `exposureChanged()` | Range 0.01–10000, suffix " ms" |
| Gain input | `QDoubleSpinBox` | `setGain()` / `gainChanged()` | Range 0.0–24.0, suffix " dB" |
| ROI X | `QSpinBox` | `setRoi()` | 0 to sensor width |
| ROI Y | `QSpinBox` | `setRoi()` | 0 to sensor height |
| ROI Width | `QSpinBox` | `setRoi()` | 1 to sensor width |
| ROI Height | `QSpinBox` | `setRoi()` | 1 to sensor height |
| Grab single btn | `QPushButton` | `grabSingle()` | Disabled while capturing |
| Continuous btn | `QPushButton` (checkable) | `startContinuousCapture()` / `stopCapture()` | Toggle |
| Batch count | `QSpinBox` | `startBatchCapture()` | Range 1–10000 |
| Batch interval | `QSpinBox` | `startBatchCapture()` | Range 0–60000, suffix " ms" |
| Batch start btn | `QPushButton` | `startBatchCapture()` | |
| Batch progress | `QProgressBar` | Count frames via `frameReady()` | 0 to batch count |
| Status indicator | `QLabel` | `stateChanged()` | Connection state |

#### Mock Test Data

```
Connect → stateChanged(kConnected)
setExposure(25.0) → exposureChanged(25.0)
setGain(6.0) → gainChanged(6.0)
setRoi(QRect(0, 0, 640, 480))
grabSingle() → frameReady(QImage(640, 480, Format_Grayscale8))
startContinuousCapture()
  → frameReady(frame1)  (every ~40ms for 25fps)
  → frameReady(frame2)
  → ...
stopCapture()
startBatchCapture(10, 500)
  → frameReady(frame1) ... frameReady(frame10) → batchComplete()
```

Mock frames: synthetic gradient or noise pattern, 640×480 Grayscale8.

---

## 6. Motorized XYZ Stage (DEV-STAGE)

### Real Hardware
- **Devices:** Zaber X-series, ASI MS-2000, Prior ProScan III
- **Protocols:** Zaber ASCII (`/1 move abs <steps>`), ASI (`M X=<pos>`, `W X Y Z`), Prior ProScan
- **Unit conversions:** MWA uses mm; Zaber uses microsteps; ASI uses 0.1µm units (×10000 for mm)

### Data Contract (StageControllerInterface)

#### GUI → Hardware (User Actions)

| Action | Method | Parameter Type | Range | Unit |
|--------|--------|---------------|-------|------|
| Home all axes | `home()` | — | — | — |
| Move absolute | `moveAbsolute(double, double, double)` | `double` × 3 | -100.0 – 100.0 | mm |
| Move relative | `moveRelative(double, double, double)` | `double` × 3 | -100.0 – 100.0 | mm |
| Stop motion | `stopMotion()` | — | — | — |
| Set speed | `setSpeed(double)` | `double` | 0.001 – 10.0 | mm/s |
| Connect | `connectDevice()` | — | — | — |
| Disconnect | `disconnectDevice()` | — | — | — |

#### Hardware → GUI (Device Feedback)

| Event | Signal | Parameter Type | Example Value |
|-------|--------|---------------|---------------|
| Position update | `positionChanged(double, double, double)` | `double` × 3 | `12.5, 8.3, 0.15` (mm) |
| Home complete | `homeComplete()` | — | — |
| Move complete | `moveComplete()` | — | — |
| Speed changed | `speedChanged(double)` | `double` | `2.0` (mm/s) |
| State changed | `stateChanged(DeviceState)` | `DeviceState` | `kConnected` |
| Error | `errorOccurred(QString)` | `QString` | `"Axis X limit reached"` |

#### GUI Panel Widgets

| Widget | Type | Bound To | Notes |
|--------|------|----------|-------|
| X position display | `QLabel` or `QLCDNumber` | `positionChanged()` | 3 decimal places, suffix " mm" |
| Y position display | `QLabel` or `QLCDNumber` | `positionChanged()` | Same |
| Z position display | `QLabel` or `QLCDNumber` | `positionChanged()` | Same |
| X target input | `QDoubleSpinBox` | `moveAbsolute()` | Range -100.0–100.0 |
| Y target input | `QDoubleSpinBox` | `moveAbsolute()` | Same |
| Z target input | `QDoubleSpinBox` | `moveAbsolute()` | Same |
| Go button | `QPushButton` | `moveAbsolute()` | Reads X/Y/Z inputs |
| Jog buttons (±X,±Y,±Z) | 6 × `QPushButton` | `moveRelative()` | Step size from spinbox |
| Jog step size | `QDoubleSpinBox` | Used by jog buttons | Default 0.1 mm |
| Home button | `QPushButton` | `home()` | Disabled while moving |
| Stop button | `QPushButton` | `stopMotion()` | Always enabled, red |
| Speed input | `QDoubleSpinBox` | `setSpeed()` / `speedChanged()` | Range 0.001–10.0, suffix " mm/s" |
| Moving indicator | `QLabel` | `isMoving()` | "Moving..." or "Idle" |
| Status indicator | `QLabel` | `stateChanged()` | Connection state |

#### Mock Test Data

```
Connect → stateChanged(kConnected)
home() → positionChanged(0.0, 0.0, 0.0) → homeComplete()
setSpeed(2.0) → speedChanged(2.0)
moveAbsolute(10.0, 5.0, 1.0)
  → positionChanged(2.0, 1.0, 0.2)   (interpolated)
  → positionChanged(5.0, 2.5, 0.5)
  → positionChanged(10.0, 5.0, 1.0)  → moveComplete()
moveRelative(-1.0, 0.0, 0.0)
  → positionChanged(9.0, 5.0, 1.0) → moveComplete()
stopMotion() → (immediate stop at current position)
```

---

## 7. Device Status Dashboard (GUI-REQ-007)

The dashboard aggregates state from **all** devices via `DeviceManager`.

### Data Sources

| Source | Signal | Data |
|--------|--------|------|
| DeviceManager | `deviceRegistered(DeviceType)` | New device available |
| DeviceManager | `deviceRemoved(DeviceType)` | Device removed |
| DeviceManager | `deviceStateChanged(DeviceType, DeviceState)` | Connection state change |
| DeviceManager | `deviceError(DeviceType, QString)` | Error from any device |

### Dashboard Widgets

| Widget | Type | Description |
|--------|------|-------------|
| Device list | `QTableWidget` or `QListWidget` | One row per registered device |
| Status column | Color-coded `QLabel` per row | Green=connected, Yellow=connecting, Red=error, Gray=disconnected |
| Name column | `QLabel` | Device name from `deviceName()` |
| Connect All button | `QPushButton` | Calls `DeviceManager::connectAll()` |
| Disconnect All button | `QPushButton` | Calls `DeviceManager::disconnectAll()` |

---

## 8. Logging Panel (GUI-REQ-009)

### Data Source

| Source | Signal | Data |
|--------|--------|------|
| Logger | `newLogEntry(LogEntry)` | timestamp, severity, source, message |

### LogEntry Struct (already defined)

```cpp
struct LogEntry {
  QDateTime timestamp;
  LogSeverity severity;  // kDebug, kInfo, kWarning, kError
  QString source;        // "LED", "Pump", "Stage", etc.
  QString message;
};
```

### Panel Widgets

| Widget | Type | Description |
|--------|------|-------------|
| Log view | `QTableView` + `QStandardItemModel` | Scrolling log with columns: Time, Severity, Source, Message |
| Severity filter | `QComboBox` or checkboxes | Filter by severity level |
| Source filter | `QComboBox` | Filter by device source |
| Clear button | `QPushButton` | Clear log entries |
| Auto-scroll toggle | `QCheckBox` | Auto-scroll to latest entry |
| Export button | `QPushButton` | Save log to file |

---

## 9. Settings Persistence (GUI-REQ-010)

### Settings Groups and Keys

| Group | Key | Type | Default | Description |
|-------|-----|------|---------|-------------|
| `LED` | `port_name` | `QString` | `""` | Last used serial port |
| `LED` | `baud_rate` | `int` | `115200` | Baud rate |
| `LED` | `intensity` | `double` | `50.0` | Last intensity |
| `Pump` | `port_name` | `QString` | `""` | Last used port |
| `Pump` | `flow_rate` | `double` | `1.0` | Last flow rate (µL/min) |
| `Pump` | `target_volume` | `double` | `100.0` | Last target volume (µL) |
| `SigGen` | `port_name` | `QString` | `""` | Last used port |
| `SigGen` | `frequency` | `double` | `1000000.0` | Last frequency (Hz) |
| `SigGen` | `amplitude` | `double` | `1.0` | Last amplitude (Vpp) |
| `SigGen` | `waveform` | `int` | `0` | kSine=0, kSquare=1, kTriangle=2 |
| `VNA` | `port_name` | `QString` | `""` | Last used port |
| `VNA` | `start_freq` | `double` | `1e6` | Start frequency (Hz) |
| `VNA` | `stop_freq` | `double` | `1e9` | Stop frequency (Hz) |
| `VNA` | `num_points` | `int` | `201` | Sweep points |
| `Camera` | `exposure` | `double` | `25.0` | Exposure (ms) |
| `Camera` | `gain` | `double` | `0.0` | Gain (dB) |
| `Camera` | `roi_x` | `int` | `0` | ROI X offset |
| `Camera` | `roi_y` | `int` | `0` | ROI Y offset |
| `Camera` | `roi_w` | `int` | `640` | ROI width |
| `Camera` | `roi_h` | `int` | `480` | ROI height |
| `Stage` | `port_name` | `QString` | `""` | Last used port |
| `Stage` | `speed` | `double` | `1.0` | Speed (mm/s) |
| `Stage` | `jog_step` | `double` | `0.1` | Jog step size (mm) |
| `MainWindow` | `geometry` | `QByteArray` | — | Window size/position |
| `MainWindow` | `state` | `QByteArray` | — | Dock widget layout |
| `Log` | `severity_filter` | `int` | `0` | Min severity to display |
| `Log` | `auto_scroll` | `bool` | `true` | Auto-scroll enabled |

---

## 10. Mock Controller Strategy

Each device gets a `Mock<Device>Controller` that:
1. **Implements the same interface** as the real controller
2. **Simulates realistic timing** (delays, async responses)
3. **Produces realistic data** (mock frames, S-parameter curves, position interpolation)
4. **Validates parameter ranges** (same as real hardware would)
5. **Can inject errors** for error-handling testing

### Mock Classes Needed

| Mock Class | Extends | Key Simulation |
|------------|---------|----------------|
| `MockLedController` | `LedControllerInterface` | Instant on/off, intensity echo |
| `MockPumpController` | `PumpControllerInterface` | Timed dispensing, fill level decrement |
| `MockSignalGeneratorController` | `SignalGeneratorControllerInterface` | Parameter echo, sweep timer |
| `MockNetworkAnalyzerController` | `NetworkAnalyzerControllerInterface` | Generate synthetic S21 curve with resonance |
| `MockCameraController` | `CameraControllerInterface` | Generate gradient/noise QImages |
| `MockStageController` | `StageControllerInterface` | Interpolated position updates |

---

## 11. Unit Conversion Reference

When real hardware is integrated, these conversions happen **inside the concrete controller**, not in the GUI.

| Device | MWA Interface Unit | Real Hardware Unit | Conversion |
|--------|-------------------|-------------------|------------|
| Camera exposure | ms | µs (Pylon) | ×1000 |
| Camera gain | dB | dB (Pylon) | none (already dB) |
| LED intensity | percent (0–100) | percent or amps | depends on model |
| Pump flow rate | µL/min | µL/min (QmixSDK) | none (SDK configurable) |
| Pump volume | µL | µL (QmixSDK) | none |
| SigGen frequency | Hz | Hz (SCPI) | none |
| SigGen amplitude | Vpp | Vpp (SCPI) | none |
| VNA frequency | Hz | Hz (SCPI) | none |
| VNA magnitude | dB | dB (SCPI FDATA) | none |
| Stage position | mm | microsteps (Zaber) or 0.1µm (ASI) | Zaber: ÷steps_per_mm; ASI: ÷10000 |
| Stage speed | mm/s | device-specific | Zaber: ÷steps_per_mm; ASI: direct |
