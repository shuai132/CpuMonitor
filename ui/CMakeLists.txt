cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 14)
add_compile_options(-Wall)

project(CpuMonitor)

# OpenGL
find_package(OpenGL REQUIRED)
link_libraries(${OPENGL_LIBRARIES})

# GLFW
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)
add_subdirectory(../thirdparty/glfw glfw EXCLUDE_FROM_ALL)
link_libraries(glfw)

# ImGui
set(IMGUI_DIR ${CMAKE_CURRENT_LIST_DIR}/../thirdparty/imgui)
set(GL_DIR ${IMGUI_DIR}/examples/libs/gl3w)
add_library(imgui
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl2.cpp
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp)
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${GL_DIR} ${IMGUI_DIR}/backends)
link_libraries(imgui)
target_compile_definitions(imgui PUBLIC
        -DIMGUI_USER_CONFIG=\"${CMAKE_CURRENT_LIST_DIR}/imconfig.h\"
        -DIMGUI_DISABLE_INCLUDE_IMCONFIG_H
)

# implot
include_directories(../thirdparty/implot)
set(SRCS_IMPLOT
        ../thirdparty/implot/implot.cpp
        ../thirdparty/implot/implot_demo.cpp
        ../thirdparty/implot/implot_items.cpp
)

if (MINGW)
    link_libraries(ws2_32)
endif ()

if (CMAKE_BUILD_TYPE MATCHES "Release")
    if (MINGW)
        set(CMAKE_EXE_LINKER_FLAGS "-static -s")
    endif ()
    set(BUILD_AS_APP WIN32 MACOSX_BUNDLE)
endif ()

include_directories(
        .
        base
)

file(GLOB_RECURSE SRCS *.cpp)
add_executable(${PROJECT_NAME} ${BUILD_AS_APP} ${SRCS} ${SRCS_IMPLOT})
target_link_libraries(${PROJECT_NAME} pthread cpu_monitor_common)
target_compile_definitions(${PROJECT_NAME} PRIVATE MSG_SERIALIZE_SUPPORT_TO_JSON)
