cmake_minimum_required(VERSION 3.15)

project(cpu_monitor_common)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE .)

# asio
add_definitions(-DASIO_STANDALONE)
if (WIN32)
    # 0x0601 means Win7. So our application targets Win7 and above.
    add_definitions(-D_WIN32_WINNT=0x0601)
endif ()

# asio_net
add_subdirectory(../thirdparty/asio_net asio_net EXCLUDE_FROM_ALL)
target_include_directories(asio_net INTERFACE ../thirdparty/asio/asio/include)
target_link_libraries(${PROJECT_NAME} INTERFACE asio_net)
