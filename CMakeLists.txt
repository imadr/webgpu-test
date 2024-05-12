cmake_minimum_required(VERSION 3.0...3.25)
project(
    webgpu-test
    VERSION 0.1.0
    LANGUAGES CXX C
)

if (NOT EMSCRIPTEN)
    add_subdirectory(ext/glfw)
endif()
add_subdirectory(ext/glfw3webgpu)
add_subdirectory(ext/webgpu)

add_executable(app src/main.cpp)

target_link_libraries(app PRIVATE glfw webgpu glfw3webgpu)
target_copy_webgpu_binaries(app)


if (EMSCRIPTEN)
    target_link_options(app PRIVATE
        -sUSE_GLFW=3
        -sUSE_WEBGPU
        -sASYNCIFY
    )

    set_target_properties(app PROPERTIES SUFFIX ".html")
endif()

set_target_properties(app PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS OFF
    COMPILE_WARNING_AS_ERROR ON
)