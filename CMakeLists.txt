cmake_minimum_required(VERSION 3.15)

project(cpu_monitor)

option(BUILD_UI "" ON)
option(MSG_SERIALIZE_USE_JSON "" OFF)

set(CMAKE_CXX_STANDARD 14)
add_compile_options(-Wall)

include_directories(thirdparty)

if (MSG_SERIALIZE_USE_JSON)
    add_definitions(-DMSG_SERIALIZE_USE_JSON)
endif ()

# common
add_subdirectory(common)

if (BUILD_UI)
    add_subdirectory(ui)
endif ()

if (WIN32)
    message(STATUS "daemon not support WIN32 for now!")
else ()
    add_subdirectory(daemon)
endif ()
