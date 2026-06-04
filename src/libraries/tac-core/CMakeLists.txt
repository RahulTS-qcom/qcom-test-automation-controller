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

    # -----------------------------------------------------------------------
    # ftd2xx — in the full Qt build this imported target is provided by
    # third-party/. In standalone top-level mode nothing sets it up, so do it
    # here (mirrors standalone/CMakeLists.txt). The static lib is shared with
    # the other builds under __Builds/; download + extract if it is missing.
    #
    # ftd2xx.h ships inside qcommon-console, so the header dir is attached to
    # the imported target's interface includes — FTDIDevice.cpp and friends
    # then resolve it transitively via target_link_libraries(... ftd2xx).
    # -----------------------------------------------------------------------
    if(WIN32)
        set(_ftdi_cfg "x64")
    else()
        set(_ftdi_cfg "Linux")
    endif()
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_ftdi_cfg "${_ftdi_cfg}/Debug")
    else()
        set(_ftdi_cfg "${_ftdi_cfg}/Release")
    endif()
    set(_ftdi_buildroot "${REPO_ROOT}/__Builds/${_ftdi_cfg}")

    if(WIN32)
        set(FTDI_LIB "${_ftdi_buildroot}/lib/ftd2xx.lib")
        set(FTDI_URL "https://ftdichip.com/wp-content/uploads/2025/03/CDM-v2.12.36.20-WHQL-Certified.zip")
        set(_ftdi_main_archive "${REPO_ROOT}/third-party/CDM-v2.12.36.20-WHQL-Certified.zip")
    else()
        set(FTDI_LIB "${_ftdi_buildroot}/lib/libftd2xx.a")
        set(FTDI_URL "https://ftdichip.com/wp-content/uploads/2025/03/libftd2xx-linux-x86_64-1.4.33.tgz")
        set(_ftdi_main_archive "${REPO_ROOT}/third-party/libftd2xx-linux-x86_64-1.4.33.tgz")
    endif()

    if(NOT EXISTS "${FTDI_LIB}")
        message(STATUS "ftd2xx library not found at ${FTDI_LIB}")
        file(MAKE_DIRECTORY "${_ftdi_buildroot}/lib")

        if(EXISTS "${_ftdi_main_archive}")
            set(FTDI_ARCHIVE "${_ftdi_main_archive}")
            message(STATUS "Using pre-downloaded archive: ${FTDI_ARCHIVE}")
        else()
            set(FTDI_ARCHIVE "${CMAKE_CURRENT_BINARY_DIR}/ftdi_archive")
            if(EXISTS "${FTDI_ARCHIVE}")
                file(REMOVE "${FTDI_ARCHIVE}")
            endif()
            message(STATUS "Downloading ftd2xx from ${FTDI_URL}...")
            file(DOWNLOAD "${FTDI_URL}" "${FTDI_ARCHIVE}" SHOW_PROGRESS STATUS _dl_status TIMEOUT 300)
            list(GET _dl_status 0 _dl_code)
            if(NOT _dl_code EQUAL 0)
                message(FATAL_ERROR
                    "Failed to download ftd2xx (error ${_dl_code}).\n"
                    "The FTDI website may block automated downloads.\n"
                    "Fix: run the full build once (build.bat) to download via third-party/CMakeLists.txt,\n"
                    "     or manually download from:\n"
                    "       ${FTDI_URL}\n"
                    "     and place it at:\n"
                    "       ${_ftdi_main_archive}")
            endif()
        endif()

        set(FTDI_EXTRACT "${CMAKE_CURRENT_BINARY_DIR}/ftdi_extract")
        file(REMOVE_RECURSE "${FTDI_EXTRACT}")
        file(MAKE_DIRECTORY "${FTDI_EXTRACT}")
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf "${FTDI_ARCHIVE}"
                        WORKING_DIRECTORY "${FTDI_EXTRACT}"
                        RESULT_VARIABLE _tar_result)
        if(NOT _tar_result EQUAL 0)
            message(FATAL_ERROR "Failed to extract ${FTDI_ARCHIVE} (exit code ${_tar_result})")
        endif()

        if(WIN32)
            file(GLOB _lib "${FTDI_EXTRACT}/amd64/ftd2xx.lib")
            if(NOT _lib)
                file(GLOB _lib "${FTDI_EXTRACT}/Static/amd64/ftd2xx.lib")
            endif()
            if(NOT _lib)
                file(GLOB_RECURSE _lib "${FTDI_EXTRACT}/**/ftd2xx.lib")
            endif()
            list(LENGTH _lib _lib_count)
            if(_lib_count EQUAL 0)
                message(FATAL_ERROR "Could not find ftd2xx.lib in ${FTDI_EXTRACT}")
            endif()
            list(GET _lib 0 _lib)
            file(COPY "${_lib}" DESTINATION "${_ftdi_buildroot}/lib")
        else()
            file(GLOB _lib "${FTDI_EXTRACT}/*/libftd2xx-static.a")
            if(NOT _lib)
                file(GLOB_RECURSE _lib "${FTDI_EXTRACT}/**/libftd2xx-static.a")
            endif()
            list(LENGTH _lib _lib_count)
            if(_lib_count EQUAL 0)
                message(FATAL_ERROR "Could not find libftd2xx in ${FTDI_EXTRACT}")
            endif()
            list(GET _lib 0 _lib)
            file(COPY "${_lib}" DESTINATION "${_ftdi_buildroot}/lib")
            get_filename_component(_lib_name "${_lib}" NAME)
            if(NOT _lib_name STREQUAL "libftd2xx.a")
                file(RENAME "${_ftdi_buildroot}/lib/${_lib_name}" "${FTDI_LIB}")
            endif()
        endif()

        file(REMOVE_RECURSE "${FTDI_EXTRACT}")
        message(STATUS "ftd2xx installed to ${FTDI_LIB}")
    endif()

    add_library(ftd2xx STATIC IMPORTED)
    set_target_properties(ftd2xx PROPERTIES
        IMPORTED_LOCATION "${FTDI_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${QCOMMONCONSOLE}")
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
    SerialPortInfo.cpp SerialPortInfo.h SerialPortInfoPlatform.h
    TickCount.cpp TickCount.h

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

# Platform-divergent SerialPortInfo details — select the implementation at build
# time rather than guarding with preprocessor directives (mirrors the
# windows.c / linux.c selection for libserialport above).
if(WIN32)
    target_sources(${LIBRARY_NAME} PRIVATE SerialPortInfo_win.cpp)
else()
    target_sources(${LIBRARY_NAME} PRIVATE SerialPortInfo_posix.cpp)
endif()

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
