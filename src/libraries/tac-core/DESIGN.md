# tac-core Design Overview: Qt-Free Backend Transformation

## 1. Motivation

The Qualcomm Test Automation Controller (QTAC) originally used Qt throughout its
entire stack — from the GUI layer down through device communication. This created
a hard dependency: **automation scripts** (Python, C#, Java) that only needed
hardware control were forced to deploy the full Qt runtime (~150 MB).

Goals of the tac-core transformation:

| Goal | Rationale |
|------|-----------|
| Remove Qt from the hardware control layer | Enable lightweight automation without Qt runtime |
| Single codebase | Avoid maintaining two parallel implementations |
| Keep GUI unchanged | Qt remains the UI framework — only the backend decouples |
| Cross-platform | C++20 standard library + platform-specific serial I/O |
| C API boundary | `TACDev.dll` / `libTACDev.so` consumable from any language |

---

## 2. Original Qt-Based Architecture (`qcommon-console`)

```
┌─────────────────────────────────────────────────────┐
│  Qt GUI (TAC Application)                           │
│  TACFrame / TACWindow / TACDeviceSelection          │
├─────────────────────────────────────────────────────┤
│  qcommon-console (Qt-based shared library)          │
│  ├── QThread (DriveThread)                          │
│  ├── QSerialPort (PSOC/PIC32CX serial comms)       │
│  ├── Qt Signals/Slots (event notification)          │
│  ├── QString, QByteArray, QList, QMap               │
│  ├── QSharedPointer (lifetime management)           │
│  ├── QCoreApplication::processEvents() (wait loop) │
│  └── QSettings (preferences)                        │
├─────────────────────────────────────────────────────┤
│  FTDI D2XX (direct USB — no Qt involvement)         │
│  libserialport (PSOC/PIC32CX serial comms)          │
└─────────────────────────────────────────────────────┘
```

**Key Qt dependencies in the backend:**

| Qt Feature | Usage |
|---|---|
| `QThread` | Worker thread lifecycle (start/stop/isRunning) |
| `QSerialPort` | Serial communication to PSOC and PIC32CX boards |
| Qt Signals/Slots | Pin state changes, device events, progress notifications |
| `QString` / `QByteArray` | All string handling |
| `QList<T>` / `QMap<K,V>` | All containers |
| `QSharedPointer<T>` | Device object lifetime |
| `QCoreApplication::processEvents()` | Synchronous wait-for-completion |
| `QSettings` / `QStandardPaths` | Configuration file I/O |
| `QJsonDocument` | Platform configuration parsing |

---

## 3. New tac-core Architecture

```
┌─────────────────────────────────────────────────────┐
│  Qt GUI (unchanged)                                 │
│  Uses TACDev C API for backend communication        │
├────────────────────────┬────────────────────────────┤
│  TACDev.dll (C API)    │  Python/C#/Java bindings   │
│  extern "C" boundary   │  ctypes / P/Invoke / JNA   │
├────────────────────────┴────────────────────────────┤
│  tac-core (Qt-free static library, C++20)           │
│  ├── std::thread (DriveThread)                      │
│  ├── libserialport (PSOC/PIC32CX serial comms)      │
│  ├── std::function<> callbacks (event notification) │
│  ├── std::string, std::vector, std::map             │
│  ├── std::shared_ptr (lifetime management)          │
│  ├── std::condition_variable (wait-for-completion)  │
│  └── nlohmann/json (configuration parsing)          │
├─────────────────────────────────────────────────────┤
│  FTDI D2XX (unchanged — never used Qt)              │
│  libserialport (fetched via CMake FetchContent)     │
└─────────────────────────────────────────────────────┘
```

**Design principles:**

1. **Zero Qt headers** — No file in tac-core includes any `Q*` header
2. **C++20 standard library only** — Threading, containers, smart pointers
3. **Single external dependency** — `nlohmann/json` (header-only, fetched by CMake)
4. **Platform I/O abstracted** — libserialport for serial, FTDI D2XX for USB
5. **Static library** — Linked into TACDev.dll; no separate deployment artifact
6. **Exception-safe C boundary** — All `extern "C"` functions wrapped in try/catch

---

## 4. Module-by-Module Transformation

### 4.1 Type System

tac-core uses C++ standard library types directly throughout:

- `std::string` — replaces `QString` / `QByteArray`
- `std::vector<T>` — replaces `QList<T>`
- `std::map<K,V>` — replaces `QMap<K,V>`
- `std::shared_ptr<T>` — replaces `QSharedPointer<T>`

A `TACCoreTypes.h` header exists with type aliases (`TString`, `TList`, etc.)
from an earlier plan to share source files between Qt and non-Qt builds via a
compile switch. In practice, tac-core was written as a separate implementation
and uses STL types directly — the aliases are vestigial and not widely used in
the actual code.

### 4.2 Threading (`DriveThread`)

| Aspect | Qt (qcommon-console) | tac-core |
|--------|---------------------|----------|
| Base class | `QThread` | `std::thread` wrapper |
| Start | `QThread::start()` | `_thread = std::thread([this]{ run(); })` |
| Stop | `requestInterruption()` + `wait()` | `stopRunning()` + `join()` |
| Running check | `QThread::isRunning()` | `_thread.joinable()` + `_running` flag |
| Thread ID | `QThread::currentThread()` | `std::this_thread::get_id()` |
| Sleep | `QThread::msleep(ms)` | `std::this_thread::sleep_for(ms)` |

The `DriveThread` class owns a `std::thread` and provides:
- `start()` — spawns the thread calling virtual `run()`
- `shutDown()` — sets `_running = false`, then `join()`s
- `stopRunning()` — non-blocking signal to exit
- `threadId()` — for deadlock avoidance (self-join detection in `close()`)

### 4.3 Serial Port (`SerialPort`)

| Aspect | Qt (qcommon-console) | tac-core |
|--------|---------------------|----------|
| Library | `QSerialPort` | `libserialport` (C library) |
| Model | Async with signals | Synchronous/blocking |
| Read | Signal-driven `readyRead` | `readAll(timeout_ms)` blocking call |
| Write | `QSerialPort::write()` | `sp_blocking_write()` |
| Config | `QSerialPort::setBaudRate()` etc. | `sp_set_baudrate()` etc. |
| Enumeration | `QSerialPortInfo::availablePorts()` | `sp_list_ports()` |

The synchronous model is natural because the drive thread's `run()` loop was
already effectively synchronous — it called `waitForReadyRead(10ms)` in a loop.

### 4.4 Event System (Signals → Callbacks)

Qt signals/slots replaced with `std::function<>` callbacks:

```cpp
// Qt version (qcommon-console)
signals:
    void pinStateChanged(uint64_t pin, bool state);
    void deviceConnected();
    void errorOnOpen(const QString& error);

// tac-core version
std::function<void(uint64_t pin, bool state)>  onPinStateChanged;
std::function<void()>                          onDeviceConnected;
std::function<void(const std::string&)>        onErrorOnOpen;
```

Callbacks are set before `start()` and invoked from the worker thread. The
caller is responsible for thread safety (typically the callback captures `this`
and dispatches to the appropriate context).

### 4.5 Wait-for-Completion

| Qt (qcommon-console) | tac-core |
|---------------------|----------|
| `while(_wait) { QCoreApplication::processEvents(); sleep(10ms); }` | `std::condition_variable::wait_for(5s)` |

The Qt version relied on the event loop to process responses. The tac-core
version uses a proper condition variable:

```cpp
void TACDriveThread::waitForCompletion()
{
    std::unique_lock<std::mutex> lock(_completionMutex);
    _completionCV.wait_for(lock, std::chrono::seconds(5),
        [this]{ return !_waitForCompletion.load(); });
}
```

The worker thread calls `clearWaitForCompletion()` which sets the atomic flag
and notifies the CV — unblocking the API thread immediately.

### 4.6 Configuration Parsing

| Qt (qcommon-console) | tac-core |
|---------------------|----------|
| `QJsonDocument::fromJson()` | `nlohmann::json::parse()` |
| `QFile` | `std::ifstream` |
| `QStandardPaths` | `TACDEV_CONFIG_PATH` env var or platform defaults |

Platform configurations (`.tcnf` JSON files) and `devicelist.json` are parsed
using nlohmann/json with the same logical structure.

### 4.7 Application Core (`AppCore`)

Replaces `QCoreApplication` infrastructure:

- **Singleton** providing logging, preferences, and license checking
- **ThreadedLog** — Dedicated thread writing log entries from a queue (replaces
  `qDebug()` / Qt logging categories)
- **PreferencesBase** — INI-style file I/O using `std::fstream` (replaces
  `QSettings`)
- **No event loop** — tac-core has no main event loop; threads communicate via
  mutexes, condition variables, and callbacks

### 4.8 FTDI Communication (Unchanged)

The FTDI path was always Qt-free (uses the `ftd2xx` C library directly). The
`TACLiteDriveThread` → `TACLiteProtocol` → `TACLiteCoder` stack for FTDI
devices required no architectural changes — only type alias swaps.

---

## 5. C API Layer (`TACDev.h` / `TACDev.cpp`)

The public interface is a flat C API exported from `TACDev.dll`:

```
┌──────────────────────────────────────────────┐
│  TACDev.h  (extern "C" declarations)         │
│  • InitializeTACDev()                        │
│  • GetDeviceCount() / GetPortData()          │
│  • OpenHandleByDescription() / CloseTACHandle│
│  • SendCommand() / GetCommandState()         │
│  • PowerKey() / Usb0() / PrimaryEDL() / ...  │
│  • GetName() / GetFirmwareVersion() / ...    │
├──────────────────────────────────────────────┤
│  TACDevCore  (C++ implementation class)      │
│  • Manages device enumeration & lifecycle    │
│  • Maps TAC_HANDLE → AlpacaDevice            │
│  • Thread-safe via std::mutex                │
├──────────────────────────────────────────────┤
│  AlpacaDevice / TACDriveThread / Protocol    │
│  (internal C++ classes — not exposed)        │
└──────────────────────────────────────────────┘
```

**Exception safety:** Every `extern "C"` function is wrapped in:
```cpp
try {
    // ... C++ implementation ...
} catch (const std::exception& e) {
    gDevTACCore.setLastError(e.what());
    return TACDEV_INIT_FAILED;
} catch (...) {
    gDevTACCore.setLastError("Unknown exception");
    return TACDEV_INIT_FAILED;
}
```

---

## 6. Build System

### CMake Structure

```
standalone/CMakeLists.txt       ← Standalone build (no Qt required)
  ├── FetchContent: nlohmann/json, libserialport
  ├── tac-core (static library)
  └── TACDev (shared library / DLL)

src/libraries/tac-core/CMakeLists.txt  ← Used when building within full Qt project
```

### Build Configurations

| Configuration | Qt Required | Output |
|---|---|---|
| `standalone/` | No | `TACDev.dll` + `TACDev.lib` |
| Full project (`build.bat`) | Yes (Qt 6.9+) | All applications + TACDev |

### Dependencies

| Dependency | Method | Purpose |
|---|---|---|
| nlohmann/json | FetchContent (header-only) | JSON parsing |
| libserialport | FetchContent (compiled) | Serial port I/O (PSOC/PIC32CX) |
| FTDI D2XX | Pre-built (fetched by CMake) | USB I/O (FTDI boards) |

---

## 7. Comparison: Before vs After

| Dimension | Qt Backend | tac-core |
|-----------|-----------|----------|
| **Runtime dependency** | Qt 6.9+ (~150 MB) | C++ runtime only (~2 MB) |
| **DLL size** | ~5 MB (with Qt DLLs) | ~800 KB standalone |
| **Build time** | Full Qt MOC/UIC pipeline | Standard C++20 compilation |
| **Thread model** | QThread + event loop | std::thread + CV |
| **Event delivery** | Qt signal/slot (queued) | Direct std::function callback |
| **Latency** | Event loop overhead | Direct function call |
| **Deployment** | Qt DLLs + plugins | Single DLL + FTDI driver |
| **Language bindings** | C++ only (or complex wrapping) | C API (ctypes/P-Invoke/JNA) |
| **Testability** | Requires QApplication | No framework required |
| **CI requirements** | Qt SDK installed | Standard C++ toolchain |

---

## 8. Thread Architecture

```
┌──────────────────┐     ┌──────────────────────────────┐
│  API Thread       │     │  Drive Thread (per device)    │
│  (caller context) │     │  TACLiteDriveThread::run()    │
│                   │     │  TACPSOCDriveThread::run()    │
│  SendCommand() ──────►  │  TACPIC32CXDriveThread::run() │
│                   │     │                              │
│  waitForCompletion│◄─── │  clearWaitForCompletion()    │
│  (CV wait)        │     │  (CV notify)                 │
│                   │     │                              │
│  onPinStateChanged│◄─── │  callback invocation         │
│  (std::function)  │     │  (from drive thread context) │
└──────────────────┘     └──────────────────────────────┘
```

**Key synchronization primitives:**
- `std::mutex` — Protects command maps, device lists, shared state
- `std::recursive_mutex` — Protects drive thread state (allows re-entrant access)
- `std::condition_variable` — Wait-for-completion synchronization
- `std::atomic<bool>` — Thread running flags, completion status

---

## 9. Device Communication Stack

### FTDI (FT4232H) — USB Direct

```
AlpacaDevice::sendCommand()
  → TACLiteDriveThread::sendCommand()
    → TACLiteProtocol::sendCommand()
      → TACLiteCoder::encode()
        → FramePackage queued
          → run() loop dequeues
            → FT_Write() (D2XX API)
              → FT_Read() response
                → TACLiteCoder::decode()
                  → frameComplete() callback
```

### PSOC / PIC32CX — Serial

```
AlpacaDevice::sendCommand()
  → TACPSOCDriveThread::sendCommand()
    → TACPSOCProtocol::sendCommand()
      → TACPSOCCoder::encode()
        → FramePackage queued
          → run() loop dequeues
            → SerialPort::write()
              → SerialPort::readAll()
                → TACPSOCCoder::decode()
                  → frameComplete() callback
```

---

## 10. Trade-offs and Limitations

### Accepted Trade-offs

| Trade-off | Rationale |
|---|---|
| No queued signal delivery | Callbacks fire on worker thread; caller must handle thread safety |
| No automatic memory management via `deleteLater()` | Explicit lifecycle management with `unique_ptr` + `shutDown()` + `join()` |
| No QSettings integration | Custom INI parser (simpler but less feature-rich) |
| `std::function` callbacks are not disconnectable | Unlike Qt signals, you cannot disconnect a callback at runtime |
| Hardcoded 5-second timeout for waitForCompletion | Sufficient for hardware commands; configurable if needed later |

### Known Limitations

1. **No dynamic signal routing** — Qt's signal/slot system allows N:M connections;
   callbacks are 1:1 (one handler per event type per object)
2. **Thread affinity not enforced** — Qt guarantees signal delivery to the
   receiver's thread; callbacks execute in the caller's thread context
3. **No built-in object tree** — Qt's parent-child ownership is replaced by
   explicit `unique_ptr` / `shared_ptr` ownership
4. **Serial port enumeration** — libserialport's enumeration may behave
   differently from QSerialPortInfo on some platforms

### Qt GUI Integration: Bridging std::function Callbacks to Qt Signals

tac-core fires `std::function` callbacks from worker threads. Qt widgets can only
be updated from the GUI thread. A thin **bridge class** (~50 lines) in the GUI
layer handles the thread marshalling:

```cpp
// TACDeviceBridge.h — lives in the Qt GUI layer, NOT in tac-core
class TACDeviceBridge : public QObject
{
    Q_OBJECT
public:
    explicit TACDeviceBridge(AlpacaDevice device, QObject* parent = nullptr)
        : QObject(parent), _device(device)
    {
        // Wire each tac-core callback → Qt signal with thread marshalling
        _device->onPinStateChanged = [this](uint64_t pin, bool state) {
            QMetaObject::invokeMethod(this, [this, pin, state]() {
                emit pinStateChanged(pin, state);
            }, Qt::QueuedConnection);
        };

        _device->onProgress = [this](uint8_t value, NotificationLevel level) {
            QMetaObject::invokeMethod(this, [this, value, level]() {
                emit progress(value, level);
            }, Qt::QueuedConnection);
        };

        _device->onErrorEvent = [this](const std::string& msg) {
            QMetaObject::invokeMethod(this, [this, msg]() {
                emit errorEvent(QString::fromStdString(msg));
            }, Qt::QueuedConnection);
        };
    }

    // Expose tac-core methods with Qt type conversion at boundary
    bool open() { return _device->open(); }
    void close() { _device->close(); }
    bool sendCommand(const QString& cmd, bool state) {
        return _device->sendCommand(cmd.toStdString(), state);
    }
    bool getCommandState(const QString& cmd) {
        return _device->getCommandState(cmd.toStdString());
    }
    QString name() { return QString::fromStdString(_device->name()); }

signals:
    void pinStateChanged(uint64_t pin, bool state);
    void progress(uint8_t value, int level);
    void errorEvent(const QString& message);
    void deviceConnected();
    void deviceDisconnected();

private:
    AlpacaDevice _device;
};
```

**Example: PowerKey ON → LED Update (complete flow):**

```
 GUI Thread                       Bridge (QObject)              Worker Thread
 ──────────                       ────────────────              ─────────────
                                  (lives on GUI thread,
                                   created with parent=widget)

 [User clicks PowerKey ON]
         │
         ▼
 bridge->sendCommand("pkey",true)
         │
         ▼
 TACDeviceBridge::sendCommand()
 → _device->sendCommand("pkey",true)
         │
         │── crosses thread boundary (call is thread-safe) ──►
         │                                                      │
         │                                              tac-core queues command
         │                                              drive thread sends to HW
         │                                              hardware responds
         │                                              drive thread detects pin=5 ON
         │                                                      │
         │                                                      ▼
         │                                              _device->onPinStateChanged(5, true)
         │                                                      │
         │                                              [lambda runs on WORKER thread]
         │                                              QMetaObject::invokeMethod(bridge, ...)
         │                                                      │
         │◄──────── posts to GUI thread event queue ────────────┘
         │
         ▼
 [Qt event loop picks up]
         │
         ▼
 Bridge lambda executes (now on GUI thread):
   emit pinStateChanged(5, true)
         │
         ▼
 TACFrame::updateLED(5, true)        ← connected via connect(bridge, &signal, this, &slot)
         │
         ▼
 _powerKeyLED->setColor(Qt::green)   ← widget updated safely on GUI thread
```

**Where does the bridge live?**

The `TACDeviceBridge` is a `QObject` created on the GUI thread with a parent widget.
Its thread affinity is the GUI thread. This is what makes `QMetaObject::invokeMethod`
with `Qt::QueuedConnection` work — Qt posts the invocation to the bridge's owning
thread (= GUI thread), where the lambda executes and emits the signal.

**GUI code using the bridge (nearly identical to old Qt code):**

```cpp
// TACFrame.cpp
void TACFrame::openDevice(AlpacaDevice device)
{
    _bridge = new TACDeviceBridge(device, this);

    // Standard Qt connect — identical to how it worked with qcommon-console
    connect(_bridge, &TACDeviceBridge::pinStateChanged,
            this, &TACFrame::updateLED);
    connect(_bridge, &TACDeviceBridge::errorEvent,
            this, &TACFrame::showError);

    _bridge->open();
}

void TACFrame::updateLED(uint64_t pin, bool state)
{
    // Runs on GUI thread — safe to update widgets
    _ledWidgets[pin]->setState(state);
}
```

**The bridge pattern per callback is 3 lines:**

```cpp
_device->onSomeEvent = [this](args...) {                    // 1. fires on worker
    QMetaObject::invokeMethod(this, [this, args...]() {     // 2. posts to GUI queue
        emit someSignal(args...);                           // 3. emits on GUI thread
    }, Qt::QueuedConnection);
};
```

**Comparison with old architecture:**

| Aspect | Old (qcommon-console) | New (tac-core + bridge) |
|--------|----------------------|------------------------|
| Event source | `emit pinStateChanged(...)` in backend | `std::function` callback in backend |
| Thread marshalling | `Qt::QueuedConnection` (automatic) | `QMetaObject::invokeMethod` (explicit) |
| GUI connection code | `connect(device, &signal, this, &slot)` | `connect(bridge, &signal, this, &slot)` |
| Widget code | Unchanged | Unchanged |
| Lines of bridge code | 0 (built into Qt) | ~50 (one-time boilerplate) |

The bridge is the ONLY file that includes both Qt headers and tac-core headers.
It is the single integration point — pure type conversion and thread marshalling,
zero business logic.

---

## 11. File Organization

```
src/libraries/tac-core/
├── TACCoreTypes.h              ← Type aliases (std:: equivalents of Qt types)
├── QCommonConsoleGlobal.h      ← Export macro (empty for static lib)
├── DebugLog.h                  ← Consolidated debug logger
├── version.h                   ← Version constants
│
├── AppCore.h/.cpp              ← Singleton: logging, preferences, licensing
├── PreferencesBase.h/.cpp      ← INI-style config (replaces QSettings)
├── ThreadedLog.h/.cpp          ← Async file logger (replaces qDebug)
│
├── DriveThread.h/.cpp          ← std::thread wrapper (replaces QThread)
├── SerialPort.h/.cpp           ← libserialport wrapper (replaces QSerialPort)
├── SerialPortInfo.h/.cpp       ← Port enumeration (replaces QSerialPortInfo)
│
├── AlpacaDevice.h/.cpp         ← Device abstraction (shared_ptr managed)
├── FTDIDevice.h/.cpp           ← FTDI board implementation
├── FTDIChipset.h/.cpp          ← FTDI enumeration & EEPROM
├── PSOCDevice.h/.cpp           ← PSoC board implementation
├── PIC32CXDevice.h/.cpp        ← PIC32CX board implementation
│
├── PlatformConfiguration.h/.cpp ← JSON config loading (nlohmann/json)
├── FTDIPlatformConfiguration.h/.cpp
├── PSOCPlatformConfiguration.h/.cpp
├── PIC32CXPlatformConfiguration.h/.cpp
│
├── ProtocolInterface.h/.cpp    ← Command queue + frame routing
├── FrameCoder.h/.cpp           ← Base class for frame encode/decode
├── FramePackage.h              ← Command/response data structure
│
├── private/
│   ├── TACDriveThread.h/.cpp   ← TAC-specific drive thread base
│   ├── TACLiteDriveThread.h/.cpp  ← FTDI drive thread
│   ├── TACPSOCDriveThread.h/.cpp  ← PSOC drive thread
│   ├── TACPIC32CXDriveThread.h/.cpp ← PIC32CX drive thread
│   ├── TACLiteProtocol.h/.cpp  ← FTDI protocol layer
│   ├── TACPSOCProtocol.h/.cpp  ← PSOC protocol layer
│   ├── TACPIC32CXProtocol.h/.cpp ← PIC32CX protocol layer
│   ├── TACLiteCoder.h/.cpp     ← FTDI frame encoder/decoder
│   ├── TACPSOCCoder.h/.cpp     ← PSOC frame encoder/decoder
│   └── TACPIC32CXCoder.h/.cpp  ← PIC32CX frame encoder/decoder
│
└── standalone/
    └── CMakeLists.txt          ← Standalone (no-Qt) build target

interfaces/C++/TACDev/
├── TACDev.h                    ← Public C API header
├── TACDev.cpp                  ← C API implementation
└── TACDevCore.h/.cpp           ← Device manager (maps handles → devices)
```

---

## 12. Summary

The tac-core transformation replaces Qt framework dependencies with C++20
standard library equivalents while preserving the existing architectural patterns
(drive threads, protocol interfaces, frame coders). The key insight is that the
hardware communication layer was **already synchronous and thread-based** — Qt's
event loop was never driving the core logic, only providing convenience wrappers.

The result is a 100% Qt-free static library that compiles with any C++20
toolchain, links into a single shared library (`TACDev.dll`), and is consumable
from Python, C#, Java, and C++ without requiring Qt installation.
