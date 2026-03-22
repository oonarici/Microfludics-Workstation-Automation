# MWA Sprint Plan

**Created by:** Lead
**Date:** 2026-03-22

---

## Sprint Overview

| Sprint | Focus | Phase | Status |
|--------|-------|-------|--------|
| Sprint 1 | Development Environment Setup | 0 | COMPLETE |
| Sprint 2 | Core Architecture & Main Window | 1 | IN PROGRESS |
| Sprint 3 | Device Control Panels (GUI) | 1 | PLANNED |
| Sprint 4 | Hardware Abstraction Layer | 2 | PLANNED |
| Sprint 5 | Hardware Device Drivers | 2 | PLANNED |
| Sprint 6 | Analysis Module Foundation | 3 | PLANNED |

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
