# Microfluidics Workstation Automation (MWA)

## Project Overview

Microfluidics Workstation Automation (MWA) is a cross-platform desktop application for controlling and automating microfluidic and acoustofluidic experiment workstations. The application provides a unified graphical interface for operating multiple laboratory devices, acquiring experimental data, and analyzing results.

The software targets researchers and engineers working with microfluidic chips who need coordinated control of illumination, fluid injection, signal generation, imaging, and microscope stage positioning.

---

## Build System & Tooling

| Item | Choice | Rationale |
|------|--------|-----------|
| **Language** | C++20 | Modern language features, strong type safety, high performance |
| **Build System** | CMake (≥ 3.21) | Officially recommended for Qt 6; native Qt Creator support; superior cross-platform toolchain handling |
| **UI Framework** | Qt 6 (LGPL v3) | Cross-platform GUI, mature widget set, hardware integration friendly |
| **Platforms** | macOS (ARM64/x86_64), Windows (x86_64) | Dual-platform from day one |
| **IDE** | Qt Creator | CMake projects open natively (no .pro file required) |
| **License** | LGPL-compatible toolchain | All third-party components must be LGPL-compatible or more permissive |

> **Why CMake over qmake?**
> Qt Company officially recommends CMake for Qt 6 and has deprecated qmake for new projects. CMake provides better cross-platform generator support (Xcode, Visual Studio, Ninja), integrates with package managers (vcpkg, conan), and Qt Creator opens CMakeLists.txt projects with full feature parity.

---

## Scientific Context

### Experimental Setup

A microfluidic workstation consists of:

- A **microfluidic chip** containing microchannels through which particle-laden fluids flow.
- A **microscope** with an objective lens focused on the microchannels.
- An **LED light source** illuminating the chip from below or above.
- A **syringe pump** injecting fluids at precise flow rates.
- A **frequency generator or network analyzer** driving piezoelectric transducers (PZTs) to create acoustic fields inside the chip.
- A **camera** mounted on the microscope capturing images of the microchannels.
- A **motorized XYZ stage** adjusting focus (Z) and panning across channels (X, Y).

### Experimental Objective

The goal is to observe and manipulate particles (cells, beads, droplets) inside the microchip using acoustofluidic or microfluidic forces. Typical operations include:

- **Focusing** — concentrating particles into narrow streams
- **Alignment** — arranging particles into single-file lines
- **Rotation** — spinning particles using acoustic torque
- **Separation** — sorting particles by size, density, or acoustic contrast
- **Trapping** — holding particles at fixed positions

Images are captured in batches during experiments and later analyzed to determine particle locations, velocities, and the effectiveness of manipulation.

---

## System Architecture — Three Modules

The application is structured into three major modules developed in phases:

### Module 1: Graphical User Interface (GUI) — *Phase 1 (current)*

The GUI module provides the operator interface for the entire workstation. It is developed first so that hardware integration has a tested front-end to connect to.

**Responsibilities:**

- Unified main window with device control panels
- Real-time device status display
- Experiment workflow configuration (sequence of operations)
- Image preview and batch capture controls
- Logging and error reporting
- Settings persistence (per-device and global)

**Controlled Hardware (via GUI panels):**

| Device | Typical Hardware | Control Interface |
|--------|-----------------|-------------------|
| LED Light Source | Thorlabs UPLEDUSB LED driver or custom power supply | USB / Serial |
| Syringe Pump | Cetoni Nemesys syringe pump | Cetoni SDK (USB) |
| Frequency Generator / Network Analyzer | Signal generator or VNA for PZT excitation | VISA / USB / Serial |
| Camera | Basler area-scan camera | Basler Pylon SDK (USB3 / GigE) |
| Motorized XYZ Stage | Stepper/servo motors with controller | Serial / USB |

**Key Design Principles:**

- Device panels are independent widgets that can be shown/hidden
- Each device has a dedicated controller class behind an abstract interface
- GUI never talks to hardware directly — always through the hardware abstraction layer
- Settings are stored in platform-appropriate locations (QSettings)

### Module 2: Hardware Integration — *Phase 2*

The hardware module implements the actual communication with each device using vendor SDKs, serial protocols, or embedded firmware.

**Responsibilities:**

- Device discovery and connection management
- Protocol implementation for each device
- Thread-safe command queuing
- Real-time data streaming (camera frames, sensor readings)
- Error recovery and timeout handling

**Integration Approach by Device:**

| Device | SDK / Protocol | Notes |
|--------|---------------|-------|
| LED Light Source | Thorlabs Kinesis SDK or custom serial protocol | Simple on/off and intensity control |
| Syringe Pump | Cetoni Nemesys SDK (C/C++) | Flow rate, volume, start/stop, refill |
| Frequency Generator | SCPI commands over VISA or serial | Frequency, amplitude, sweep, modulation |
| Network Analyzer | SCPI commands over VISA | S-parameter measurement, frequency sweep |
| Camera | Basler Pylon C++ SDK | Frame grab, exposure, gain, ROI, triggering |
| XYZ Stage | Serial/USB protocol (device-specific) | Absolute/relative positioning, homing, limits |

### Module 3: Analysis Tool Integration — *Phase 3*

Once data acquisition is functional, the analysis module provides post-processing capabilities.

**Responsibilities:**

- Particle detection and tracking
- Velocity field computation
- Focusing efficiency metrics
- Separation efficiency metrics
- Integration with or inspired by tools like Gwyddion
- Export of results (CSV, images, reports)

---

## Development Phases

```
Phase 1 — GUI Shell & Architecture
├── Project scaffolding (CMake, Qt 6, CI)
├── Main window layout with device panels
├── Mock/simulated device controllers
├── Settings and configuration management
├── Logging framework
└── Cross-platform build verification (macOS + Windows)

Phase 2 — Hardware Integration
├── Abstract device interface implementation
├── LED driver integration
├── Syringe pump SDK integration
├── Frequency generator / network analyzer integration
├── Camera SDK integration (Basler Pylon)
├── XYZ stage motor control
└── Device discovery and hot-plug handling

Phase 3 — Analysis Integration
├── Image batch management
├── Particle detection algorithms
├── Velocity and trajectory computation
├── Statistical analysis and metrics
├── Gwyddion-style visualization
└── Report generation and export
```

---

## Cross-Platform Considerations

### macOS
- Build with Clang (Apple Clang or Homebrew LLVM)
- Qt 6 installed via Homebrew (`brew install qt@6`) or Qt online installer
- Application bundle (.app) packaging
- Camera SDK: Basler Pylon for macOS
- Serial ports: `/dev/tty.*` or `/dev/cu.*`

### Windows
- Build with MSVC 2019+ or MinGW
- Qt 6 installed via Qt online installer or vcpkg
- Installer packaging (NSIS or WiX)
- Camera SDK: Basler Pylon for Windows
- Serial ports: `COM*` via Windows API
- Some vendor SDKs (Thorlabs Kinesis) are Windows-only — abstraction layer must handle platform availability

### Platform Abstraction Strategy

- Use Qt abstractions wherever possible (QSerialPort, QThread, QSettings, QDir)
- Wrap platform-specific SDK calls behind interfaces
- Use CMake platform checks and conditional compilation
- Devices unavailable on a platform are disabled in the GUI with a clear message

---

## Directory Structure (Planned)

```
MWA/
├── CMakeLists.txt              # Root CMake configuration
├── cmake/                      # CMake modules and Find scripts
├── docs/                       # Project documentation
│   ├── PROJECT_DESCRIPTION.md
│   └── requirements.json
├── src/
│   ├── main.cpp                # Application entry point
│   ├── app/                    # Application-level code
│   │   ├── mainwindow.h / .cpp
│   │   └── application.h / .cpp
│   ├── gui/                    # GUI widgets and panels
│   │   ├── panels/             # Device control panels
│   │   │   ├── led_panel.h / .cpp
│   │   │   ├── pump_panel.h / .cpp
│   │   │   ├── signal_panel.h / .cpp
│   │   │   ├── camera_panel.h / .cpp
│   │   │   └── stage_panel.h / .cpp
│   │   └── widgets/            # Reusable custom widgets
│   ├── hardware/               # Hardware abstraction layer
│   │   ├── device_interface.h  # Abstract base
│   │   ├── led/
│   │   ├── pump/
│   │   ├── signal_generator/
│   │   ├── camera/
│   │   └── stage/
│   ├── analysis/               # Analysis module (Phase 3)
│   └── core/                   # Shared utilities, logging, settings
├── resources/                  # Qt resources (icons, QSS, translations)
├── tests/                      # Unit and integration tests
├── libs/                       # Third-party libraries (vendored or submodules)
└── scripts/                    # Build and packaging scripts
```

---

## Quality & CI/CD

- **Testing:** Qt Test framework + CTest integration
- **Static Analysis:** clang-tidy, cppcheck
- **Formatting:** clang-format with project-wide .clang-format
- **CI:** GitHub Actions — build and test on macOS and Windows for every PR
- **Code Coverage:** gcov/llvm-cov (optional)
