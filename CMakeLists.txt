cmake_minimum_required(VERSION 3.16)

project(cpu_monitor)

option(BUILD_GUI "" ON)

if (BUILD_GUI)
    add_subdirectory(gui)
else ()
    add_subdirectory(app)
endif ()
