cmake_minimum_required(VERSION 3.15)

project(cpu_monitor)

option(BUILD_UI "" ON)

set(CMAKE_CXX_STANDARD 14)
add_compile_options(-Wall)

include_directories(thirdparty)

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
