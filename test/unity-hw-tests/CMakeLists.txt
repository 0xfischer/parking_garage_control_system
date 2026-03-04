# Unity Hardware Tests - Standalone Test Application
cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Coverage build option (for QEMU coverage builds)
option(ENABLE_QEMU_COVERAGE "Enable GCOV coverage for QEMU (experimental)" OFF)

# Include parking_system component from main project
set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(unity_hw_tests)

# If coverage enabled, add preprocessor define
if(ENABLE_QEMU_COVERAGE)
    message(STATUS "QEMU Coverage enabled (experimental) - adding ENABLE_GCOV_COVERAGE define")
    add_compile_definitions(ENABLE_GCOV_COVERAGE)
endif()
