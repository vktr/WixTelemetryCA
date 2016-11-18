cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

project("WixTelemetryExtension")

# Get current architecture
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH "x64" )
else(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH "x86")
endif(CMAKE_SIZEOF_VOID_P EQUAL 8)

set(SOURCES
    src/TelemetryExtension
)

include_directories(
    tools/WiX/tools/sdk/inc
)

link_directories(
    tools/WiX/tools/sdk/vs2015/lib/${ARCH}/
)

add_definitions(
    -D_SCL_SECURE_NO_WARNINGS
    -D_UNICODE
    -D_WIN32
    -D_WIN32_WINNT=0x0600
    -DUNICODE
    -DWIN32
    -DWIN32_LEAN_AND_MEAN
)

add_library(
    WixTelemetryExtension
    SHARED
    ${SOURCES}
)

target_link_libraries(
    WixTelemetryExtension
    dutil
    mincore
    msi
    wcautil
)
