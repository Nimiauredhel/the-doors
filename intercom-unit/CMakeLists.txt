# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
add_library(common "")
target_include_directories(common
    PUBLIC
    ../common
)
FILE(GLOB CommonSources ../common/*)
target_sources(common
    PUBLIC
    ${CommonSources}
)
project(intercom-unit)
