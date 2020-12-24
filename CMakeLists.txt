cmake_minimum_required(VERSION 3.0)

option(BUILD_LIME "Build the program with LimeSDR support" OFF)
option(BUILD_AIRSPY "Build the program with LimeSDR support" OFF)
option(BUILD_HACKRF "Build the program with HackRF support" OFF)
option(BUILD_RTLSDR "Build the program with RTLSDR support" OFF)

if(APPLE)
    include_directories(/usr/local/include)
    link_directories(/usr/local/lib)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

project(BasebandRecorder)
file(GLOB_RECURSE BasebandRecorder src/*.cpp src/*.c)

add_executable(BasebandRecorder ${BasebandRecorder})

set(wxWidgets_CONFIGURATION mswu)
find_package(wxWidgets COMPONENTS core base REQUIRED)
include(${wxWidgets_USE_FILE})

target_link_libraries(BasebandRecorder PUBLIC ${wxWidgets_LIBRARIES})

find_library(DSP_LIBRARY dsp REQUIRED)
target_link_libraries(BasebandRecorder PUBLIC ${DSP_LIBRARY})

find_library(FFT_LIBRARY fftw3 REQUIRED)
target_link_libraries(BasebandRecorder PUBLIC ${FFT_LIBRARY})

if(BUILD_LIME AND BUILD_AIRSPY)
    message(FATAL_ERROR "Only one option can be enabled!" )
endif()

if(BUILD_AIRSPY)
    find_library(AIRSPY_LIBRARY airspy REQUIRED)
    target_link_libraries(BasebandRecorder PUBLIC ${AIRSPY_LIBRARY})
    add_compile_definitions(BUILD_AIRSPY=1)
    add_compile_definitions(BUILD_LIME=0)
    add_compile_definitions(BUILD_HACKRF=0)
    add_compile_definitions(BUILD_RTLSDR=0)
endif()

if(BUILD_LIME)
    find_library(LIMESUITE_LIBRARY LimeSuite REQUIRED)
    target_link_libraries(BasebandRecorder PUBLIC ${LIMESUITE_LIBRARY})
    add_compile_definitions(BUILD_AIRSPY=0)
    add_compile_definitions(BUILD_LIME=1)
    add_compile_definitions(BUILD_HACKRF=0)
    add_compile_definitions(BUILD_RTLSDR=0)
endif()

if(BUILD_HACKRF)
    find_library(HACKRF_LIBRARY hackrf REQUIRED)
    target_link_libraries(BasebandRecorder PUBLIC ${HACKRF_LIBRARY})
    add_compile_definitions(BUILD_AIRSPY=0)
    add_compile_definitions(BUILD_LIME=0)
    add_compile_definitions(BUILD_HACKRF=1)
    add_compile_definitions(BUILD_RTLSDR=0)
endif()

if(BUILD_RTLSDR)
    find_library(RTLSDR_LIBRARY rtlsdr REQUIRED)
    target_link_libraries(BasebandRecorder PUBLIC ${RTLSDR_LIBRARY})
    add_compile_definitions(BUILD_AIRSPY=0)
    add_compile_definitions(BUILD_LIME=0)
    add_compile_definitions(BUILD_HACKRF=0)
    add_compile_definitions(BUILD_RTLSDR=1)
endif()