# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted (subject to the limitations in the
# disclaimer below) provided that the following conditions are met:
#
#     * Redistributions of source code must retain the above copyright
#         notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above
#         copyright notice, this list of conditions and the following
#         disclaimer in the documentation and/or other materials provided
#         with the distribution.
#
#     * Neither the name of Qualcomm Technologies, Inc. nor the names of its
#         contributors may be used to endorse or promote products derived
#         from this software without specific prior written permission.
#
# NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
# GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
# HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
# IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# tac-core: Qt-free backend library for TACDev.
#
# This library has one purpose: zero Qt dependency.
# qcommon-console is untouched and continues to use Qt.
# tac-core is the target state — std types, no Qt runtime required.
#
# During migration, files still containing Qt includes will cause link errors
# against Qt symbols. That is the correct signal that migration is incomplete.

# When built as a standalone project (not as subdirectory of the full Qt project),
# provide minimal configuration without Common.cmake.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    # Standalone mode — tac-core is the top-level project
    cmake_minimum_required(VERSION 3.16)
    project(tac-core VERSION 1.0 LANGUAGES CXX C)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(ALPACA_VERSION "5.5.2")

    get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
    set(QCOMMONCONSOLE "${REPO_ROOT}/src/libraries/qcommon-console")

    set(BUILDROOT "${CMAKE_BINARY_DIR}/output")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BUILDROOT}/bin")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${BUILDROOT}/lib")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${BUILDROOT}/lib")

    function(QCommonConsoleSettings target_name)
        # No-op in standalone mode
    endfunction()
else()
    # Subdirectory mode — included from full project
    include(${CMAKE_CURRENT_SOURCE_DIR}/Common.cmake)
endif()

include(FetchContent)

# ---------------------------------------------------------------------------
# nlohmann/json — header-only, for USBDescriptors and PlatformConfiguration
# ---------------------------------------------------------------------------
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

# ---------------------------------------------------------------------------
# libserialport — compiled from source for PSOC/PIC32CX serial communication
# ---------------------------------------------------------------------------
FetchContent_Declare(
    libserialport
    GIT_REPOSITORY https://github.com/sigrokproject/libserialport.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(libserialport)
if(NOT libserialport_POPULATED)
    FetchContent_Populate(libserialport)
endif()

set(LIBSERIALPORT_CONFIG_DIR "${CMAKE_CURRENT_BINARY_DIR}/libserialport_config")
file(MAKE_DIRECTORY "${LIBSERIALPORT_CONFIG_DIR}")

if(NOT WIN32)
    file(WRITE "${LIBSERIALPORT_CONFIG_DIR}/config.h" [=[
/* Generated config.h for libserialport on Linux */
#define SP_PRIV __attribute__((visibility("hidden")))
#define SP_API  __attribute__((visibility("default")))
#define HAVE_STRUCT_SERIAL_STRUCT 1
#define HAVE_STRUCT_TERMIOS2 1
#define HAVE_DECL_BOTHER 1
#define HAVE_REALPATH 1
#define HAVE_FLOCK 1
]=])
endif()

set(LIBSERIALPORT_SOURCES
    "${libserialport_SOURCE_DIR}/serialport.c"
    "${libserialport_SOURCE_DIR}/timing.c"
)

if(WIN32)
    list(APPEND LIBSERIALPORT_SOURCES "${libserialport_SOURCE_DIR}/windows.c")
else()
    list(APPEND LIBSERIALPORT_SOURCES
        "${libserialport_SOURCE_DIR}/linux.c"
        "${libserialport_SOURCE_DIR}/linux_termios.c"
    )
endif()

enable_language(C)
add_library(serialport STATIC ${LIBSERIALPORT_SOURCES})
target_include_directories(serialport PUBLIC
    "${libserialport_SOURCE_DIR}"
)
if(NOT WIN32)
    target_include_directories(serialport PRIVATE "${LIBSERIALPORT_CONFIG_DIR}")
endif()
target_compile_definitions(serialport
    PUBLIC  $<$<PLATFORM_ID:Windows>:LIBSERIALPORT_MSBUILD>
    PRIVATE $<$<PLATFORM_ID:Windows>:_WIN32>
    PRIVATE $<$<NOT:$<PLATFORM_ID:Windows>>:LIBSERIALPORT_ATBUILD>
)
if(WIN32)
    target_link_libraries(serialport PRIVATE setupapi)
else()
    target_link_libraries(serialport PRIVATE pthread)
endif()
if(MSVC)
    target_compile_options(serialport PRIVATE /W0)
else()
    target_compile_options(serialport PRIVATE -w)
endif()

# ---------------------------------------------------------------------------

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(LIBRARY_NAME tac-cored)
else()
    set(LIBRARY_NAME tac-core)
endif()

add_library(${LIBRARY_NAME} SHARED
    # Core infra
    TACCoreExport.h
    QCommonConsoleGlobal.h
    TACCoreTypes.h
    DebugLog.h
    version.h
    StringUtilities.cpp StringUtilities.h
    StringProof.cpp StringProof.h
    RangedContainer.h
    Range.h
    Notification.h
    TACException.h

    # Types & commands
    PinID.h
    PinEntry.h
    FTDIPinSet.h
    CommandGroup.h
    DebugBoardType.cpp DebugBoardType.h
    PlatformID.cpp PlatformID.h
    TACCommand.cpp TACCommand.h
    TACCommands.cpp TACCommands.h
    TACCommandHashes.h
    CharBit.cpp CharBit.h

    # Device layer
    AlpacaDevice.cpp AlpacaDevice.h
    FTDIDevice.cpp FTDIDevice.h
    PSOCDevice.cpp PSOCDevice.h
    PIC32CXDevice.cpp PIC32CXDevice.h

    # Platform configuration
    PlatformConfiguration.cpp PlatformConfiguration.h
    PlatformConfigurationException.cpp PlatformConfigurationException.h
    FTDIPlatformConfiguration.cpp FTDIPlatformConfiguration.h
    PSOCPlatformConfiguration.cpp PSOCPlatformConfiguration.h
    PIC32CXPlatformConfiguration.cpp PIC32CXPlatformConfiguration.h
    Button.cpp Button.h
    Tabs.cpp Tabs.h
    USBDescriptors.cpp USBDescriptors.h
    ScriptVariable.cpp ScriptVariable.h
    AlpacaScript.cpp AlpacaScript.h

    # Communication layer
    DriveThread.cpp DriveThread.h
    SendInterface.h
    ReceiveInterface.h
    ProtocolInterface.cpp ProtocolInterface.h
    FrameCoder.cpp FrameCoder.h
    FramePackage.h
    SerialPort.cpp SerialPort.h
    SerialPortInfo.cpp SerialPortInfo.h

    # Private drive threads & protocols
    private/TACDriveThread.cpp private/TACDriveThread.h
    private/TACLiteDriveThread.cpp private/TACLiteDriveThread.h
    private/TACLiteCoder.cpp private/TACLiteCoder.h
    private/TACLiteCommand.cpp private/TACLiteCommand.h
    private/TACLiteProtocol.cpp private/TACLiteProtocol.h
    private/TACPSOCDriveThread.cpp private/TACPSOCDriveThread.h
    private/TACPSOCCoder.cpp private/TACPSOCCoder.h
    private/TACPSOCCommand.cpp private/TACPSOCCommand.h
    private/TACPSOCProtocol.cpp private/TACPSOCProtocol.h
    private/TACPIC32CXDriveThread.cpp private/TACPIC32CXDriveThread.h
    private/TACPIC32CXCoder.cpp private/TACPIC32CXCoder.h
    private/TACPIC32CXCommand.cpp private/TACPIC32CXCommand.h
    private/TACPIC32CXProtocol.cpp private/TACPIC32CXProtocol.h

    # App infrastructure
    AppCore.cpp AppCore.h
    AlpacaSharedLibrary.cpp AlpacaSharedLibrary.h
    PreferencesBase.cpp PreferencesBase.h
    TACPreferences.cpp TACPreferences.h
    ThreadedLog.cpp ThreadedLog.h
    QCommonConsole.cpp QCommonConsole.h
    ConsoleApplicationEnhancements.cpp ConsoleApplicationEnhancements.h
    AlpacaDefines.cpp AlpacaDefines.h
    TACDefines.h

    # FTDI specific
    FTDIChipset.cpp FTDIChipset.h
)

set_target_properties(${LIBRARY_NAME} PROPERTIES
    VERSION ALPACA_VERSION
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)

QCommonConsoleSettings(${LIBRARY_NAME})

target_include_directories(${LIBRARY_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(${LIBRARY_NAME}
    PRIVATE
        TACCORE_LIBRARY
        $<$<PLATFORM_ID:Windows>:FTD2XX_STATIC>
        $<$<PLATFORM_ID:Windows>:_WIN32>
    PUBLIC
        $<$<CONFIG:Debug>:DEBUG;_DEBUG>
        $<$<NOT:$<CONFIG:Debug>>:NDEBUG;_NDEBUG;RELEASE>
)

# tac-core links only ftd2xx + libserialport + nlohmann_json — no Qt libraries.
target_link_libraries(${LIBRARY_NAME} PUBLIC
    ftd2xx
    nlohmann_json::nlohmann_json
    serialport
)

if(WIN32)
    target_link_libraries(${LIBRARY_NAME} PRIVATE setupapi)
endif()
