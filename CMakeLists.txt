cmake_minimum_required(VERSION 3.30)
project(SomeParticles)

set(CMAKE_CXX_STANDARD 20)

# GLM
find_package(glm REQUIRED)
# GLFW3
find_package(glfw3 REQUIRED 3.4)

# ImGui (https://github.com/ocornut/imgui/tree/docking)
# imconfig.h - uncommented #define ImDrawIdx unsigned int
add_library(imgui STATIC
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/backends/imgui_impl_glfw.cpp
        imgui/backends/imgui_impl_opengl3.cpp
        imgui/misc/cpp/imgui_stdlib.cpp
)
target_include_directories(imgui PUBLIC imgui imgui/backends)

# Glad
add_library(glad STATIC
        glad/src/glad.c
)
target_include_directories(glad PUBLIC glad/include)

# SomeParticles
add_executable(SomeParticles
        SomeParticles/main.cpp
        SomeParticles/Shader.cpp
        SomeParticles/Shader.hpp
        SomeParticles/GLM.hpp
        SomeParticles/SSBO.cpp
        SomeParticles/SSBO.hpp
)
target_link_libraries(SomeParticles PUBLIC imgui glm::glm glfw glad GL)
