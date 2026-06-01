cmake_minimum_required(VERSION 3.16)
project(tacdev-standalone VERSION 1.0 LANGUAGES CXX C)

# ---------------------------------------------------------------------------
# Standalone build: tac-core (static) + TACDev (shared DLL).
# No Qt required.
#
# Usage (from repo root, x64 Native Tools Command Prompt for VS 2022):
#
#   cmake -S standalone -B build/standalone -DCMAKE_BUILD_TYPE=Release -G Ninja
#   cmake --build build/standalone
# ---------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# REPO_ROOT — defaults to parent of this CMakeLists.txt (standalone -> repo root)
if(NOT DEFINED REPO_ROOT)
    get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
endif()
message(STATUS "REPO_ROOT = ${REPO_ROOT}")

# ---------------------------------------------------------------------------
# Build output paths (mirror the main build layout)
# ---------------------------------------------------------------------------
set(ALPACA_VERSION "5.5.2")

if(WIN32)
    set(CONFIGURATION "x64")
else()
    set(CONFIGURATION "Linux")
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CONFIGURATION "${CONFIGURATION}/Debug")
else()
    set(CONFIGURATION "${CONFIGURATION}/Release")
endif()

set(BUILDROOT "${REPO_ROOT}/__Builds/${CONFIGURATION}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BUILDROOT}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${BUILDROOT}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${BUILDROOT}/lib")

# ---------------------------------------------------------------------------
# ftd2xx — download if not present, then import
# ---------------------------------------------------------------------------
if(WIN32)
    set(FTDI_LIB "${BUILDROOT}/lib/ftd2xx.lib")
    set(FTDI_URL "https://ftdichip.com/wp-content/uploads/2025/03/CDM-v2.12.36.20-WHQL-Certified.zip")
    set(FTDI_ARCHIVE "${CMAKE_CURRENT_BINARY_DIR}/ftdi.zip")
    set(FTDI_EXTRACT "${CMAKE_CURRENT_BINARY_DIR}/ftdi_extract")
else()
    set(FTDI_LIB "${BUILDROOT}/lib/libftd2xx.a")
    set(FTDI_URL "https://ftdichip.com/wp-content/uploads/2025/03/libftd2xx-linux-x86_64-1.4.33.tgz")
    set(FTDI_ARCHIVE "${CMAKE_CURRENT_BINARY_DIR}/ftdi.tgz")
    set(FTDI_EXTRACT "${CMAKE_CURRENT_BINARY_DIR}/ftdi_extract")
endif()

if(NOT EXISTS "${FTDI_LIB}")
    message(STATUS "ftd2xx library not found at ${FTDI_LIB}")
    file(MAKE_DIRECTORY "${BUILDROOT}/lib")

    # Check if the main build already downloaded the archive to third-party/
    if(WIN32)
        set(FTDI_MAIN_ARCHIVE "${REPO_ROOT}/third-party/CDM-v2.12.36.20-WHQL-Certified.zip")
    else()
        set(FTDI_MAIN_ARCHIVE "${REPO_ROOT}/third-party/libftd2xx-linux-x86_64-1.4.33.tgz")
    endif()

    if(EXISTS "${FTDI_MAIN_ARCHIVE}")
        set(FTDI_ARCHIVE "${FTDI_MAIN_ARCHIVE}")
        message(STATUS "Using pre-downloaded archive: ${FTDI_ARCHIVE}")
    else()
        # Try downloading from FTDI (may fail if site blocks automated requests)
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
                "       ${FTDI_MAIN_ARCHIVE}")
        endif()
    endif()

    # Extract
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
        file(COPY "${_lib}" DESTINATION "${BUILDROOT}/lib")
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
        file(COPY "${_lib}" DESTINATION "${BUILDROOT}/lib")
        get_filename_component(_lib_name "${_lib}" NAME)
        if(NOT _lib_name STREQUAL "libftd2xx.a")
            file(RENAME "${BUILDROOT}/lib/${_lib_name}" "${FTDI_LIB}")
        endif()
    endif()

    file(REMOVE_RECURSE "${FTDI_EXTRACT}")
    message(STATUS "ftd2xx installed to ${FTDI_LIB}")
endif()

add_library(ftd2xx STATIC IMPORTED)
set_target_properties(ftd2xx PROPERTIES IMPORTED_LOCATION "${FTDI_LIB}")

# ---------------------------------------------------------------------------
# nlohmann/json — header-only
# ---------------------------------------------------------------------------
include(FetchContent)
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
# libserialport — compiled from source
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

add_library(serialport STATIC ${LIBSERIALPORT_SOURCES})
target_include_directories(serialport PUBLIC "${libserialport_SOURCE_DIR}")
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
# tac-core — static library
# ---------------------------------------------------------------------------
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(TAC_CORE_NAME tac-cored)
else()
    set(TAC_CORE_NAME tac-core)
endif()

set(TAC_CORE_DIR "${REPO_ROOT}/src/libraries/tac-core")

add_library(${TAC_CORE_NAME} STATIC
    # Core infra
    ${TAC_CORE_DIR}/QCommonConsoleGlobal.h
    ${TAC_CORE_DIR}/TACCoreTypes.h
    ${TAC_CORE_DIR}/DebugLog.h
    ${TAC_CORE_DIR}/version.h
    ${TAC_CORE_DIR}/StringUtilities.cpp
    ${TAC_CORE_DIR}/StringUtilities.h
    ${TAC_CORE_DIR}/StringProof.cpp
    ${TAC_CORE_DIR}/StringProof.h
    ${TAC_CORE_DIR}/RangedContainer.h
    ${TAC_CORE_DIR}/Range.h
    ${TAC_CORE_DIR}/Notification.h
    ${TAC_CORE_DIR}/TACException.h

    # Types & commands
    ${TAC_CORE_DIR}/PinID.h
    ${TAC_CORE_DIR}/FTDIPinSet.h
    ${TAC_CORE_DIR}/CommandGroup.h
    ${TAC_CORE_DIR}/DebugBoardType.cpp
    ${TAC_CORE_DIR}/DebugBoardType.h
    ${TAC_CORE_DIR}/PlatformID.cpp
    ${TAC_CORE_DIR}/PlatformID.h
    ${TAC_CORE_DIR}/TACCommand.cpp
    ${TAC_CORE_DIR}/TACCommand.h
    ${TAC_CORE_DIR}/TACCommands.cpp
    ${TAC_CORE_DIR}/TACCommands.h
    ${TAC_CORE_DIR}/TACCommandHashes.h
    ${TAC_CORE_DIR}/CharBit.cpp
    ${TAC_CORE_DIR}/CharBit.h

    # Device layer
    ${TAC_CORE_DIR}/AlpacaDevice.cpp
    ${TAC_CORE_DIR}/AlpacaDevice.h
    ${TAC_CORE_DIR}/FTDIDevice.cpp
    ${TAC_CORE_DIR}/FTDIDevice.h
    ${TAC_CORE_DIR}/PSOCDevice.cpp
    ${TAC_CORE_DIR}/PSOCDevice.h
    ${TAC_CORE_DIR}/PIC32CXDevice.cpp
    ${TAC_CORE_DIR}/PIC32CXDevice.h

    # Platform configuration
    ${TAC_CORE_DIR}/PlatformConfiguration.cpp
    ${TAC_CORE_DIR}/PlatformConfiguration.h
    ${TAC_CORE_DIR}/PlatformConfigurationException.cpp
    ${TAC_CORE_DIR}/PlatformConfigurationException.h
    ${TAC_CORE_DIR}/FTDIPlatformConfiguration.cpp
    ${TAC_CORE_DIR}/FTDIPlatformConfiguration.h
    ${TAC_CORE_DIR}/PSOCPlatformConfiguration.cpp
    ${TAC_CORE_DIR}/PSOCPlatformConfiguration.h
    ${TAC_CORE_DIR}/PIC32CXPlatformConfiguration.cpp
    ${TAC_CORE_DIR}/PIC32CXPlatformConfiguration.h
    ${TAC_CORE_DIR}/Button.cpp
    ${TAC_CORE_DIR}/Button.h
    ${TAC_CORE_DIR}/Tabs.cpp
    ${TAC_CORE_DIR}/Tabs.h
    ${TAC_CORE_DIR}/USBDescriptors.cpp
    ${TAC_CORE_DIR}/USBDescriptors.h
    ${TAC_CORE_DIR}/ScriptVariable.cpp
    ${TAC_CORE_DIR}/ScriptVariable.h
    ${TAC_CORE_DIR}/AlpacaScript.cpp
    ${TAC_CORE_DIR}/AlpacaScript.h

    # Communication layer
    ${TAC_CORE_DIR}/DriveThread.cpp
    ${TAC_CORE_DIR}/DriveThread.h
    ${TAC_CORE_DIR}/SendInterface.h
    ${TAC_CORE_DIR}/ReceiveInterface.h
    ${TAC_CORE_DIR}/ProtocolInterface.cpp
    ${TAC_CORE_DIR}/ProtocolInterface.h
    ${TAC_CORE_DIR}/FrameCoder.cpp
    ${TAC_CORE_DIR}/FrameCoder.h
    ${TAC_CORE_DIR}/TickCount.cpp
    ${TAC_CORE_DIR}/TickCount.h
    ${TAC_CORE_DIR}/FramePackage.h
    ${TAC_CORE_DIR}/SerialPort.cpp
    ${TAC_CORE_DIR}/SerialPort.h
    ${TAC_CORE_DIR}/SerialPortInfo.cpp
    ${TAC_CORE_DIR}/SerialPortInfo.h

    # Private drive threads & protocols
    ${TAC_CORE_DIR}/private/TACDriveThread.cpp
    ${TAC_CORE_DIR}/private/TACDriveThread.h
    ${TAC_CORE_DIR}/private/TACLiteDriveThread.cpp
    ${TAC_CORE_DIR}/private/TACLiteDriveThread.h
    ${TAC_CORE_DIR}/private/TACLiteCoder.cpp
    ${TAC_CORE_DIR}/private/TACLiteCoder.h
    ${TAC_CORE_DIR}/private/TACLiteCommand.cpp
    ${TAC_CORE_DIR}/private/TACLiteCommand.h
    ${TAC_CORE_DIR}/private/TACLiteProtocol.cpp
    ${TAC_CORE_DIR}/private/TACLiteProtocol.h
    ${TAC_CORE_DIR}/private/TACPSOCDriveThread.cpp
    ${TAC_CORE_DIR}/private/TACPSOCDriveThread.h
    ${TAC_CORE_DIR}/private/TACPSOCCoder.cpp
    ${TAC_CORE_DIR}/private/TACPSOCCoder.h
    ${TAC_CORE_DIR}/private/TACPSOCCommand.cpp
    ${TAC_CORE_DIR}/private/TACPSOCCommand.h
    ${TAC_CORE_DIR}/private/TACPSOCProtocol.cpp
    ${TAC_CORE_DIR}/private/TACPSOCProtocol.h
    ${TAC_CORE_DIR}/private/TACPIC32CXDriveThread.cpp
    ${TAC_CORE_DIR}/private/TACPIC32CXDriveThread.h
    ${TAC_CORE_DIR}/private/TACPIC32CXCoder.cpp
    ${TAC_CORE_DIR}/private/TACPIC32CXCoder.h
    ${TAC_CORE_DIR}/private/TACPIC32CXCommand.cpp
    ${TAC_CORE_DIR}/private/TACPIC32CXCommand.h
    ${TAC_CORE_DIR}/private/TACPIC32CXProtocol.cpp
    ${TAC_CORE_DIR}/private/TACPIC32CXProtocol.h

    # App infrastructure
    ${TAC_CORE_DIR}/AppCore.cpp
    ${TAC_CORE_DIR}/AppCore.h
    ${TAC_CORE_DIR}/AlpacaSharedLibrary.cpp
    ${TAC_CORE_DIR}/AlpacaSharedLibrary.h
    ${TAC_CORE_DIR}/PreferencesBase.cpp
    ${TAC_CORE_DIR}/PreferencesBase.h
    ${TAC_CORE_DIR}/TACPreferences.cpp
    ${TAC_CORE_DIR}/TACPreferences.h
    ${TAC_CORE_DIR}/ThreadedLog.cpp
    ${TAC_CORE_DIR}/ThreadedLog.h
    ${TAC_CORE_DIR}/QCommonConsole.cpp
    ${TAC_CORE_DIR}/QCommonConsole.h
    ${TAC_CORE_DIR}/ConsoleApplicationEnhancements.cpp
    ${TAC_CORE_DIR}/ConsoleApplicationEnhancements.h
    ${TAC_CORE_DIR}/AlpacaDefines.cpp
    ${TAC_CORE_DIR}/AlpacaDefines.h
    ${TAC_CORE_DIR}/TACDefines.h

    # FTDI specific
    ${TAC_CORE_DIR}/FTDIChipset.cpp
    ${TAC_CORE_DIR}/FTDIChipset.h
)

target_include_directories(${TAC_CORE_NAME} PUBLIC
    ${TAC_CORE_DIR}
    # ftd2xx.h lives in qcommon-console
    ${REPO_ROOT}/src/libraries/qcommon-console
)

target_compile_definitions(${TAC_CORE_NAME} PUBLIC
    TACCORE_STATIC
    $<$<CONFIG:Debug>:DEBUG;_DEBUG>
    $<$<NOT:$<CONFIG:Debug>>:NDEBUG;_NDEBUG;RELEASE>
    $<$<PLATFORM_ID:Windows>:FTD2XX_STATIC;_WIN32>
)

if(MSVC)
    target_compile_options(${TAC_CORE_NAME} PRIVATE /W3 /WX-)
endif()

target_link_libraries(${TAC_CORE_NAME} PUBLIC ftd2xx nlohmann_json::nlohmann_json serialport)

# ---------------------------------------------------------------------------
# TACDev — shared library (DLL)
# ---------------------------------------------------------------------------
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(TACDEV_NAME TACDevd)
else()
    set(TACDEV_NAME TACDev)
endif()

set(TACDEV_DIR "${REPO_ROOT}/interfaces/C++/TACDev")

add_library(${TACDEV_NAME} SHARED
    ${TACDEV_DIR}/TACDev.cpp
    ${TACDEV_DIR}/TACDev.h
    ${TACDEV_DIR}/TACDevCore.cpp
    ${TACDEV_DIR}/TACDevCore.h
)

target_include_directories(${TACDEV_NAME} PUBLIC
    ${TAC_CORE_DIR}
    ${REPO_ROOT}/src/libraries/qcommon-console
)

target_compile_definitions(${TACDEV_NAME} PRIVATE
    TACDEV_LIBRARY
)

target_link_libraries(${TACDEV_NAME} PRIVATE ${TAC_CORE_NAME})

if(WIN32)
    target_link_libraries(${TACDEV_NAME} PRIVATE
        Advapi32
        Ole32
        SetupAPI
        Shell32
        Winusb
        Wtsapi32
    )

    target_compile_options(${TACDEV_NAME} PRIVATE
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Debug>>:/Zi>
    )
endif()

if(UNIX)
    target_compile_options(${TACDEV_NAME} PRIVATE
        -Wno-comment
        -Wno-conversion-null
        -Wno-reorder
        -Wno-unknown-pragmas
        -Wno-unused-function
        -Wno-unused-result
        -Wno-write-strings
    )
endif()

message(STATUS "TACDev standalone target: ${TACDEV_NAME}")
message(STATUS "Build output: ${BUILDROOT}/bin")
