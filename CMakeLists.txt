cmake_minimum_required(VERSION 3.12)
project(ccool
	LANGUAGES CXX
	VERSION 0.1.0
)

option(CCOOL_CLI_TOOL   "Build ccool-cli tool"             ON)
option(CCOOL_TESTS      "Build ccool tests"                OFF)
option(CCOOL_STATIC_LIB "Build libccool as static library" OFF)

macro(enable_warnings TARGET_NAME)
	target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wconversion)
endmacro()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Threads REQUIRED)
find_package(LibUSB "1.0.22" REQUIRED)
find_package(fmt REQUIRED)
if(fmt_FOUND)
	message(STATUS "Found fmt version ${fmt_VERSION}")
endif()
find_package(spdlog REQUIRED)
if(spdlog_FOUND)
	message(STATUS "Found spdlog version ${spdlog_VERSION}")
endif()
find_package(Systemd REQUIRED)

add_subdirectory(deps)
add_subdirectory(src)
if(CCOOL_TESTS)
	add_subdirectory(tests)
endif()
