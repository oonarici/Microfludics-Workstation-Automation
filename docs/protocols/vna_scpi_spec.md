# VNA SCPI Specification -- MWA Project

> **Version:** 1.0
> **Date:** 2026-04-02
> **Purpose:** Reference for implementing the `NetworkAnalyzerController` driver and
> enhancing `MockNetworkAnalyzerController` with realistic PZT resonance data.
> **Protocol:** SCPI over VISA / TCP / Serial

---

## Table of Contents

1. [Overview](#1-overview)
2. [Supported Instruments](#2-supported-instruments)
3. [SCPI Command Reference](#3-scpi-command-reference)
4. [Measurement Workflow](#4-measurement-workflow)
5. [Data Formats](#5-data-formats)
6. [Vendor Differences Table](#6-vendor-differences-table)
7. [PZT Resonance Physics](#7-pzt-resonance-physics)
8. [Calibration](#8-calibration)
9. [Connection Methods](#9-connection-methods)
10. [MWA Interface Mapping](#10-mwa-interface-mapping)
11. [Mock Data Generation Algorithm](#11-mock-data-generation-algorithm)
12. [References](#12-references)

---

## 1. Overview

A **Vector Network Analyzer (VNA)** measures the S-parameters (scattering parameters)
of a device under test (DUT) by sweeping an RF stimulus signal across a frequency range
and measuring the reflected and transmitted signals.

### Role in Microfluidics

In the MWA workstation, the VNA characterizes **piezoelectric transducers (PZTs)**
bonded to microfluidic chips:

1. Connect VNA Port 1 to PZT electrical terminals
2. Sweep frequency across the PZT's expected resonance range (1--10 MHz)
3. Measure **S11** (input reflection coefficient) -- dips indicate resonance
4. Identify the resonance frequency where acoustic coupling is strongest
5. Set the signal generator to this frequency for particle manipulation

The resonance frequency depends on the PZT geometry, bonding quality, fluid loading,
and chip design. It must be found experimentally for each setup.

---

## 2. Supported Instruments

### 2.1 Keysight ENA Series (Primary Target)

| Model | Frequency Range | Ports | Key Features |
|-------|----------------|-------|--------------|
| E5063A | 100 kHz -- 500 MHz / 1.5 / 3 / 4.5 / 6.5 / 8.5 / 14 / 18 GHz | 2 | Entry-level ENA |
| E5080A | 9 kHz -- 4.5 / 6.5 / 9 / 14 / 20 GHz | 2 or 4 | Mid-range ENA |
| E5071C | 9 kHz -- 4.5 / 6.5 / 8.5 GHz | 2 or 4 | Discontinued, widely deployed |

**Interfaces:** USB-TMC, LAN (LXI, port 5025), GPIB
**SCPI dialect:** Standard Keysight ENA command set

### 2.2 Rohde & Schwarz ZNB/ZVA Series

| Model | Frequency Range | Ports | Key Features |
|-------|----------------|-------|--------------|
| ZNB4 | 9 kHz -- 4.5 GHz | 2 or 4 | Mid-range |
| ZNB8 | 9 kHz -- 8.5 GHz | 2 or 4 | SCPI recorder feature |
| ZVA40 | 10 MHz -- 40 GHz | 2 or 4 | High-end |

**Interfaces:** USB-TMC, LAN (LXI, port 5025), GPIB
**SCPI dialect:** R&S specific extensions, largely compatible with Keysight

### 2.3 Budget Instruments

| Model | Frequency Range | Notes |
|-------|----------------|-------|
| miniVNA Tiny | 1 -- 3000 MHz | **NOT SCPI** -- proprietary USB protocol |
| nanoVNA V2 | 50 kHz -- 3 GHz | **NOT SCPI** -- custom USB/serial protocol |
| Copper Mountain TR1300/1 | 300 kHz -- 1.3 GHz | SCPI-compatible (limited) |

> Budget VNAs (miniVNA, nanoVNA) are **not supported** by the MWA SCPI driver.
> They would need separate, device-specific drivers.

---

## 3. SCPI Command Reference

All commands below use channel 1 (`<cnum>` = 1) and measurement 1 (`<mnum>` = 1).

### 3.1 Frequency Configuration

| Command | Description | Example |
|---------|-------------|---------|
| `SENSe<cnum>:FREQuency:STARt <freq>` | Set sweep start frequency | `SENS1:FREQ:STAR 1e6` |
| `SENSe<cnum>:FREQuency:STOP <freq>` | Set sweep stop frequency | `SENS1:FREQ:STOP 10e6` |
| `SENSe<cnum>:FREQuency:STARt?` | Query start frequency | → `1000000` |
| `SENSe<cnum>:FREQuency:STOP?` | Query stop frequency | → `10000000` |
| `SENSe<cnum>:FREQuency:CENTer <freq>` | Set center frequency | `SENS1:FREQ:CENT 5.5e6` |
| `SENSe<cnum>:FREQuency:SPAN <freq>` | Set frequency span | `SENS1:FREQ:SPAN 9e6` |

### 3.2 Sweep Configuration

| Command | Description | Example |
|---------|-------------|---------|
| `SENSe<cnum>:SWEep:POINts <n>` | Set number of sweep points | `SENS1:SWE:POIN 201` |
| `SENSe<cnum>:SWEep:POINts?` | Query number of points | → `201` |
| `SENSe<cnum>:SWEep:TIME?` | Query estimated sweep time | → `0.150` (seconds) |
| `SENSe<cnum>:SWEep:TYPE LINear` | Set linear sweep | `SENS1:SWE:TYPE LIN` |
| `SENSe<cnum>:SWEep:TYPE LOGarithmic` | Set logarithmic sweep | `SENS1:SWE:TYPE LOG` |
| `SENSe<cnum>:BANDwidth <bw>` | Set IF bandwidth | `SENS1:BAND 1000` (1 kHz) |

**IF Bandwidth** affects measurement noise vs. speed:

| IF Bandwidth | Noise Floor | Relative Speed |
|-------------|-------------|----------------|
| 100 Hz | ~ -100 dB | Slow (reference) |
| 1 kHz | ~ -80 dB | 10x faster |
| 10 kHz | ~ -60 dB | 100x faster |
| 100 kHz | ~ -40 dB | 1000x faster |

For PZT characterization (S11 around -30 to -50 dB), an IF BW of 1 kHz is typical.

### 3.3 Measurement Parameter Selection

#### Keysight ENA

```
CALCulate<cnum>:PARameter:DEFine:EXTended <mname>,<param>
```

| S-Parameter | Meaning | Usage |
|-------------|---------|-------|
| `S11` | Input reflection | PZT impedance characterization |
| `S21` | Forward transmission | Through-chip acoustic measurement |
| `S12` | Reverse transmission | Rarely used |
| `S22` | Output reflection | Rarely used |

Example:
```
CALC1:PAR:DEF:EXT "MyMeas","S11"
DISP:WIND1:TRAC1:FEED "MyMeas"
```

#### R&S ZNB

```
CALCulate<cnum>:PARameter:SDEFine <tname>,<param>
```

Example:
```
CALC1:PAR:SDEF "Trc1","S11"
```

### 3.4 Data Format Selection

| Command | Format | Data Meaning |
|---------|--------|-------------|
| `CALCulate<cnum>:SELected:FORMat MLOGarithmic` | Magnitude (dB) | `20 × log10(|S|)` |
| `CALCulate<cnum>:SELected:FORMat PHASe` | Phase (degrees) | `arg(S) × 180/π` |
| `CALCulate<cnum>:SELected:FORMat SMITh` | Smith chart | Real, Imaginary |
| `CALCulate<cnum>:SELected:FORMat POLar` | Polar | Magnitude, Phase |
| `CALCulate<cnum>:SELected:FORMat REAL` | Real part only | `Re(S)` |
| `CALCulate<cnum>:SELected:FORMat IMAGinary` | Imaginary part only | `Im(S)` |
| `CALCulate<cnum>:SELected:FORMat SWR` | Standing Wave Ratio | `(1+|S|)/(1-|S|)` |

**For MWA:** Use `MLOGarithmic` (magnitude in dB) as the default. This is what
`traceMagnitudes()` returns.

### 3.5 Trigger & Initiation

| Command | Description |
|---------|-------------|
| `INITiate<cnum>[:IMMediate]` | Start a single sweep |
| `TRIGger:SEQuence:SOURce BUS` | Set trigger source to bus (software) |
| `TRIGger:SEQuence:SOURce IMMediate` | Set to continuous (free-run) |
| `SENSe<cnum>:SWEep:MODE SING` | Single sweep mode |
| `SENSe<cnum>:SWEep:MODE CONTinuous` | Continuous sweep mode |
| `*OPC?` | Query operation complete (returns `1` when sweep done) |
| `ABORt` | Abort current sweep |

### 3.6 Data Retrieval

| Command | Returns | Format |
|---------|---------|--------|
| `CALCulate<cnum>:SELected:DATA? FDATA` | Formatted trace data | Comma-separated ASCII |
| `CALCulate<cnum>:SELected:DATA? SDATA` | Complex S-parameter | Comma-separated ASCII (real,imag pairs) |
| `CALCulate<cnum>:DATA:SNP? 1` | Touchstone (S1P) | S-parameter file format |

**FDATA response example** (3 points, MLOG format):
```
-8.23456,-15.67890,-8.01234\n
```

**SDATA response example** (3 points, complex S11):
```
0.38268,-0.03214,0.12345,-0.45678,0.37654,-0.02876\n
```
(6 values = 3 points × 2 values per point: real₁,imag₁,real₂,imag₂,real₃,imag₃)

### 3.7 Status Monitoring

| Command | Description | Response |
|---------|-------------|----------|
| `*OPC?` | Operation complete query | `1` when done |
| `STATus:OPERation:CONDition?` | Operation status register | Bit 0 set = sweeping |
| `*ESR?` | Event status register | Error bits |
| `SYST:ERR?` | Error queue | `0,"No error"` when clean |

---

## 4. Measurement Workflow

### Complete Step-by-Step (Keysight ENA)

```
Step  Command                                    Purpose
────  ─────────────────────────────────────────  ──────────────────────
 1    *RST                                       Preset instrument
 2    *CLS                                       Clear status/errors
 3    SENS1:SWE:MODE SING                        Single sweep mode
 4    TRIG:SEQ:SOUR BUS                          Bus trigger mode
 5    SENS1:FREQ:STAR 1000000                    Start = 1 MHz
 6    SENS1:FREQ:STOP 10000000                   Stop = 10 MHz
 7    SENS1:SWE:POIN 201                         201 sweep points
 8    SENS1:BAND 1000                            IF BW = 1 kHz
 9    CALC1:PAR:DEF:EXT "CH1_S11","S11"          Define S11 measurement
10    DISP:WIND1:TRAC1:FEED "CH1_S11"            Display on trace 1
11    CALC1:SEL:FORM MLOG                        Format = dB magnitude
12    INIT1:IMM; *OPC?                           Trigger sweep, wait
13    (wait for "1" response)                    Sweep in progress...
14    CALC1:SEL:DATA? FDATA                      Retrieve formatted data
15    (parse comma-separated dB values)          201 values received
16    SYST:ERR?                                  Check for errors
```

### Timing Estimates

| Points | IF BW = 1 kHz | IF BW = 10 kHz | IF BW = 100 kHz |
|--------|--------------|----------------|-----------------|
| 51 | ~0.3 s | ~0.05 s | ~0.02 s |
| 201 | ~1.0 s | ~0.15 s | ~0.05 s |
| 401 | ~2.0 s | ~0.30 s | ~0.10 s |
| 1001 | ~5.0 s | ~0.75 s | ~0.25 s |

---

## 5. Data Formats

### 5.1 ASCII FDATA

Comma-separated ASCII values, one per sweep point. For `MLOGarithmic` format,
each value is the S-parameter magnitude in dB.

```
Response: "-8.234,-8.189,-8.156,...,-7.943\n"
         ← ─── 201 comma-separated values ─── →
```

Parsing:
```cpp
QVector<double> magnitudes;
QStringList tokens = response.split(',');
for (const auto& t : tokens) {
  magnitudes.append(t.trimmed().toDouble());
}
```

### 5.2 ASCII SDATA

Comma-separated ASCII values, pairs of (real, imaginary) per point.
Total values = 2 × num_points.

```
Response: "0.383,-0.032,0.123,-0.457,...,0.377,-0.029\n"
         ← ─── 402 values for 201 points ─── →
```

Conversion to magnitude (dB):
```cpp
for (int i = 0; i < num_points; ++i) {
  double re = values[2*i];
  double im = values[2*i + 1];
  double mag_db = 20.0 * std::log10(std::sqrt(re*re + im*im));
  magnitudes.append(mag_db);
}
```

### 5.3 IEEE 488.2 Block Data

For binary transfer (faster for large datasets):

```
Format: #<num_digits><byte_count><binary_data>

Example (4 bytes per point, 201 points = 804 bytes):
#3804<804 bytes of float32 data>

Parsing:
  1. Read '#'
  2. Read 1 digit: num_digits = 3
  3. Read 3 digits: byte_count = 804
  4. Read 804 bytes
  5. Interpret as 201 × float32 (little-endian or big-endian per FORM command)
```

### 5.4 Frequency Array

The frequency array is **not returned** by the data query -- it is computed:

```cpp
QVector<double> frequencies;
double step = (stop_hz - start_hz) / (num_points - 1);
for (int i = 0; i < num_points; ++i) {
  frequencies.append(start_hz + i * step);
}
```

For logarithmic sweep:
```cpp
double log_start = std::log10(start_hz);
double log_stop = std::log10(stop_hz);
double log_step = (log_stop - log_start) / (num_points - 1);
for (int i = 0; i < num_points; ++i) {
  frequencies.append(std::pow(10.0, log_start + i * log_step));
}
```

---

## 6. Vendor Differences Table

| Feature | Keysight ENA | R&S ZNB | Notes |
|---------|-------------|---------|-------|
| **S-param definition** | `CALC:PAR:DEF:EXT "name","S11"` | `CALC:PAR:SDEF "Trc1","S11"` | Different command |
| **Trace feed** | `DISP:WIND:TRAC:FEED "name"` | Automatic | Keysight needs explicit display |
| **Data query** | `CALC:SEL:DATA? FDATA` | `CALC:DATA? FDAT` | Slightly different keyword |
| **Complex data** | `CALC:SEL:DATA? SDATA` | `CALC:DATA? SDAT` | Same difference |
| **IF bandwidth** | `SENS:BAND <bw>` | `SENS:BAND <bw>` | Same |
| **Frequency** | `SENS:FREQ:STAR/STOP` | `SENS:FREQ:STAR/STOP` | Same |
| **Sweep points** | `SENS:SWE:POIN` | `SENS:SWE:POIN` | Same |
| **Trigger** | `INIT:IMM; *OPC?` | `INIT1:IMM; *WAI` | R&S prefers *WAI |
| **Sweep status** | `STAT:OPER:COND?` | `STAT:OPER:COND?` | Same |
| **Preset** | `SYST:PRES` or `*RST` | `*RST` | Both work on both |
| ***IDN?** | `Keysight Technologies,E5063A,...` | `Rohde&Schwarz,ZNB8,...` | For auto-detection |

**Driver strategy:** Use vendor enum + command table. Detect vendor from `*IDN?`
response, select the appropriate command variant.

---

## 7. PZT Resonance Physics

### 7.1 Background: PZT in Microfluidics

Piezoelectric transducers (PZTs) bonded to microfluidic chips generate acoustic
standing waves when driven at their resonance frequency. The resonance frequency
depends on:

- PZT material (PZT-4, PZT-5A, PZT-8)
- PZT geometry (thickness, width, length)
- Bonding quality (epoxy layer thickness)
- Fluid loading (water, buffer, cell suspension)
- Chip material (glass, silicon, PDMS)

### 7.2 Lorentzian Resonance Model

The S11 (reflection) magnitude near a resonance follows a **Lorentzian dip**:

```
S_dB(f) = background_dB - Σᵢ [ depth_dBᵢ / (1 + ((f - f₀ᵢ) / (BWᵢ / 2))²) ]
```

Where:
- `background_dB`: Off-resonance reflection level (typically -5 to -10 dB)
- `f₀ᵢ`: Resonance center frequency of mode i (Hz)
- `depth_dBᵢ`: Depth of the resonance dip (positive number, dB below background)
- `BWᵢ`: -3 dB bandwidth of the resonance (Hz)
- `Qᵢ = f₀ᵢ / BWᵢ`: Quality factor of the resonance

### 7.3 Typical PZT Parameters for Microfluidics

| Parameter | Typical Value | Range | Notes |
|-----------|---------------|-------|-------|
| Fundamental frequency | 2 -- 5 MHz | 0.5 -- 20 MHz | Depends on PZT thickness |
| S11 background | -8 dB | -5 to -15 dB | Off-resonance impedance mismatch |
| Primary resonance depth | 30 -- 40 dB | 15 -- 50 dB | Strong coupling = deeper dip |
| Primary bandwidth | 30 -- 80 kHz | 10 -- 200 kHz | Narrow = high Q |
| Q factor | 50 -- 300 | 20 -- 1000 | Fluid-loaded is lower than air |
| Harmonic spacing | ~2× fundamental | | Overtone modes |
| Number of visible modes | 2 -- 4 | 1 -- 8 | Fundamental + harmonics |
| Measurement noise | ±0.3 -- 1.0 dB | | Depends on IF bandwidth |

### 7.4 Example: PZT Bonded to Glass Microfluidic Chip

A typical setup: PZT-5A disc (∅10mm, 1mm thick) epoxy-bonded to a borosilicate
glass chip with 375 µm wide channels, water-filled.

| Mode | Center Frequency | Depth | Bandwidth | Q Factor |
|------|-----------------|-------|-----------|----------|
| Fundamental (λ/2) | 2.0 MHz | 35 dB | 40 kHz | 50 |
| 1st harmonic (3λ/2) | 4.15 MHz | 22 dB | 60 kHz | 69 |
| 2nd harmonic (5λ/2) | 6.35 MHz | 12 dB | 90 kHz | 71 |

Background: -8 dB
Noise: σ = 0.5 dB (Gaussian)

### 7.5 Acoustic Coupling vs. S11 Depth

| S11 Depth | Coupling | Acoustic Effect |
|-----------|----------|-----------------|
| < 10 dB | Weak | Minimal particle manipulation |
| 10 -- 20 dB | Moderate | Partial focusing |
| 20 -- 35 dB | Good | Clear particle manipulation |
| > 35 dB | Excellent | Strong acoustic trapping |

---

## 8. Calibration

VNA calibration removes systematic errors from the measurement. Not needed for
the mock, but documented for the real driver.

### 8.1 SOL Calibration (1-Port)

For S11 measurements, calibrate with three standards:

| Standard | Command | Purpose |
|----------|---------|---------|
| Short | `SENS:CORR:COLL:METH:SOLT1 1` then connect short | Reflects with 180° phase |
| Open | Connect open standard, then measure | Reflects with 0° phase |
| Load | Connect 50Ω load, then measure | No reflection |

### 8.2 Typical SCPI Calibration Sequence

```
SENS1:CORR:COLL:METH:SOLT1 1        // Select 1-port SOL cal on port 1
// Connect SHORT standard
SENS1:CORR:COLL:ACQ:SHOR 1           // Measure short
// Connect OPEN standard
SENS1:CORR:COLL:ACQ:OPEN 1           // Measure open
// Connect LOAD standard
SENS1:CORR:COLL:ACQ:LOAD 1           // Measure load
SENS1:CORR:COLL:SAVE                 // Apply calibration
```

### 8.3 Electronic Calibration (ECal)

Modern VNAs support electronic calibration modules that automate the process:
```
SENS1:CORR:COLL:ECAL:SOLT1 1         // 1-port ECal on port 1
```

---

## 9. Connection Methods

### 9.1 USB-TMC (VISA)

```
VISA Resource: USB0::0x0957::0x0D09::MY12345678::INSTR
               ────  ──────  ──────  ───────────  ────
               bus   vendor  model   serial       type
```

- Keysight vendor ID: `0x0957` (Agilent/Keysight)
- R&S vendor ID: `0x0AAD`

### 9.2 TCP/IP (LXI)

```
VISA Resource: TCPIP0::192.168.1.100::5025::SOCKET
               ─────  ───────────────  ────  ──────
               type   IP address       port  protocol
```

- Default SCPI socket port: **5025** (standard LXI)
- Alternative: HiSLIP protocol (port 4880) for newer instruments

### 9.3 GPIB

```
VISA Resource: GPIB0::16::INSTR
               ────  ──  ────
               bus   addr type
```

### 9.4 Identification Examples

```
*IDN? responses:

Keysight:  "Keysight Technologies,E5063A,MY54321234,A.11.75"
           "Agilent Technologies,E5071C,MY45678901,A.09.70"

R&S:       "Rohde&Schwarz,ZNB8-4Port,1311.6010K08/100234,3.30"
```

---

## 10. MWA Interface Mapping

| MWA Method | SCPI Command(s) | Direction | Notes |
|-----------|-----------------|-----------|-------|
| `connectDevice()` | Open transport → `*IDN?` → detect vendor → `*RST` → `*CLS` → set MLOG format → set S11 param | GUI → HW | Multi-step init |
| `disconnectDevice()` | Close transport | GUI → HW | |
| `setFrequencyRange(start, stop)` | `SENS:FREQ:STAR <start>` + `SENS:FREQ:STOP <stop>` | GUI → HW | |
| `setNumPoints(n)` | `SENS:SWE:POIN <n>` | GUI → HW | |
| `measureSParameters()` | `INIT:IMM; *OPC?` → wait → `CALC:SEL:DATA? FDATA` → parse | GUI → HW | Async via CommandQueue |
| `traceFrequencies()` | Computed: `start + i * (stop-start)/(n-1)` | cache → GUI | Not queried from instrument |
| `traceMagnitudes()` | Parsed from FDATA response | HW → GUI | Stored after measurement |
| `startFrequency()` | Return cached value (or `SENS:FREQ:STAR?`) | cache → GUI | |
| `stopFrequency()` | Return cached value (or `SENS:FREQ:STOP?`) | cache → GUI | |
| `numPoints()` | Return cached value (or `SENS:SWE:POIN?`) | cache → GUI | |
| `isMeasuring()` | Check internal flag set during sweep | cache → GUI | |
| `measurementStarted()` | Emit before `INIT:IMM` | HW → GUI | Signal |
| `measurementComplete()` | Emit after FDATA parsed | HW → GUI | Signal |
| `frequencyRangeChanged()` | Emit after freq set confirmed | HW → GUI | Signal |
| `numPointsChanged()` | Emit after points set confirmed | HW → GUI | Signal |

---

## 11. Mock Data Generation Algorithm

### 11.1 Default Resonance Model

```cpp
// Default PZT resonance configuration for mock
struct Resonance {
  double center_hz;     // Center frequency (Hz)
  double depth_db;      // Dip depth (positive, dB)
  double bandwidth_hz;  // -3 dB bandwidth (Hz)
};

static constexpr double kBackgroundDb = -8.0;
static constexpr double kNoiseSigmaDb = 0.5;

static const QVector<Resonance> kDefaultResonances = {
  {2.0e6,  35.0, 40e3},   // Fundamental: 2.0 MHz, 35 dB deep, 40 kHz BW
  {4.15e6, 22.0, 60e3},   // 1st harmonic: 4.15 MHz
  {6.35e6, 12.0, 90e3},   // 2nd harmonic: 6.35 MHz
};
```

### 11.2 Generation Algorithm

```cpp
void generateTrace(double start_hz, double stop_hz, int num_points,
                   const QVector<Resonance>& resonances,
                   double background_db, double noise_sigma,
                   QVector<double>& out_frequencies,
                   QVector<double>& out_magnitudes)
{
  std::mt19937 rng(std::random_device{}());
  std::normal_distribution<double> noise(0.0, noise_sigma);

  out_frequencies.resize(num_points);
  out_magnitudes.resize(num_points);

  double step = (num_points > 1)
    ? (stop_hz - start_hz) / (num_points - 1) : 0.0;

  for (int i = 0; i < num_points; ++i) {
    double f = start_hz + i * step;
    double s = background_db;

    // Sum Lorentzian contributions from each resonance
    for (const auto& r : resonances) {
      double delta = (f - r.center_hz) / (r.bandwidth_hz / 2.0);
      s -= r.depth_db / (1.0 + delta * delta);
    }

    // Add measurement noise
    s += noise(rng);

    out_frequencies[i] = f;
    out_magnitudes[i] = s;
  }
}
```

### 11.3 Measurement Time Simulation

Scale measurement time with the number of points (simulating IF bandwidth settling):

```cpp
int estimatedMeasurementTimeMs(int num_points) {
  // ~5 ms per point (equivalent to 1 kHz IF BW)
  int time_ms = num_points * 5;
  // Clamp to usable range
  return std::clamp(time_ms, 500, 10000);
}

// 201 points → ~1.0 s
// 401 points → ~2.0 s
// 1001 points → ~5.0 s
```

### 11.4 Example Output (201 points, 1--10 MHz)

First and last few points of the generated trace:

```
 Index   Freq (MHz)   S11 (dB)
 ─────   ──────────   ────────
   0      1.000        -8.12
   1      1.045        -8.35
   2      1.090        -7.89
  ...
  20      1.900        -8.54
  21      1.945        -9.21      ← approaching resonance
  22      1.990       -14.67
  23      2.000       -42.85      ← resonance dip (f₀ = 2.0 MHz)
  24      2.045       -14.23
  25      2.090        -9.18
  ...
  70      4.135       -29.45      ← 2nd resonance (f₀ = 4.15 MHz)
  ...
 140      6.350       -19.87      ← 3rd resonance (f₀ = 6.35 MHz)
  ...
 200     10.000        -7.95
```

---

## 12. References

1. [Keysight ENA E5063A Help](https://ena.support.keysight.com/e5063a/manuals/webhelp/eng/) --
   Complete SCPI command reference and programming guide
2. [Keysight E5080A SCPI Command Tree](https://ena.support.keysight.com/e5080/manuals/webhelp/eng/Programming/GP-IB_Command_Finder/SCPI_Command_Tree.htm) --
   Hierarchical command listing
3. [Keysight ENA SCPI Command Messages](https://ena.support.keysight.com/e5063a/manuals/webhelp/eng4/programming/remote_control/overview/sending_scpi_command_messages.htm) --
   Command syntax and formatting rules
4. [R&S ZNB VNA SCPI Examples (GitHub)](https://github.com/Terrabits/vna-scpi-examples) --
   Python examples with raw SCPI for R&S VNAs
5. [R&S ZNB SCPI Python Driver (QCoDeS)](https://microsoft.github.io/Qcodes/_modules/qcodes/instrument_drivers/rohde_schwarz/ZNB.html) --
   Python instrument driver showing R&S SCPI dialect
6. [R&S VNA Remote Handling FAQ](https://www.rohde-schwarz.com/us/faq/vna-remote-handling-of-measurements-results-using-scpi-commands-faq_78704-1165571.html) --
   Measurement result retrieval via SCPI
7. [Copper Mountain S2VNA/S4VNA SCPI Manual](https://coppermountaintech.com/wp-content/uploads/2020/09/SxVNA-SCPI-Programming-Manual-2.pdf) --
   Alternative SCPI VNA reference (budget-friendly)
8. [SCPI-1999 Standard (IVI Foundation)](https://www.ivifoundation.org/scpi/) --
   Official SCPI specification
9. [Z. Hatab, R&S VNA SCPI Python (GitHub)](https://github.com/ZiadHatab/scpi-rohde-schwarz-vna) --
   Trace data collection examples
