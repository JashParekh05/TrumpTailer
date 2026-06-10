cmake_minimum_required(VERSION 3.26)
project(trumptailer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(TT_BUILD_TESTS "Build C++ unit tests" OFF)

# Core library: pure compute, no Python dependency.
add_library(tt_core STATIC
    cpp/src/placeholder.cpp
    cpp/src/clock.cpp
    cpp/src/bar_store.cpp
    cpp/src/event_study.cpp
    cpp/src/metrics.cpp
    cpp/src/backtester.cpp
    cpp/src/sizing.cpp
)
target_include_directories(tt_core PUBLIC cpp/include)
target_compile_options(tt_core PRIVATE -Wall -Wextra)
# Linked into the pybind11 shared module, so it must be position-independent.
set_target_properties(tt_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Python bindings (skipped in standalone test builds without Python dev headers).
find_package(Python COMPONENTS Interpreter Development.Module QUIET)
find_package(pybind11 CONFIG QUIET)
if(NOT pybind11_FOUND AND Python_FOUND)
    # Standalone CMake build: locate pip-installed pybind11.
    execute_process(
        COMMAND "${Python_EXECUTABLE}" -m pybind11 --cmakedir
        OUTPUT_VARIABLE _pybind11_dir OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(_pybind11_dir)
        list(APPEND CMAKE_PREFIX_PATH "${_pybind11_dir}")
        find_package(pybind11 CONFIG QUIET)
    endif()
endif()

if(pybind11_FOUND)
    pybind11_add_module(_tt_core cpp/bindings/bindings.cpp)
    target_link_libraries(_tt_core PRIVATE tt_core)
    install(TARGETS _tt_core DESTINATION trumptailer)
endif()

if(TT_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.5.4
    )
    FetchContent_MakeAvailable(Catch2)

    add_executable(tt_tests
        cpp/tests/test_smoke.cpp
        cpp/tests/test_clock_pit.cpp
        cpp/tests/test_event_study.cpp
        cpp/tests/test_metrics.cpp
        cpp/tests/test_backtester.cpp
        cpp/tests/test_sizing.cpp
    )
    target_link_libraries(tt_tests PRIVATE tt_core Catch2::Catch2WithMain)

    enable_testing()
    include(CTest)
    include(Catch)
    catch_discover_tests(tt_tests)
endif()
