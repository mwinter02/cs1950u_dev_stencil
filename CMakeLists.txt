cmake_minimum_required(VERSION 3.16)
project(Test_project) # Enter your project name here

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# Find dependencies
find_package(OpenGL REQUIRED)

# Enable all dependencies
set(ENABLE_ALL_DEPENDENCIES ON)

# Add all external dependencies
add_subdirectory(External)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.h)
add_executable(${PROJECT_NAME} main.cpp ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        glew_static
        OpenGL::GL
        glm::glm
        freetype
        assimp
        stb
        imgui
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
        <glm/glm.hpp>
        <glm/ext.hpp>
)

# Defines PROJECT_ROOT macro for use in source files
target_compile_definitions(${PROJECT_NAME} PRIVATE PROJECT_ROOT="${PROJECT_SOURCE_DIR}")
