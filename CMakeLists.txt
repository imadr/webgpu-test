cmake_minimum_required(VERSION 3.13)
project(webgpu-test)

set(CMAKE_CXX_STANDARD 20)

add_executable(app main.cpp)

set(DAWN_FETCH_DEPENDENCIES ON)
add_subdirectory("dawn" EXCLUDE_FROM_ALL)
target_link_libraries(app PRIVATE webgpu_cpp webgpu_dawn webgpu_glfw)