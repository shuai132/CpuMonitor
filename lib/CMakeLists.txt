project(cpu_monitor_lib)

option(cpu_monitor_lib_BUILD_TEST "" OFF)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(cpu_monitor_lib_BUILD_TEST ON)
endif ()

file(GLOB SRCS *.cpp)

if (WIN32)
    message(FATAL_ERROR "WIN32 not support for now")
elseif (APPLE)
    file(GLOB PLATFORM_SRCS apple/*.cpp)
elseif (UNIX)
    file(GLOB PLATFORM_SRCS linux/*.cpp)
endif ()

add_library(${PROJECT_NAME} STATIC ${SRCS} ${PLATFORM_SRCS})
target_include_directories(${PROJECT_NAME} PUBLIC .)

if (cpu_monitor_lib_BUILD_TEST)
    file(GLOB_RECURSE CPP_SRC_LIST ${CMAKE_CURRENT_LIST_DIR}/tests/*.cpp)
    foreach (file ${CPP_SRC_LIST})
        string(REGEX REPLACE ".*tests|.cpp|/|\\\\" "" target_name ${file})
        set(target_name ${PROJECT_NAME}_${target_name})
        add_executable(${target_name} ${file})
        target_link_libraries(${target_name} PRIVATE ${PROJECT_NAME})
    endforeach ()
endif ()
