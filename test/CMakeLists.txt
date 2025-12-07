cmake_minimum_required(VERSION 3.16)
project(parking_tests)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Coverage flags (GCC)
option(ENABLE_COVERAGE "Enable coverage flags" OFF)
if(ENABLE_COVERAGE)
  add_compile_options(--coverage -O0 -g)
  add_link_options(--coverage)
endif()

# Collect test sources
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "*.cpp")

# Build one executable per test source
include(CTest)
enable_testing()

foreach(src ${TEST_SOURCES})
  get_filename_component(name ${src} NAME_WE)
  add_executable(${name} ${src})
  target_include_directories(${name} PRIVATE ../components/parking_system/include ../main)
  add_test(NAME ${name} COMMAND ${name})
endforeach()
