project(cpu_monitor_lib)

file(GLOB SRCS *.cpp)
add_library(${PROJECT_NAME} STATIC ${SRCS})
target_include_directories(${PROJECT_NAME} PUBLIC .)
