cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 14)

project(cpu_monitor)

link_libraries(pthread)

# cpu_monitor_lib
add_subdirectory(src)
link_libraries(cpu_monitor_lib)

add_executable(${PROJECT_NAME} app/main.cpp)
add_executable(${PROJECT_NAME}_cli app/main_cli.cpp)
add_executable(${PROJECT_NAME}_test_thread test/test_thread.cpp)
