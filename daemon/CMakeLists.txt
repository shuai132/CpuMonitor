cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 14)
add_compile_options(-Wall)

project(cpu_monitor)

link_libraries(pthread)

# for android standalone e.g. termux
add_definitions(-DANDROID_STANDALONE)

# cpu_monitor_lib
add_subdirectory(../lib cpu_monitor_lib)
link_libraries(cpu_monitor_lib)

# asio
add_definitions(-DASIO_STANDALONE)
include_directories(../thirdparty/asio/asio/include)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} cpu_monitor_common)

add_executable(${PROJECT_NAME}_test_thread test/test_thread.cpp)
