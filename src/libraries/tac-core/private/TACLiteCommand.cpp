// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
// SPDX-License-Identifier: BSD-3-Clause

// Author: msimpson

#include "TACLiteCommand.h"
#include "TACCommands.h"
#include "TACCommandHashes.h"

#include "ReceiveInterface.h"
#include "SendInterface.h"

#include <stdexcept>

TACLiteCommand::TACLiteCommand(SendInterface* sender, ReceiveInterface* receiver)
    : _receiver(receiver), _sender(sender), _ready(false)
{
}

TACLiteCommand::~TACLiteCommand()
{
    // CRITICAL: The protocol layer only processes/flushes queued commands when
    // it sees the end-transaction marker. Without this, commands sit in the
    // queue and are never sent to hardware.
    addEndTransaction();
}

void TACLiteCommand::version()
{
    send(kVersionCommand, {}, false);
}

void TACLiteCommand::name()
{
    send(kGetNameCommand, {}, false);
}

void TACLiteCommand::uuid()
{
    send(kGetUUIDCommand, {}, false);
}

void TACLiteCommand::setPinState(uint16_t pin, bool state)
{
    // IMPORTANT: argument order must be (state, pin) — the protocol coder
    // expects state first, pin second. Matches qcommon-console original.
    Arguments args;
    args.push_back(state);
    args.push_back(static_cast<uint32_t>(pin));
    send(kSetPinCommand, args, false);
}

void TACLiteCommand::battery(bool state)
{
    Arguments args;
    args.push_back(state);
    send(kSetBatteryCommand, args, false);
}

void TACLiteCommand::usb0(bool state)
{
    Arguments args;
    args.push_back(state);
    send(kSetUSB0Command, args, false);
}

void TACLiteCommand::usb1(bool state)
{
    Arguments args;
    args.push_back(state);
    send(kSetUSB1Command, args, false);
}

void TACLiteCommand::externalPowerControl(bool state)
{
    Arguments args;
    args.push_back(state);
    send(kSetExternalPowerControlCommand, args, false);
}

void TACLiteCommand::powerKey(bool onState)
{
    Arguments args;
    args.push_back(onState);
    send(kSetPowerKeyCommand, args, false);
}

void TACLiteCommand::volumeUp(bool onState)
{
    Arguments args;
    args.push_back(onState);
    send(kSetVolumeUpCommand, args, false);
}

void TACLiteCommand::volumeDown(bool onState)
{
    Arguments args;
    args.push_back(onState);
    send(kSetVolumeDownCommand, args, false);
}

void TACLiteCommand::setName(const std::string& newName)
{
    Arguments args;
    args.push_back(newName);
    send(kSetNameCommand, args, false);
}

void TACLiteCommand::getResetCount()
{
    send(kGetResetCountCommand, {}, false);
}

void TACLiteCommand::clearResetCount()
{
    send(kClearResetCountCommand, {}, false);
}

void TACLiteCommand::i2CReadRegister(uint32_t addr, uint32_t reg)
{
    Arguments args;
    args.push_back(addr);
    args.push_back(reg);
    send(kI2CReadRegisterCommand, args, false);
}

void TACLiteCommand::i2CWriteRegister(uint32_t addr, uint32_t reg, uint32_t data)
{
    Arguments args;
    args.push_back(addr);
    args.push_back(reg);
    args.push_back(data);
    send(kI2CWriteRegisterCommand, args, false);
}

void TACLiteCommand::send(const std::string& command, const Arguments& arguments, bool console, bool store)
{
    if (_sender)
    {
        _sender->send(command, arguments, console, _receiver, store);
    }
}

void TACLiteCommand::addDelay(uint32_t delayInMilliSeconds)
{
    if (_sender) _sender->addDelay(delayInMilliSeconds, _receiver);
}

void TACLiteCommand::addLogComment(const std::string& comment)
{
    if (_sender) _sender->addLogComment(comment);
}

void TACLiteCommand::addEndTransaction()
{
    if (_sender) _sender->addEndTransaction(_receiver);
}

void TACLiteCommand::disconnectUIM1Button(bool state)
{
    Arguments args;
    args.push_back(state);
    send(kSetDisconnectUIM1Command, args, false);
}

void TACLiteCommand::disconnectUIM2Button(bool state)
{
    Arguments args;
    args.push_back(state);
    send(kSetDisconnectUIM2Command, args, false);
}

void TACLiteCommand::forcePSHoldHigh(bool state)
{
    Arguments args;
    args.push_back(state);
    send(kSetForcePSHoldHighCommand, args, false);
}

void TACLiteCommand::disconnectSDCard(bool state)
{
    Arguments args;
    args.push_back(state);
    send(kSetDisconnectSDCardCommand, args, false);
}

void TACLiteCommand::primaryEDL(bool state)
{
    Arguments args;
    args.push_back(state);
    send(kSetPrimaryEDLCommand, args, false);
}

void TACLiteCommand::secondaryEDL(bool state)
{
    Arguments args;
    args.push_back(state);
    send(kSetSecondaryEDLCommand, args, false);
}

void TACLiteCommand::secondaryPMResinN(bool state)
{
    Arguments args;
    args.push_back(state);
    send(kSetSecondaryPM_RESIN_NCommand, args, false);
}

void TACLiteCommand::eud(bool state)
{
    Arguments args;
    args.push_back(state);
    send(kSetEUDCommand, args, false);
}

void TACLiteCommand::platformID()
{
    send(kGetPlatformIDCommand, {}, false);
}

void TACLiteCommand::headsetDisconnect(bool state)
{
    Arguments args;
    args.push_back(state);
    send(kSetHeadsetDisconnectCommand, args, false);
}
