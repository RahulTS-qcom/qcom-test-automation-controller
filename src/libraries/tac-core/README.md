# tac-core — Qt-Free Backend Library

Qt-free implementation of the QTAC hardware control backend. Produces `TACDev.dll` (Windows) / `libTACDev.so` (Linux) for automation scripting without requiring Qt runtime.

## Prerequisites

- CMake 3.16+
- C++20 compiler (MSVC 2022, GCC 11+, Clang 14+)
- Ninja (recommended) or Visual Studio generator
- FTDI D2XX driver installed (Windows: bundled, Linux: install `libftd2xx`)
- **No Qt required**

## Build (Standalone TACDev.dll)

From the repository root:

```bash
# Configure (one-time)
cmake -S standalone -B build/standalone -DCMAKE_BUILD_TYPE=Release -G Ninja

# Build
cmake --build build/standalone --config Release
```

Output:
- Windows: `__Builds/x64/Release/bin/TACDev.dll` + `__Builds/x64/Release/lib/TACDev.lib`
- Linux: `__Builds/Linux/Release/lib/libTACDev.so`

## Build (tac-core as shared library for C++ consumers)

```bash
cmake -S src/libraries/tac-core -B build/tac-core -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build build/tac-core --config Release
```

Output: `build/tac-core/output/bin/tac-core.dll` (or `libtac-core.so`)

## Configuration

Set the `TACDEV_CONFIG_PATH` environment variable to point to your platform configurations directory:

```bash
# Windows
set TACDEV_CONFIG_PATH=C:\ProgramData\Qualcomm\Alpaca\tac_configs\

# Linux
export TACDEV_CONFIG_PATH=/opt/qcom/QTAC/configurations/
```

This directory must contain `DeviceList.json` and the `.tcnf` platform configuration files.

## Debug Logging

Set `TACDEV_DEBUG=1` environment variable to enable debug trace output to `tacdev_debug.log` in the current working directory:

```bash
set TACDEV_DEBUG=1
python sample_script.py
# Check tacdev_debug.log for detailed trace
```

## Python Usage

### Setup

Ensure `TACDev.dll` is accessible. Set the DLL path via environment variable:

```bash
set TACDEV_DLL_PATH=C:\workspace\Alpaca\qcom-test-automation-controller\__Builds\x64\Release\bin\TACDev.dll
```

Or place `TACDev.dll` in one of the default search paths (see `interfaces/Python/TACDev/TACDev.py`).

### Basic Script

```python
import sys
sys.path.insert(0, "interfaces/Python/TACDev")
from TACDev import *

# Enumerate devices
count = GetDeviceCount()
print(f"Devices found: {count}")

for i in range(count):
    dev = GetDevice(i)
    print(f"  [{i}] {dev.PortName()} - {dev.Description()} ({dev.SerialNumber()})")

# Open first device
dev = GetDevice(0)
if dev and dev.Open():
    print(f"Name: {dev.Get_Name()}")
    print(f"Firmware: {dev.GetFirmwareVersion()}")
    print(f"Hardware: {dev.GetHardware()}")

    # Send a command
    dev.SendCommand("pkey", True)
    import time
    time.sleep(2)
    dev.SendCommand("pkey", False)

    # Query state
    state = dev.GetCommandState("pkey")
    print(f"PowerKey state: {state}")

    # List all available commands
    cmd_count = dev.GetCommandCount()
    for j in range(cmd_count):
        print(f"  Command: {dev.GetCommand(j)}")

    dev.Close()
else:
    print("Failed to open device")
```

### Quick Commands (Power On/Off, Boot to EDL, etc.)

```python
from TACDev import *

dev = GetDevice(0)
if dev and dev.Open():
    dev.PowerOnButton()       # Press power key sequence
    dev.PowerOffButton()      # Press power off sequence
    dev.BootToEDLButton()     # Boot to EDL mode
    dev.BootToFastBootButton()  # Boot to fastboot

    # Wait for command queue to clear
    while dev.IsCommandQueueClear():
        import time
        time.sleep(0.5)

    dev.Close()
```

### Script Variables

```python
from TACDev import *

dev = GetDevice(0)
if dev and dev.Open():
    # List script variables
    var_count = dev.GetScriptVariableCount()
    for i in range(var_count):
        print(dev.GetScriptVariable(i))

    # Update a variable
    dev.UpdateScriptVariableValue("fastboot", "9000")

    dev.Close()
```

## API Reference

| Function | Description |
|----------|-------------|
| `GetDeviceCount()` | Returns number of connected TAC devices |
| `GetDevice(index)` | Returns TACDevice object for the device at index |
| `TACDevice.Open()` | Opens connection to the device |
| `TACDevice.Close()` | Closes the connection |
| `TACDevice.SendCommand(cmd, state)` | Sends a pin command (e.g., "pkey", True) |
| `TACDevice.GetCommandState(cmd)` | Returns current state of a command |
| `TACDevice.GetCommandCount()` | Number of available commands |
| `TACDevice.GetCommand(index)` | Command info at index (semicolon-delimited) |
| `TACDevice.PowerOnButton()` | Execute power-on sequence |
| `TACDevice.PowerOffButton()` | Execute power-off sequence |
| `TACDevice.BootToEDLButton()` | Boot to EDL mode |
| `TACDevice.BootToFastBootButton()` | Boot to fastboot mode |
| `TACDevice.Get_Name()` | Device name |
| `TACDevice.GetFirmwareVersion()` | Firmware version string |
| `TACDevice.GetHardware()` | Hardware type (PSOC/FTDI/PIC32CX) |
| `TACDevice.Get_HardwareVersion()` | Hardware version string |
| `TACDevice.Get_UUID()` | Device UUID |
| `TACDevice.SetName(name)` | Set device name (max 32 chars) |
| `TACDevice.SetPin(pin, state)` | Set raw pin state |
| `TACDevice.IsCommandQueueClear()` | True if commands are still being processed |

## Supported Hardware

| Board Type | Communication | Protocol |
|-----------|--------------|----------|
| FTDI FT4232H (Alpaca-Lite) | USB Direct (D2XX) | TACLite binary protocol |
| Cypress PSoC5LP | USB Serial (COM port) | Text command protocol |
| Microchip PIC32CX SG41 | USB Serial (COM port) | SCPI-like protocol |

## Architecture

See `DESIGN.md` for the full architectural overview of the Qt-free transformation.
