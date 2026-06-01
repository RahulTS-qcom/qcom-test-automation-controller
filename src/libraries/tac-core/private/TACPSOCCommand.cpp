// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

#include "TACPSOCCommand.h"
#include "TACCommands.h"
#include "TACCommandHashes.h"
#include <cassert>
#include <cstdio>
#include <stdexcept>

TACPSOCCommand::TACPSOCCommand(SendInterface* sender, ReceiveInterface* receiver)
    : _receiver(receiver), _sender(sender), _ready(false)
{
    assert(_sender != nullptr);
    assert(_receiver != nullptr);
}

TACPSOCCommand::~TACPSOCCommand()
{
    addEndTransaction();
}

void TACPSOCCommand::send(const std::string& command, const Arguments& arguments, bool console, bool store)
{
    if (_sender) _sender->send(command, arguments, console, _receiver, store);
}

void TACPSOCCommand::addDelay(uint32_t ms)
{ if (_sender) _sender->addDelay(ms, _receiver); }

void TACPSOCCommand::addLogComment(const std::string& comment)
{ if (_sender) _sender->addLogComment(comment); }

void TACPSOCCommand::addEndTransaction()
{ if (_sender) _sender->addEndTransaction(_receiver); }

void TACPSOCCommand::version()     { send(kVersionCommand,      {}, false); }
void TACPSOCCommand::name()        { send(kGetNameCommand,       {}, false); }
void TACPSOCCommand::uuid()        { send(kGetUUIDCommand,       {}, false); }
void TACPSOCCommand::platformID()  { send(kGetPlatformIDCommand, {}, false); }
void TACPSOCCommand::getResetCount()  { send(kGetResetCountCommand,   {}, false); }
void TACPSOCCommand::clearResetCount(){ send(kClearResetCountCommand, {}, false); }

void TACPSOCCommand::setPinState(uint16_t pin, bool state)
{
    Arguments args;
    args.push_back(state);
    args.push_back(static_cast<uint32_t>(pin));
    send(kSetPinCommand, args, false);
}

void TACPSOCCommand::setName(const std::string& newName)
{
    Arguments args;
    args.push_back(newName);
    send(kSetNameCommand, args, false);
}

static std::string toHexByte(uint32_t value)
{
    char buf[8];
    std::snprintf(buf, sizeof(buf), "0x%02x", value & 0xFF);
    return buf;
}

void TACPSOCCommand::i2CReadRegister(uint32_t addr, uint32_t reg)
{
    if (addr > 0xFF) throw std::out_of_range("Error: invalid address");
    if (reg > 0xFF) throw std::out_of_range("Error: invalid register");

    Arguments args;
    args.push_back(toHexByte(addr) + " " + toHexByte(reg));
    send(kI2CReadRegisterCommand, args, false, false);

    addDelay(500);

    Arguments empty;
    send(kI2CReadRegisterValueCommand, empty, false, false);
}

void TACPSOCCommand::i2CWriteRegister(uint32_t addr, uint32_t reg, uint32_t data)
{
    if (addr > 0xFF) throw std::out_of_range("Error: invalid address");
    if (reg > 0xFF) throw std::out_of_range("Error: invalid register");
    if (data > 0xFF) throw std::out_of_range("Error: invalid data");

    Arguments args;
    args.push_back(toHexByte(addr) + " " + toHexByte(reg) + " " + toHexByte(data));
    send(kI2CWriteRegisterCommand, args, false, false);
}
