# Signal Generator SCPI Specification -- MWA Project

## 1. Overview

This document defines the SCPI (Standard Commands for Programmable Instruments) command
set required to implement the `SignalGeneratorControllerInterface` for the MWA project.
The driver must control signal/function generators used for PZT (piezoelectric transducer)
excitation in microfluidic chips. Typical operating parameters are 1--10 MHz sine waves
at 0.1--10 Vpp.

Three instrument families are targeted:

| Vendor     | Primary Model   | Also Compatible With  |
|------------|-----------------|-----------------------|
| Rigol      | DG1062Z         | DG1032Z, DG800 series |
| Keysight   | 33512B (33500B) | 33522B, 33600A series |
| Tektronix  | AFG1022         | AFG3052C, AFG3000C    |

All three vendors implement SCPI (IEEE 488.2 / SCPI-1999), but there are vendor-specific
differences in command trees and parameter enumerations documented in this specification.

---

## 2. Supported Instruments

### 2.1 Rigol DG1062Z / DG1000Z Series

- **Channels:** 2
- **Sine frequency range:** 1 uHz -- 60 MHz
- **Square frequency range:** 1 uHz -- 25 MHz
- **Amplitude range (50 Ohm):** 1 mVpp -- 10 Vpp (up to 2.5 Vpp at 60 MHz)
- **Amplitude range (High-Z):** 2 mVpp -- 20 Vpp
- **Resolution:** 14-bit, 200 MSa/s
- **Memory depth:** 2 Mpts (16 Mpts optional)
- **Waveform types:** Sine, Square, Ramp, Pulse, Triangle, Noise, DC, Arb
- **Sweep:** Linear and logarithmic, internal/external/manual trigger
- **Interfaces:** USB-TMC, LAN (LXI), GPIB (optional)

### 2.2 Keysight 33500B / 33600A Series (Trueform)

- **Channels:** 1 or 2 (model dependent: 33511B=1ch, 33512B=2ch)
- **Sine frequency range:** 1 uHz -- 20 MHz (33500B) / 80 MHz (33600A)
- **Square frequency range:** 1 uHz -- 20 MHz (33500B) / 80 MHz (33600A)
- **Amplitude range (50 Ohm):** 1 mVpp -- 10 Vpp
- **Amplitude range (High-Z):** 2 mVpp -- 20 Vpp
- **Resolution:** 16-bit, 250 MSa/s (33500B) / 1 GSa/s (33600A)
- **Memory depth:** 1 MSa standard (16 MSa optional)
- **Waveform types:** Sine, Square, Triangle, Ramp, Pulse, PRBS, Noise, Arb, DC
- **Sweep:** Linear and logarithmic, auto/bus/external trigger
- **Interfaces:** USB-TMC, LAN (LXI, port 5025), GPIB

### 2.3 Tektronix AFG1022 / AFG3000C Series

- **Channels:** 2
- **Sine frequency range:** 1 uHz -- 25 MHz (AFG1022) / 50 MHz (AFG3052C)
- **Square frequency range:** 1 uHz -- 25 MHz (AFG1022) / 50 MHz (AFG3052C)
- **Amplitude range (50 Ohm):** 1 mVpp -- 10 Vpp
- **Amplitude range (High-Z):** 2 mVpp -- 20 Vpp
- **Resolution:** 14-bit, 125 MSa/s (AFG1022) / 1 GSa/s (AFG3000C)
- **Memory depth:** 8,192 points (AFG1022) / 128 Kpts (AFG3000C)
- **Waveform types:** Sine, Square, Pulse, Ramp, Triangle, Noise, Sinc, Exp Rise/Fall, Gaussian, Lorentzian, Haversine, DC, Arb
- **Sweep:** Linear and logarithmic on channel 1 only (AFG1022); both channels (AFG3000C)
- **Interfaces:** USB-TMC, LAN (LXI, port 5025)
- **Note:** AFG1022 supports sweep/modulation on channel 1 only

---

## 3. Common SCPI Command Reference

Commands below use `[n]` for the channel number (1 or 2). When omitted, channel 1
is assumed. SCPI short-form abbreviations are shown in UPPER case; the full long-form
keyword follows in lower case. Both forms are accepted.

### 3.1 Identification and Reset

These commands are identical across all three vendors (IEEE 488.2 mandatory).

| Command   | Description                    | Example Response                                        |
|-----------|--------------------------------|---------------------------------------------------------|
| `*IDN?`   | Query instrument identity      | `RIGOL TECHNOLOGIES,DG1062Z,DG1ZA12345678,00.01.12`    |
| `*RST`    | Reset to factory defaults      | (no response)                                           |
| `*CLS`    | Clear status registers/errors  | (no response)                                           |
| `*OPC?`   | Operation complete query       | `1`                                                     |
| `*OPC`    | Set OPC bit when done          | (no response)                                           |
| `*WAI`    | Wait for pending operations    | (no response)                                           |

### 3.2 Frequency Control

| Vendor     | Set Command                                     | Query Command                              |
|------------|--------------------------------------------------|--------------------------------------------|
| Rigol      | `:SOURce[n]:FREQuency[:FIXed] <value>`           | `:SOURce[n]:FREQuency[:FIXed]?`            |
| Keysight   | `SOURce[n]:FREQuency <value>`                    | `SOURce[n]:FREQuency?`                     |
| Tektronix  | `SOURce[n]:FREQuency[:CW\|FIXed] <value>`        | `SOURce[n]:FREQuency[:CW\|FIXed]?`         |

**Parameter:** Frequency in Hz. Scientific notation accepted (e.g., `1.5E6` for 1.5 MHz).

**Units suffix:** Optional -- `HZ`, `KHZ`, `MHZ` (e.g., `:SOUR1:FREQ 5MHZ`).

**Examples:**
```
:SOUR1:FREQ 1000000        -- Set channel 1 to 1 MHz
:SOUR1:FREQ 5.5E6          -- Set channel 1 to 5.5 MHz
:SOUR1:FREQ?               -- Query -> "1.000000E+06"
```

**Common subset:** All three vendors accept `:SOURce[n]:FREQuency <value>` and
`:SOURce[n]:FREQuency?`. The `:FIXed` / `:CW` sub-keywords are optional synonyms
and can be omitted for cross-vendor compatibility.

### 3.3 Amplitude Control

| Vendor     | Set Command                                                    | Query Command                                     |
|------------|----------------------------------------------------------------|----------------------------------------------------|
| Rigol      | `:SOURce[n]:VOLTage[:LEVel][:IMMediate][:AMPLitude] <value>`  | `:SOURce[n]:VOLTage[:LEVel][:IMMediate][:AMPLitude]?` |
| Keysight   | `SOURce[n]:VOLTage[:LEVel][:IMMediate][:AMPLitude] <value>`   | `SOURce[n]:VOLTage[:LEVel][:IMMediate][:AMPLitude]?`  |
| Tektronix  | `SOURce[n]:VOLTage[:LEVel][:IMMediate][:AMPLitude] <value>`   | `SOURce[n]:VOLTage[:LEVel][:IMMediate][:AMPLitude]?`  |

**Parameter:** Amplitude in Vpp (default). All optional sub-keywords can be omitted.

**Shorthand accepted by all:** `:SOUR[n]:VOLT <value>` and `:SOUR[n]:VOLT?`

**Additional amplitude commands (all vendors):**

| Command                                     | Description                      |
|---------------------------------------------|----------------------------------|
| `:SOURce[n]:VOLTage:OFFSet <value>`         | Set DC offset in volts           |
| `:SOURce[n]:VOLTage:OFFSet?`                | Query DC offset                  |
| `:SOURce[n]:VOLTage:HIGH <value>`           | Set high voltage level           |
| `:SOURce[n]:VOLTage:LOW <value>`            | Set low voltage level            |
| `:SOURce[n]:VOLTage:UNIT {VPP\|VRMS\|DBM}` | Set amplitude unit (Rigol/Keysight) |

**Examples:**
```
:SOUR1:VOLT 2.5             -- Set channel 1 amplitude to 2.5 Vpp
:SOUR1:VOLT?                -- Query -> "2.500000E+00"
:SOUR1:VOLT:OFFS 0.5        -- Set DC offset to 0.5 V
```

**Common subset:** `:SOURce[n]:VOLTage <value>` works identically across all three vendors.
The `UNIT` sub-command varies slightly but is not needed for the MWA use case (Vpp only).

### 3.4 Waveform Selection

| Vendor     | Set Command                                 | Query Command                          | Enum Values                                                     |
|------------|---------------------------------------------|----------------------------------------|-----------------------------------------------------------------|
| Rigol      | `:SOURce[n]:FUNCtion[:SHAPe] {enum}`        | `:SOURce[n]:FUNCtion[:SHAPe]?`         | `SINusoid\|SQUare\|RAMP\|PULSe\|NOISe\|USER\|HARMonic\|DC\|KAISER\|...` |
| Keysight   | `SOURce[n]:FUNCtion[:SHAPe] {enum}`         | `SOURce[n]:FUNCtion[:SHAPe]?`          | `SIN\|SQU\|TRI\|RAMP\|PULS\|PRBS\|NOIS\|ARB\|DC`              |
| Tektronix  | `SOURce[n]:FUNCtion[:SHAPe] {enum}`         | `SOURce[n]:FUNCtion[:SHAPe]?`          | `SINusoid\|SQUare\|PULSe\|RAMP\|NOISe\|DC\|SINC\|GAUSsian\|LORentz\|...` |

**CRITICAL VENDOR DIFFERENCE -- Waveform Enumerations:**

| MWA Waveform | Rigol        | Keysight | Tektronix    |
|--------------|--------------|----------|--------------|
| Sine         | `SIN`        | `SIN`    | `SIN`        |
| Square       | `SQU`        | `SQU`    | `SQU`        |
| Triangle     | `RAMP` *     | `TRI`    | `RAMP` *     |

> \* **Important:** Rigol and Tektronix do not have a dedicated `TRI` (triangle)
> waveform enum in the same way as Keysight. On Rigol, use `RAMP` with 50% symmetry
> to produce a triangle wave. On Tektronix AFG1022, triangle is also implemented
> as a ramp with 50% symmetry. The driver must apply the appropriate symmetry setting
> when the MWA interface requests a triangle waveform on Rigol/Tektronix instruments.

**Query response formats differ:**

| Vendor     | Response to `:SOUR1:FUNC?` when set to sine |
|------------|----------------------------------------------|
| Rigol      | `SIN`                                        |
| Keysight   | `SIN`                                        |
| Tektronix  | `SIN`                                        |

**Examples:**
```
:SOUR1:FUNC SIN              -- Set channel 1 to sine wave
:SOUR1:FUNC SQU              -- Set channel 1 to square wave
:SOUR1:FUNC?                 -- Query -> "SIN"
```

**Common subset:** `:SOURce[n]:FUNCtion {SIN|SQU}` works identically. Triangle requires
vendor-specific branching (see table above).

### 3.5 Output Enable/Disable

| Vendor     | Set Command                        | Query Command              |
|------------|------------------------------------|----------------------------|
| Rigol      | `:OUTPut[n][:STATe] {ON\|OFF\|1\|0}` | `:OUTPut[n][:STATe]?`     |
| Keysight   | `OUTPut[n][:STATe] {ON\|OFF\|1\|0}`  | `OUTPut[n][:STATe]?`      |
| Tektronix  | `OUTPut[n][:STATe] {ON\|OFF\|1\|0}`  | `OUTPut[n][:STATe]?`      |

**Query response:**

| Vendor     | Response when output is ON | Response when output is OFF |
|------------|----------------------------|-----------------------------|
| Rigol      | `ON` or `1`                | `OFF` or `0`                |
| Keysight   | `1`                        | `0`                         |
| Tektronix  | `1`                        | `0`                         |

**Implementation note:** The driver should accept both numeric (`1`/`0`) and string
(`ON`/`OFF`) responses when parsing output state queries for robustness.

**Additional output commands:**

| Command                                     | Description                         |
|---------------------------------------------|-------------------------------------|
| `:OUTPut[n]:IMPedance {50\|INFinity}`       | Set output impedance (50 Ohm or Hi-Z) |
| `:OUTPut[n]:IMPedance?`                     | Query output impedance              |
| `:OUTPut[n]:POLarity {NORMal\|INVerted}`    | Set output polarity                 |

**Examples:**
```
:OUTP1 ON                    -- Enable channel 1 output
:OUTP1 OFF                   -- Disable channel 1 output
:OUTP1?                      -- Query -> "1" or "ON"
:OUTP1:IMP 50                -- Set 50 Ohm output impedance
```

**Common subset:** `:OUTPut[n] {ON|OFF}` and `:OUTPut[n]?` work identically across all vendors.

### 3.6 Sweep Configuration

Sweep configuration has the most vendor-specific variation. All three vendors support
linear and logarithmic frequency sweeps, but the command trees differ significantly.

#### 3.6.1 Rigol DG1000Z Sweep Commands

```
:SOURce[n]:SWEep:STATe {ON|OFF}          -- Enable/disable sweep mode
:SOURce[n]:FREQuency:STARt <value>        -- Set sweep start frequency (Hz)
:SOURce[n]:FREQuency:STOP <value>         -- Set sweep stop frequency (Hz)
:SOURce[n]:SWEep:SPACing {LINear|LOGarithmic}  -- Sweep type
:SOURce[n]:SWEep:TIME <seconds>           -- Sweep time
:SOURce[n]:SWEep:HTIMe <seconds>          -- Hold time at stop frequency
:SOURce[n]:SWEep:RTIMe <seconds>          -- Return time from stop to start
:SOURce[n]:SWEep:TRIGger:SOURce {INTernal|EXTernal|MANual}
```

#### 3.6.2 Keysight 33500B Sweep Commands

```
SOURce[n]:FREQuency:MODE {CW|SWEep|LIST}  -- Select frequency mode
SOURce[n]:FREQuency:STARt <value>          -- Set sweep start frequency (Hz)
SOURce[n]:FREQuency:STOP <value>           -- Set sweep stop frequency (Hz)
SOURce[n]:SWEep:SPACing {LINear|LOGarithmic}  -- Sweep type
SOURce[n]:SWEep:TIME <seconds>             -- Sweep time
SOURce[n]:SWEep:HTIMe <seconds>            -- Hold time at stop frequency
SOURce[n]:SWEep:RTIMe <seconds>            -- Return time from stop to start
TRIGger[n]:SOURce {IMMediate|EXTernal|BUS} -- Trigger source
```

**Key Keysight difference:** Sweep is activated by setting `FREQ:MODE SWEep` rather
than a dedicated `SWEep:STATe` command.

#### 3.6.3 Tektronix AFG1022 / AFG3000 Sweep Commands

```
SOURce[n]:SWEep:STATe {ON|OFF}            -- Enable/disable sweep mode
SOURce[n]:FREQuency:STARt <value>          -- Set sweep start frequency (Hz)
SOURce[n]:FREQuency:STOP <value>           -- Set sweep stop frequency (Hz)
SOURce[n]:SWEep:SPACing {LINear|LOGarithmic}  -- Sweep type (AFG3000)
SOURce[n]:SWEep:TIME <seconds>             -- Sweep time
SOURce[n]:SWEep:HTIMe <seconds>            -- Hold time at stop frequency
SOURce[n]:SWEep:RTIMe <seconds>            -- Return time from stop to start
SOURce[n]:SWEep:MODE {AUTO|MANual}         -- Trigger mode
```

**Key Tektronix limitation:** On AFG1022, sweep is available on channel 1 only.

#### 3.6.4 Sweep Configuration Summary

| Action                 | Rigol                              | Keysight                           | Tektronix                        |
|------------------------|------------------------------------|-------------------------------------|----------------------------------|
| Enable sweep           | `:SOUR:SWE:STAT ON`               | `SOUR:FREQ:MODE SWE`              | `SOUR:SWE:STAT ON`              |
| Disable sweep          | `:SOUR:SWE:STAT OFF`              | `SOUR:FREQ:MODE CW`               | `SOUR:SWE:STAT OFF`             |
| Start frequency        | `:SOUR:FREQ:STAR <Hz>`            | `SOUR:FREQ:STAR <Hz>`             | `SOUR:FREQ:STAR <Hz>`           |
| Stop frequency         | `:SOUR:FREQ:STOP <Hz>`            | `SOUR:FREQ:STOP <Hz>`             | `SOUR:FREQ:STOP <Hz>`           |
| Sweep spacing          | `:SOUR:SWE:SPAC {LIN\|LOG}`      | `SOUR:SWE:SPAC {LIN\|LOG}`        | `SOUR:SWE:SPAC {LIN\|LOG}`      |
| Sweep time             | `:SOUR:SWE:TIME <s>`              | `SOUR:SWE:TIME <s>`               | `SOUR:SWE:TIME <s>`             |
| Trigger source         | `:SOUR:SWE:TRIG:SOUR {INT\|EXT}` | `TRIG:SOUR {IMM\|EXT\|BUS}`       | `SOUR:SWE:MODE {AUTO\|MAN}`     |
| Query sweep state      | `:SOUR:SWE:STAT?`                 | `SOUR:FREQ:MODE?` (returns `SWE`) | `SOUR:SWE:STAT?`                |

---

## 4. Vendor Differences Table

This table summarizes all commands needed by the MWA driver and highlights where
vendor-specific branching is required.

| Function              | Command (Common)                      | Rigol       | Keysight    | Tektronix   | Branching Needed? |
|-----------------------|---------------------------------------|-------------|-------------|-------------|-------------------|
| Identify              | `*IDN?`                               | Same        | Same        | Same        | No                |
| Reset                 | `*RST`                                | Same        | Same        | Same        | No                |
| Set frequency         | `:SOUR[n]:FREQ <Hz>`                 | Same        | Same        | Same        | No                |
| Query frequency       | `:SOUR[n]:FREQ?`                     | Same        | Same        | Same        | No                |
| Set amplitude (Vpp)   | `:SOUR[n]:VOLT <Vpp>`               | Same        | Same        | Same        | No                |
| Query amplitude       | `:SOUR[n]:VOLT?`                     | Same        | Same        | Same        | No                |
| Set waveform (Sine)   | `:SOUR[n]:FUNC SIN`                 | Same        | Same        | Same        | No                |
| Set waveform (Square) | `:SOUR[n]:FUNC SQU`                 | Same        | Same        | Same        | No                |
| Set waveform (Tri)    | `:SOUR[n]:FUNC ...`                 | `RAMP`+sym  | `TRI`       | `RAMP`+sym  | **YES**           |
| Query waveform        | `:SOUR[n]:FUNC?`                     | Same        | Same        | Same        | No                |
| Output ON             | `:OUTP[n] ON`                        | Same        | Same        | Same        | No                |
| Output OFF            | `:OUTP[n] OFF`                       | Same        | Same        | Same        | No                |
| Query output state    | `:OUTP[n]?`                          | `ON`/`OFF`  | `1`/`0`     | `1`/`0`     | **YES** (parse)   |
| Enable sweep          | *(see below)*                        | `SWE:STAT`  | `FREQ:MODE` | `SWE:STAT`  | **YES**           |
| Set sweep start freq  | `:SOUR[n]:FREQ:STAR <Hz>`           | Same        | Same        | Same        | No                |
| Set sweep stop freq   | `:SOUR[n]:FREQ:STOP <Hz>`           | Same        | Same        | Same        | No                |
| Sweep spacing         | `:SOUR[n]:SWE:SPAC {LIN\|LOG}`     | Same        | Same        | Same        | No                |
| Sweep time            | `:SOUR[n]:SWE:TIME <s>`             | Same        | Same        | Same        | No                |
| System error query    | `:SYST:ERR?`                         | Same        | Same        | Same        | No                |

**Summary:** Only 3 areas require vendor-specific branching:
1. Triangle waveform (Rigol/Tektronix use RAMP + symmetry)
2. Output state query parsing (Rigol returns ON/OFF, Keysight/Tektronix return 1/0)
3. Sweep enable/disable mechanism

---

## 5. Connection Methods

### 5.1 USB-TMC

USB-TMC (USB Test & Measurement Class) is the most common connection method.

**VISA Resource String Format:**
```
USB0::0x<VID>::0x<PID>::<SERIAL>::INSTR
```

| Vendor     | USB Vendor ID (VID) | Typical PID     | Example VISA String                              |
|------------|---------------------|-----------------|--------------------------------------------------|
| Rigol      | `0x1AB1`            | `0x0641` (DG1Z) | `USB0::0x1AB1::0x0641::DG1ZA12345678::INSTR`    |
| Keysight   | `0x0957`            | `0x2C07` (335x) | `USB0::0x0957::0x2C07::MY12345678::INSTR`        |
| Tektronix  | `0x0699`            | `0x0353` (AFG1) | `USB0::0x0699::0x0353::C012345::INSTR`            |

**Driver requirements:**
- Windows: NI-VISA or Keysight IO Libraries provide USB-TMC driver
- macOS: NI-VISA for macOS or libusb-based backends
- The instrument appears as a USB-TMC device; no baud rate configuration needed

### 5.2 Serial (RS-232)

Only some models include RS-232. The Keysight 33500B and Rigol DG1062Z have optional
serial interfaces; the Tektronix AFG1022 does not have RS-232.

**VISA Resource String Format:**
```
ASRL<port>::INSTR
```

**Common serial settings:**

| Parameter      | Rigol DG1062Z    | Keysight 33500B  | Tektronix AFG1022 |
|----------------|------------------|------------------|--------------------|
| Baud rate      | 9600 (default)   | 9600 (default)   | N/A (no RS-232)   |
| Data bits      | 8                | 8                | N/A                |
| Stop bits      | 1                | 1                | N/A                |
| Parity         | None             | None             | N/A                |
| Flow control   | None             | None (DTR/DSR opt.) | N/A             |
| Terminator TX  | `\n` (LF)       | `\n` (LF)       | N/A                |
| Terminator RX  | `\n` (LF)       | `\n` (LF)       | N/A                |

**Note:** For MWA, USB-TMC or LAN/TCP are preferred over RS-232 due to higher throughput
and simpler auto-detection.

### 5.3 TCP/IP (LXI)

All three vendors support LAN connectivity via the LXI (LAN eXtensions for
Instrumentation) standard.

**VISA Resource String Format:**
```
TCPIP0::<IP_ADDRESS>::<PORT>::SOCKET
```

**Default SCPI socket port:** `5025` (standard across all LXI instruments)

| Vendor     | Default Port | mDNS/Discovery         | Example VISA String                         |
|------------|-------------|------------------------|----------------------------------------------|
| Rigol      | 5025        | Yes (LXI discovery)    | `TCPIP0::192.168.1.100::5025::SOCKET`       |
| Keysight   | 5025        | Yes (LXI discovery)    | `TCPIP0::192.168.1.101::5025::SOCKET`       |
| Tektronix  | 5025        | Yes (LXI discovery)    | `TCPIP0::192.168.1.102::5025::SOCKET`       |

**VXI-11 alternative (RPC-based):**
```
TCPIP0::<IP_ADDRESS>::INSTR
```

**TCP/IP connection details:**
- Protocol: Raw TCP (SCPI over socket) on port 5025
- Terminator: `\n` (LF, 0x0A)
- No authentication required
- Timeout recommended: 5000 ms for commands, 30000 ms for `*RST`

---

## 6. Parameter Ranges

### 6.1 Frequency Ranges by Model

| Model           | Sine Max  | Square Max | Ramp Max  | Pulse Max | Arb Max    |
|-----------------|-----------|------------|-----------|-----------|------------|
| Rigol DG1032Z   | 30 MHz    | 15 MHz     | 1 MHz     | 10 MHz    | 10 MHz     |
| Rigol DG1062Z   | 60 MHz    | 25 MHz     | 1 MHz     | 25 MHz    | 10 MHz     |
| Keysight 33511B | 20 MHz    | 20 MHz     | 200 kHz   | 20 MHz    | 20 MHz     |
| Keysight 33512B | 20 MHz    | 20 MHz     | 200 kHz   | 20 MHz    | 20 MHz     |
| Keysight 33522B | 30 MHz    | 30 MHz     | 200 kHz   | 30 MHz    | 30 MHz     |
| Tek AFG1022     | 25 MHz    | 25 MHz     | 500 kHz   | 12.5 MHz  | 10 MHz     |
| Tek AFG3052C    | 50 MHz    | 50 MHz     | 500 kHz   | 25 MHz    | 50 MHz     |

**Minimum frequency (all models):** 1 uHz

### 6.2 Amplitude Ranges by Model

| Model           | Min (50 Ohm) | Max (50 Ohm) | Min (Hi-Z) | Max (Hi-Z) |
|-----------------|--------------|--------------|------------|------------|
| Rigol DG1062Z   | 1 mVpp       | 10 Vpp       | 2 mVpp     | 20 Vpp     |
| Keysight 33512B | 1 mVpp       | 10 Vpp       | 2 mVpp     | 20 Vpp     |
| Tek AFG1022     | 1 mVpp       | 10 Vpp       | 2 mVpp     | 20 Vpp     |

**Note:** Maximum amplitude may be derated at higher frequencies. At 60 MHz on the
DG1062Z, the maximum amplitude is approximately 2.5 Vpp (50 Ohm). Always query
the instrument after setting amplitude to confirm the applied value.

### 6.3 DC Offset Ranges

| Model           | Max Offset (50 Ohm) | Max Offset (Hi-Z) |
|-----------------|---------------------|--------------------|
| Rigol DG1062Z   | +/- 5 V (combined with amplitude) | +/- 10 V  |
| Keysight 33512B | +/- 5 V             | +/- 10 V           |
| Tek AFG1022     | +/- 5 V             | +/- 10 V           |

**Constraint:** `|Offset| + Amplitude/2 <= Vmax` where Vmax is 5 V (50 Ohm) or 10 V (Hi-Z).

---

## 7. Error Handling

### 7.1 System Error Query

All vendors use the standard SCPI error queue:

```
:SYST:ERR?
```

**Response format:** `<error_code>,"<error_message>"`

**Example responses:**
```
0,"No error"                           -- Queue empty
-100,"Command error"                   -- Syntax error in command
-200,"Execution error"                 -- Valid command cannot execute
-222,"Data out of range"               -- Parameter out of range
-410,"Query INTERRUPTED"               -- Query interrupted
```

### 7.2 Standard SCPI Error Code Ranges

| Range        | Category             | Description                                     |
|--------------|----------------------|-------------------------------------------------|
| 0            | No error             | Error queue empty                               |
| -100 to -199 | Command errors       | Syntax errors detected by parser                |
| -200 to -299 | Execution errors     | Valid command could not execute                  |
| -300 to -399 | Device-specific      | Hardware or device condition errors              |
| -400 to -499 | Query errors         | Output queue or data flow errors                |
| 1 to 32767   | Device-dependent     | Vendor-specific positive error codes             |

### 7.3 Common Error Codes Relevant to MWA

| Code  | Message                    | Typical Cause                               |
|-------|----------------------------|---------------------------------------------|
| 0     | `No error`                 | Queue empty                                 |
| -100  | `Command error`            | Unrecognized command                        |
| -102  | `Syntax error`             | Malformed command string                    |
| -109  | `Missing parameter`        | Required parameter omitted                  |
| -113  | `Undefined header`         | Unknown SCPI header keyword                 |
| -200  | `Execution error`          | Generic execution failure                   |
| -222  | `Data out of range`        | Parameter value outside instrument limits   |
| -224  | `Illegal parameter value`  | Invalid enum value for parameter            |
| -350  | `Queue overflow`           | Too many unread errors in queue              |

### 7.4 Error Handling Strategy

The MWA driver should implement the following error check sequence after every command:

```
1. Send SCPI command (e.g., ":SOUR1:FREQ 5E6")
2. Send ":SYST:ERR?"
3. Read response
4. If response != '0,"No error"':
   a. Log the error code and message
   b. Continue reading ":SYST:ERR?" until '0,"No error"' to drain queue
   c. Report error to the application layer
5. For set commands, optionally query back (e.g., ":SOUR1:FREQ?") to verify
```

For performance-critical paths (e.g., rapid sweep parameter changes), error checking
can be deferred by using `*OPC?` after a batch of commands instead of checking after
each individual command.

---

## 8. Sample Responses (for Mock)

These responses should be used by `MockSignalGeneratorController` for unit testing
and simulation. They match actual instrument response formats.

### 8.1 Identification Responses

```
*IDN? -> "RIGOL TECHNOLOGIES,DG1062Z,DG1ZA12345678,00.01.12"
*IDN? -> "Keysight Technologies,33512B,MY12345678,1.07-1.00-43-00"
*IDN? -> "TEKTRONIX,AFG1022,C012345,SCPI:99.0 FV:V1.3.2"
```

**IDN format:** `<Manufacturer>,<Model>,<Serial>,<Firmware>`

### 8.2 Frequency Responses

```
:SOUR1:FREQ? -> "1.000000E+06"          -- 1 MHz
:SOUR1:FREQ? -> "5.500000E+06"          -- 5.5 MHz
:SOUR1:FREQ? -> "1.000000E+03"          -- 1 kHz
```

### 8.3 Amplitude Responses

```
:SOUR1:VOLT? -> "2.500000E+00"          -- 2.5 Vpp
:SOUR1:VOLT? -> "1.000000E-01"          -- 0.1 Vpp
:SOUR1:VOLT? -> "1.000000E+01"          -- 10.0 Vpp
:SOUR1:VOLT:OFFS? -> "0.000000E+00"     -- 0 V offset
```

### 8.4 Waveform Responses

```
:SOUR1:FUNC? -> "SIN"                   -- Sine wave
:SOUR1:FUNC? -> "SQU"                   -- Square wave
:SOUR1:FUNC? -> "RAMP"                  -- Ramp (triangle on Rigol/Tek)
:SOUR1:FUNC? -> "TRI"                   -- Triangle (Keysight only)
```

### 8.5 Output State Responses

```
:OUTP1? -> "ON"                          -- Rigol style
:OUTP1? -> "1"                           -- Keysight/Tektronix style
:OUTP1? -> "OFF"                         -- Rigol style
:OUTP1? -> "0"                           -- Keysight/Tektronix style
```

### 8.6 Sweep Responses

```
:SOUR1:SWE:STAT? -> "ON"                -- Rigol/Tektronix sweep active
:SOUR1:FREQ:MODE? -> "SWE"              -- Keysight sweep active
:SOUR1:FREQ:STAR? -> "1.000000E+06"     -- 1 MHz start
:SOUR1:FREQ:STOP? -> "1.000000E+07"     -- 10 MHz stop
:SOUR1:SWE:TIME? -> "1.000000E+00"      -- 1 second sweep time
:SOUR1:SWE:SPAC? -> "LIN"               -- Linear spacing
```

### 8.7 Error Responses

```
:SYST:ERR? -> '0,"No error"'
:SYST:ERR? -> '-222,"Data out of range"'
:SYST:ERR? -> '-100,"Command error"'
```

---

## 9. MWA Interface Mapping

This section maps each method of `SignalGeneratorControllerInterface` (defined in
`src/hardware/signal_generator/signal_generator_controller_interface.h`) to the
corresponding SCPI commands.

### 9.1 Method-to-SCPI Mapping Table

| MWA Interface Method              | SCPI Set Command(s)                    | SCPI Query Command(s)        | Notes                                             |
|-----------------------------------|----------------------------------------|------------------------------|---------------------------------------------------|
| `setFrequency(double hz)`         | `:SOUR1:FREQ <hz>`                    | `:SOUR1:FREQ?` (verify)     | Value in Hz, scientific notation                  |
| `frequency() const`               | --                                     | `:SOUR1:FREQ?`              | Parse response as double                          |
| `setAmplitude(double volts)`      | `:SOUR1:VOLT <volts>`                 | `:SOUR1:VOLT?` (verify)     | Value in Vpp                                      |
| `amplitude() const`               | --                                     | `:SOUR1:VOLT?`              | Parse response as double                          |
| `setWaveform(Waveform::kSine)`    | `:SOUR1:FUNC SIN`                     | `:SOUR1:FUNC?` (verify)     | Common across all vendors                         |
| `setWaveform(Waveform::kSquare)`  | `:SOUR1:FUNC SQU`                     | `:SOUR1:FUNC?` (verify)     | Common across all vendors                         |
| `setWaveform(Waveform::kTriangle)`| `:SOUR1:FUNC TRI` (Keysight)          | `:SOUR1:FUNC?` (verify)     | **Vendor-specific** -- see section 3.4            |
|                                   | `:SOUR1:FUNC RAMP` + symmetry (Rigol/Tek) |                          |                                                   |
| `waveform() const`                | --                                     | `:SOUR1:FUNC?`              | Map `SIN`/`SQU`/`TRI`/`RAMP` back to enum        |
| `setOutputEnabled(true)`          | `:OUTP1 ON`                            | `:OUTP1?` (verify)          | Common across all vendors                         |
| `setOutputEnabled(false)`         | `:OUTP1 OFF`                           | `:OUTP1?` (verify)          | Common across all vendors                         |
| `isOutputEnabled() const`         | --                                     | `:OUTP1?`                   | Parse `ON`/`OFF`/`1`/`0` -- vendor differs        |
| `configureSweep(start, stop, step)`| *(sequence below)*                    | --                           | Requires multiple SCPI commands                   |

### 9.2 Sweep Configuration Sequence

The `configureSweep(double start_hz, double stop_hz, double step_hz)` method requires
a multi-command sequence because SCPI does not have a single sweep-configuration command.

The `step_hz` parameter from the MWA interface must be converted to a sweep time.
The calculation is:

```
sweep_time = (stop_hz - start_hz) / step_hz * dwell_time_per_step
```

Since SCPI sweep is time-based (not step-based), the driver should compute an appropriate
sweep time. For linear sweeps, a reasonable default dwell time per step could be derived
from the instrument's minimum sweep time constraints.

**Rigol/Tektronix sequence:**
```
:SOUR1:SWE:STAT OFF          -- Disable sweep while configuring
:SOUR1:FUNC SIN              -- Set waveform (sweep requires a base waveform)
:SOUR1:FREQ:STAR <start_hz>  -- Set start frequency
:SOUR1:FREQ:STOP <stop_hz>   -- Set stop frequency
:SOUR1:SWE:SPAC LIN          -- Linear sweep
:SOUR1:SWE:TIME <seconds>    -- Computed sweep time
:SOUR1:SWE:STAT ON           -- Enable sweep
```

**Keysight sequence:**
```
SOUR1:FREQ:MODE CW            -- Ensure CW mode while configuring
SOUR1:FUNC SIN                -- Set waveform
SOUR1:FREQ:STAR <start_hz>    -- Set start frequency
SOUR1:FREQ:STOP <stop_hz>     -- Set stop frequency
SOUR1:SWE:SPAC LIN            -- Linear sweep
SOUR1:SWE:TIME <seconds>      -- Computed sweep time
TRIG1:SOUR IMM                -- Internal (immediate) trigger
SOUR1:FREQ:MODE SWE           -- Activate sweep mode
```

### 9.3 Auto-Detection via *IDN?

The driver should auto-detect the instrument vendor by parsing the `*IDN?` response:

```cpp
// Parse *IDN? response: "<Manufacturer>,<Model>,<Serial>,<Firmware>"
QString idn_response = sendQuery("*IDN?");
QStringList parts = idn_response.split(",");
if (parts.size() >= 2) {
  QString manufacturer = parts[0].trimmed().toUpper();
  QString model = parts[1].trimmed().toUpper();
  if (manufacturer.contains("RIGOL"))
    vendor_ = Vendor::kRigol;
  else if (manufacturer.contains("KEYSIGHT") || manufacturer.contains("AGILENT"))
    vendor_ = Vendor::kKeysight;
  else if (manufacturer.contains("TEKTRONIX"))
    vendor_ = Vendor::kTektronix;
}
```

### 9.4 Recommended Initialization Sequence

When connecting to a signal generator, execute the following sequence:

```
*RST                          -- Reset to known state
*CLS                          -- Clear error queue
*IDN?                         -- Identify vendor/model for branching
:SYST:ERR?                    -- Drain any startup errors
:OUTP1 OFF                    -- Ensure output is OFF (safety)
:SOUR1:FUNC SIN               -- Default waveform
:SOUR1:FREQ 1E6               -- Default 1 MHz
:SOUR1:VOLT 1.0               -- Default 1 Vpp
:OUTP1:IMP 50                 -- Set 50 Ohm impedance
```

---

## 10. References

### 10.1 Official Programming Guides

- **Rigol DG1000Z Programming Guide:**
  https://www.batronix.com/pdf/Rigol/ProgrammingGuide/DG1000Z_ProgrammingGuide_EN.pdf

- **Rigol DG800 Programming Guide:**
  https://www.manualslib.com/manual/2260873/Rigol-Dg800-Series.html

- **Keysight 33500B/33600A Operating and Service Guide:**
  https://www.keysight.com/us/en/assets/9018-03714/service-manuals/9018-03714.pdf

- **Keysight 33500 Series User's Guide:**
  https://www.keysight.com/us/en/assets/9018-03290/user-manuals/9018-03290.pdf

- **Tektronix AFG1022 Programmer Manual:**
  https://download.tek.com/manual/AFG1022-Programmer-Manual-EN.pdf

- **Tektronix AFG3000 Series Programmer Manual:**
  https://download.tek.com/manual/AFG3000-Series-Arbitrary-Function-Generator-Programmer-EN.pdf

### 10.2 Data Sheets

- **Rigol DG1062Z:**
  https://www.batronix.com/shop/waveform-generator/Rigol-DG1062Z.html

- **Keysight 33500B/33600A Data Sheet:**
  https://www.keysight.com/us/en/assets/7018-05928/data-sheets/5992-2572.pdf

- **Tektronix AFG1022 Data Sheet:**
  https://www.mouser.com/datasheet/2/403/AFG1022-Arbitrary-Function-Generator-Datasheet-1-540840.pdf

### 10.3 SCPI Standards

- **SCPI-1999 Standard:** https://www.ivifoundation.org/scpi/

- **IEEE 488.2 Standard:** Defines mandatory common commands (*IDN?, *RST, *CLS, etc.)

### 10.4 Community Resources

- **tektronix-func-gen Python library:**
  https://github.com/asvela/tektronix-func-gen

- **Rigol SCPI command discussion (EEVblog):**
  https://www.eevblog.com/forum/testgear/lists-of-rigol-scpi-commands/

- **Keysight PyArbTools:**
  https://github.com/morgan-at-keysight/pyarbtools
