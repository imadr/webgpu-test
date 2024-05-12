#include <GLFW/glfw3.h>
#include <iostream>

int main() {
    GLFWwindow* window;
    if(!glfwInit()){
        return -1;
    }

    window = glfwCreateWindow(800, 600, "webgpu-test", NULL, NULL);
    if (!window){
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    while (!glfwWindowShouldClose(window))
    {
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();


    return 0;
}