# MWA Sprint Plan

**Created by:** Lead
**Date:** 2026-03-22

---

## Sprint Overview

| Sprint | Focus | Phase | Status |
|--------|-------|-------|--------|
| Sprint 1 | Development Environment Setup | 0 | COMPLETE |
| Sprint 2 | Core Architecture & Main Window | 1 | COMPLETE |
| Sprint 3 | Device Control Panels (GUI) | 1 | COMPLETE |
| Sprint 4 | Hardware Abstraction Layer | 2 | COMPLETE |
| Sprint 5 | Hardware Device Drivers (LED, Stage) | 2 | COMPLETE |
| Sprint 6 | Analysis Module Foundation | 3 | PLANNED |
| Sprint 7 | SCPI Protocol Infrastructure | 2 | PLANNED |
| Sprint 8 | Syringe Pump — Cetoni Nemesys Research & Mock | 2 | PLANNED |
| Sprint 9 | Signal Generator — SCPI Research & Mock | 2 | PLANNED |
| Sprint 10 | Network Analyzer — SCPI VNA Research & Mock | 2 | PLANNED |
| Sprint 11 | Camera — Basler Pylon Research & Mock | 2 | PLANNED |

---

## Sprint 1: Development Environment Setup

**Status:** COMPLETE
**Task file:** `MWA-01-dev-environment.md`

| Task | Description | Status |
|------|-------------|--------|
| MWA-01-A | CMake project foundation | DONE |
| MWA-01-B | CI/CD pipeline (matrix) | DONE |
| MWA-01-C | Directory structure | DONE |
| MWA-01-D | .clang-format | DONE |
| MWA-01-E | .clang-tidy | DONE |
| MWA-01-F | Qt Test skeleton | DONE |
| MWA-01-G | Doxyfile | DONE |

---

## Sprint 2: Core Architecture & Main Window

**Goal:** Establish the application shell — main window, logging, settings, and core abstractions that all subsequent features plug into.
**Branch:** `MWA-02-Core-Architecture`
**Depends on:** Sprint 1

### MWA-02-A: Core Logger

```
TASK: MWA-02-A
REQUIREMENT: GUI-REQ-009, NFR-004
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Create a singleton Logger class in src/core/ that wraps Qt message handling.

  - src/core/logger.h / logger.cpp
  - Singleton accessed via Logger::instance()
  - Methods: logInfo(), logWarning(), logError(), logDebug()
  - Each log entry stores: timestamp, severity, source, message
  - Emits a Qt signal newLogEntry(const LogEntry&) so GUI can subscribe
  - LogEntry struct: timestamp (QDateTime), severity (enum), source (QString),
    message (QString)
  - Thread-safe (QMutex)
  - Installs a custom Qt message handler (qInstallMessageHandler) so qDebug/
    qWarning/qCritical route through the logger
  - Doxygen documentation mandatory

ACCEPTANCE CRITERIA:
  - Logger::instance().logInfo("test") compiles and works
  - Signal newLogEntry emitted on every log call
  - Thread-safe (can be called from worker threads)
  - All public APIs have Doxygen comments
  - Builds with zero warnings
DEPENDENCIES: MWA-01-A
PRIORITY: HIGH
```

### MWA-02-B: Settings Manager

```
TASK: MWA-02-B
REQUIREMENT: GUI-REQ-010, NFR-004
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Create a SettingsManager class in src/core/ wrapping QSettings.

  - src/core/settings_manager.h / settings_manager.cpp
  - Singleton accessed via SettingsManager::instance()
  - Groups settings by device: "LED", "Pump", "Camera", etc.
  - Methods: setValue(group, key, QVariant), value(group, key, default),
    saveAll(), loadAll()
  - Uses QSettings with INI format for cross-platform consistency
  - Emits settingChanged(group, key, value) signal
  - Doxygen documentation mandatory

ACCEPTANCE CRITERIA:
  - Can save/restore values across app restarts
  - Groups isolate device settings
  - Signal emitted on change
  - All public APIs have Doxygen comments
  - Builds with zero warnings
DEPENDENCIES: MWA-01-A
PRIORITY: HIGH
```

### MWA-02-C: Device Interface (Abstract Base)

```
TASK: MWA-02-C
REQUIREMENT: HW-REQ-001, NFR-004, NFR-007
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Define the abstract device interface in src/hardware/.

  - src/hardware/device_interface.h
  - Pure virtual class DeviceInterface : public QObject
  - Enum DeviceState { kDisconnected, kConnecting, kConnected, kError }
  - Enum DeviceType { kLed, kPump, kSignalGenerator, kNetworkAnalyzer,
    kCamera, kStage }
  - Pure virtual methods: connect(), disconnect(), deviceName(),
    deviceType(), state(), isConnected()
  - Signals: stateChanged(DeviceState), errorOccurred(QString)
  - Doxygen documentation mandatory on every method, enum, and enum value

ACCEPTANCE CRITERIA:
  - DeviceInterface is a pure abstract class (cannot be instantiated)
  - All enums use kCamelCase values per project convention
  - Signals declared for state changes and errors
  - All public APIs have Doxygen comments
  - Builds with zero warnings
DEPENDENCIES: MWA-01-A
PRIORITY: HIGH
```

### MWA-02-D: Main Window Shell

```
TASK: MWA-02-D
REQUIREMENT: GUI-REQ-001, NFR-005
ASSIGNED TO: SWE
UX REQUIRED: YES — UX must approve layout BEFORE SWE implements
STATUS: PLANNED
DESCRIPTION:
  Create the main application window in src/app/.

  - src/app/main_window.h / main_window.cpp
  - Class MainWindow : public QMainWindow
  - Menu bar with: File (Exit), View (toggle panels), Help (About)
  - Toolbar with placeholder icons
  - Central widget: QStackedWidget or QSplitter as workspace area
  - Status bar showing "Ready" by default
  - Left dock area: reserved for device panels (empty for now)
  - Bottom dock area: reserved for log panel (empty for now)
  - Update src/main.cpp to create and show MainWindow
  - Window title: "MWA — Microfluidics Workstation Automation"
  - Minimum size: 1024x768
  - Doxygen documentation mandatory

ACCEPTANCE CRITERIA:
  - Application launches and shows the main window
  - Menu bar, toolbar, status bar visible
  - Window title correct
  - Dock areas exist (left and bottom)
  - All public APIs have Doxygen comments
  - Builds with zero warnings on both platforms
DEPENDENCIES: MWA-02-A, MWA-02-B
PRIORITY: HIGH
```

### MWA-02-E: Log Panel Widget

```
TASK: MWA-02-E
REQUIREMENT: GUI-REQ-009
ASSIGNED TO: SWE
UX REQUIRED: YES
STATUS: PLANNED
DESCRIPTION:
  Create a log panel widget in src/gui/widgets/ that displays log entries.

  - src/gui/widgets/log_panel.h / log_panel.cpp
  - Class LogPanel : public QDockWidget
  - Contains a QTableView or QListView displaying log entries
  - Columns: Timestamp, Severity (with color), Source, Message
  - Connects to Logger::instance().newLogEntry signal
  - Filter by severity (combo box or checkboxes)
  - Clear button
  - Auto-scroll to latest entry
  - Integrate into MainWindow bottom dock
  - Doxygen documentation mandatory

ACCEPTANCE CRITERIA:
  - Log entries appear in real-time
  - Severity filtering works
  - Clear button works
  - Auto-scroll works
  - Doxygen complete
  - Zero warnings
DEPENDENCIES: MWA-02-A, MWA-02-D
PRIORITY: MEDIUM
```

### MWA-02-F: Device Status Dashboard

```
TASK: MWA-02-F
REQUIREMENT: GUI-REQ-007
ASSIGNED TO: SWE
UX REQUIRED: YES
STATUS: PLANNED
DESCRIPTION:
  Create a device status dashboard widget in src/gui/widgets/.

  - src/gui/widgets/device_status_dashboard.h / .cpp
  - Class DeviceStatusDashboard : public QWidget
  - Displays a row/grid of device status indicators
  - Each device shows: icon, name, state (colored indicator)
  - States: Disconnected (gray), Connecting (yellow), Connected (green),
    Error (red)
  - Observes DeviceInterface::stateChanged signals
  - Initially shows all 6 device types as "Disconnected"
  - Integrate into MainWindow (toolbar area or top of central widget)
  - Doxygen documentation mandatory

ACCEPTANCE CRITERIA:
  - All 6 device types shown with correct names
  - Color-coded state indicators
  - Doxygen complete
  - Zero warnings
DEPENDENCIES: MWA-02-C, MWA-02-D
PRIORITY: MEDIUM
```

**Dependency graph:**
```
MWA-02-A (Logger)         MWA-02-B (Settings)    MWA-02-C (DeviceInterface)
   │                         │                       │
   ├─────────────────────────┤                       │
   │                         │                       │
   └──→ MWA-02-D (MainWindow) ←─────────────────────┘
            │                                        │
            ├──→ MWA-02-E (LogPanel)                 │
            └──→ MWA-02-F (DeviceStatusDashboard) ←──┘
```

**Execution order:**
- Round 1 (parallel): MWA-02-A, MWA-02-B, MWA-02-C
- Round 2: MWA-02-D (needs UX approval first)
- Round 3 (parallel): MWA-02-E, MWA-02-F

---

## Sprint 3: Device Control Panels (GUI)

**Goal:** Build the GUI panels for each hardware device using mock controllers. No real hardware yet.
**Branch:** `MWA-03-Device-Panels`
**Depends on:** Sprint 2

### MWA-03-A: Mock Device Controllers

```
TASK: MWA-03-A
REQUIREMENT: HW-REQ-001, NFR-004
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Create mock implementations of DeviceInterface for all 6 device types.
  These allow GUI development without real hardware.

  For each device (LED, Pump, SignalGenerator, NetworkAnalyzer, Camera, Stage):
  - src/hardware/<device>/mock_<device>_controller.h / .cpp
  - Implements DeviceInterface
  - connect() sets state to kConnected after a simulated delay (QTimer 500ms)
  - disconnect() sets state to kDisconnected
  - Device-specific mock methods return sensible defaults
  - Emits stateChanged signals appropriately
  - Doxygen documentation mandatory

ACCEPTANCE CRITERIA:
  - All 6 mock controllers compile and implement DeviceInterface
  - connect()/disconnect() cycle works with signals
  - Builds with zero warnings
  - All public APIs documented
DEPENDENCIES: MWA-02-C
PRIORITY: HIGH
```

### MWA-03-B: LED Control Panel

```
TASK: MWA-03-B
REQUIREMENT: GUI-REQ-002
ASSIGNED TO: SWE
UX REQUIRED: YES
STATUS: PLANNED
DESCRIPTION:
  Create LED control panel in src/gui/panels/.

  - src/gui/panels/led_panel.h / .cpp
  - Class LedPanel : public QDockWidget
  - Controls: On/Off toggle button, intensity slider (0-100%), status indicator
  - Connects to DeviceInterface (accepts any LED controller)
  - Displays current state from mock controller
  - Doxygen documentation mandatory

ACCEPTANCE CRITERIA:
  - Toggle switches LED on/off via controller
  - Slider adjusts intensity
  - Status indicator reflects device state
  - Works with MockLedController
  - Doxygen complete, zero warnings
DEPENDENCIES: MWA-03-A, MWA-02-D
PRIORITY: HIGH
```

### MWA-03-C: Syringe Pump Control Panel

```
TASK: MWA-03-C
REQUIREMENT: GUI-REQ-003
ASSIGNED TO: SWE
UX REQUIRED: YES
STATUS: PLANNED
DESCRIPTION:
  Create syringe pump control panel in src/gui/panels/.

  - src/gui/panels/pump_panel.h / .cpp
  - Controls: flow rate input (QDoubleSpinBox), volume input, start/stop
    buttons, refill button, current position display
  - Units: µL/min for flow rate, µL for volume
  - Connects to DeviceInterface
  - Doxygen documentation mandatory

ACCEPTANCE CRITERIA:
  - Flow rate and volume inputs validated
  - Start/stop/refill buttons functional with mock
  - Current state displayed
  - Doxygen complete, zero warnings
DEPENDENCIES: MWA-03-A, MWA-02-D
PRIORITY: HIGH
```

### MWA-03-D: Signal Generator / Network Analyzer Panel

```
TASK: MWA-03-D
REQUIREMENT: GUI-REQ-004
ASSIGNED TO: SWE
UX REQUIRED: YES
STATUS: PLANNED
DESCRIPTION:
  Create frequency/signal control panel in src/gui/panels/.

  - src/gui/panels/signal_panel.h / .cpp
  - Controls: frequency input (Hz/kHz/MHz), amplitude input, waveform selector
    (sine/square/triangle), enable/disable output, sweep config (start/stop/step)
  - S-parameter display area (placeholder QLabel or QChartView)
  - Connects to DeviceInterface
  - Doxygen documentation mandatory

ACCEPTANCE CRITERIA:
  - Frequency/amplitude inputs with unit selectors
  - Waveform dropdown
  - Output enable/disable
  - Works with mock controller
  - Doxygen complete, zero warnings
DEPENDENCIES: MWA-03-A, MWA-02-D
PRIORITY: HIGH
```

### MWA-03-E: Camera Control Panel

```
TASK: MWA-03-E
REQUIREMENT: GUI-REQ-005
ASSIGNED TO: SWE
UX REQUIRED: YES
STATUS: PLANNED
DESCRIPTION:
  Create camera control panel in src/gui/panels/.

  - src/gui/panels/camera_panel.h / .cpp
  - Controls: exposure (QDoubleSpinBox), gain, ROI settings, single capture
    button, continuous capture toggle, batch capture (count + interval)
  - Image preview area (QLabel with scaled QPixmap placeholder)
  - Connects to DeviceInterface
  - Doxygen documentation mandatory

ACCEPTANCE CRITERIA:
  - Parameter inputs validated
  - Capture buttons functional with mock
  - Preview area shows placeholder image
  - Doxygen complete, zero warnings
DEPENDENCIES: MWA-03-A, MWA-02-D
PRIORITY: HIGH
```

### MWA-03-F: XYZ Stage Control Panel

```
TASK: MWA-03-F
REQUIREMENT: GUI-REQ-006
ASSIGNED TO: SWE
UX REQUIRED: YES
STATUS: PLANNED
DESCRIPTION:
  Create XYZ stage control panel in src/gui/panels/.

  - src/gui/panels/stage_panel.h / .cpp
  - Controls: jog buttons (X+/X-/Y+/Y-/Z+/Z-), step size selector,
    absolute position inputs (X/Y/Z), Go To button, Home button,
    current position display (read-only)
  - Speed control (QSlider or QSpinBox)
  - Connects to DeviceInterface
  - Doxygen documentation mandatory

ACCEPTANCE CRITERIA:
  - Jog buttons send relative moves via controller
  - Absolute positioning works
  - Home button functional
  - Current position updates from mock
  - Doxygen complete, zero warnings
DEPENDENCIES: MWA-03-A, MWA-02-D
PRIORITY: HIGH
```

**Dependency graph:**
```
MWA-03-A (Mock Controllers)
   │
   ├──→ MWA-03-B (LED Panel)
   ├──→ MWA-03-C (Pump Panel)
   ├──→ MWA-03-D (Signal Panel)
   ├──→ MWA-03-E (Camera Panel)
   └──→ MWA-03-F (Stage Panel)
```

**Execution order:**
- Round 1: MWA-03-A (mock controllers)
- Round 2 (parallel): MWA-03-B, C, D, E, F (all panels — each needs UX first)

---

## Sprint 4: Hardware Abstraction Layer

**Goal:** Build the real communication infrastructure — thread-safe command queuing, error recovery, device discovery.
**Branch:** `MWA-04-Hardware-Abstraction`
**Depends on:** Sprint 2

### MWA-04-A: Command Queue System

```
TASK: MWA-04-A
REQUIREMENT: HW-REQ-009, NFR-001
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Create a thread-safe command queue for device communication.

  - src/hardware/command_queue.h / .cpp
  - Class CommandQueue : public QObject
  - Runs on a dedicated QThread per device
  - Methods: enqueue(std::function<void()>), clear(), shutdown()
  - Signals: commandStarted(), commandFinished(), commandFailed(QString)
  - FIFO ordering, one command at a time per device
  - Timeout support per command
  - Doxygen documentation mandatory

ACCEPTANCE CRITERIA:
  - Commands execute on worker thread, not GUI thread
  - Thread-safe enqueue from any thread
  - Timeout triggers error signal
  - Clean shutdown without deadlocks
  - Doxygen complete, zero warnings
DEPENDENCIES: MWA-02-C
PRIORITY: HIGH
```

### MWA-04-B: Device Manager

```
TASK: MWA-04-B
REQUIREMENT: HW-REQ-008, HW-REQ-011, NFR-006
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Create a DeviceManager that owns and coordinates all device controllers.

  - src/hardware/device_manager.h / .cpp
  - Singleton: DeviceManager::instance()
  - Owns all DeviceInterface instances (real or mock)
  - Methods: registerDevice(), removeDevice(), device(DeviceType),
    allDevices(), connectAll(), disconnectAll()
  - Signals: deviceRegistered, deviceRemoved, deviceStateChanged
  - Platform check: disable devices whose SDKs are unavailable
  - Doxygen documentation mandatory

ACCEPTANCE CRITERIA:
  - Can register and retrieve devices by type
  - connectAll()/disconnectAll() work
  - Platform-unavailable devices reported
  - Doxygen complete, zero warnings
DEPENDENCIES: MWA-02-C, MWA-04-A
PRIORITY: HIGH
```

### MWA-04-C: Error Recovery Framework

```
TASK: MWA-04-C
REQUIREMENT: HW-REQ-010, NFR-006
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Create error recovery utilities for device communication.

  - src/hardware/error_handler.h / .cpp
  - Retry logic with configurable attempts and backoff
  - Timeout handling with QTimer
  - Disconnection detection and auto-reconnect option
  - Emits signals for GUI notification
  - Integrates with Logger
  - Doxygen documentation mandatory

ACCEPTANCE CRITERIA:
  - Retry logic configurable (count, delay)
  - Timeout triggers error path
  - Disconnection detected and reported
  - Logs all errors via Logger
  - Doxygen complete, zero warnings
DEPENDENCIES: MWA-02-A, MWA-04-A
PRIORITY: MEDIUM
```

**Execution order:**
- Round 1: MWA-04-A
- Round 2 (parallel): MWA-04-B, MWA-04-C

---

## Sprint 5: Hardware Device Drivers

**Goal:** Implement real device controllers using vendor SDKs. Each driver is independent.
**Branch:** `MWA-05-Device-Drivers`
**Depends on:** Sprint 4

### MWA-05-A: LED Driver (Serial)

```
TASK: MWA-05-A
REQUIREMENT: HW-REQ-002
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Implement real LED controller using QSerialPort.

  - src/hardware/led/led_controller.h / .cpp
  - Implements DeviceInterface
  - Uses QSerialPort for USB/serial communication
  - Commands: power on/off, set intensity (0-100%)
  - Uses CommandQueue for thread-safe I/O
  - Doxygen documentation mandatory

DEPENDENCIES: MWA-04-A, MWA-04-B
PRIORITY: HIGH
```

### MWA-05-B: Syringe Pump Driver (Cetoni SDK)

```
TASK: MWA-05-B
REQUIREMENT: HW-REQ-003
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Implement syringe pump controller using Cetoni Nemesys SDK.

  - src/hardware/pump/pump_controller.h / .cpp
  - Wraps Cetoni SDK calls behind DeviceInterface
  - Platform-conditional: only available if SDK is found by CMake
  - Uses CommandQueue for thread-safe I/O
  - Doxygen documentation mandatory

DEPENDENCIES: MWA-04-A, MWA-04-B
PRIORITY: HIGH
```

### MWA-05-C: Frequency Generator Driver (SCPI)

```
TASK: MWA-05-C
REQUIREMENT: HW-REQ-004
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Implement frequency generator controller using SCPI over serial/VISA.

  - src/hardware/signal_generator/signal_generator_controller.h / .cpp
  - SCPI command set for frequency, amplitude, waveform, sweep
  - Uses QSerialPort or NI-VISA wrapper
  - Doxygen documentation mandatory

DEPENDENCIES: MWA-04-A, MWA-04-B
PRIORITY: HIGH
```

### MWA-05-D: Network Analyzer Driver (SCPI)

```
TASK: MWA-05-D
REQUIREMENT: HW-REQ-005
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Implement network analyzer controller using SCPI over VISA.

  - src/hardware/signal_generator/network_analyzer_controller.h / .cpp
  - S-parameter measurement, frequency sweep, trace data retrieval
  - Doxygen documentation mandatory

DEPENDENCIES: MWA-04-A, MWA-04-B
PRIORITY: HIGH
```

### MWA-05-E: Camera Driver (Basler Pylon)

```
TASK: MWA-05-E
REQUIREMENT: HW-REQ-006
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Implement camera controller using Basler Pylon C++ SDK.

  - src/hardware/camera/camera_controller.h / .cpp
  - Frame acquisition (single, continuous, batch)
  - Parameter control (exposure, gain, ROI)
  - Converts Pylon images to QImage for GUI display
  - Platform-conditional: only if Pylon SDK found
  - Doxygen documentation mandatory

DEPENDENCIES: MWA-04-A, MWA-04-B
PRIORITY: HIGH
```

### MWA-05-F: XYZ Stage Driver (Serial)

```
TASK: MWA-05-F
REQUIREMENT: HW-REQ-007
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Implement XYZ stage controller using serial protocol.

  - src/hardware/stage/stage_controller.h / .cpp
  - Absolute/relative positioning, homing, speed control
  - Uses QSerialPort
  - Doxygen documentation mandatory

DEPENDENCIES: MWA-04-A, MWA-04-B
PRIORITY: HIGH
```

**Execution order:**
- All parallel: MWA-05-A through MWA-05-F (independent drivers)

---

## Sprint 6: Analysis Module Foundation

**Goal:** Build the image analysis pipeline foundation.
**Branch:** `MWA-06-Analysis`
**Depends on:** Sprint 3 (camera panel), Sprint 5 (camera driver)

### MWA-06-A: Image Batch Manager

```
TASK: MWA-06-A
REQUIREMENT: ANA-REQ-001
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Create image batch loading and management in src/analysis/.

  - src/analysis/image_batch.h / .cpp
  - Load, organize, and navigate batches of images
  - Support common formats (PNG, TIFF, BMP)
  - Thumbnail generation
  - Doxygen documentation mandatory

DEPENDENCIES: MWA-03-E
PRIORITY: HIGH
```

### MWA-06-B: Particle Detection

```
TASK: MWA-06-B
REQUIREMENT: ANA-REQ-002
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Implement particle detection algorithms in src/analysis/.

  - src/analysis/particle_detector.h / .cpp
  - Detect particles in microscopy images
  - Output: list of particle positions and sizes
  - May use OpenCV if available, or custom implementation
  - Doxygen documentation mandatory

DEPENDENCIES: MWA-06-A
PRIORITY: HIGH
```

### MWA-06-C: Data Export

```
TASK: MWA-06-C
REQUIREMENT: ANA-REQ-007
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Implement analysis result export in src/analysis/.

  - src/analysis/data_exporter.h / .cpp
  - Export to CSV, annotated images, summary reports
  - Configurable output directory
  - Doxygen documentation mandatory

DEPENDENCIES: MWA-06-B
PRIORITY: MEDIUM
```

**Execution order:**
- Round 1: MWA-06-A
- Round 2: MWA-06-B
- Round 3: MWA-06-C

---

## Agent Parallelism Guide

For running agents in parallel, use these groupings:

| Sprint | Parallel Group | Tasks |
|--------|---------------|-------|
| 1 | Remaining | MWA-01-E |
| 2 | Round 1 | MWA-02-A, MWA-02-B, MWA-02-C |
| 2 | Round 2 | MWA-02-D (needs UX) |
| 2 | Round 3 | MWA-02-E, MWA-02-F |
| 3 | Round 1 | MWA-03-A |
| 3 | Round 2 | MWA-03-B, C, D, E, F (all need UX) |
| 4 | Round 1 | MWA-04-A |
| 4 | Round 2 | MWA-04-B, MWA-04-C |
| 5 | Round 1 | MWA-05-A, B, C, D, E, F (all parallel) |
| 6 | Round 1 | MWA-06-A |
| 6 | Round 2 | MWA-06-B |
| 6 | Round 3 | MWA-06-C |
| 7 | Round 1 | MWA-07-A (SCPI research) |
| 7 | Round 2 | MWA-07-B (ScpiClient class) |
| 8 | Round 1 | MWA-08-A (Cetoni research) |
| 8 | Round 2 | MWA-08-B (enhanced mock) |
| 8 | Round 3 | MWA-08-C (driver skeleton) |
| 9 | Round 1 | MWA-09-A (SigGen SCPI research) |
| 9 | Round 2 | MWA-09-B (enhanced mock) |
| 9 | Round 3 | MWA-09-C (driver skeleton) |
| 10 | Round 1 | MWA-10-A (VNA SCPI research) |
| 10 | Round 2 | MWA-10-B (enhanced mock, S-param data) |
| 10 | Round 3 | MWA-10-C (driver skeleton) |
| 11 | Round 1 | MWA-11-A (Pylon SDK research) |
| 11 | Round 2 | MWA-11-B (enhanced mock, synthetic frames) |
| 11 | Round 3 | MWA-11-C (driver skeleton) |

> **Note:** Sprints 8–11 each depend on Sprint 7 for SCPI-based devices (9 & 10).
> Sprint 8 (Pump) and Sprint 11 (Camera) are independent of Sprint 7 and can run in parallel.

---

## Sprint 7: SCPI Protocol Infrastructure

**Goal:** Build a reusable SCPI command layer that Signal Generator (Sprint 9) and Network Analyzer (Sprint 10) drivers share. Research the SCPI standard and common instrument command sets. No GUI changes.
**Branch:** `MWA-07-SCPI-Protocol`
**Depends on:** Sprint 4 (CommandQueue, serial_utils)

### Context: What is SCPI?

SCPI (Standard Commands for Programmable Instruments) is an IEEE 488.2-based command language used by virtually all modern test instruments. Commands are ASCII strings sent over VISA (USB-TMC, GPIB, TCP/IP) or serial ports. The grammar has two layers:
- **IEEE 488.2 common commands**: `*IDN?`, `*RST`, `*OPC?`, `*ESR?`, `*CLS` — supported by every SCPI instrument
- **SCPI subsystem commands**: hierarchical tree like `SOURce:FREQuency`, `SENSe:SWEep:POINts`, etc.

The key insight is that although every instrument vendor implements slightly different subsystem trees, the transport layer (send command → read response → parse errors) is identical. A reusable `ScpiClient` class eliminates duplicated code in every SCPI-based driver.

### MWA-07-A: SCPI Protocol Research & Specification

```
TASK: MWA-07-A
REQUIREMENT: HW-REQ-004, HW-REQ-005
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Research the SCPI protocol standard and create a comprehensive protocol
  specification document at docs/protocols/scpi_protocol_spec.md.

  Research scope:
  1. IEEE 488.2 common command set (*IDN?, *RST, *OPC?, *ESR?, *CLS, *WAI)
  2. SCPI command syntax: keyword hierarchy, short/long form, query suffix (?),
     numeric suffixes, parameter types (NR1/NR2/NR3, boolean, string, block data)
  3. SCPI error handling: SYST:ERR? queue, status byte, SRQ mechanism
  4. Transport layers: USB-TMC (NI-VISA), TCP/IP socket (port 5025), serial
     (RS-232, typically 9600-115200 baud, LF or CR+LF terminator)
  5. Command response format: definite/indefinite block data (#<digits><length>),
     ASCII numeric responses, comma-separated lists
  6. Common SCPI subsystem commands used by signal generators:
     - SOURce:FREQuency[:CW] <freq>
     - SOURce:VOLTage:AMPLitude <volts>
     - SOURce:FUNCtion:SHAPe {SINusoid|SQUare|TRIangle}
     - OUTPut[:STATe] {ON|OFF}
     - SOURce:SWEep:... (sweep configuration)
  7. Common SCPI subsystem commands used by VNAs:
     - SENSe:FREQuency:STARt / STOP
     - SENSe:SWEep:POINts
     - CALCulate:DATA? FDATA / SDATA
     - INITiate[:IMMediate]
     - Trigger model: INITiate → trigger → sweep → data ready
  8. NI-VISA C API overview: viOpen(), viWrite(), viRead(), viClose(),
     resource strings (USB::0x1234::0x5678::INSTR, TCPIP::192.168.1.1::INSTR)

  Document structure:
  - docs/protocols/scpi_protocol_spec.md
  - Sections: Standard overview, Command syntax, Error model, Transport options,
    Signal Generator command reference, VNA command reference, NI-VISA integration,
    Cross-platform notes (NI-VISA on macOS vs Windows)

ACCEPTANCE CRITERIA:
  - Protocol specification document created at docs/protocols/scpi_protocol_spec.md
  - Covers both IEEE 488.2 common commands and device-specific SCPI trees
  - Documents all three transport options (VISA, TCP, serial)
  - Includes command/response examples for both SigGen and VNA use cases
  - Documents error handling flow (SYST:ERR? polling)
  - Documents NI-VISA C API functions needed
  - Cross-platform considerations noted (NI-VISA availability on macOS)
DEPENDENCIES: none (research task)
PRIORITY: HIGH
```

### MWA-07-B: ScpiClient Reusable Class

```
TASK: MWA-07-B
REQUIREMENT: HW-REQ-004, HW-REQ-005, NFR-004
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Create a reusable ScpiClient class in src/hardware/scpi/ that handles the
  SCPI transport and command formatting layer. Both the Signal Generator and
  Network Analyzer drivers will use this class.

  Files:
  - src/hardware/scpi/scpi_client.h
  - src/hardware/scpi/scpi_client.cpp
  - src/hardware/scpi/scpi_transport.h (abstract transport interface)
  - src/hardware/scpi/serial_transport.h / .cpp (QSerialPort implementation)

  ScpiTransport (abstract base):
  - virtual bool open(const QString& resource) = 0
  - virtual void close() = 0
  - virtual bool isOpen() const = 0
  - virtual bool write(const QByteArray& data) = 0
  - virtual QByteArray read(int timeout_ms) = 0

  SerialTransport : ScpiTransport:
  - Wraps QSerialPort for serial/USB-serial instruments
  - Configurable baud rate, terminator (LF vs CR+LF)
  - Uses blocking I/O (called from CommandQueue worker thread)

  ScpiClient:
  - Takes ownership of a ScpiTransport
  - QString query(const QString& cmd, int timeout_ms)
      → sends cmd + terminator, reads response, trims
  - bool command(const QString& cmd, int timeout_ms)
      → sends cmd, waits for *OPC? = "1" (or no response for serial)
  - QString identify() → sends "*IDN?", returns identity string
  - void reset() → sends "*RST"
  - QString lastError() → sends "SYST:ERR?", returns error string
  - bool checkErrors() → polls SYST:ERR? until "0,No error"
  - QVector<double> queryDoubleList(const QString& cmd, int timeout_ms)
      → parses comma-separated numeric response into QVector<double>
  - QByteArray queryBlockData(const QString& cmd, int timeout_ms)
      → reads IEEE 488.2 definite-length block data (#<d><n><data>)

  Design principles:
  - ScpiClient is NOT a QObject — it's a plain utility class used from
    within CommandQueue commands (worker thread)
  - Thread-safe by design: one ScpiClient per device, used only from
    the owning CommandQueue's worker thread
  - No VISA dependency in this task — VISA transport can be added later
    as a second ScpiTransport implementation
  - Doxygen documentation mandatory

ACCEPTANCE CRITERIA:
  - ScpiClient compiles and links
  - SerialTransport opens/closes QSerialPort correctly
  - query() sends command and reads response
  - queryDoubleList() parses "1.0,2.0,3.0\n" into QVector<double>{1.0,2.0,3.0}
  - queryBlockData() parses "#3100<100 bytes>" format
  - identify() returns *IDN? response
  - checkErrors() polls SYST:ERR? until clean
  - All public APIs have Doxygen comments
  - Builds with zero warnings on both platforms
DEPENDENCIES: MWA-07-A (protocol spec informs API design)
PRIORITY: HIGH
```

**Dependency graph:**
```
MWA-07-A (SCPI Research)
   │
   └──→ MWA-07-B (ScpiClient class)
            │
            ├──→ Sprint 9 (Signal Generator)
            └──→ Sprint 10 (Network Analyzer)
```

---

## Sprint 8: Syringe Pump — Cetoni Nemesys Research & Mock

**Goal:** Research the Cetoni Nemesys SDK, create a protocol specification, enhance the mock pump controller with realistic timed dispensing and position tracking, and design the real driver header. No GUI changes.
**Branch:** `MWA-08-Pump-Nemesys`
**Depends on:** Sprint 4 (CommandQueue)

### Context: Cetoni Nemesys System

The Cetoni Nemesys is a modular syringe pump system. Unlike serial-based instruments, it uses a proprietary compiled C API (QmixSDK). The SDK provides:
- **Device bus**: `LCB_Open()`, `LCB_Start()`, `LCB_Stop()` — manages the CAN bus to pump modules
- **Pump control**: `LCP_Aspirate()`, `LCP_Dispense()`, `LCP_GenerateFlow()`, `LCP_StopPumping()`
- **Status queries**: `LCP_GetFillLevel()`, `LCP_IsCalibrationFinished()`, `LCP_GetFlowIs()`
- **Syringe config**: `LCP_SetSyringeParam()` — sets inner diameter and stroke
- **Units**: Configurable via `LCP_SetVolumeUnit()` and `LCP_SetFlowUnit()`

The SDK is Windows-primary. macOS/Linux support via the newer Cetoni SDK (Python-based) is uncertain for the C API. This makes platform-conditional compilation essential.

### MWA-08-A: Cetoni Nemesys SDK Research & Protocol Specification

```
TASK: MWA-08-A
REQUIREMENT: HW-REQ-003
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Research the Cetoni Nemesys/QmixSDK and create a protocol specification
  document at docs/protocols/cetoni_nemesys_spec.md.

  Research scope:
  1. QmixSDK C API (qmixsdk.h / lcp.h / lcb.h):
     - Bus management: LCB_Open(), LCB_Start(), LCB_Stop(), LCB_Close()
     - Device lookup: LCB_GetDeviceCount(), LCB_GetDevice()
     - Pump initialization: LCP_SetSyringeParam(inner_diameter_mm, stroke_mm),
       LCP_SetVolumeUnit(), LCP_SetFlowUnit()
     - Pump operations: LCP_Aspirate(volume, flow_rate), LCP_Dispense(volume,
       flow_rate), LCP_GenerateFlow(flow_rate), LCP_StopPumping()
     - Status queries: LCP_GetFillLevel(), LCP_GetFlowIs(), LCP_IsPumping(),
       LCP_GetDosedVolume(), LCP_IsCalibrationFinished()
     - Valve control: LCP_SwitchValveToPosition()
     - Calibration: LCP_SyringeCalibration()
  2. SDK error model: return codes, error strings via LCB_GetLastError()
  3. Threading model: SDK thread safety, callback mechanism
  4. Physical units and conversions:
     - Volume: nL, µL, mL (SDK enum)
     - Flow rate: nL/s, µL/s, µL/min, mL/min (SDK enum)
     - Our interface uses µL and µL/min
  5. Syringe specifications:
     - Typical syringes: 100 µL, 250 µL, 500 µL, 1 mL, 5 mL, 25 mL
     - Inner diameter and stroke for each size
     - Maximum flow rate depends on syringe size
  6. Platform availability:
     - Windows: Full QmixSDK C API available
     - macOS: Check if Cetoni SDK (newer Python/C++ hybrid) supports macOS
     - Fallback: platform-conditional compilation, disabled on unsupported platforms
  7. Typical workflow:
     - LCB_Open(config_path) → LCB_Start() → LCP_SetSyringeParam() →
       LCP_SetVolumeUnit() → LCP_SetFlowUnit() → ready for operations
     - Dispensing: LCP_Dispense(target_volume, flow_rate) → poll LCP_IsPumping()
       + LCP_GetDosedVolume() → LCP_StopPumping() when done
     - Refill: LCP_Aspirate(max_volume, refill_flow_rate)
  8. SDK installation and CMake integration:
     - Find script for QmixSDK (FindQmixSDK.cmake)
     - Library files to link
     - Header paths

  Document structure:
  - docs/protocols/cetoni_nemesys_spec.md
  - Sections: SDK overview, API reference (key functions), Error model,
    Units & conversions, Syringe specifications, Typical workflows,
    CMake integration, Platform notes, Mock data patterns

  Sample data to document (for mock enhancement):
  - Typical dispensing curve: position vs time for 100 µL at 5 µL/min
  - Refill timing: how long to aspirate 100 µL at max flow rate
  - Error scenarios: syringe empty, mechanical stall, bus disconnection

ACCEPTANCE CRITERIA:
  - Protocol spec document created at docs/protocols/cetoni_nemesys_spec.md
  - All key QmixSDK C API functions documented with signatures and descriptions
  - Unit conversion table complete (SDK units ↔ MWA interface units)
  - Syringe specification table with at least 4 common syringe sizes
  - Dispensing/refill workflow diagrams or step-by-step sequences
  - Error scenarios documented
  - CMake integration approach documented (FindQmixSDK.cmake design)
  - Platform availability notes for Windows and macOS
DEPENDENCIES: none (research task)
PRIORITY: HIGH
```

### MWA-08-B: Enhanced Mock Pump Controller

```
TASK: MWA-08-B
REQUIREMENT: HW-REQ-003, NFR-004
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Enhance MockPumpController to simulate realistic syringe pump behavior
  based on the Cetoni Nemesys protocol specification (MWA-08-A).

  Current mock limitations:
  - startInfusion() just sets a flag — no timed dispensing
  - No position updates during infusion (positionChanged never emitted
    periodically)
  - refill() instantly resets position — no aspiration timing
  - No syringe capacity concept
  - No error injection capability
  - No parameter validation (flow rate limits depend on syringe size)

  Enhanced behavior:
  1. Syringe configuration:
     - Add setSyringeVolume(double uL) method to set syringe capacity
     - Default 250 µL syringe
     - Position tracks fill level (0 = empty, max = full)
     - After connect, syringe starts full (position = syringe_volume)

  2. Timed dispensing simulation:
     - startInfusion() starts a QTimer that ticks every 100ms
     - Each tick: position -= (flow_rate / 60.0) * (tick_interval_s)
       i.e. flow_rate is µL/min, convert to µL per tick
     - Emit positionChanged(position_) on each tick
     - When position <= 0 or dispensed >= target_volume:
       auto-stop, emit infusionStopped()
     - When position <= 0 and target not reached:
       emit errorOccurred("Syringe empty")

  3. Timed refill simulation:
     - refill() starts aspirating at a fixed refill rate (e.g. 50 µL/min)
     - QTimer ticks, position increases until position == syringe_volume
     - Emit positionChanged() on each tick
     - Emit infusionStopped() when refill complete (reuse signal)

  4. Flow rate validation:
     - Reject flow rates < 0.001 or > max_flow_rate (depends on syringe)
     - Max flow rates by syringe size:
       100 µL → 15 µL/min, 250 µL → 40 µL/min,
       500 µL → 75 µL/min, 1000 µL → 150 µL/min
     - Emit errorOccurred() for out-of-range values

  5. Error injection:
     - Add setSimulateError(bool) method for testing
     - When enabled, randomly fail one in 10 commands with
       errorOccurred("Simulated mechanical stall")

  Files to modify:
  - src/hardware/pump/mock_pump_controller.h (add members, methods)
  - src/hardware/pump/mock_pump_controller.cpp (implement timed behavior)

  Doxygen documentation mandatory for all new/modified methods.

ACCEPTANCE CRITERIA:
  - startInfusion() emits periodic positionChanged() updates
  - Position decreases proportionally to flow rate during dispensing
  - Auto-stops when target volume reached or syringe empty
  - refill() takes time proportional to syringe capacity
  - Flow rate validation rejects out-of-range values
  - Error injection mode works when enabled
  - Existing tests still pass (backward compatible)
  - All new public APIs have Doxygen comments
  - Builds with zero warnings
DEPENDENCIES: MWA-08-A (spec informs realistic timing/data)
PRIORITY: HIGH
```

### MWA-08-C: Pump Controller Driver Skeleton

```
TASK: MWA-08-C
REQUIREMENT: HW-REQ-003
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Design the real PumpController header file as a driver skeleton.
  This is a HEADER-ONLY design task — the .cpp implementation will be
  written when hardware is available.

  Files to create:
  - src/hardware/pump/pump_controller.h (fully documented header)
  - cmake/FindQmixSDK.cmake (CMake find module stub)

  PumpController design:
  - Class PumpController : public PumpControllerInterface
  - Uses CommandQueue for thread-safe SDK calls
  - Private members:
    - CommandQueue for async operations
    - Cached state (position, flow rate, infusing flag) under QMutex
    - SDK handle (void* or typed handle from QmixSDK)
    - Syringe parameters (volume, inner_diameter, stroke)
    - Polling timer for position updates during infusion
  - Constructor: takes config_path (QString) for QmixSDK bus config
  - setConfigPath() / configPath() — set before connectDevice()
  - setSyringeParams(double inner_diameter_mm, double stroke_mm)
  - connectDevice():
    - Enqueues: LCB_Open → LCB_Start → LCP_SetSyringeParam →
      LCP_SetVolumeUnit → LCP_SetFlowUnit → kConnected
  - disconnectDevice():
    - Enqueues: LCP_StopPumping → LCB_Stop → LCB_Close → kDisconnected
  - setFlowRate(): enqueues flow rate validation + cache update
  - startInfusion(): enqueues LCP_Dispense, starts position polling timer
  - stopInfusion(): enqueues LCP_StopPumping, stops polling
  - refill(): enqueues LCP_Aspirate with max flow rate
  - Position polling: QTimer triggers enqueue of LCP_GetDosedVolume query

  CMake integration:
  - cmake/FindQmixSDK.cmake:
    - Searches for QmixSDK headers and libraries
    - Sets QmixSDK_FOUND, QmixSDK_INCLUDE_DIRS, QmixSDK_LIBRARIES
    - Windows: search in C:/QmixSDK and Program Files
    - macOS: mark as not found (SDK unavailable)
  - src/hardware/pump/CMakeLists.txt or conditional in parent:
    - if(QmixSDK_FOUND) → add pump_controller.cpp to build
    - else → log message, only mock available

  Doxygen documentation mandatory on every method and member.

ACCEPTANCE CRITERIA:
  - pump_controller.h compiles (no .cpp yet — methods declared, not defined)
  - FindQmixSDK.cmake is syntactically valid
  - Header documents the full Cetoni workflow in class/method Doxygen
  - Platform-conditional logic designed in CMake
  - All public APIs have Doxygen comments
  - Builds with zero warnings (header-only, included in at least one TU)
DEPENDENCIES: MWA-08-A, MWA-08-B
PRIORITY: MEDIUM
```

**Execution order:**
- Round 1: MWA-08-A (research)
- Round 2: MWA-08-B (enhanced mock)
- Round 3: MWA-08-C (driver skeleton)

---

## Sprint 9: Signal Generator — SCPI Research & Mock

**Goal:** Research common signal generator SCPI command sets, enhance the mock with realistic sweep simulation, and design the real driver using ScpiClient. No GUI changes.
**Branch:** `MWA-09-SignalGenerator-SCPI`
**Depends on:** Sprint 7 (ScpiClient)

### Context: SCPI Signal Generators

Common lab signal generators (Rigol DG1062Z, Keysight 33500B, Tektronix AFG1022) all use SCPI over USB-TMC or serial. While each vendor has slight differences in command trees, the core functionality is identical:
- Set frequency, amplitude, waveform shape
- Enable/disable output
- Configure frequency sweeps (linear/log, start/stop/step, dwell time)

The key research is identifying the common SCPI subset that works across vendors, and designing the driver to be configurable for vendor-specific quirks.

### MWA-09-A: Signal Generator SCPI Command Research

```
TASK: MWA-09-A
REQUIREMENT: HW-REQ-004
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Research signal generator SCPI command sets from major vendors and create
  a protocol specification at docs/protocols/signal_generator_scpi_spec.md.

  Research scope:
  1. Rigol DG1062Z SCPI command set:
     - SOURce<n>:FREQuency[:CW] <freq>
     - SOURce<n>:VOLTage[:LEVel][:IMMediate][:AMPLitude] <voltage>
     - SOURce<n>:FUNCtion[:SHAPe] {SINusoid|SQUare|TRIangle|RAMP|...}
     - OUTPut<n>[:STATe] {ON|OFF|1|0}
     - SOURce<n>:SWEep:STATe {ON|OFF}
     - SOURce<n>:FREQuency:STARt / STOP / STEP
     - SOURce<n>:SWEep:TIME <seconds>
     - SOURce<n>:SWEep:SPACing {LINear|LOGarithmic}
  2. Keysight 33500B command set (note differences from Rigol):
     - Same general SCPI tree but with vendor extensions
     - FUNC:ARB for arbitrary waveforms
     - FREQ:MODE {CW|SWEep|LIST}
  3. Tektronix AFG1022 command set:
     - Slightly different tree: SOUR:FREQ:FIX, SOUR:VOLT:AMPL, etc.
  4. Common subset across all three:
     - Identify the "safe" command set that works on all three
     - Document vendor-specific differences as configuration options
  5. Connection and identification:
     - *IDN? response format for each vendor
     - USB-TMC connection (VISA resource string)
     - Serial connection (baud rate, terminator)
     - TCP/IP connection (port 5025)
  6. Parameter ranges (typical):
     - Frequency: 1 µHz – 60 MHz (DG1062Z), 1 µHz – 30 MHz (33500B)
     - Amplitude: 1 mVpp – 20 Vpp (50Ω load)
     - Waveform types: sine, square, triangle, ramp, pulse, noise, DC
  7. Error handling:
     - SYST:ERR? polling
     - Typical error codes: -100 (command error), -200 (execution error)
  8. Sweep execution model:
     - Trigger modes: auto, bus trigger, external
     - Sweep status query (how to know when sweep is done)
     - Dwell time per step

  Document structure:
  - docs/protocols/signal_generator_scpi_spec.md
  - Sections: Supported instruments, Common command reference,
    Vendor differences table, Connection options, Parameter ranges,
    Sweep configuration, Error handling, Mock data patterns

  Sample data for mock (realistic signal generator responses):
  - *IDN? → "RIGOL TECHNOLOGIES,DG1062Z,DG1Z123456789,00.01.12"
  - SOUR:FREQ? → "1.000000E+06" (1 MHz)
  - SOUR:VOLT? → "2.500000E+00" (2.5 Vpp)
  - SOUR:FUNC? → "SIN"
  - OUTP? → "ON" or "OFF"

ACCEPTANCE CRITERIA:
  - Protocol spec document created at docs/protocols/signal_generator_scpi_spec.md
  - At least 3 vendors documented (Rigol, Keysight, Tektronix)
  - Common command subset identified and highlighted
  - Vendor-specific differences documented as configuration table
  - Parameter ranges documented per vendor
  - Sweep configuration fully documented
  - Sample *IDN? strings and query responses for mock use
  - Connection methods (USB-TMC, serial, TCP) documented
DEPENDENCIES: MWA-07-A (general SCPI knowledge)
PRIORITY: HIGH
```

### MWA-09-B: Enhanced Mock Signal Generator Controller

```
TASK: MWA-09-B
REQUIREMENT: HW-REQ-004, NFR-004
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Enhance MockSignalGeneratorController to simulate realistic SCPI-based
  signal generator behavior.

  Current mock limitations:
  - Simple state echo — setFrequency() immediately sets and emits
  - No sweep execution simulation
  - No parameter validation (frequency/amplitude ranges)
  - No command latency simulation (real SCPI has ~10–50ms per command)
  - No *IDN?-like identification response
  - No error injection capability

  Enhanced behavior:
  1. Parameter validation:
     - Frequency range: 0.000001 Hz – 120,000,000 Hz (120 MHz max)
     - Amplitude range: 0.001 V – 20.0 Vpp
     - Reject out-of-range with errorOccurred()

  2. Command latency simulation:
     - Each set* method applies after a 20ms QTimer::singleShot delay
       to simulate SCPI round-trip time
     - This tests GUI responsiveness when commands aren't instant

  3. Sweep execution simulation:
     - Add startSweep() and stopSweep() methods (extend interface if needed,
       or use configureSweep() + setOutputEnabled(true) as trigger)
     - When sweep is active: QTimer steps frequency from start_hz to stop_hz
       by step_hz at ~100ms intervals
     - Emit frequencyChanged() at each sweep step so GUI can track progress
     - Emit a custom sweepComplete() or just stop and emit outputStateChanged()

  4. Instrument identity simulation:
     - Add QString instrumentIdentity() const → returns a fake *IDN? string
       "MOCK INSTRUMENTS,MWA-SG1000,SN001,V1.0"

  5. Output state interaction:
     - Frequency/amplitude changes only take effect when output is enabled
     - Setting frequency while output is off: accepted, queued
     - Enabling output: applies queued settings

  6. Error injection:
     - Add setSimulateError(bool) method
     - When enabled, setFrequency() occasionally fails with
       errorOccurred("Frequency lock failed")

  Files to modify:
  - src/hardware/signal_generator/mock_signal_generator_controller.h
  - src/hardware/signal_generator/mock_signal_generator_controller.cpp

  Doxygen documentation mandatory.

ACCEPTANCE CRITERIA:
  - Parameter validation rejects out-of-range frequency/amplitude
  - Command latency (20ms delay) simulated on all set* methods
  - Sweep mode steps through frequencies with frequencyChanged() emissions
  - instrumentIdentity() returns a mock *IDN? string
  - Error injection mode works when enabled
  - Existing tests still pass (backward compatible)
  - All new public APIs have Doxygen comments
  - Builds with zero warnings
DEPENDENCIES: MWA-09-A (spec informs realistic behavior)
PRIORITY: HIGH
```

### MWA-09-C: Signal Generator Controller Driver Skeleton

```
TASK: MWA-09-C
REQUIREMENT: HW-REQ-004
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Design the real SignalGeneratorController header file using ScpiClient.
  HEADER-ONLY design — .cpp implementation when hardware available.

  Files to create:
  - src/hardware/signal_generator/signal_generator_controller.h

  SignalGeneratorController design:
  - Class SignalGeneratorController : public SignalGeneratorControllerInterface
  - Uses CommandQueue + ScpiClient for thread-safe SCPI I/O
  - Constructor: takes a ScpiTransport* (serial or VISA)
  - Private members:
    - std::unique_ptr<CommandQueue> command_queue_
    - std::unique_ptr<ScpiClient> scpi_client_
    - Cached state under QMutex (frequency, amplitude, waveform, output)
    - Vendor enum { kRigol, kKeysight, kTektronix, kGeneric }
    - Vendor-specific command table (configurable)
  - setPortName() / setBaudRate() — for serial transport
  - setVendor(Vendor) — selects vendor-specific command set
  - connectDevice():
    - Enqueues: open transport → *IDN? → detect vendor → *RST → query
      current settings → kConnected
  - disconnectDevice():
    - Enqueues: OUTPut OFF → close transport → kDisconnected
  - setFrequency():
    - Enqueues: SOURce:FREQuency <freq> → query back → emit
  - setAmplitude():
    - Enqueues: SOURce:VOLTage <volts> → query back → emit
  - setWaveform():
    - Enqueues: SOURce:FUNCtion {SIN|SQU|TRI} → query back → emit
  - setOutputEnabled():
    - Enqueues: OUTPut {ON|OFF} → query back → emit
  - configureSweep():
    - Enqueues: FREQ:START, FREQ:STOP, SWE:STEP → no query needed

  Doxygen documentation mandatory on every method.

ACCEPTANCE CRITERIA:
  - signal_generator_controller.h compiles (no .cpp yet)
  - Header documents full SCPI workflow in Doxygen
  - Vendor-configurable command table designed
  - Uses ScpiClient from Sprint 7
  - All public APIs have Doxygen comments
  - Builds with zero warnings
DEPENDENCIES: MWA-07-B (ScpiClient), MWA-09-A
PRIORITY: MEDIUM
```

**Execution order:**
- Round 1: MWA-09-A (research — can run parallel with MWA-07-A)
- Round 2: MWA-09-B (enhanced mock — needs MWA-09-A)
- Round 3: MWA-09-C (driver skeleton — needs MWA-07-B and MWA-09-A)

---

## Sprint 10: Network Analyzer — SCPI VNA Research & Mock

**Goal:** Research VNA SCPI command sets, enhance the mock with realistic S-parameter resonance data (Lorentzian dip model for PZT characterization), and design the real driver using ScpiClient. No GUI changes.
**Branch:** `MWA-10-VNA-SCPI`
**Depends on:** Sprint 7 (ScpiClient)

### Context: Vector Network Analyzers for PZT Characterization

In microfluidics, VNAs are used to characterize PZT (piezoelectric transducer) resonance behavior. The experiment workflow is:
1. Connect VNA to PZT transducer on the microfluidic chip
2. Sweep frequency range (typically 1–10 MHz for microfluidic PZTs)
3. Measure S11 (reflection) or S21 (transmission) parameters
4. Identify resonance frequencies where S-parameter shows a dip (minimum)
5. Set the signal generator to the identified resonance frequency

The mock must produce realistic S-parameter data that looks like PZT resonance curves — not the current simple sine wave.

### MWA-10-A: VNA SCPI Command Research

```
TASK: MWA-10-A
REQUIREMENT: HW-REQ-005
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Research VNA SCPI command sets from major vendors and create a protocol
  specification at docs/protocols/vna_scpi_spec.md.

  Research scope:
  1. Keysight ENA/PNA series (E5080A, E5063A) SCPI:
     - SENSe:FREQuency:STARt / STOP / CENTer / SPAN
     - SENSe:SWEep:POINts
     - SENSe:SWEep:TIME?
     - CALCulate:PARameter:DEFine <S11|S21|S12|S22>
     - CALCulate:SELected:FORMat {MLOGarithmic|PHASe|SMITh|...}
     - CALCulate:SELected:DATA? FDATA (formatted), SDATA (complex S-param)
     - INITiate[:IMMediate]
     - TRIGger:SEQuence:SOURce {BUS|IMMediate|EXTernal}
     - STATus:OPERation:CONDition? (check if sweep in progress)
  2. Rohde & Schwarz ZNB/ZVA series SCPI:
     - Same general SCPI tree with R&S extensions
     - SWE:COUN (number of sweeps)
     - CALC:DATA? FDAT, SDAT
  3. miniVNA / nanoVNA (budget instruments):
     - Often non-SCPI, custom USB protocols
     - Document as "not directly supported — needs custom driver"
  4. Common VNA workflow:
     - Preset instrument → set frequency range → set num points →
       select measurement parameter (S11 or S21) → initiate sweep →
       wait for completion → retrieve data
  5. Data formats:
     - FDATA: formatted data (magnitude in dB, one value per point)
     - SDATA: complex data (real,imag pairs, two values per point)
     - Block transfer: IEEE 488.2 definite-length block for large datasets
     - ASCII transfer: comma-separated values
  6. Typical S-parameter data for PZT characterization:
     - Frequency range: 1 MHz – 10 MHz
     - 201–1001 points
     - S11 shows resonance as a dip (minimum) at ~2–5 MHz
     - Typical S11 dip: -30 to -50 dB at resonance, -5 to -10 dB off-resonance
     - Multiple resonance modes possible (fundamental + harmonics)
     - Lorentzian line shape: S(f) = S_bg - (depth / (1 + ((f-f0)/BW)²))
  7. Calibration:
     - OPEN/SHORT/LOAD standards
     - Not needed for mock, but document for driver implementation

  Sample data (for mock — realistic PZT S11 curve):
  - Frequency: 1 MHz to 10 MHz, 201 points
  - Background: -8 dB (reflection without resonance)
  - Primary resonance: f0 = 3.2 MHz, depth = 35 dB, bandwidth = 50 kHz
  - Secondary resonance: f0 = 6.5 MHz, depth = 18 dB, bandwidth = 80 kHz
  - Noise floor: ±0.5 dB random variation

  Document structure:
  - docs/protocols/vna_scpi_spec.md
  - Sections: Supported instruments, Common command reference,
    Measurement workflow, Data retrieval formats, S-parameter physics
    (Lorentzian model), PZT resonance example data, Mock data generation

ACCEPTANCE CRITERIA:
  - Protocol spec document created at docs/protocols/vna_scpi_spec.md
  - At least 2 VNA vendors documented (Keysight, R&S)
  - Full measurement workflow documented step-by-step
  - Data retrieval formats (FDATA, SDATA, block) fully specified
  - PZT resonance physics explained with Lorentzian model formula
  - Sample S-parameter dataset provided (201 points with realistic dip)
  - Calibration workflow documented (for future driver)
  - miniVNA/nanoVNA limitations noted
DEPENDENCIES: MWA-07-A (general SCPI knowledge)
PRIORITY: HIGH
```

### MWA-10-B: Enhanced Mock Network Analyzer Controller

```
TASK: MWA-10-B
REQUIREMENT: HW-REQ-005, NFR-004
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Enhance MockNetworkAnalyzerController to produce physically realistic
  S-parameter data modelling PZT transducer resonance.

  Current mock limitations:
  - finishMeasurement() generates a simple sine wave for magnitude
    (not physically meaningful — real S-parameters don't look like sine waves)
  - No realistic resonance dips
  - No noise simulation
  - No measurement-time scaling (always 1s regardless of point count)
  - No error injection
  - Not thread-safe (single-threaded mock, but should match real driver pattern)

  Enhanced behavior:
  1. Realistic S-parameter generation using Lorentzian resonance model:
     - Background level: -8 dB (typical reflection off-resonance)
     - Primary resonance: Lorentzian dip at f0 = 3.2 MHz, depth 35 dB,
       bandwidth 50 kHz
     - Secondary resonance: f0 = 6.5 MHz, depth 18 dB, bandwidth 80 kHz
     - Formula per point: S(f) = background - Σ(depth_i / (1 + ((f - f0_i) / bw_i)²))
     - Add Gaussian noise: ±0.5 dB standard deviation
     - Resonance parameters can be configured for different "virtual PZTs"

  2. Configurable resonance model:
     - Add setResonances(QVector<Resonance>) where
       struct Resonance { double freq_hz; double depth_db; double bandwidth_hz; }
     - Default: the PZT example above
     - Allows testing with different resonance patterns

  3. Measurement time scaling:
     - measurement_time_ms = num_points * 5 (5ms per point, like a real VNA)
     - 201 points → ~1s, 1001 points → ~5s
     - Minimum 500ms, maximum 10s (capped for usability)

  4. SDATA (complex) support:
     - Add traceComplexData() returning QVector<std::complex<double>>
     - Generate complex S-parameter from magnitude + phase
     - Phase: 0° off-resonance, rapid phase shift through resonance

  5. Measurement progress:
     - Emit measurementProgress(int percent) during simulated sweep
     - Update every 10% of measurement time

  6. Error injection:
     - setSimulateError(bool) — when enabled, 1 in 5 measurements
       fails with errorOccurred("Measurement timeout: instrument not responding")

  Files to modify:
  - src/hardware/network_analyzer/mock_network_analyzer_controller.h
  - src/hardware/network_analyzer/mock_network_analyzer_controller.cpp

  Doxygen documentation mandatory.

ACCEPTANCE CRITERIA:
  - traceMagnitudes() produces a realistic Lorentzian resonance curve
  - Resonance dips visible at configured frequencies
  - Gaussian noise added to trace data
  - Measurement time scales with number of points
  - Configurable resonance model via setResonances()
  - Error injection mode works when enabled
  - Existing tests still pass (backward compatible — default resonances match
    old test expectations where applicable)
  - All new public APIs have Doxygen comments
  - Builds with zero warnings
DEPENDENCIES: MWA-10-A (spec provides resonance model and sample data)
PRIORITY: HIGH
```

### MWA-10-C: Network Analyzer Controller Driver Skeleton

```
TASK: MWA-10-C
REQUIREMENT: HW-REQ-005
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Design the real NetworkAnalyzerController header file using ScpiClient.
  HEADER-ONLY design — .cpp implementation when hardware available.

  Files to create:
  - src/hardware/network_analyzer/network_analyzer_controller.h

  NetworkAnalyzerController design:
  - Class NetworkAnalyzerController : public NetworkAnalyzerControllerInterface
  - Uses CommandQueue + ScpiClient for thread-safe SCPI I/O
  - Constructor: takes a ScpiTransport* (serial or VISA)
  - Private members:
    - std::unique_ptr<CommandQueue> command_queue_
    - std::unique_ptr<ScpiClient> scpi_client_
    - Cached state under QMutex
    - Vendor enum { kKeysight, kRohdeSchwarz, kGeneric }
    - Measurement parameter: S11 or S21
  - setPortName() / setBaudRate() — for serial transport
  - setVendor(Vendor)
  - setMeasurementParameter(QString param) — "S11", "S21", etc.
  - connectDevice():
    - Enqueues: open transport → *IDN? → detect vendor → *RST →
      set format (MLOG) → kConnected
  - disconnectDevice():
    - Enqueues: close transport → kDisconnected
  - setFrequencyRange():
    - Enqueues: SENS:FREQ:STAR <start> → SENS:FREQ:STOP <stop> → emit
  - setNumPoints():
    - Enqueues: SENS:SWE:POIN <n> → emit
  - measureSParameters():
    - Enqueues: INITiate → poll OPC? → CALC:DATA? FDATA →
      parse double list → store traces → emit measurementComplete()
  - Data retrieval:
    - traceFrequencies() — computed from start/stop/points
    - traceMagnitudes() — from FDATA response

  Doxygen documentation mandatory.

ACCEPTANCE CRITERIA:
  - network_analyzer_controller.h compiles (no .cpp yet)
  - Full VNA measurement workflow documented in Doxygen
  - Vendor-configurable command table designed
  - Uses ScpiClient from Sprint 7
  - All public APIs have Doxygen comments
  - Builds with zero warnings
DEPENDENCIES: MWA-07-B (ScpiClient), MWA-10-A
PRIORITY: MEDIUM
```

**Execution order:**
- Round 1: MWA-10-A (research — can run parallel with MWA-07-A and MWA-09-A)
- Round 2: MWA-10-B (enhanced mock — needs MWA-10-A)
- Round 3: MWA-10-C (driver skeleton — needs MWA-07-B and MWA-10-A)

---

## Sprint 11: Camera — Basler Pylon SDK Research & Mock

**Goal:** Research the Basler Pylon C++ SDK, enhance the mock camera with synthetic microscopy frames (particles, noise, exposure/gain response), and design the real driver header. No GUI changes.
**Branch:** `MWA-11-Camera-Pylon`
**Depends on:** Sprint 4 (CommandQueue)

### Context: Basler Pylon SDK for Microscopy Cameras

Basler Pylon is the industry-standard SDK for Basler area-scan cameras (ace, ace2, dart series) used extensively in microscopy. Unlike SCPI instruments, it's a compiled C++ SDK using GenICam standards. Key characteristics:
- **Transport layers**: USB3 Vision, GigE Vision — SDK abstracts both
- **Grab strategies**: OneByOne, LatestImageOnly, LatestImages
- **Image format**: Mono8, Mono12, BayerBG8, etc. → must convert to QImage
- **Threading**: Camera grab runs on a background thread, callback delivers frames
- **Platform**: Available on Windows, macOS, Linux

The mock camera currently generates a simple checkerboard. For meaningful application testing, it should generate synthetic microscopy-like frames that respond to exposure and gain settings.

### MWA-11-A: Basler Pylon SDK Research & Protocol Specification

```
TASK: MWA-11-A
REQUIREMENT: HW-REQ-006
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Research the Basler Pylon C++ SDK and create a specification document
  at docs/protocols/basler_pylon_spec.md.

  Research scope:
  1. Pylon C++ SDK architecture:
     - PylonInitialize() / PylonTerminate() — global init/shutdown
     - CTlFactory — transport layer factory for device enumeration
     - CInstantCamera — high-level camera class (main API entry point)
     - CGrabResultPtr — smart pointer to a grabbed image buffer
     - CImageEventHandler — callback interface for asynchronous grabs
  2. Device enumeration and connection:
     - CTlFactory::GetInstance().EnumerateDevices(DeviceInfoList_t&)
     - CInstantCamera(CTlFactory::GetInstance().CreateDevice(info))
     - camera.Open() / camera.Close()
     - camera.GetDeviceInfo().GetModelName()
  3. Parameter access (GenICam GenApi):
     - camera.ExposureTime.SetValue(25000.0) — exposure in microseconds
     - camera.Gain.SetValue(6.0) — gain in dB
     - camera.Width.SetValue(640), camera.Height.SetValue(480)
     - camera.OffsetX.SetValue(0), camera.OffsetY.SetValue(0)
     - camera.PixelFormat.SetValue(PixelFormat_Mono8)
     - All parameters accessed via GenApi node map
  4. Image acquisition modes:
     - Single frame: camera.GrabOne(timeout, grabResult)
     - Continuous: camera.StartGrabbing(GrabStrategy_OneByOne) →
       camera.RetrieveResult(timeout, grabResult) in loop →
       camera.StopGrabbing()
     - Event-driven: register CImageEventHandler, calls OnImageGrabbed()
  5. Image format conversion:
     - grabResult->GetWidth(), GetHeight(), GetPixelType(), GetBuffer()
     - CImageFormatConverter — converts Bayer/packed to Mono8/RGB8
     - Mono8 → QImage::Format_Grayscale8
     - RGB8 → QImage::Format_RGB888
  6. Triggered acquisition:
     - Software trigger: camera.TriggerMode.SetValue(TriggerMode_On),
       camera.TriggerSource.SetValue(TriggerSource_Software),
       camera.ExecuteSoftwareTrigger()
     - Hardware trigger: TriggerSource_Line1
  7. Unit conversions (MWA interface ↔ Pylon):
     - Exposure: MWA ms → Pylon µs (×1000)
     - Gain: MWA dB → Pylon dB (no conversion, but check GainAuto mode)
     - ROI: MWA QRect → Pylon Width/Height/OffsetX/OffsetY
  8. Error handling:
     - GenericException — base exception for all Pylon errors
     - TimeoutException — grab timeout
     - grabResult->GrabSucceeded() check
  9. CMake integration:
     - find_package(pylon) or FindPylon.cmake
     - Pylon_INCLUDE_DIRS, Pylon_LIBRARIES
     - Environment variable PYLON_ROOT
  10. Performance considerations:
     - Memory allocation: grab engine pre-allocates buffers
     - MaxNumBuffer — number of buffers in grab queue
     - Frame rate depends on exposure time and transport bandwidth

  Sample data (for mock — typical microscopy camera):
  - Sensor: 2448×2048 (Basler ace acA2440-75µm)
  - Pixel format: Mono8 (8-bit grayscale)
  - Exposure range: 10 µs – 10,000,000 µs (10 µs – 10 s)
  - Gain range: 0 – 24 dB
  - Frame rate at full resolution: up to 75 fps
  - Typical microscopy settings: 25 ms exposure, 6 dB gain, 640×480 ROI

  Document structure:
  - docs/protocols/basler_pylon_spec.md
  - Sections: SDK architecture, Device enumeration, Camera parameters,
    Acquisition modes, Image conversion to QImage, Triggered acquisition,
    Error handling, CMake integration, Performance notes, Mock frame
    generation strategy

ACCEPTANCE CRITERIA:
  - Protocol spec document created at docs/protocols/basler_pylon_spec.md
  - Pylon C++ SDK API fully documented (key classes and methods)
  - GenICam parameter access pattern documented with code examples
  - All three acquisition modes documented (single, continuous, triggered)
  - Image format conversion to QImage fully specified
  - Unit conversion table (MWA ↔ Pylon) complete
  - CMake integration approach (FindPylon.cmake) documented
  - Performance considerations documented (buffer management, frame rate)
  - Mock frame generation strategy specified
DEPENDENCIES: none (research task)
PRIORITY: HIGH
```

### MWA-11-B: Enhanced Mock Camera Controller

```
TASK: MWA-11-B
REQUIREMENT: HW-REQ-006, NFR-004
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Enhance MockCameraController to generate synthetic microscopy-like frames
  that respond to exposure and gain settings. Replace the simple checkerboard
  with frames that exercise the full image pipeline.

  Current mock limitations:
  - Generates a static checkerboard pattern (no variation between frames)
  - Exposure and gain don't affect the generated image
  - No frame counter or timestamp in mock images
  - No synthetic "particles" for analysis module testing
  - No noise simulation
  - Continuous capture at fixed 30fps regardless of exposure setting

  Enhanced behavior:
  1. Synthetic microscopy frame generation:
     - Base: dark grey background (pixel value ~20) simulating microscope field
     - Channel walls: two horizontal bright lines (pixel value ~200) simulating
       microchannel boundaries visible in darkfield microscopy
     - Particles: 5–15 bright circles (radius 3–8 pixels) randomly positioned
       between channel walls, simulating cells/beads
     - Each frame generates NEW random particle positions (simulating flow)
     - Particles drift leftward by ~2 pixels per frame (simulating flow direction)

  2. Exposure response:
     - Base brightness scales with exposure: brightness = base * (exposure_ms / 25.0)
     - Clamped to [0, 255]
     - Low exposure (1ms): very dark image, particles barely visible
     - Normal exposure (25ms): well-exposed, particles clearly visible
     - High exposure (200ms): overexposed, saturation artifacts

  3. Gain response:
     - Gain multiplier: pixel = pixel * 10^(gain_dB / 20.0)
     - Higher gain → brighter image + more noise

  4. Noise simulation:
     - Shot noise: Poisson-distributed, √(signal) standard deviation
     - Read noise: Gaussian, σ = 3 + (gain_dB * 0.5) pixel values
     - Combined noise makes images look realistic

  5. Frame rate based on exposure:
     - Continuous mode: fps = min(30, 1000.0 / exposure_ms)
     - 25ms exposure → 30fps, 100ms → 10fps, 1ms → 30fps (capped)
     - capture_timer_ interval adjusted dynamically

  6. ROI support:
     - Generated image size matches ROI dimensions, not full sensor
     - Particles only generated within ROI bounds

  7. Frame counter and timestamp:
     - Each frame carries a sequence number (incrementing counter)
     - Optionally overlay frame number as text (top-left corner, small font)
     - Useful for verifying frame ordering in batch captures

  8. Batch capture with progress:
     - startBatchCapture() emits progress: emit batchProgress(captured, total)
       after each frame (add signal to interface if appropriate, or track internally)

  Files to modify:
  - src/hardware/camera/mock_camera_controller.h
  - src/hardware/camera/mock_camera_controller.cpp

  Doxygen documentation mandatory.

ACCEPTANCE CRITERIA:
  - Generated frames show recognizable microscopy-like content
    (dark background, bright channel walls, bright particle dots)
  - Particle positions change between frames (simulating flow)
  - Brightness responds to exposure setting
  - Noise increases with gain setting
  - Frame rate scales with exposure in continuous mode
  - ROI correctly limits image dimensions
  - Existing tests still pass (backward compatible)
  - All new public APIs have Doxygen comments
  - Builds with zero warnings
DEPENDENCIES: MWA-11-A (spec informs realistic frame generation)
PRIORITY: HIGH
```

### MWA-11-C: Camera Controller Driver Skeleton

```
TASK: MWA-11-C
REQUIREMENT: HW-REQ-006
ASSIGNED TO: SWE
STATUS: PLANNED
DESCRIPTION:
  Design the real CameraController header file wrapping the Basler Pylon SDK.
  HEADER-ONLY design — .cpp implementation when hardware/SDK available.

  Files to create:
  - src/hardware/camera/camera_controller.h
  - cmake/FindPylon.cmake (CMake find module)

  CameraController design:
  - Class CameraController : public CameraControllerInterface
  - Uses CommandQueue for thread-safe Pylon SDK calls
  - Private members:
    - std::unique_ptr<CommandQueue> command_queue_
    - Pylon CInstantCamera* camera_ (forward-declared, conditionally compiled)
    - Cached state under QMutex (exposure, gain, roi, capturing flag)
    - Frame counter for batch capture tracking
    - QTimer for continuous capture polling
  - Static methods:
    - static void initializePylon() — calls PylonInitialize() once
    - static void terminatePylon() — calls PylonTerminate()
    - static QStringList availableCameras() — enumerate connected cameras
  - setCameraIndex(int) / setCameraSerialNumber(QString) — select camera
  - connectDevice():
    - Enqueues: create CInstantCamera → Open() → read sensor info →
      set initial parameters → kConnected
  - disconnectDevice():
    - Enqueues: StopGrabbing() → Close() → destroy camera → kDisconnected
  - setExposure(double ms):
    - Enqueues: camera.ExposureTime.SetValue(ms * 1000) → emit
  - setGain(double dB):
    - Enqueues: camera.Gain.SetValue(dB) → emit
  - setRoi(QRect):
    - Enqueues: set Width/Height/OffsetX/OffsetY → emit
  - grabSingle():
    - Enqueues: GrabOne(timeout) → convert to QImage → emit frameReady()
  - startContinuousCapture():
    - Enqueues: StartGrabbing(LatestImageOnly) → start polling timer →
      each tick: RetrieveResult() → convert → emit frameReady()
  - stopCapture():
    - Stop polling timer → enqueue StopGrabbing()
  - startBatchCapture(count, interval):
    - Enqueues: software trigger mode → loop count times:
      ExecuteSoftwareTrigger() → RetrieveResult() → convert → emit →
      wait interval → next
    - Emit batchComplete() when done
  - Image conversion helper (private):
    - convertToQImage(CGrabResultPtr) → QImage
    - Handles Mono8 → Format_Grayscale8
    - Uses CImageFormatConverter for other pixel formats

  CMake integration:
  - cmake/FindPylon.cmake:
    - Check PYLON_ROOT environment variable
    - Search standard paths (/opt/pylon, C:/Program Files/Basler/pylon)
    - Set Pylon_FOUND, Pylon_INCLUDE_DIRS, Pylon_LIBRARIES
    - Windows: link pylon base, GCBase, GenApi, PylonBase, PylonUtility
    - macOS: link pylon.framework or individual dylibs
  - Conditional compilation: if(Pylon_FOUND) → add camera_controller.cpp

  Doxygen documentation mandatory.

ACCEPTANCE CRITERIA:
  - camera_controller.h compiles (no .cpp yet)
  - FindPylon.cmake is syntactically valid
  - Header documents full Pylon workflow in Doxygen
  - Image conversion strategy documented (Mono8/RGB8 → QImage)
  - Static initialization/termination design documented
  - Camera enumeration design documented
  - Platform-conditional CMake logic designed
  - All public APIs have Doxygen comments
  - Builds with zero warnings
DEPENDENCIES: MWA-11-A
PRIORITY: MEDIUM
```

**Execution order:**
- Round 1: MWA-11-A (research)
- Round 2: MWA-11-B (enhanced mock)
- Round 3: MWA-11-C (driver skeleton)

---

## Cross-Sprint Dependency Graph

```
Sprint 7 (SCPI Infrastructure)
├─ MWA-07-A (SCPI research)
│  ├──→ MWA-09-A (SigGen research)
│  └──→ MWA-10-A (VNA research)
└─ MWA-07-B (ScpiClient)
   ├──→ MWA-09-C (SigGen driver skeleton)
   └──→ MWA-10-C (VNA driver skeleton)

Sprint 8 (Pump)              ← Independent, can run in parallel with Sprint 7
├─ MWA-08-A → MWA-08-B → MWA-08-C

Sprint 9 (Signal Generator)  ← Depends on Sprint 7
├─ MWA-09-A → MWA-09-B → MWA-09-C

Sprint 10 (Network Analyzer) ← Depends on Sprint 7
├─ MWA-10-A → MWA-10-B → MWA-10-C

Sprint 11 (Camera)           ← Independent, can run in parallel with Sprint 7
├─ MWA-11-A → MWA-11-B → MWA-11-C
```

## Optimal Parallel Execution Plan

```
Wave 1 (all research — fully parallel):
  MWA-07-A, MWA-08-A, MWA-09-A, MWA-10-A, MWA-11-A

Wave 2 (enhanced mocks + SCPI client — parallel):
  MWA-07-B, MWA-08-B, MWA-09-B, MWA-10-B, MWA-11-B

Wave 3 (driver skeletons — parallel):
  MWA-08-C, MWA-09-C, MWA-10-C, MWA-11-C
```

> **Total: 5 sprints, 14 tasks, 3 waves of execution.**
> Each wave can be run with multiple parallel agents for maximum throughput.
