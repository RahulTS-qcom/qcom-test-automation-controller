/*
	Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
	SPDX-License-Identifier: BSD-3-Clause

	Task 2B: SerialPort implementation using libserialport.

	This file provides the implementation. The sp_port* handle is managed
	directly via libserialport C API.

	For the Week 3 POC: plug in a PSOC board and verify the handshake
	completes before enabling the full migration.
*/

#include "SerialPort.h"
#include "SerialPortInfo.h"

#include <chrono>
#include <functional>
#include <thread>

// libserialport — include only in the .cpp to avoid polluting headers
#include <libserialport.h>

SerialPort::SerialPort(const std::string& portName)
    : _portName(portName)
{
}

SerialPort::SerialPort(const SerialPortInfo& portInfo)
    : _portName(portInfo.portName())
{
}

SerialPort::~SerialPort()
{
    close();
}

bool SerialPort::open()
{
    sp_port* port = nullptr;
    if (sp_get_port_by_name(_portName.c_str(), &port) != SP_OK)
    {
        _lastError = "Port not found: " + _portName;
        return false;
    }

    if (sp_open(port, SP_MODE_READ_WRITE) != SP_OK)
    {
        _lastError = sp_last_error_message();
        sp_free_port(port);
        return false;
    }

    _port = port;
    applySettings();

    if (onPortOpened) onPortOpened();
    return true;
}

void SerialPort::close()
{
    if (_port != nullptr)
    {
        sp_close(_port);
        sp_free_port(_port);
        _port = nullptr;
        if (onPortClosed) onPortClosed();
    }
}

void SerialPort::applySettings()
{
    if (_port == nullptr) return;
    sp_set_baudrate(_port, _settings._baudRate);
    sp_set_bits(_port, _settings._dataBits);
    sp_set_parity(_port, static_cast<sp_parity>(_settings._parity));
    sp_set_stopbits(_port, _settings._stopBits);
    sp_set_flowcontrol(_port, static_cast<sp_flowcontrol>(_settings._flowControl));

    // Assert DTR and RTS — QSerialPort does this by default on open().
    // Many USB-serial devices (PSoC, PIC32CX) require DTR high to enable communication.
    sp_set_dtr(_port, SP_DTR_ON);
    sp_set_rts(_port, SP_RTS_ON);
}

std::string SerialPort::readAll(uint32_t timeout_ms)
{
    if (_port == nullptr) return {};

    char buf[4096];
    sp_return n = sp_blocking_read_next(_port, buf, sizeof(buf), timeout_ms);
    if (n <= 0) return {};

    std::string result(buf, static_cast<size_t>(n));

    // Drain any remaining bytes non-blocking
    sp_return extra;
    while ((extra = sp_nonblocking_read(_port, buf, sizeof(buf))) > 0)
        result.append(buf, static_cast<size_t>(extra));

    return result;
}

int SerialPort::write(const std::string& data)
{
    if (_port == nullptr) return -1;
    sp_return n = sp_blocking_write(_port, data.data(), data.size(), 1000);
    return static_cast<int>(n);
}

bool SerialPort::waitForReadyRead(uint32_t timeout_ms)
{
    if (_port == nullptr) return false;

    // Poll for incoming data without consuming it.
    // sp_input_waiting returns number of bytes in the OS buffer.
    // If zero, sleep briefly and retry until timeout expires.
    uint32_t elapsed = 0;
    const uint32_t pollInterval = 1; // ms

    while (elapsed < timeout_ms)
    {
        sp_return waiting = sp_input_waiting(_port);
        if (waiting > 0)
            return true;
        if (waiting < 0)
            return false; // error

        std::this_thread::sleep_for(std::chrono::milliseconds(pollInterval));
        elapsed += pollInterval;
    }

    return sp_input_waiting(_port) > 0;
}

bool SerialPort::clear()
{
    if (_port == nullptr) return false;
    return sp_flush(_port, SP_BUF_BOTH) == SP_OK;
}
