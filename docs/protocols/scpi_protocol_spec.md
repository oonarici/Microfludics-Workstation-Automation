# SCPI Protocol Specification --- MWA Project

> **Version:** 1.0
> **Date:** 2026-03-27
> **Purpose:** Reference for implementing the `ScpiClient` class in the MWA application.
> **Standard:** SCPI-1999 (IVI Foundation), built on IEEE 488.2

---

## Table of Contents

1. [Overview](#1-overview)
2. [IEEE 488.2 Common Commands](#2-ieee-4882-common-commands)
3. [SCPI Command Syntax](#3-scpi-command-syntax)
4. [Parameter Types](#4-parameter-types)
5. [Response Formats](#5-response-formats)
6. [Error Handling Model](#6-error-handling-model)
7. [Transport Layers](#7-transport-layers)
8. [Signal Generator Command Overview](#8-signal-generator-command-overview)
9. [VNA Command Overview](#9-vna-command-overview)
10. [NI-VISA C API Reference](#10-ni-visa-c-api-reference)
11. [Cross-Platform Notes](#11-cross-platform-notes)
12. [References](#12-references)

---

## 1. Overview

**SCPI** (Standard Commands for Programmable Instruments, pronounced "skippy") is
an ASCII-based command language for controlling test and measurement instruments.
First released in 1990 by the SCPI Consortium, it is now maintained by the
IVI Foundation.

SCPI builds on **IEEE 488.2**, which defines:

- Common data formats and status reporting
- A set of mandatory common commands (prefixed with `*`)
- Controller/device protocol handshaking

SCPI adds:

- A hierarchical command tree organized by instrument function
- Standardized subsystem commands (e.g., `SOURce`, `SENSe`, `CALCulate`)
- A uniform error-handling model with a standard error queue
- Consistent parameter and response formats

**Key design principles:**

| Principle | Meaning |
|---|---|
| Forgiving listening | Instruments accept short or long form, any case |
| Precise talking | Instruments always respond in a canonical format |
| Transport independence | Same commands work over GPIB, USB, TCP/IP, serial |

### Relevance to MWA

The MWA application communicates with two SCPI-controlled instruments:

- **Signal Generator** (Sprint 9) --- waveform generation for microfluidic actuation
- **Vector Network Analyzer (VNA)** (Sprint 10) --- impedance measurement

Both share a common SCPI transport and parsing layer (`ScpiClient`), which this
document specifies.

---

## 2. IEEE 488.2 Common Commands

Common commands are defined by IEEE 488.2 and are recognized by all
SCPI-compliant instruments regardless of the current command tree position.
They are always prefixed with an asterisk (`*`).

### 2.1 Command Reference Table

| Command | Type | Description |
|---|---|---|
| `*IDN?` | Query | Returns instrument identification string |
| `*RST` | Command | Resets instrument to factory default state |
| `*CLS` | Command | Clears all status registers and the error queue |
| `*OPC` | Command | Sets bit 0 (OPC) in ESR when all pending operations complete |
| `*OPC?` | Query | Returns `1` when all pending operations complete |
| `*ESR?` | Query | Reads and clears the Standard Event Status Register |
| `*ESE <mask>` | Command | Sets the Standard Event Status Enable register |
| `*ESE?` | Query | Returns the Standard Event Status Enable register value |
| `*STB?` | Query | Returns the Status Byte Register (bits 0--5 and 7) |
| `*SRE <mask>` | Command | Sets the Service Request Enable register |
| `*SRE?` | Query | Returns the Service Request Enable register value |
| `*WAI` | Command | Waits until all pending operations complete before continuing |
| `*TST?` | Query | Executes self-test; returns `0` for pass, non-zero for fail |
| `*SAV <n>` | Command | Saves instrument state to memory location `n` |
| `*RCL <n>` | Command | Recalls instrument state from memory location `n` |

### 2.2 Detailed Command Descriptions

#### `*IDN?` --- Identification Query

Returns a comma-separated string with four fields:

```
<Manufacturer>,<Model>,<Serial Number>,<Firmware Version>
```

**Example:**

```
>> *IDN?
<< Keysight Technologies,N5182B,MY12345678,A.01.23
```

Response rules:
- Each field is ASCII, no commas within fields
- Total length must not exceed 255 characters
- Fields must not contain `LF`, `CR`, or semicolons

#### `*RST` --- Reset

Resets the instrument to a known, manufacturer-defined default state. Does
**not** affect:
- Communication settings (baud rate, IP address)
- Calibration data
- Stored states (`*SAV` registers)
- Status enable registers (`*ESE`, `*SRE`)

**Example:**

```
>> *RST
```

#### `*CLS` --- Clear Status

Clears all event registers and the error queue in a single operation:
- Standard Event Status Register (ESR) is cleared
- Operation Status Event Register is cleared
- Questionable Status Event Register is cleared
- Error/event queue is emptied
- Cancels any pending `*OPC` flag

**Example:**

```
>> *CLS
```

#### `*OPC` / `*OPC?` --- Operation Complete

Two forms with different behavior:

| Form | Behavior |
|---|---|
| `*OPC` (command) | Sets bit 0 of ESR when all pending overlapped operations complete |
| `*OPC?` (query) | Blocks until complete, then returns ASCII `1` |

**Typical synchronization pattern:**

```
>> INIT:IMM;*OPC?
<< 1
```

The controller sends a measurement trigger and waits for `1` before reading
results. This is the recommended synchronization approach for the `ScpiClient`.

#### `*ESR?` --- Event Status Register Query

Returns the value of the ESR as an NR1 integer (0--255), then clears the register.

**ESR bit definitions:**

| Bit | Weight | Name | Meaning |
|---|---|---|---|
| 7 | 128 | PON | Power-on since last read |
| 6 | 64 | URQ | User request (front-panel button) |
| 5 | 32 | CME | Command error detected |
| 4 | 16 | EXE | Execution error detected |
| 3 | 8 | DDE | Device-dependent error |
| 2 | 4 | QYE | Query error detected |
| 1 | 2 | RQC | Request control |
| 0 | 1 | OPC | Operation complete |

**Example:**

```
>> *ESR?
<< 32
```

Value `32` = bit 5 set = a command error occurred since last `*ESR?` or `*CLS`.

#### `*STB?` --- Status Byte Query

Returns the Status Byte Register as an NR1 integer (0--255). Does **not** clear
the register (unlike `*ESR?`).

**STB bit definitions:**

| Bit | Weight | Name | Meaning |
|---|---|---|---|
| 7 | 128 | OPER | Operation status summary |
| 6 | 64 | MSS/RQS | Master Status Summary / Request Service |
| 5 | 32 | ESB | Event Status Bit (ESR summary) |
| 4 | 16 | MAV | Message Available (data in output queue) |
| 3 | 8 | QUES | Questionable status summary |
| 2 | 4 | EAV | Error Available (error queue not empty) |
| 1 | 0 | --- | Not used (instrument-specific) |
| 0 | 0 | --- | Not used (instrument-specific) |

**Example:**

```
>> *STB?
<< 4
```

Value `4` = bit 2 set = errors are present in the error queue.

#### `*WAI` --- Wait-to-Continue

Prevents the instrument from processing subsequent commands until all pending
overlapped operations are complete. Unlike `*OPC?`, it does not return a
response --- the controller must poll or use a timeout.

**Example:**

```
>> INIT:IMM;*WAI;CALC:DATA?
```

#### `*TST?` --- Self-Test Query

Triggers an instrument self-test and returns the result as an NR1 integer:

| Value | Meaning |
|---|---|
| `0` | Self-test passed |
| Non-zero | Self-test failed (value is instrument-specific) |

**Example:**

```
>> *TST?
<< 0
```

Note: Self-test may take several seconds. Use an appropriate timeout.

---

## 3. SCPI Command Syntax

### 3.1 Keyword Hierarchy (Command Tree)

SCPI commands are organized in a tree structure, separated by colons. The tree
resembles a file system:

```
:SOURce
    :FREQuency
        [:CW]          -- default node (optional keyword)
        :STARt
        :STOP
    :VOLTage
        [:LEVel]
            [:IMMediate]
                [:AMPLitude]
    :FUNCtion
        [:SHAPe]
:SENSe
    :FREQuency
        :STARt
        :STOP
    :SWEep
        :POINts
:OUTPut
    [:STATe]
```

The leading colon resets to the root of the tree. A message terminator (`\n`)
also resets to root.

### 3.2 Short Form vs. Long Form

Every SCPI keyword has a **long form** (full spelling) and a **short form**
(minimum unambiguous abbreviation). The short form is indicated by uppercase
letters in documentation:

| Long Form | Short Form | Valid Inputs |
|---|---|---|
| `SOURce` | `SOUR` | `SOURCE`, `source`, `SOUR`, `sour` |
| `FREQuency` | `FREQ` | `FREQUENCY`, `frequency`, `FREQ`, `freq` |
| `VOLTage` | `VOLT` | `VOLTAGE`, `voltage`, `VOLT`, `volt` |
| `IMMediate` | `IMM` | `IMMEDIATE`, `immediate`, `IMM`, `imm` |
| `OUTPut` | `OUTP` | `OUTPUT`, `output`, `OUTP`, `outp` |
| `FUNCtion` | `FUNC` | `FUNCTION`, `function`, `FUNC`, `func` |

**Rules:**

- Commands are **case-insensitive** (forgiving listening)
- You must use either the exact short form or the exact long form
- Intermediate lengths are **invalid** (e.g., `SOURC` is wrong)

**Example --- equivalent commands:**

```
:SOURce:FREQuency:CW 1E6
:SOUR:FREQ:CW 1E6
:source:frequency:cw 1E6
:sour:freq:cw 1e6
```

All set the CW frequency to 1 MHz.

### 3.3 Query Suffix (`?`)

Appending `?` to a command converts it to a query. Queries return the current
setting or measured value:

```
>> :SOUR:FREQ?
<< 1.000000E+06
```

Query rules:
- Not all commands have a corresponding query (e.g., `*RST` has no `*RST?`)
- A query **must** be followed by reading the response before sending the next
  command, or a query error (-400) will occur
- Queries may accept parameters (e.g., `:SOUR:FREQ? MIN` returns the minimum
  allowed frequency)

### 3.4 Optional Keywords (`[ ]`)

Brackets indicate optional keywords. The instrument accepts the command with or
without the optional keyword:

```
:SOURce:VOLTage[:LEVel][:IMMediate][:AMPLitude] 1.5
```

All of these are equivalent:

```
:SOUR:VOLT:LEV:IMM:AMPL 1.5
:SOUR:VOLT 1.5
```

### 3.5 Numeric Suffixes for Channel Selection

Numeric suffixes select a specific channel, trace, or source. The suffix is
appended directly to the keyword with no separator:

```
:SOURce1:FREQuency 1E6       -- Channel 1 frequency
:SOURce2:FREQuency 2E6       -- Channel 2 frequency
:SENSe1:SWEep:POINts 201     -- Trace 1 sweep points
:CALCulate2:DATA?             -- Trace 2 data
```

Rules:
- Suffix `1` is the default and may be omitted (`:SOURce` = `:SOURce1`)
- Valid suffixes depend on the instrument (typically 1--4)
- The suffix is part of the keyword, not a parameter

### 3.6 Compound Commands (Semicolons)

Multiple commands on one line are separated by semicolons. The path is
**preserved** after a semicolon (relative addressing):

```
:SOUR:FREQ 1E6;:SOUR:VOLT 0.5
```

If the next command is within the same subtree, the colon prefix can be omitted:

```
:SOUR:FREQ 1E6;VOLT 0.5
```

Both forms set frequency to 1 MHz and voltage to 0.5 V on the default source.

A common command (prefixed with `*`) does not change the current path:

```
:SOUR:FREQ 1E6;*OPC;VOLT 0.5
```

### 3.7 Command Terminators

Every SCPI message must end with a terminator:

| Terminator | Description | Usage |
|---|---|---|
| `LF` (`\n`, `0x0A`) | Line Feed | Standard terminator (most common) |
| `CR+LF` (`\r\n`) | Carriage Return + Line Feed | Accepted by most instruments |
| `EOI` | End-Or-Identify (GPIB only) | Hardware signal on GPIB bus |

**For the ScpiClient implementation:**
- TCP/IP and serial: always append `\n` to commands
- USB-TMC: the VISA driver handles termination via the USBTMC protocol
- When reading responses: look for `\n` as the response terminator

### 3.8 Whitespace Rules

- **Required:** Exactly one space between the last keyword and the first
  parameter (e.g., `:FREQ 1E6`)
- **Optional:** Additional spaces are ignored
- **Forbidden:** No spaces within keywords or around colons
  (`:SOUR : FREQ` is invalid)

---

## 4. Parameter Types

### 4.1 Numeric Types

| Type | Name | Format | Examples |
|---|---|---|---|
| NR1 | Integer | `[+\|-]<digits>` | `0`, `+1`, `-42`, `12345` |
| NR2 | Fixed-point | `[+\|-]<digits>.<digits>` | `1.5`, `-0.001`, `+100.0` |
| NR3 | Scientific | `[+\|-]<digits>.<digits>E[+\|-]<digits>` | `1.5E+6`, `-3.14E-3`, `2.0E0` |
| NRf | Flexible | Any of NR1, NR2, or NR3 | Accepted on input; response uses a fixed type |

**Input (forgiving):** Instruments accept any NRf variant:

```
:SOUR:FREQ 1000000         -- NR1
:SOUR:FREQ 1000000.0       -- NR2
:SOUR:FREQ 1.0E6           -- NR3
:SOUR:FREQ 1E6             -- NR3 shorthand (also accepted)
```

**Output (precise):** Responses always use a consistent type, typically NR3:

```
>> :SOUR:FREQ?
<< 1.000000E+06
```

### 4.2 Numeric with Units

Some commands accept optional engineering unit suffixes:

| Suffix | Unit | Example |
|---|---|---|
| `HZ`, `KHZ`, `MHZ`, `GHZ` | Hertz | `:FREQ 1.5GHZ` |
| `V`, `MV`, `UV` | Volts | `:VOLT 500MV` |
| `DBM` | dBm | `:POW -10DBM` |
| `S`, `MS`, `US`, `NS` | Seconds | `:PULS:WIDT 100US` |
| `PCT` | Percent | `:VOLT:OFFS 10PCT` |

### 4.3 Special Numeric Keywords

These keywords can be used in place of a numeric value:

| Keyword | Short | Meaning |
|---|---|---|
| `MINimum` | `MIN` | Minimum allowed value |
| `MAXimum` | `MAX` | Maximum allowed value |
| `DEFault` | `DEF` | Factory default value |
| `UP` | `UP` | Increment by one step |
| `DOWN` | `DOWN` | Decrement by one step |
| `INFinity` | `INF` | IEEE 754 positive infinity (`9.9E+37`) |
| `NANot a number` | `NAN` | IEEE 754 NaN (`9.91E+37`) |

**Example:**

```
>> :SOUR:FREQ MIN
>> :SOUR:FREQ? MIN
<< 1.000000E+02
```

The first command sets frequency to minimum; the second queries what the minimum is.

### 4.4 Boolean

| Input Values | Output Value |
|---|---|
| `ON`, `1` | `1` |
| `OFF`, `0` | `0` |

**Example:**

```
>> :OUTP ON
>> :OUTP?
<< 1
```

### 4.5 Discrete (Enumerated)

Discrete parameters use SCPI keywords with short/long forms. Documentation
typically lists them in curly braces:

```
:SOUR:FUNC {SINusoid|SQUare|TRIangle|RAMP|NOISe|DC|PULSe}
```

**Input (forgiving):** Any valid short or long form, any case:

```
:SOUR:FUNC SIN
:SOUR:FUNC SINUSOID
:SOUR:FUNC sin
```

**Output (precise):** Always uppercase short form:

```
>> :SOUR:FUNC?
<< SIN
```

### 4.6 String

String parameters are enclosed in single or double quotes:

```
:DISP:TEXT "Hello World"
:DISP:TEXT 'Hello World'
```

Response strings are always returned in double quotes:

```
>> :DISP:TEXT?
<< "Hello World"
```

### 4.7 Block Data

Used for transferring binary data (waveforms, screenshots, calibration data).

#### Definite-Length Block Data

Format: `#<num_digits><byte_count><binary_data>`

| Field | Description |
|---|---|
| `#` | Block data header prefix |
| `<num_digits>` | Single digit: number of digits in `<byte_count>` |
| `<byte_count>` | ASCII decimal: number of bytes of binary data that follow |
| `<binary_data>` | Raw binary payload |

**Examples:**

```
#3100<100 bytes of data>      -- 3 digits, 100 bytes
#41000<1000 bytes of data>    -- 4 digits, 1000 bytes
#15<5 bytes of data>          -- 1 digit, 5 bytes
#216<16 bytes of data>        -- 2 digits, 16 bytes
```

**Parsing algorithm:**

```
1. Read '#'
2. Read one digit N (the "num_digits")
3. Read N digits to get the byte count B
4. Read exactly B bytes of binary data
5. Read the trailing LF terminator (if present)
```

#### Indefinite-Length Block Data

Format: `#0<binary_data><LF><EOI>`

- The `#0` prefix indicates an indefinite-length block
- Data continues until a `LF` terminator with EOI (GPIB) or connection close
- Primarily used with GPIB; less common over TCP/IP or serial

### 4.8 Numeric Base Formats

Integers can be specified in different bases using prefix identifiers:

| Prefix | Base | Example | Decimal Equivalent |
|---|---|---|---|
| (none) | Decimal | `255` | 255 |
| `#H` | Hexadecimal | `#HFF` | 255 |
| `#B` | Binary | `#B11111111` | 255 |
| `#Q` | Octal | `#Q377` | 255 |

---

## 5. Response Formats

Instruments follow the "precise talking" principle --- responses use a
consistent, predictable format.

### 5.1 ASCII Numeric Responses

| Response Type | Format | Example |
|---|---|---|
| NR1 (integer) | `[+\|-]<digits>\n` | `+1\n`, `0\n`, `-42\n` |
| NR2 (fixed-point) | `[+\|-]<digits>.<digits>\n` | `1.500000\n` |
| NR3 (scientific) | `[+\|-]<digits>.<digits>E[+\|-]<digits>\n` | `1.000000E+06\n` |

### 5.2 Comma-Separated Lists

Multiple values are returned as comma-separated ASCII:

```
>> :SOUR:LIST:FREQ?
<< 1.000000E+06,2.000000E+06,3.000000E+06
```

**Parsing note:** Split on `,` then parse each element as NR3.

### 5.3 Definite-Length Block Data Response

Binary trace data (e.g., from a VNA) is typically returned in block format:

```
>> :CALC:DATA? SDATA
<< #42048<2048 bytes of IEEE 754 float data>
```

The `ScpiClient` must parse the `#<n><count>` header to determine how many bytes
to read. See [Section 4.7](#47-block-data) for the parsing algorithm.

### 5.4 ASCII Trace Data Response

When the data format is set to ASCII, trace data is returned as comma-separated
NR3 values:

```
>> :FORM ASC
>> :CALC:DATA? SDATA
<< 1.234567E-03,5.678901E-02,1.111111E-01,...
```

### 5.5 String Responses

```
>> :SYST:NAME?
<< "MyInstrument"
```

Always double-quoted in responses, even if single quotes were used as input.

### 5.6 Discrete Responses

Always uppercase short form:

```
>> :SOUR:FUNC?
<< SIN
```

### 5.7 Boolean Responses

Always `0` or `1`:

```
>> :OUTP?
<< 1
```

---

## 6. Error Handling Model

### 6.1 Error Queue

Every SCPI instrument maintains a **first-in, first-out (FIFO) error queue**.
Errors are added as they occur and retrieved with `SYSTem:ERRor?`.

**Query syntax:**

```
>> :SYST:ERR?
<< -100,"Command error"
```

**Response format:** `<error_code>,\"<error_message>\"`

- Error code: NR1 integer
- Error message: Quoted ASCII string
- When the queue is empty: `0,"No error"`

**Draining the error queue (required pattern for ScpiClient):**

```cpp
// Pseudocode
do {
    send(":SYST:ERR?\n");
    response = read();
    parse(response, &code, &message);
    if (code != 0) {
        log_error(code, message);
    }
} while (code != 0);
```

### 6.2 Standard Error Code Ranges

| Range | Category | ESR Bit | Description |
|---|---|---|---|
| -100 to -199 | Command Error | 5 (CME) | Syntax, unrecognized command, wrong parameter type |
| -200 to -299 | Execution Error | 4 (EXE) | Valid command but cannot execute (out of range, etc.) |
| -300 to -399 | Device-Specific Error | 3 (DDE) | Hardware/firmware problem |
| -400 to -499 | Query Error | 2 (QYE) | Output queue protocol violation |
| 1 to 32767 | Instrument-Specific | Varies | Manufacturer-defined positive error codes |

### 6.3 Common Error Codes

| Code | Name | Typical Cause |
|---|---|---|
| 0 | No error | Queue is empty |
| -100 | Command error (generic) | Unspecified syntax error |
| -102 | Syntax error | Unrecognized character in command |
| -103 | Invalid separator | Wrong separator character |
| -104 | Data type error | Parameter of wrong type |
| -108 | Parameter not allowed | Too many parameters |
| -109 | Missing parameter | Required parameter omitted |
| -110 | Command header error | Unrecognized keyword |
| -113 | Undefined header | Command not supported by this instrument |
| -120 | Numeric data error | Malformed number |
| -121 | Invalid character in number | Non-digit in numeric parameter |
| -131 | Invalid suffix | Unrecognized unit suffix |
| -141 | Invalid character data | Unrecognized discrete value |
| -200 | Execution error (generic) | Unspecified execution error |
| -211 | Trigger ignored | Trigger received when not expected |
| -221 | Settings conflict | Parameter combination not allowed |
| -222 | Data out of range | Value exceeds instrument limits |
| -223 | Too much data | Data block exceeds memory capacity |
| -224 | Illegal parameter value | Legal type but invalid specific value |
| -230 | Data corrupt or stale | Calibration data or measurement invalid |
| -300 | Device-specific error (generic) | Hardware problem |
| -310 | System error | Instrument firmware/system failure |
| -350 | Queue overflow | Too many errors; oldest discarded |
| -400 | Query error (generic) | Unspecified query protocol error |
| -410 | Query INTERRUPTED | New command sent before reading query response |
| -420 | Query UNTERMINATED | Instrument addressed to talk with no data in output buffer |
| -430 | Query DEADLOCKED | Input and output buffers both full |

### 6.4 Status Register System

The SCPI/IEEE 488.2 status system is a hierarchical set of registers:

```
+---------------------+
| Status Byte (STB)   |  <-- Read via *STB? or serial poll
|---------------------|
| Bit 7: OPER summary |----+
| Bit 6: MSS / RQS    |    |   +--------------------------+
| Bit 5: ESB          |----|-->| Standard Event Status    |
| Bit 4: MAV          |    |   | Register (ESR)           |
| Bit 3: QUES summary |    |   | Read/clear via *ESR?     |
| Bit 2: EAV          |    |   +--------------------------+
| Bit 1: (unused)     |    |
| Bit 0: (unused)     |    |   +--------------------------+
+---------------------+    +-->| Operation Status Reg     |
        |                  |   | STAT:OPER?               |
        v                  |   +--------------------------+
+---------------------+    |
| Service Request     |    |   +--------------------------+
| Enable Reg (SRE)    |    +-->| Questionable Status Reg  |
| Set via *SRE <n>    |        | STAT:QUES?               |
+---------------------+        +--------------------------+
```

**How SRQ (Service Request) works:**

1. An event occurs (e.g., error, operation complete)
2. The corresponding bit is set in the appropriate event register (ESR, OPER,
   QUES)
3. If the corresponding enable bit is set, the summary bit in STB is asserted
4. If the corresponding SRE bit is set, bit 6 (MSS/RQS) is asserted
5. On GPIB, this triggers a hardware SRQ interrupt
6. On TCP/IP or USB, the controller must poll `*STB?`

**Typical SRQ setup for error notification:**

```
*CLS                          -- Clear all status
*ESE 32                       -- Enable CME (command error) in ESE
*SRE 32                       -- Enable ESB in SRE
```

Now any command error will trigger MSS in the status byte.

### 6.5 Recommended Error-Checking Strategy for ScpiClient

```
After each critical command:
  1. Send the command
  2. Send *OPC? and wait for "1" (ensures command executed)
  3. Query *ESR? and check for error bits (CME, EXE, DDE, QYE)
  4. If error bits set, drain SYST:ERR? queue

For non-critical commands (e.g., display text):
  1. Send the command
  2. Periodically check *ESR? or drain SYST:ERR? in a maintenance loop
```

---

## 7. Transport Layers

SCPI is transport-independent. The same ASCII commands work over any supported
physical layer. The `ScpiClient` will abstract the transport behind a common
interface.

### 7.1 USB-TMC (NI-VISA)

**USB-TMC** (USB Test and Measurement Class) is a USB device class that emulates
GPIB-style message-based communication over USB 2.0.

| Property | Value |
|---|---|
| Standard | USB-IF USBTMC Specification |
| Data rate | Up to 480 Mbps (USB 2.0 High Speed) |
| Message framing | Handled by USBTMC protocol (no manual terminator needed) |
| Discovery | Plug-and-play via USB VID/PID |
| Driver | NI-VISA (cross-platform) |

**VISA resource string:**

```
USB[board]::VID::PID::serial_number[::interface]::INSTR
```

**Examples:**

```
USB0::0x0957::0x1F01::MY12345678::INSTR    -- Keysight instrument
USB0::0x1AB1::0x0641::DS1ZA000000001::INSTR -- Rigol instrument
```

**Characteristics:**
- USBTMC handles message boundaries --- the VISA driver knows when a response
  is complete
- No manual LF termination needed when using `viWrite()` / `viRead()`
- Maximum message size is negotiated during USBTMC enumeration
- Supports both Bulk-IN (responses) and Bulk-OUT (commands) transfers

### 7.2 TCP/IP Socket

Raw SCPI over TCP is the simplest network transport.

| Property | Value |
|---|---|
| Default port | **5025** (IANA-assigned for SCPI RAW) |
| Protocol | TCP (reliable, stream-oriented) |
| Framing | LF-terminated ASCII messages |
| Discovery | Manual IP configuration or mDNS/DNS-SD |

**VISA resource string:**

```
TCPIP[board]::host::port::SOCKET
```

**Examples:**

```
TCPIP0::192.168.1.100::5025::SOCKET
TCPIP::10.0.0.5::5025::SOCKET
TCPIP::my-instrument.local::5025::SOCKET
```

**Implementation notes for ScpiClient:**

```
1. Open TCP connection to instrument_ip:5025
2. Send command: write("SOUR:FREQ 1E6\n")
3. For queries: write("SOUR:FREQ?\n"), then read until '\n'
4. Timeout: use 5-10 second read timeout (adjustable)
5. Keep connection open for the session lifetime
6. Close socket on disconnect
```

**Important:** When using raw sockets (not NI-VISA), the `ScpiClient` must:
- Append `\n` to every command
- Read responses until `\n` is received
- Handle TCP stream reassembly (a single `read()` may not return the full
  response)
- Implement a read timeout to detect hung instruments

### 7.3 Serial (RS-232)

| Property | Typical Value |
|---|---|
| Baud rate | 9600, 19200, 38400, 57600, or **115200** |
| Data bits | 8 |
| Parity | None |
| Stop bits | 1 |
| Flow control | None, or RTS/CTS (hardware) |
| Terminator | LF (`\n`) |

**VISA resource string:**

```
ASRL<port>::INSTR
```

**Examples:**

```
ASRL1::INSTR        -- COM1 (Windows)
ASRL3::INSTR        -- COM3 (Windows)
ASRL/dev/ttyUSB0::INSTR  -- Linux USB-serial
```

**Implementation notes:**
- Serial parameters (baud, parity, etc.) must be configured before communication
- The instrument's serial settings must match the controller's settings
- Flow control prevents buffer overruns on high-speed data transfers
- Response terminator is `\n`; some instruments also send `\r\n`

### 7.4 GPIB (IEEE 488)

| Property | Value |
|---|---|
| Standard | IEEE 488.1 (physical), IEEE 488.2 (protocol) |
| Data rate | Up to 1 MB/s (IEEE 488.1) or 8 MB/s (HS488) |
| Addressing | 0--30 (primary), 0--30 (secondary, optional) |
| Terminator | EOI hardware signal and/or LF |
| Bus | Parallel, 24-pin connector, max 15 devices |
| Controller | NI GPIB-USB-HS adapter or PCI/PCIe card |

**VISA resource string:**

```
GPIB[board]::primary_address[::secondary_address]::INSTR
```

**Examples:**

```
GPIB0::1::INSTR         -- Primary address 1, no secondary
GPIB0::5::INSTR         -- Primary address 5
GPIB0::1::2::INSTR      -- Primary 1, secondary 2
```

**Characteristics:**
- GPIB supports hardware SRQ (Service Request) --- the instrument can interrupt
  the controller
- Addressed commands (`*RST`, `*CLS`) are sent to a specific device address
- `EOI` (End-Or-Identify) signal marks the end of a message
- GPIB is a legacy interface but still widely used in lab environments

### 7.5 Transport Comparison

| Feature | USB-TMC | TCP/IP | Serial | GPIB |
|---|---|---|---|---|
| Speed | High | High | Low-Medium | Medium |
| Cable length | 5 m | Unlimited (LAN) | 15 m | 20 m total |
| Multi-device | Hub | Switch/Router | Point-to-point | 15 devices |
| SRQ support | Via VISA | Poll only | No | Hardware |
| Plug-and-play | Yes | Manual IP | Manual COM | Manual address |
| NI-VISA required | Yes | Optional | Optional | Yes |

---

## 8. Signal Generator Command Overview

> **Note:** Detailed command specifications for the MWA signal generator will be
> documented in Sprint 9. This section provides a brief overview of the standard
> SCPI subsystems used by signal generators.

### 8.1 Frequency Control

```
[:SOURce[n]]:FREQuency[:CW|:FIXed] <frequency>
[:SOURce[n]]:FREQuency[:CW|:FIXed]?
[:SOURce[n]]:FREQuency:MODE {CW|FIXed|LIST|SWEep}
[:SOURce[n]]:FREQuency:STARt <frequency>
[:SOURce[n]]:FREQuency:STOP <frequency>
[:SOURce[n]]:FREQuency:CENTer <frequency>
[:SOURce[n]]:FREQuency:SPAN <frequency>
[:SOURce[n]]:FREQuency:STEP[:INCRement] <frequency>
```

**Example --- set CW frequency to 10 MHz:**

```
>> :SOUR:FREQ 10E6
>> :SOUR:FREQ?
<< 1.000000E+07
```

### 8.2 Amplitude / Voltage Control

```
[:SOURce[n]]:VOLTage[:LEVel][:IMMediate][:AMPLitude] <voltage>
[:SOURce[n]]:VOLTage[:LEVel][:IMMediate]:OFFSet <voltage>
[:SOURce[n]]:POWer[:LEVel][:IMMediate][:AMPLitude] <power>
[:SOURce[n]]:POWer:ATTenuation <attenuation>
```

**Example --- set output amplitude to 1 Vpp:**

```
>> :SOUR:VOLT 1.0
>> :SOUR:VOLT?
<< 1.000000E+00
```

### 8.3 Waveform Function

```
[:SOURce[n]]:FUNCtion[:SHAPe] {SINusoid|SQUare|TRIangle|RAMP|NOISe|DC|PULSe}
[:SOURce[n]]:FUNCtion[:SHAPe]?
[:SOURce[n]]:FUNCtion:SQUare:DCYCle <percent>
[:SOURce[n]]:FUNCtion:RAMP:SYMMetry <percent>
[:SOURce[n]]:FUNCtion:PULSe:WIDTh <time>
```

**Example --- select square wave with 30% duty cycle:**

```
>> :SOUR:FUNC SQU
>> :SOUR:FUNC:SQU:DCYC 30
```

### 8.4 Output Control

```
:OUTPut[n][:STATe] {ON|OFF|1|0}
:OUTPut[n][:STATe]?
:OUTPut[n]:IMPedance <impedance>
:OUTPut[n]:LOAD <impedance>
```

**Example --- enable output:**

```
>> :OUTP ON
>> :OUTP?
<< 1
```

### 8.5 Sweep Control

```
[:SOURce[n]]:SWEep:SPACing {LINear|LOGarithmic}
[:SOURce[n]]:SWEep:TIME <seconds>
[:SOURce[n]]:SWEep:POINts <integer>
[:SOURce[n]]:SWEep:DIRection {UP|DOWN}
```

**Example --- configure a linear frequency sweep:**

```
>> :SOUR:FREQ:STAR 1E6
>> :SOUR:FREQ:STOP 10E6
>> :SOUR:SWE:SPAC LIN
>> :SOUR:SWE:TIME 1.0
>> :SOUR:SWE:POIN 201
```

---

## 9. VNA Command Overview

> **Note:** Detailed command specifications for the MWA VNA will be documented
> in Sprint 10. This section provides a brief overview of the standard SCPI
> subsystems used by Vector Network Analyzers.

### 9.1 Frequency Configuration

```
:SENSe[n]:FREQuency:STARt <frequency>
:SENSe[n]:FREQuency:STOP <frequency>
:SENSe[n]:FREQuency:CENTer <frequency>
:SENSe[n]:FREQuency:SPAN <frequency>
:SENSe[n]:FREQuency:DATA?
```

**Example --- set sweep range 300 kHz to 6 GHz:**

```
>> :SENS:FREQ:STAR 300E3
>> :SENS:FREQ:STOP 6E9
```

### 9.2 Sweep Configuration

```
:SENSe[n]:SWEep:POINts <integer>
:SENSe[n]:SWEep:TIME <seconds>
:SENSe[n]:SWEep:TYPE {LINear|LOGarithmic|SEGMent|POWer}
:SENSe[n]:SWEep:MODE {CONTinuous|SINGle|HOLD}
:SENSe[n]:BANDwidth[:RESolution] <bandwidth>
```

**Example --- configure 201-point linear sweep with 1 kHz IF bandwidth:**

```
>> :SENS:SWE:POIN 201
>> :SENS:SWE:TYPE LIN
>> :SENS:BAND 1E3
```

### 9.3 Measurement Data Retrieval

```
:CALCulate[n]:DATA? {SDATa|FDATa|MDATa}
:CALCulate[n]:DATA:SNP:PORTs? <port_list>
:FORMat[:DATA] {ASCii|REAL[,32|,64]}
```

**Data format options:**

| Format | `FORMat` Setting | Response Type |
|---|---|---|
| ASCII | `ASC` | Comma-separated NR3 values |
| Binary 32-bit | `REAL,32` | Definite-length block of IEEE 754 single-precision floats |
| Binary 64-bit | `REAL,64` | Definite-length block of IEEE 754 double-precision floats |

**Example --- read S-parameter data in ASCII:**

```
>> :FORM ASC
>> :CALC:DATA? SDAT
<< 1.234567E-03,5.678901E-02,1.111111E-01,-2.222222E-01,...
```

The response contains alternating real/imaginary pairs: `Re1,Im1,Re2,Im2,...`

**Example --- read S-parameter data in binary:**

```
>> :FORM REAL,64
>> :CALC:DATA? SDAT
<< #42408<2408 bytes of 64-bit IEEE 754 doubles>
```

For 201 points of complex data: 201 points x 2 (real+imag) x 8 bytes = 3216
bytes.

### 9.4 Trigger and Initiate

```
:INITiate[n][:IMMediate]
:INITiate[n]:CONTinuous {ON|OFF}
:TRIGger[:SEQuence]:SOURce {IMMediate|MANual|EXTernal|BUS}
:ABORt
```

**Example --- trigger a single sweep and wait:**

```
>> :INIT:CONT OFF
>> :TRIG:SOUR BUS
>> :INIT;*OPC?
<< 1
>> :CALC:DATA? SDAT
<< ...
```

### 9.5 S-Parameter Selection

```
:CALCulate[n]:PARameter:DEFine <trace_name>,<S_parameter>
:CALCulate[n]:PARameter:SELect <trace_name>
:CALCulate[n]:FORMat {MLINear|MLOGarithmic|PHASe|UPHase|GDELay|REAL|IMAGinary|SWR|SMITh}
```

**Example --- measure S21 in log magnitude:**

```
>> :CALC:PAR:DEF 'Trc1','S21'
>> :CALC:PAR:SEL 'Trc1'
>> :CALC:FORM MLOG
```

---

## 10. NI-VISA C API Reference

NI-VISA (Virtual Instrument Software Architecture) provides a
transport-independent API for instrument communication. The `ScpiClient` will
use NI-VISA for USB-TMC and GPIB transport; raw sockets may be used for TCP/IP.

### 10.1 Core Data Types

| Type | Description |
|---|---|
| `ViSession` | Handle to a VISA session (resource manager or instrument) |
| `ViStatus` | Return code (signed 32-bit integer) |
| `ViBuf` / `ViByte[]` | Buffer for read/write operations |
| `ViUInt32` | Unsigned 32-bit integer (counts, sizes) |
| `ViRsrc` | Resource string (e.g., `"USB0::0x1234::0x5678::INSTR"`) |
| `ViAttr` | Attribute identifier for `viGetAttribute` / `viSetAttribute` |
| `ViBoolean` | `VI_TRUE` or `VI_FALSE` |

### 10.2 Key Functions

#### `viOpenDefaultRM` --- Open Resource Manager

```c
ViStatus viOpenDefaultRM(ViSession *defaultRM);
```

Must be called before any other VISA operation. Initializes the VISA system and
returns a session handle to the Default Resource Manager.

**Example:**

```c
ViSession rm;
ViStatus status = viOpenDefaultRM(&rm);
if (status < VI_SUCCESS) {
    // Handle error
}
```

#### `viOpen` --- Open Instrument Session

```c
ViStatus viOpen(ViSession sesn,
                ViRsrc    rsrcName,
                ViAccessMode accessMode,
                ViUInt32  openTimeout,
                ViSession *vi);
```

| Parameter | Description |
|---|---|
| `sesn` | Resource Manager session from `viOpenDefaultRM` |
| `rsrcName` | VISA resource string (see Section 7) |
| `accessMode` | `VI_NO_LOCK` (typical), `VI_EXCLUSIVE_LOCK`, `VI_SHARED_LOCK` |
| `openTimeout` | Timeout in milliseconds (`VI_TMO_IMMEDIATE` or `VI_TMO_INFINITE`) |
| `vi` | Output: instrument session handle |

**Example:**

```c
ViSession instr;
ViStatus status = viOpen(rm,
                         "USB0::0x0957::0x1F01::MY12345678::INSTR",
                         VI_NO_LOCK,
                         5000,
                         &instr);
```

#### `viWrite` --- Send Command

```c
ViStatus viWrite(ViSession vi,
                 ViBuf     buf,
                 ViUInt32  count,
                 ViUInt32  *retCount);
```

Synchronously writes `count` bytes from `buf` to the instrument. Returns the
number of bytes actually written in `retCount`.

**Example:**

```c
const char *cmd = ":SOUR:FREQ 1E6\n";
ViUInt32 written;
ViStatus status = viWrite(instr,
                          (ViBuf)cmd,
                          (ViUInt32)strlen(cmd),
                          &written);
```

#### `viRead` --- Read Response

```c
ViStatus viRead(ViSession vi,
                ViBuf     buf,
                ViUInt32  count,
                ViUInt32  *retCount);
```

Synchronously reads up to `count` bytes into `buf`. Returns the number of bytes
actually read in `retCount`. The read terminates when:

- `count` bytes have been read, or
- The termination character is received (default: `\n`), or
- EOI is asserted (GPIB/USBTMC), or
- Timeout expires

**Example:**

```c
char response[256];
ViUInt32 read_count;
ViStatus status = viRead(instr,
                         (ViBuf)response,
                         sizeof(response) - 1,
                         &read_count);
response[read_count] = '\0';  // Null-terminate
```

#### `viClose` --- Close Session

```c
ViStatus viClose(ViSession vi);
```

Closes an instrument session or the resource manager session. Always close
instrument sessions before closing the resource manager.

**Example:**

```c
viClose(instr);
viClose(rm);
```

#### `viStatusDesc` --- Error Description

```c
ViStatus viStatusDesc(ViSession vi,
                      ViStatus  statusVal,
                      ViString  desc);
```

Converts a `ViStatus` error code to a human-readable string. The `desc` buffer
must be at least 256 bytes.

**Example:**

```c
char desc[256];
viStatusDesc(instr, status, desc);
qWarning() << "VISA error:" << desc;
```

### 10.3 Useful Attributes

| Attribute | Type | Description |
|---|---|---|
| `VI_ATTR_TMO_VALUE` | `ViUInt32` | Read/write timeout in ms |
| `VI_ATTR_TERMCHAR` | `ViByte` | Termination character (default `0x0A`) |
| `VI_ATTR_TERMCHAR_EN` | `ViBoolean` | Enable/disable termination character |
| `VI_ATTR_SEND_END_EN` | `ViBoolean` | Assert EOI with last byte of write |
| `VI_ATTR_ASRL_BAUD` | `ViUInt32` | Serial baud rate |
| `VI_ATTR_ASRL_DATA_BITS` | `ViUInt16` | Serial data bits (5--8) |
| `VI_ATTR_ASRL_PARITY` | `ViUInt16` | Serial parity |
| `VI_ATTR_ASRL_STOP_BITS` | `ViUInt16` | Serial stop bits |

**Example --- set timeout to 10 seconds:**

```c
viSetAttribute(instr, VI_ATTR_TMO_VALUE, 10000);
```

### 10.4 Resource String Format Summary

| Transport | Format | Example |
|---|---|---|
| GPIB | `GPIB[board]::primary[::secondary]::INSTR` | `GPIB0::5::INSTR` |
| USB-TMC | `USB[board]::VID::PID::serial::INSTR` | `USB0::0x0957::0x1F01::MY123::INSTR` |
| TCP/IP Socket | `TCPIP[board]::host::port::SOCKET` | `TCPIP0::192.168.1.100::5025::SOCKET` |
| TCP/IP HiSLIP | `TCPIP[board]::host::hislip0::INSTR` | `TCPIP0::192.168.1.100::hislip0::INSTR` |
| Serial | `ASRL<port>::INSTR` | `ASRL3::INSTR` |

**Notes:**
- Resource strings are **case-insensitive**
- VID and PID can be decimal or `0x`-prefixed hexadecimal
- The `board` number defaults to `0` if omitted

### 10.5 Typical VISA Session Lifecycle

```c
ViSession rm, instr;
ViStatus  status;
ViUInt32  ret_count;
char      response[4096];

// 1. Initialize VISA
status = viOpenDefaultRM(&rm);

// 2. Open instrument
status = viOpen(rm, "TCPIP0::192.168.1.100::5025::SOCKET",
                VI_NO_LOCK, 5000, &instr);

// 3. Configure session
viSetAttribute(instr, VI_ATTR_TMO_VALUE, 10000);
viSetAttribute(instr, VI_ATTR_TERMCHAR_EN, VI_TRUE);
viSetAttribute(instr, VI_ATTR_TERMCHAR, '\n');

// 4. Identify instrument
viWrite(instr, (ViBuf)"*IDN?\n", 6, &ret_count);
viRead(instr, (ViBuf)response, sizeof(response) - 1, &ret_count);
response[ret_count] = '\0';
// response: "Keysight Technologies,N5182B,MY12345678,A.01.23\n"

// 5. Reset and clear
viWrite(instr, (ViBuf)"*RST;*CLS\n", 10, &ret_count);

// 6. Configure instrument
viWrite(instr, (ViBuf)":SOUR:FREQ 1E6\n", 16, &ret_count);

// 7. Query setting
viWrite(instr, (ViBuf)":SOUR:FREQ?\n", 12, &ret_count);
viRead(instr, (ViBuf)response, sizeof(response) - 1, &ret_count);
response[ret_count] = '\0';
// response: "1.000000E+06\n"

// 8. Check errors
viWrite(instr, (ViBuf)":SYST:ERR?\n", 11, &ret_count);
viRead(instr, (ViBuf)response, sizeof(response) - 1, &ret_count);
response[ret_count] = '\0';
// response: "0,\"No error\"\n"

// 9. Close sessions
viClose(instr);
viClose(rm);
```

---

## 11. Cross-Platform Notes

### 11.1 NI-VISA Platform Support

| Platform | Support Level | Notes |
|---|---|---|
| **Windows x86_64** | Full | All transports (GPIB, USB, TCP/IP, Serial) |
| **macOS ARM64/x86_64** | Limited | USB-TMC and Serial supported; GPIB requires adapter with driver support; 64-bit only |
| **Linux x86_64** | Limited | Similar to macOS; community alternatives exist (linux-gpib) |

### 11.2 MWA Transport Strategy

For the MWA application:

| Transport | Windows | macOS | Implementation |
|---|---|---|---|
| TCP/IP Socket | Qt `QTcpSocket` | Qt `QTcpSocket` | Native Qt (no VISA needed) |
| USB-TMC | NI-VISA | NI-VISA | VISA API wrapper |
| Serial | Qt `QSerialPort` | Qt `QSerialPort` | Native Qt (no VISA needed) |
| GPIB | NI-VISA | NI-VISA (if available) | VISA API wrapper |

**Recommendation:** Prefer **TCP/IP** as the primary transport for maximum
cross-platform compatibility and simplicity. Use NI-VISA only when
USB-TMC or GPIB is required.

### 11.3 NI-VISA Linking

**Windows (MSVC):**

```cmake
find_library(VISA_LIB visa64 PATHS "C:/Program Files/IVI Foundation/VISA/Win64/Lib_x64/msc")
target_include_directories(myapp PRIVATE "C:/Program Files/IVI Foundation/VISA/Win64/Include")
target_link_libraries(myapp PRIVATE ${VISA_LIB})
```

**macOS:**

```cmake
find_library(VISA_LIB visa PATHS "/Library/Frameworks/VISA.framework")
target_include_directories(myapp PRIVATE "/Library/Frameworks/VISA.framework/Headers")
target_link_libraries(myapp PRIVATE ${VISA_LIB})
```

**Header:**

```c
#include "visa.h"     // Windows
#include <visa/visa.h> // macOS / Linux (path varies)
```

### 11.4 Conditional Compilation

Since NI-VISA may not be installed, the `ScpiClient` should support optional
VISA compilation:

```cpp
#ifdef MWA_HAS_VISA
#include "visa.h"
// USB-TMC and GPIB transport available
#else
// TCP/IP and Serial only
#endif
```

The CMake build should detect VISA availability and define `MWA_HAS_VISA`
accordingly.

### 11.5 Timeout Considerations

| Scenario | Recommended Timeout |
|---|---|
| `*IDN?` query | 5 seconds |
| `*RST` command + `*OPC?` | 15 seconds |
| `*TST?` self-test | 30--60 seconds |
| Frequency/amplitude set + `*OPC?` | 5 seconds |
| VNA sweep (201 pts, narrow BW) | 30 seconds |
| VNA sweep (10001 pts, 10 Hz BW) | 300 seconds |
| Data transfer (`CALC:DATA?`, large) | 30 seconds |

---

## 12. References

1. **SCPI-1999 Standard** --- IVI Foundation
   https://www.ivifoundation.org/downloads/SCPI/scpi-99.pdf

2. **IEEE 488.2-1992** --- IEEE Standard Codes, Formats, Protocols, and
   Common Commands
   https://standards.ieee.org/ieee/488.2/718/

3. **NI-VISA Programmer Reference Manual**
   https://www.ni.com/docs/en-US/bundle/ni-visa-api-ref/page/ni-visa-api-ref/viopendefaultrm.html

4. **VISA Resource Syntax and Examples** --- NI
   https://www.ni.com/docs/en-US/bundle/ni-visa/page/visa-resource-syntax-and-examples.html

5. **NI-VISA Features and OS Compatibility** --- NI
   https://www.ni.com/en/support/documentation/compatibility/17/ni-visa-features-and-operating-system-compatibility.html

6. **USB-TMC Specification** --- USB-IF
   https://www.usb.org/document-library/test-measurement-class-specification

7. **SCPI Basics** --- Keysight
   https://helpfiles.keysight.com/csg/n5106a/scpi_basics.htm

8. **Rohde & Schwarz SCPI Programming Guide**
   https://www.rohde-schwarz.com/us/driver-pages/remote-control/remote-programming-environments_231250.html

9. **Rohde & Schwarz Instrument Error Checking**
   https://www.rohde-schwarz.com/us/driver-pages/remote-control/instrument-error-checking_231244.html

10. **EEZ SCPI Registers and Queues**
    https://www.envox.eu/bench-power-supply/psu-scpi-reference-manual/psu-scpi-registers-and-queues/

11. **Wikipedia --- Standard Commands for Programmable Instruments**
    https://en.wikipedia.org/wiki/Standard_Commands_for_Programmable_Instruments
