# CMakeLists.txt for Dear ImGui
# This file should be placed ONE LEVEL ABOVE the imgui directory
# Usage: add_subdirectory(path/to/this/directory) and link against "imgui"
#
# Directory structure expected:
#   your_project/
#   ├── CMakeLists.txt          <- This file
#   └── imgui/                  <- Clone of https://github.com/ocornut/imgui
#       ├── imgui.cpp
#       ├── imgui.h
#       ├── ...
#       └── backends/
#           └── ...

# Vibed to you by: Faisal Zaghloul

cmake_minimum_required(VERSION 3.16)
project(imgui LANGUAGES CXX)

# Options for backend selection
option(IMGUI_BUILD_DEMO "Include ImGui demo window" ON)

# Platform backends
option(IMGUI_BACKEND_GLFW "Build GLFW platform backend" ON)
option(IMGUI_BACKEND_SDL2 "Build SDL2 platform backend" OFF)
option(IMGUI_BACKEND_SDL3 "Build SDL3 platform backend" OFF)
option(IMGUI_BACKEND_WIN32 "Build Win32 platform backend" OFF)
option(IMGUI_BACKEND_OSX "Build OSX platform backend" OFF)
option(IMGUI_BACKEND_ANDROID "Build Android platform backend" OFF)
option(IMGUI_BACKEND_GLUT "Build GLUT platform backend" OFF)

# Renderer backends
option(IMGUI_BACKEND_OPENGL2 "Build OpenGL2 renderer backend" OFF)
option(IMGUI_BACKEND_OPENGL3 "Build OpenGL3/4 renderer backend" ON)
option(IMGUI_BACKEND_VULKAN "Build Vulkan renderer backend" OFF)
option(IMGUI_BACKEND_DX9 "Build DirectX9 renderer backend" OFF)
option(IMGUI_BACKEND_DX10 "Build DirectX10 renderer backend" OFF)
option(IMGUI_BACKEND_DX11 "Build DirectX11 renderer backend" OFF)
option(IMGUI_BACKEND_DX12 "Build DirectX12 renderer backend" OFF)
option(IMGUI_BACKEND_METAL "Build Metal renderer backend" OFF)
option(IMGUI_BACKEND_WGPU "Build WebGPU renderer backend" OFF)
option(IMGUI_BACKEND_SDLRENDERER2 "Build SDL_Renderer2 backend" OFF)
option(IMGUI_BACKEND_SDLRENDERER3 "Build SDL_Renderer3 backend" OFF)
option(IMGUI_BACKEND_SDLGPU3 "Build SDL_GPU3 backend" OFF)

# Misc
option(IMGUI_STDLIB "Include std::string support (misc/cpp/imgui_stdlib)" OFF)
option(IMGUI_FREETYPE "Include FreeType font rasterizer" ON)

# Source directory paths (required when respective backends are enabled)
set(GLFW_SOURCE_DIR "" CACHE PATH "Path to GLFW source directory")
set(FREETYPE_SOURCE_DIR "" CACHE PATH "Path to FreeType source directory")

# Set the path to imgui source directory
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui")

# ============================================================================
# Validation
# ============================================================================

# Verify imgui directory exists
if(NOT EXISTS "${IMGUI_DIR}/imgui.h")
    message(FATAL_ERROR
            "ImGui source not found at ${IMGUI_DIR}\n"
            "Please clone imgui into the 'imgui' subdirectory:\n"
            "  git clone https://github.com/ocornut/imgui.git ${IMGUI_DIR}"
    )
endif()

# Verify GLFW_SOURCE_DIR if GLFW backend is enabled and target doesn't exist
if(IMGUI_BACKEND_GLFW AND NOT TARGET glfw)
    if(NOT GLFW_SOURCE_DIR)
        message(FATAL_ERROR
                "IMGUI_BACKEND_GLFW is enabled but GLFW_SOURCE_DIR is not set.\n"
                "Either provide a glfw target before adding imgui, or set:\n"
                "  -DGLFW_SOURCE_DIR=/path/to/glfw"
        )
    endif()
    if(NOT EXISTS "${GLFW_SOURCE_DIR}/include/GLFW/glfw3.h")
        message(FATAL_ERROR
                "GLFW source not found at ${GLFW_SOURCE_DIR}\n"
                "Expected to find: ${GLFW_SOURCE_DIR}/include/GLFW/glfw3.h\n"
                "Please provide a valid GLFW source directory."
        )
    endif()
endif()

# Verify FREETYPE_SOURCE_DIR if FreeType is enabled and target doesn't exist
if(IMGUI_FREETYPE AND NOT TARGET freetype)
    if(NOT FREETYPE_SOURCE_DIR)
        message(FATAL_ERROR
                "IMGUI_FREETYPE is enabled but FREETYPE_SOURCE_DIR is not set.\n"
                "Either provide a freetype target before adding imgui, or set:\n"
                "  -DFREETYPE_SOURCE_DIR=/path/to/freetype"
        )
    endif()
    if(NOT EXISTS "${FREETYPE_SOURCE_DIR}/include/freetype/freetype.h" AND
            NOT EXISTS "${FREETYPE_SOURCE_DIR}/include/ft2build.h")
        message(FATAL_ERROR
                "FreeType source not found at ${FREETYPE_SOURCE_DIR}\n"
                "Expected to find freetype headers in: ${FREETYPE_SOURCE_DIR}/include/\n"
                "Please provide a valid FreeType source directory."
        )
    endif()
endif()

# ============================================================================
# Create the imgui library
# ============================================================================

add_library(imgui STATIC)

# Add alias for use with FetchContent or add_subdirectory
add_library(imgui::imgui ALIAS imgui)

# Core source files (required)
target_sources(imgui PRIVATE
        "${IMGUI_DIR}/imgui.cpp"
        "${IMGUI_DIR}/imgui_draw.cpp"
        "${IMGUI_DIR}/imgui_tables.cpp"
        "${IMGUI_DIR}/imgui_widgets.cpp"
)

# Demo source (optional but commonly used)
if(IMGUI_BUILD_DEMO)
    target_sources(imgui PRIVATE "${IMGUI_DIR}/imgui_demo.cpp")
endif()

# Core header files (for IDE visibility)
target_sources(imgui PRIVATE
        "${IMGUI_DIR}/imgui.h"
        "${IMGUI_DIR}/imconfig.h"
        "${IMGUI_DIR}/imgui_internal.h"
        "${IMGUI_DIR}/imstb_rectpack.h"
        "${IMGUI_DIR}/imstb_textedit.h"
        "${IMGUI_DIR}/imstb_truetype.h"
)

# Set include directories (using generator expressions for proper build/install handling)
target_include_directories(imgui PUBLIC
        $<BUILD_INTERFACE:${IMGUI_DIR}>
        $<BUILD_INTERFACE:${IMGUI_DIR}/..>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/imgui>
)

# Require C++20
target_compile_features(imgui PUBLIC cxx_std_20)

# ============================================================================
# Platform Backends
# ============================================================================

if(IMGUI_BACKEND_GLFW)
    target_sources(imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp")

    # Check if glfw target already exists
    if(TARGET glfw)
        message(STATUS "ImGui: Using existing glfw target")
        target_link_libraries(imgui PUBLIC glfw)
    else()
        # Add GLFW as subdirectory
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
        add_subdirectory("${GLFW_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/glfw" EXCLUDE_FROM_ALL)
        target_link_libraries(imgui PUBLIC glfw)
    endif()
endif()

if(IMGUI_BACKEND_SDL2)
    target_sources(imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp")

    # Check if SDL2 target already exists
    if(TARGET SDL2::SDL2)
        message(STATUS "ImGui: Using existing SDL2::SDL2 target")
        target_link_libraries(imgui PUBLIC SDL2::SDL2)
    elseif(TARGET SDL2)
        message(STATUS "ImGui: Using existing SDL2 target")
        target_link_libraries(imgui PUBLIC SDL2)
    else()
        find_package(SDL2 REQUIRED)
        target_link_libraries(imgui PUBLIC SDL2::SDL2)
    endif()
endif()

if(IMGUI_BACKEND_SDL3)
    target_sources(imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_sdl3.cpp")

    # Check if SDL3 target already exists
    if(TARGET SDL3::SDL3)
        message(STATUS "ImGui: Using existing SDL3::SDL3 target")
        target_link_libraries(imgui PUBLIC SDL3::SDL3)
    elseif(TARGET SDL3)
        message(STATUS "ImGui: Using existing SDL3 target")
        target_link_libraries(imgui PUBLIC SDL3)
    else()
        find_package(SDL3 REQUIRED)
        target_link_libraries(imgui PUBLIC SDL3::SDL3)
    endif()
endif()

if(IMGUI_BACKEND_WIN32)
    target_sources(imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_win32.cpp")
endif()

if(IMGUI_BACKEND_OSX)
    target_sources(imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_osx.mm")

    # Check for existing framework targets or find them
    if(NOT TARGET Cocoa)
        find_library(COCOA_LIBRARY Cocoa REQUIRED)
        target_link_libraries(imgui PUBLIC ${COCOA_LIBRARY})
    else()
        message(STATUS "ImGui: Using existing Cocoa target")
        target_link_libraries(imgui PUBLIC Cocoa)
    endif()

    if(NOT TARGET GameController)
        find_library(GAMECONTROLLER_LIBRARY GameController REQUIRED)
        target_link_libraries(imgui PUBLIC ${GAMECONTROLLER_LIBRARY})
    else()
        message(STATUS "ImGui: Using existing GameController target")
        target_link_libraries(imgui PUBLIC GameController)
    endif()
endif()

if(IMGUI_BACKEND_ANDROID)
    target_sources(imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_android.cpp")
endif()

if(IMGUI_BACKEND_GLUT)
    target_sources(imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_glut.cpp")

    # Check if GLUT target already exists
    if(TARGET GLUT::GLUT)
        message(STATUS "ImGui: Using existing GLUT::GLUT target")
        target_link_libraries(imgui PUBLIC GLUT::GLUT)
    else()
        find_package(GLUT REQUIRED)
        target_link_libraries(imgui PUBLIC GLUT::GLUT)
    endif()
endif()

# ============================================================================
# Renderer Backends
# ============================================================================

if(IMGUI_BACKEND_OPENGL2)
    target_sources(imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_opengl2.cpp")

    # Check if OpenGL target already exists
    if(TARGET OpenGL::GL)
        message(STATUS "ImGui: Using existing OpenGL::GL target")
        target_link_libraries(imgui PUBLIC OpenGL::GL)
    else()
        find_package(OpenGL REQUIRED)
        target_link_libraries(imgui PUBLIC OpenGL::GL)
    endif()
endif()

if(IMGUI_BACKEND_OPENGL3)
    target_sources(imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp")

    # Check if OpenGL target already exists
    if(TARGET OpenGL::GL)
        message(STATUS "ImGui: Using existing OpenGL::GL target")
        target_link_libraries(imgui PUBLIC OpenGL::GL)
    else()
        find_package(OpenGL REQUIRED)
        target_link_libraries(imgui PUBLIC OpenGL::GL)
    endif()
    # Define loader - user can override by setting IMGUI_IMPL_OPENGL_LOADER before including
endif()

if(IMGUI_BACKEND_VULKAN)
    target_sources(imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp")

    # Check if Vulkan target already exists
    if(TARGET Vulkan::Vulkan)
        message(STATUS "ImGui: Using existing Vulkan::Vulkan target")
        target_link_libraries(imgui PUBLIC Vulkan::Vulkan)
    else()
        find_package(Vulkan REQUIRED)
        target_link_libraries(imgui PUBLIC Vulkan::Vulkan)
    endif()
endif()

if(IMGUI_BACKEND_DX9)
    target_sources(imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_dx9.cpp")
    target_link_libraries(imgui PUBLIC d3d9)
endif()

if(IMGUI_BACKEND_DX10)
    target_sources(imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_dx10.cpp")
    target_link_libraries(imgui PUBLIC d3d10 dxgi)
endif()

if(IMGUI_BACKEND_DX11)
    target_sources(imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_dx11.cpp")
    target_link_libraries(imgui PUBLIC d3d11 dxgi)
endif()

if(IMGUI_BACKEND_DX12)
    target_sources(imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_dx12.cpp")
    target_link_libraries(imgui PUBLIC d3d12 dxgi)
endif()

if(IMGUI_BACKEND_METAL)
    target_sources(imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_metal.mm")

    # Check for existing framework targets or find them
    if(NOT TARGET Metal)
        find_library(METAL_LIBRARY Metal REQUIRED)
        target_link_libraries(imgui PUBLIC ${METAL_LIBRARY})
    else()
        message(STATUS "ImGui: Using existing Metal target")
        target_link_libraries(imgui PUBLIC Metal)
    endif()

    if(NOT TARGET MetalKit)
        find_library(METALKIT_LIBRARY MetalKit REQUIRED)
        target_link_libraries(imgui PUBLIC ${METALKIT_LIBRARY})
    else()
        message(STATUS "ImGui: Using existing MetalKit target")
        target_link_libraries(imgui PUBLIC MetalKit)
    endif()
endif()

if(IMGUI_BACKEND_WGPU)
    target_sources(imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_wgpu.cpp")
    # WebGPU library must be provided by user
endif()

if(IMGUI_BACKEND_SDLRENDERER2)
    target_sources(imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_sdlrenderer2.cpp")

    # Check if SDL2 target already exists
    if(TARGET SDL2::SDL2)
        message(STATUS "ImGui: Using existing SDL2::SDL2 target")
        target_link_libraries(imgui PUBLIC SDL2::SDL2)
    elseif(TARGET SDL2)
        message(STATUS "ImGui: Using existing SDL2 target")
        target_link_libraries(imgui PUBLIC SDL2)
    else()
        find_package(SDL2 REQUIRED)
        target_link_libraries(imgui PUBLIC SDL2::SDL2)
    endif()
endif()

if(IMGUI_BACKEND_SDLRENDERER3)
    target_sources(imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_sdlrenderer3.cpp")

    # Check if SDL3 target already exists
    if(TARGET SDL3::SDL3)
        message(STATUS "ImGui: Using existing SDL3::SDL3 target")
        target_link_libraries(imgui PUBLIC SDL3::SDL3)
    elseif(TARGET SDL3)
        message(STATUS "ImGui: Using existing SDL3 target")
        target_link_libraries(imgui PUBLIC SDL3)
    else()
        find_package(SDL3 REQUIRED)
        target_link_libraries(imgui PUBLIC SDL3::SDL3)
    endif()
endif()

if(IMGUI_BACKEND_SDLGPU3)
    target_sources(imgui PRIVATE "${IMGUI_DIR}/backends/imgui_impl_sdlgpu3.cpp")

    # Check if SDL3 target already exists
    if(TARGET SDL3::SDL3)
        message(STATUS "ImGui: Using existing SDL3::SDL3 target")
        target_link_libraries(imgui PUBLIC SDL3::SDL3)
    elseif(TARGET SDL3)
        message(STATUS "ImGui: Using existing SDL3 target")
        target_link_libraries(imgui PUBLIC SDL3)
    else()
        find_package(SDL3 REQUIRED)
        target_link_libraries(imgui PUBLIC SDL3::SDL3)
    endif()
endif()

# ============================================================================
# Misc Extensions
# ============================================================================

if(IMGUI_STDLIB)
    target_sources(imgui PRIVATE
            "${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp"
            "${IMGUI_DIR}/misc/cpp/imgui_stdlib.h"
    )
    target_include_directories(imgui PUBLIC
            $<BUILD_INTERFACE:${IMGUI_DIR}/misc/cpp>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/imgui>
    )
endif()

if(IMGUI_FREETYPE)
    target_sources(imgui PRIVATE "${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp")
    target_compile_definitions(imgui PUBLIC IMGUI_ENABLE_FREETYPE)

    # Check if freetype target already exists
    if(TARGET freetype)
        message(STATUS "ImGui: Using existing freetype target")
        target_link_libraries(imgui PUBLIC freetype)
    else()
        # Add FreeType as subdirectory
        set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
        set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
        set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
        set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
        set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
        add_subdirectory("${FREETYPE_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/freetype" EXCLUDE_FROM_ALL)
        target_link_libraries(imgui PUBLIC freetype)
    endif()
endif()

# ============================================================================
# Installation (optional, disabled by default)
# ============================================================================

option(IMGUI_INSTALL "Generate installation target" OFF)

if(IMGUI_INSTALL)
    include(GNUInstallDirs)

    install(TARGETS imgui
            EXPORT imguiTargets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(FILES
            "${IMGUI_DIR}/imgui.h"
            "${IMGUI_DIR}/imconfig.h"
            "${IMGUI_DIR}/imgui_internal.h"
            "${IMGUI_DIR}/imstb_rectpack.h"
            "${IMGUI_DIR}/imstb_textedit.h"
            "${IMGUI_DIR}/imstb_truetype.h"
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/imgui
    )

    install(EXPORT imguiTargets
            FILE imguiTargets.cmake
            NAMESPACE imgui::
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/imgui
    )
endif()

# Print configuration summary
message(STATUS "ImGui Configuration:")
message(STATUS "  C++ Standard: C++20")
message(STATUS "  Demo: ${IMGUI_BUILD_DEMO}")
message(STATUS "  Platform backends: GLFW=${IMGUI_BACKEND_GLFW}, SDL2=${IMGUI_BACKEND_SDL2}, SDL3=${IMGUI_BACKEND_SDL3}, Win32=${IMGUI_BACKEND_WIN32}")
message(STATUS "  Renderer backends: OpenGL2=${IMGUI_BACKEND_OPENGL2}, OpenGL3=${IMGUI_BACKEND_OPENGL3}, Vulkan=${IMGUI_BACKEND_VULKAN}")
message(STATUS "  Extensions: stdlib=${IMGUI_STDLIB}, freetype=${IMGUI_FREETYPE}")
if(IMGUI_BACKEND_GLFW)
    message(STATUS "  GLFW source: ${GLFW_SOURCE_DIR}")
endif()
if(IMGUI_FREETYPE)
    message(STATUS "  FreeType source: ${FREETYPE_SOURCE_DIR}")
endif()
