cmake_minimum_required(VERSION 3.22)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# project definitions
project(Doors_Hub_Common VERSION 0.001)

set(MODULE_NAME "hub_control")

FILE(GLOB MODULE_SOURCES ./*.c)

# target definition
add_executable(${MODULE_NAME} ${MODULE_SOURCES}
    ${DOORS_SYSTEM_COMMON_SOURCES} ${DOORS_SYSTEM_COMMON_HEADERS})
target_link_libraries(${MODULE_NAME} PUBLIC
    ${DOORS_HUB_COMMON_LIB_NAME}
    pthread
    rt
)
target_include_directories(${MODULE_NAME} PUBLIC
    .
    ${DOORS_SYSTEM_COMMON_SOURCE_DIRECTORY}
)
add_dependencies(${MODULE_NAME} ${DOORS_HUB_COMMON_LIB_NAME})
target_compile_features(${MODULE_NAME} PRIVATE c_std_99)
