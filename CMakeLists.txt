# Parking Garage Control System
cmake_minimum_required(VERSION 3.16)

# Set C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile_commands.json for tooling (clang-tidy)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(parking_garage_control_system)

# Optional lint/format helper targets
find_program(CLANG_FORMAT NAMES clang-format)
find_program(RUN_CLANG_TIDY NAMES run-clang-tidy)

if(CLANG_FORMAT)
	add_custom_target(format
		COMMAND ${CMAKE_COMMAND} -E echo "Running clang-format"
		COMMAND find ${CMAKE_SOURCE_DIR} -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) \
				-not -path "${CMAKE_SOURCE_DIR}/build/*" -not -path "${CMAKE_SOURCE_DIR}/bootloader/*" -not -path "${CMAKE_SOURCE_DIR}/esp-idf/*" \
				-print0 | xargs -0 ${CLANG_FORMAT} -i
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		COMMENT "Formatting sources with clang-format"
	)
endif()

if(RUN_CLANG_TIDY)
	add_custom_target(tidy
		COMMAND ${CMAKE_COMMAND} -E echo "Running run-clang-tidy"
		COMMAND ${RUN_CLANG_TIDY} -p ${CMAKE_BINARY_DIR} -header-filter "^(main|components|examples|test)/" -j $ENV{NPROC}
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		COMMENT "Linting sources with clang-tidy"
	)
endif()
