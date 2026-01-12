# CMakeLists.txt for project dependencies
# This file should be placed in the dependencies/ directory
#
# Usage in top-level CMakeLists.txt:
#   set(DEP_GLFW ON)
#   set(DEP_IMGUI ON)
#   # ... etc
#   add_subdirectory(dependencies)
#   target_link_libraries(your_target PRIVATE glfw imgui ...)

# Vibed to you by: Faisal Zaghloul

cmake_minimum_required(VERSION 3.16)

# ============================================================================
# Dependency toggles (set these BEFORE add_subdirectory(dependencies))
# ============================================================================

option(ENABLE_ALL_DEPENDENCIES "Build all dependencies" OFF)
option(DEP_GLFW "Build GLFW" OFF)
option(DEP_GLM "Build GLM" OFF)
option(DEP_GLEW "Build GLEW (static)" OFF)
option(DEP_FREETYPE "Build FreeType" OFF)
option(DEP_IMGUI "Build Dear ImGui" OFF)
option(DEP_STB "Include STB headers" OFF)
option(DEP_ASSIMP "Build Assimp" OFF)
option(DEP_RAUDIO "Build raudio" OFF)

# ImGui backend options (only relevant if DEP_IMGUI is ON)
option(DEP_IMGUI_BACKEND_GLFW "ImGui GLFW backend" ON)
option(DEP_IMGUI_BACKEND_OPENGL3 "ImGui OpenGL3 backend" ON)
option(DEP_IMGUI_FREETYPE "ImGui FreeType support" ON)

# ============================================================================
# Dependency paths (can be overridden before including this file)
# ============================================================================

if (ENABLE_ALL_DEPENDENCIES)
    set(DEP_GLFW ON)
    set(DEP_GLM ON)
    set(DEP_GLEW ON)
    set(DEP_FREETYPE ON)
    set(DEP_STB ON)
    set(DEP_ASSIMP ON)
    set(DEP_RAUDIO ON)

    set(DEP_IMGUI ON)
    # ImGui backends
    set(DEP_IMGUI_BACKEND_GLFW ON)
    set(DEP_IMGUI_BACKEND_OPENGL3 ON)
    set(DEP_IMGUI_FREETYPE ON)
endif()

if(NOT GLFW_SOURCE_DIR)
    set(GLFW_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/glfw")
endif()

if(NOT GLM_SOURCE_DIR)
    set(GLM_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/glm")
endif()

if(NOT GLEW_SOURCE_DIR)
    set(GLEW_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/glew")
endif()

if(NOT FREETYPE_SOURCE_DIR)
    set(FREETYPE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/freetype")
endif()

if(NOT IMGUI_SOURCE_DIR)
    set(IMGUI_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
endif()

if(NOT STB_SOURCE_DIR)
    set(STB_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/stb")
endif()

if(NOT ASSIMP_SOURCE_DIR)
    set(ASSIMP_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assimp")
endif()

if(NOT RAUDIO_SOURCE_DIR)
    set(RAUDIO_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/raudio")
endif()
# ============================================================================
# GLFW
# ============================================================================

if(DEP_GLFW)
    if(NOT TARGET glfw)
        if(NOT EXISTS "${GLFW_SOURCE_DIR}/CMakeLists.txt")
            message(FATAL_ERROR "GLFW source not found at ${GLFW_SOURCE_DIR}")
        endif()

        message(STATUS "Dependencies: Adding GLFW")
        set(GLFW_BUILD_EXAMPLES OFF)
        set(GLFW_BUILD_TESTS OFF)
        set(GLFW_BUILD_DOCS OFF)
        set(GLFW_INSTALL OFF)
        add_subdirectory("${GLFW_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/glfw" EXCLUDE_FROM_ALL)
    else()
        message(STATUS "Dependencies: Using existing glfw target")
    endif()
endif()

# ============================================================================
# GLM
# ============================================================================

if(DEP_GLM)
    if(NOT TARGET glm)
        if(NOT EXISTS "${GLM_SOURCE_DIR}/CMakeLists.txt")
            message(FATAL_ERROR "GLM source not found at ${GLM_SOURCE_DIR}")
        endif()

        message(STATUS "Dependencies: Adding GLM")
        add_subdirectory("${GLM_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/glm" EXCLUDE_FROM_ALL)
    else()
        message(STATUS "Dependencies: Using existing glm target")
    endif()
endif()

# ============================================================================
# GLEW (static library)
# ============================================================================

if(DEP_GLEW)
    if(NOT TARGET glew_static)
        if(NOT EXISTS "${GLEW_SOURCE_DIR}/src/glew.c")
            message(FATAL_ERROR "GLEW source not found at ${GLEW_SOURCE_DIR}")
        endif()

        message(STATUS "Dependencies: Adding GLEW (static)")
        add_library(glew_static STATIC "${GLEW_SOURCE_DIR}/src/glew.c")
        target_include_directories(glew_static PUBLIC
            $<BUILD_INTERFACE:${GLEW_SOURCE_DIR}/include>
        )
        target_compile_definitions(glew_static PUBLIC GLEW_STATIC)

        # Link OpenGL
        find_package(OpenGL REQUIRED)
        target_link_libraries(glew_static PUBLIC OpenGL::GL)

        # Add alias for consistency
        add_library(GLEW::GLEW ALIAS glew_static)
    else()
        message(STATUS "Dependencies: Using existing glew_static target")
    endif()
endif()

# ============================================================================
# FreeType
# ============================================================================

if(DEP_FREETYPE)
    if(NOT TARGET freetype)
        if(NOT EXISTS "${FREETYPE_SOURCE_DIR}/CMakeLists.txt")
            message(FATAL_ERROR "FreeType source not found at ${FREETYPE_SOURCE_DIR}")
        endif()

        message(STATUS "Dependencies: Adding FreeType")
        set(FT_DISABLE_ZLIB ON)
        set(FT_DISABLE_BZIP2 ON)
        set(FT_DISABLE_PNG ON)
        set(FT_DISABLE_HARFBUZZ ON)
        set(FT_DISABLE_BROTLI ON)
        add_subdirectory("${FREETYPE_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/freetype" EXCLUDE_FROM_ALL)
    else()
        message(STATUS "Dependencies: Using existing freetype target")
    endif()
endif()

# ============================================================================
# STB (header-only)
# ============================================================================

if(DEP_STB)
    if(NOT TARGET stb)
        if(NOT EXISTS "${STB_SOURCE_DIR}")
            message(FATAL_ERROR "STB source not found at ${STB_SOURCE_DIR}")
        endif()

        message(STATUS "Dependencies: Adding STB (header-only)")
        add_library(stb INTERFACE)
        target_include_directories(stb INTERFACE
            $<BUILD_INTERFACE:${STB_SOURCE_DIR}>
        )
    else()
        message(STATUS "Dependencies: Using existing stb target")
    endif()
endif()

# ============================================================================
# Assimp
# ============================================================================

if(DEP_ASSIMP)
    if(NOT TARGET assimp)
        if(NOT EXISTS "${ASSIMP_SOURCE_DIR}/CMakeLists.txt")
            message(FATAL_ERROR "Assimp source not found at ${ASSIMP_SOURCE_DIR}")
        endif()

        message(STATUS "Dependencies: Adding Assimp")

        # Assimp build options - disable unnecessary features for faster builds
        set(BUILD_SHARED_LIBS OFF)
        set(ASSIMP_BUILD_TESTS OFF)
        set(ASSIMP_BUILD_SAMPLES OFF)
        set(ASSIMP_BUILD_DOCS OFF)
        set(ASSIMP_INSTALL OFF)
        set(ASSIMP_INJECT_DEBUG_POSTFIX OFF)

        set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
        set(ASSIMP_NO_EXPORT OFF)

        add_subdirectory("${ASSIMP_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/assimp" EXCLUDE_FROM_ALL)
    else()
        message(STATUS "Dependencies: Using existing assimp target")
    endif()
endif()

# ============================================================================
# raudio
# ============================================================================

if(DEP_RAUDIO)
    if(NOT TARGET raudio)
        # raudio has its CMakeLists.txt in projects/CMake/
        if(EXISTS "${RAUDIO_SOURCE_DIR}/projects/CMake/CMakeLists.txt")
            message(STATUS "Dependencies: Adding raudio (using projects/CMake)")
            add_subdirectory("${RAUDIO_SOURCE_DIR}/projects/CMake" "${CMAKE_CURRENT_BINARY_DIR}/raudio" EXCLUDE_FROM_ALL)
        elseif(EXISTS "${RAUDIO_SOURCE_DIR}/CMakeLists.txt")
            message(STATUS "Dependencies: Adding raudio")
            add_subdirectory("${RAUDIO_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/raudio" EXCLUDE_FROM_ALL)
        else()
            message(FATAL_ERROR "raudio CMakeLists.txt not found at ${RAUDIO_SOURCE_DIR}")
        endif()

        # Ensure raudio's include directories are properly exported if not already
        if(TARGET raudio)
            get_target_property(RAUDIO_INCLUDE_DIRS raudio INTERFACE_INCLUDE_DIRECTORIES)
            if(NOT RAUDIO_INCLUDE_DIRS)
                target_include_directories(raudio PUBLIC
                    $<BUILD_INTERFACE:${RAUDIO_SOURCE_DIR}/src>
                )
            endif()
        endif()
    else()
        message(STATUS "Dependencies: Using existing raudio target")
    endif()
endif()

# ============================================================================
# Dear ImGui
# ============================================================================

if(DEP_IMGUI)
    if(NOT TARGET imgui)
        if(NOT EXISTS "${IMGUI_SOURCE_DIR}/CMakeLists.txt")
            message(FATAL_ERROR "ImGui CMakeLists.txt not found at ${IMGUI_SOURCE_DIR}")
        endif()

        message(STATUS "Dependencies: Adding Dear ImGui")

        # Configure ImGui options before adding subdirectory
        set(IMGUI_BACKEND_GLFW ${DEP_IMGUI_BACKEND_GLFW})
        set(IMGUI_BACKEND_OPENGL3 ${DEP_IMGUI_BACKEND_OPENGL3})
        set(IMGUI_FREETYPE ${DEP_IMGUI_FREETYPE})

        # Ensure dependencies are built first if needed
        if(DEP_IMGUI_BACKEND_GLFW AND NOT TARGET glfw)
            message(FATAL_ERROR "ImGui GLFW backend requires DEP_GLFW=ON or an existing glfw target")
        endif()

        if(DEP_IMGUI_FREETYPE AND NOT TARGET freetype)
            message(FATAL_ERROR "ImGui FreeType support requires DEP_FREETYPE=ON or an existing freetype target")
        endif()

        add_subdirectory("${IMGUI_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/imgui" EXCLUDE_FROM_ALL)
    else()
        message(STATUS "Dependencies: Using existing imgui target")
    endif()
endif()

# ============================================================================
# Summary
# ============================================================================

message(STATUS "Dependencies Configuration:")
message(STATUS "  GLFW:     ${DEP_GLFW}")
message(STATUS "  GLM:      ${DEP_GLM}")
message(STATUS "  GLEW:     ${DEP_GLEW}")
message(STATUS "  FreeType: ${DEP_FREETYPE}")
message(STATUS "  STB:      ${DEP_STB}")
message(STATUS "  Assimp:   ${DEP_ASSIMP}")
message(STATUS "  raudio:   ${DEP_RAUDIO}")
message(STATUS "  ImGui:    ${DEP_IMGUI}")
if(DEP_IMGUI)
    message(STATUS "    - GLFW backend:    ${DEP_IMGUI_BACKEND_GLFW}")
    message(STATUS "    - OpenGL3 backend: ${DEP_IMGUI_BACKEND_OPENGL3}")
    message(STATUS "    - FreeType:        ${DEP_IMGUI_FREETYPE}")
endif()
