cmake_minimum_required (VERSION 3.15)
project (roam)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(SDL2 REQUIRED)
add_subdirectory (src)

