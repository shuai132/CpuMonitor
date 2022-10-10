cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 14)
add_compile_options(-Wall)

project(cpu_monitor)

link_libraries(pthread)

# cpu_monitor_lib
add_subdirectory(../src cpu_monitor_lib)
link_libraries(cpu_monitor_lib)

# flatbuffers
include(../cmake/flatbuffer_cfg.cmake)
add_subdirectory(../thirdparty/flatbuffers flatbuffers EXCLUDE_FROM_ALL)
link_libraries(flatbuffers)

add_executable(${PROJECT_NAME} main.cpp)
add_executable(${PROJECT_NAME}_cli main_cli.cpp)
add_executable(${PROJECT_NAME}_test_thread ../test/test_thread.cpp)
