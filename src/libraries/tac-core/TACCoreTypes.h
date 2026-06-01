#ifndef TACCORETYPES_H
#define TACCORETYPES_H
/*
    Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted (subject to the limitations in the
    disclaimer below) provided that the following conditions are met:

        * Redistributions of source code must retain the above copyright
          notice, this list of conditions and the following disclaimer.

        * Redistributions in binary form must reproduce the above
          copyright notice, this list of conditions and the following
          disclaimer in the documentation and/or other materials provided
          with the distribution.

        * Neither the name of Qualcomm Technologies, Inc. nor the names of its
          contributors may be used to endorse or promote products derived
          from this software without specific prior written permission.

    NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
    GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
    HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
    WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
    MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
    ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
    GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
    IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
    OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
    IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
    TACCoreTypes.h — std type aliases for tac-core (the Qt-free backend library).

    tac-core has one purpose: zero Qt dependency.
    All files in tac-core use these aliases instead of Qt types.
    qcommon-console is untouched and continues to use Qt types directly.

    TString        replaces  QByteArray / QString  (byte-oriented string)
    TStringW       replaces  QString               (wide/display string, same as TString here)
    TList<T>       replaces  QList<T>
    TMap<K,V>      replaces  QMap<K,V>
    TSharedPtr<T>  replaces  QSharedPointer<T>
    HashType                 unchanged (uint64_t)
*/

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

// String types
using TString  = std::string;
using TStringW = std::string;

// Container types
template<typename T>
using TList = std::vector<T>;

template<typename K, typename V>
using TMap = std::map<K, V>;

// Smart pointer
template<typename T>
using TSharedPtr = std::shared_ptr<T>;

// Null check helper — replaces QSharedPointer::isNull()
template<typename T>
inline bool tIsNull(const std::shared_ptr<T>& p) { return p == nullptr; }

// Raw pointer helper — replaces QSharedPointer::data()
template<typename T>
inline T* tGet(const std::shared_ptr<T>& p) { return p.get(); }

// Integer types — replaces Qt quint*/qint* typedefs
using quint8_t_  = uint8_t;
using quint16_t_ = uint16_t;
using quint32_t_ = uint32_t;
using quint64_t_ = uint64_t;
using qint16_t_  = int16_t;
using qint32_t_  = int32_t;

#endif // TACCORETYPES_H

