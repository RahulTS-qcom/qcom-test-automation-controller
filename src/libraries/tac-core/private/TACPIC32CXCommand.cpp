// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

#include "TACPIC32CXCommand.h"
#include "TACCommands.h"
#include "TACCommandHashes.h"
#include <cassert>

TACPIC32CXCommand::TACPIC32CXCommand(SendInterface* sender, ReceiveInterface* receiver)
    : _receiver(receiver), _sender(sender), _ready(false)
{
    assert(_sender != nullptr);
    assert(_receiver != nullptr);
}

TACPIC32CXCommand::~TACPIC32CXCommand()
{
    addEndTransaction();
}

void TACPIC32CXCommand::send(const std::string& command, const Arguments& arguments, bool console, bool store)
{
    if (_sender) _sender->send(command, arguments, console, _receiver, store);
}

void TACPIC32CXCommand::addDelay(uint32_t ms)
{ if (_sender) _sender->addDelay(ms, _receiver); }

void TACPIC32CXCommand::addLogComment(const std::string& comment)
{ if (_sender) _sender->addLogComment(comment); }

void TACPIC32CXCommand::addEndTransaction()
{ if (_sender) _sender->addEndTransaction(_receiver); }

void TACPIC32CXCommand::version()     { send(kVersionCommand, {}, false); }
void TACPIC32CXCommand::name()        { send(kGetNameCommand,            {}, false); }
void TACPIC32CXCommand::uuid()        { send(kGetUUIDCommand,            {}, false); }
void TACPIC32CXCommand::platformID()  { send(kPIC32CXPlatformIDCommand,  {}, false); }  // "*IDN?"
void TACPIC32CXCommand::getResetCount()  { send(kGetResetCountCommand,   {}, false); }
void TACPIC32CXCommand::clearResetCount(){ send(kClearResetCountCommand, {}, false); }

void TACPIC32CXCommand::setPinState(uint16_t pin, bool state)
{
    // PIC32CX uses SCPI command "CONF:DIG:ON" — NOT the PSOC/FTDI "SetPin"
    Arguments args;
    args.push_back(state);
    args.push_back(static_cast<uint32_t>(pin));
    send(kPIC32CXSetPinCommand, args, false);  // "CONF:DIG:ON"
}

void TACPIC32CXCommand::setName(const std::string& newName)
{
    Arguments args;
    args.push_back(newName);
    send(kSetNameCommand, args, false);
}

void TACPIC32CXCommand::clearBuffer() { send(kPIC32CXClearBufferCommand, {}, false); }

void TACPIC32CXCommand::i2CReadRegister(uint32_t addr, uint32_t reg)
{
    Arguments args;
    args.push_back(addr);
    args.push_back(reg);
    send(kI2CReadRegisterCommand, args, false);
}

void TACPIC32CXCommand::i2CWriteRegister(uint32_t addr, uint32_t reg, uint32_t data)
{
    Arguments args;
    args.push_back(addr);
    args.push_back(reg);
    args.push_back(data);
    send(kI2CWriteRegisterCommand, args, false);
}
