cmake_minimum_required(VERSION 3.16)

project(cpu_monitor_common)

# flatbuffers
include(../cmake/flatbuffer_cfg.cmake)
add_subdirectory(../thirdparty/flatbuffers flatbuffers EXCLUDE_FROM_ALL)

file(GLOB SRCS *.cpp)
add_library(${PROJECT_NAME} STATIC ${SRCS})
target_include_directories(${PROJECT_NAME} PUBLIC
        .
        schema_gen
        )

add_custom_target(flatc_schema
        COMMAND ${CMAKE_BINARY_DIR}/common/flatbuffers/flatc --cpp --gen-object-api ../schema/*.fbs
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/schema_gen
        )
add_dependencies(flatc_schema flatc)

add_subdirectory(test EXCLUDE_FROM_ALL)

# asio
add_definitions(-DASIO_STANDALONE)

# asio_net
add_subdirectory(../thirdparty/asio_net asio_net EXCLUDE_FROM_ALL)
target_include_directories(asio_net INTERFACE ../thirdparty/asio/include)
target_link_libraries(${PROJECT_NAME} asio_net flatbuffers)
