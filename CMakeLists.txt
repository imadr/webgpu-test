cmake_minimum_required(VERSION 3.0)
project(
    webgpu-test
    VERSION 0.1.0
    LANGUAGES CXX C
)

add_subdirectory(ext/glfw)

add_executable(app src/main.cpp)

target_link_libraries(app PRIVATE glfw)

set_target_properties(app PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
    COMPILE_WARNING_AS_ERROR ON
)