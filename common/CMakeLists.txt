cmake_minimum_required(VERSION 3.16)

project(cpu_monitor_common)

file(GLOB SRCS *.cpp)
add_library(${PROJECT_NAME} STATIC ${SRCS})
target_include_directories(${PROJECT_NAME} PUBLIC .)

add_subdirectory(test EXCLUDE_FROM_ALL)

# asio
add_definitions(-DASIO_STANDALONE)

# asio_net
add_subdirectory(../thirdparty/asio_net asio_net EXCLUDE_FROM_ALL)
target_include_directories(asio_net INTERFACE ../thirdparty/asio/asio/include)
target_link_libraries(${PROJECT_NAME} asio_net)
