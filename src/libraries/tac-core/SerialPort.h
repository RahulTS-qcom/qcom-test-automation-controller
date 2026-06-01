#ifndef SERIALPORT_H
#define SERIALPORT_H
/*
	Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
	SPDX-License-Identifier: BSD-3-Clause
*/

/*
	Author: Michael Simpson (msimpson@qti.qualcomm.com)
			Biswajit Roy (biswroy@qti.qualcomm.com)

	Task 2B: QSerialPort replaced with libserialport.
	libserialport is synchronous/blocking — matches the existing drive thread usage pattern
	(the run() loop already used waitForReadyRead(10ms) which is effectively synchronous).
*/

#include "QCommonConsoleGlobal.h"

#include <cstdint>
#include <functional>
#include <string>

// libserialport forward declaration — avoids including the full header in every TU
struct sp_port;

struct QCOMMONCONSOLE_EXPORT SerialPortSettings
{
    SerialPortSettings() = default;
    SerialPortSettings(const SerialPortSettings&) = default;

    int32_t  _baudRate{115200};
    uint32_t _timeout{0};       // ms; 0 = use default (10ms)
    int      _dataBits{8};
    int      _parity{0};        // 0=none, 1=odd, 2=even
    int      _stopBits{1};
    int      _flowControl{0};   // 0=none
};

class SerialPortInfo;

class QCOMMONCONSOLE_EXPORT SerialPort
{
public:
    explicit SerialPort(const std::string& portName);
    explicit SerialPort(const SerialPortInfo& portInfo);
    ~SerialPort();

    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    SerialPortSettings getSerialPortSettings() const { return _settings; }
    void setSerialPortSettings(const SerialPortSettings& s) { _settings = s; }

    bool open();
    void close();

    // Blocking read — waits up to timeout_ms for at least 1 byte
    std::string readAll(uint32_t timeout_ms = 10);

    // Blocking write
    int write(const std::string& data);

    // Wait for data to arrive (returns true if data available within timeout_ms)
    bool waitForReadyRead(uint32_t timeout_ms = 10);

    // Clear input/output buffers
    bool clear();

    bool isOpen() const { return _port != nullptr; }

    std::string portName() const { return _portName; }
    std::string errorString() const { return _lastError; }

    // Callbacks replacing Qt signals
    std::function<void()>          onPortOpened;
    std::function<void()>          onPortClosed;
    std::function<void()>          onReadyRead;
    std::function<void(int)>       onErrorOccurred;

private:
    sp_port*          _port{nullptr};
    std::string       _portName;
    SerialPortSettings _settings;
    std::string       _lastError;

    void applySettings();
};

#endif // SERIALPORT_H
