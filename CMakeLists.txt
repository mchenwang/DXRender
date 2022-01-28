cmake_minimum_required (VERSION 3.18)
project (learndx12 LANGUAGES CXX)

# Enable multithreaded builds
if( MSVC )
    add_compile_options(/MP)
endif()

set(SOURCES
    src/stdafx.cpp
    src/DXWindow.cpp
    src/SwapChain.cpp
    src/CommandQueue.cpp
    src/DescriptorHeap.cpp
    src/Application.cpp
    src/Model.cpp
    include/common/ModelLoader.cpp
    src/main.cpp
)

if (MSVC_VERSION GREATER_EQUAL "1914")
    add_compile_options("/Zc:__cplusplus") 
endif()
 
if (MSVC_VERSION GREATER_EQUAL "1900")
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("/std:c++17" _compiler_supports_cxx17)
    if (_compiler_supports_cxx17)
        add_compile_options("/std:c++17")
    endif()
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ../target/)

configure_file(
    "${PROJECT_SOURCE_DIR}/config/path.h.in"
    "${PROJECT_BINARY_DIR}/config/path.h"
)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE 
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/tool
        ${PROJECT_BINARY_DIR}/config
)
# DX12 libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d12.lib dxgi.lib dxguid.lib
    D3DCompiler.lib
)