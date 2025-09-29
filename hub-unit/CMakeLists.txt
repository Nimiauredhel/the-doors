cmake_minimum_required(VERSION 3.22)

# set DOORS variables

SET(DOORS_SYSTEM_COMMON_SOURCE_DIRECTORY ${CMAKE_SOURCE_DIR}/../common)
SET(DOORS_HUB_COMMON_LIB_NAME hub_common)

FILE(GLOB DOORS_SYSTEM_COMMON_SOURCES ${DOORS_SYSTEM_COMMON_SOURCE_DIRECTORY}/*.c)
FILE(GLOB DOORS_SYSTEM_COMMON_HEADERS ${DOORS_SYSTEM_COMMON_SOURCE_DIRECTORY}/*.h)

# set CMAKE variables

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/out_runtime")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/out_library")
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "/usr/bin/doors_hub")
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "/usr/lib")

# project definition
project(Doors_Hub VERSION 0.001)

# TODO: use and maintain a config file following this example
#configure_file(common_config.h.in common_config.h)

# adding the DOORS modules as CMake subdirectories (each contains its own CMakeLists.txt)

## DOORS hub unit Hub Common library - loaded dynamically by modules
add_subdirectory("${CMAKE_SOURCE_DIR}/hub_common")

## DOORS hub unit Hub Control module - top level daemon
add_subdirectory("${CMAKE_SOURCE_DIR}/hub_control")

## DOORS hub unit Door Manager module - managed by Hub Control
add_subdirectory("${CMAKE_SOURCE_DIR}/door_manager")

## DOORS hub unit Intercom Server module - managed by Hub Control
add_subdirectory("${CMAKE_SOURCE_DIR}/intercom_server")

## DOORS hub unit Web Server module - managed by Hub Control
add_subdirectory("${CMAKE_SOURCE_DIR}/web_server")

## DOORS hub unit Database Service module - managed by Hub Control
add_subdirectory("${CMAKE_SOURCE_DIR}/db_service")

