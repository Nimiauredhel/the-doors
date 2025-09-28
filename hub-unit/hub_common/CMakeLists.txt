cmake_minimum_required(VERSION 3.22)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

# project definitions
project(Doors_Hub_Common VERSION 0.001)

FILE(GLOB LIB_SOURCES ./*.c)

# target definition
add_library(${DOORS_HUB_COMMON_LIB_NAME} SHARED ${LIB_SOURCES}
    ${DOORS_SYSTEM_COMMON_SOURCES} ${DOORS_SYSTEM_COMMON_HEADERS})
target_link_libraries(${DOORS_HUB_COMMON_LIB_NAME} PUBLIC
    pthread
    rt
)
target_include_directories(${DOORS_HUB_COMMON_LIB_NAME} PUBLIC
    .
    ${DOORS_SYSTEM_COMMON_SOURCE_DIRECTORY}
)
target_compile_features(${DOORS_HUB_COMMON_LIB_NAME} PRIVATE c_std_99)
