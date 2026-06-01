#ifndef SERIALPORTINFO_H
#define SERIALPORTINFO_H
/*
	Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
	SPDX-License-Identifier: BSD-3-Clause
*/

/*
	Author: Michael Simpson (msimpson@qti.qualcomm.com)

	Task 2B: QSerialPortInfo replaced with plain struct + libserialport enumeration.
*/

#include "QCommonConsoleGlobal.h"

#include <cstdint>
#include <string>
#include <vector>

class SerialPortInfo;
typedef std::vector<SerialPortInfo> SerialPortInfos;

class QCOMMONCONSOLE_EXPORT SerialPortInfo
{
public:
    SerialPortInfo() = default;
    explicit SerialPortInfo(const std::string& portName) : _portName(portName) {}
    SerialPortInfo(const SerialPortInfo&) = default;
    SerialPortInfo& operator=(const SerialPortInfo&) = default;

    bool isNull() const { return _portName.empty(); }

    std::string portName()     const { return _portName; }
    std::string serialNumber() const { return _serialNumber; }
    std::string description()  const { return _description; }
    uint16_t    vendorIdentifier()  const { return _vendorId; }
    uint16_t    productIdentifier() const { return _productId; }

    void setPortName(const std::string& s)     { _portName = s; }
    void setSerialNumber(const std::string& s) { _serialNumber = s; }
    void setDescription(const std::string& s)  { _description = s; }
    void setVendorIdentifier(uint16_t vid)     { _vendorId = vid; }
    void setProductIdentifier(uint16_t pid)    { _productId = pid; }

    // Returns true if this port matches the given VID/PID pair
    bool matchesVidPid(uint16_t vid, uint16_t pid) const
    {
        return _vendorId == vid && _productId == pid;
    }

    uint64_t hash() const { return _hash; }

    // Enumerate available serial ports via libserialport
    static SerialPortInfos availablePorts();

    bool operator==(const SerialPortInfo& o) const { return _portName == o._portName; }
    bool operator<=(const SerialPortInfo& o) const { return _portName <= o._portName; }
    bool operator< (const SerialPortInfo& o) const { return _portName <  o._portName; }

private:
    std::string _portName;
    std::string _serialNumber;
    std::string _description;
    uint16_t    _vendorId{0};
    uint16_t    _productId{0};
    uint64_t    _hash{0};
};

bool equal(const SerialPortInfos& si1, const SerialPortInfos& si2);

#endif // SERIALPORTINFO_H
