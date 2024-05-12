#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>
#include <webgpu/webgpu.h>

#include <iostream>
#include <vector>

WGPUAdapter requestAdapter(WGPUInstance instance, WGPURequestAdapterOptions const* options) {
    struct UserData {
        WGPUAdapter adapter = nullptr;
        bool request_ended = false;
    };
    UserData user_data;

    auto on_adapter_request_ended = [](WGPURequestAdapterStatus status, WGPUAdapter adapter,
                                       char const* message, void* p_user_data) {
        UserData& user_data = *reinterpret_cast<UserData*>(p_user_data);
        if (status == WGPURequestAdapterStatus_Success) {
            user_data.adapter = adapter;
        } else {
            std::cerr << "Could not get WebGPU adapter: " << message << std::endl;
        }
        user_data.request_ended = true;
    };

    wgpuInstanceRequestAdapter(instance, options, on_adapter_request_ended, (void*)&user_data);

    return user_data.adapter;
}

WGPUDevice requestDevice(WGPUAdapter adapter, WGPUDeviceDescriptor const* descriptor) {
    struct UserData {
        WGPUDevice device = nullptr;
        bool request_ended = false;
    };
    UserData userData;

    auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device,
                                   char const* message, void* pUserData) {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestDeviceStatus_Success) {
            userData.device = device;
        } else {
            std::cout << "Could not get WebGPU device: " << message << std::endl;
        }
        userData.request_ended = true;
    };

    wgpuAdapterRequestDevice(adapter, descriptor, onDeviceRequestEnded, (void*)&userData);

    return userData.device;
}

int main() {
    GLFWwindow* window;
    if (!glfwInit()) {
        return -1;
    }

    int window_width = 800;
    int window_height = 600;

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(window_width, window_height, "webgpu-test", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;

    WGPUInstance instance = wgpuCreateInstance(&desc);

    if (!instance) {
        std::cerr << "Could not initialize WebGPU" << std::endl;
        return 1;
    }

    std::cout << "WGPU instance: " << instance << std::endl;

    WGPUSurface surface = glfwGetWGPUSurface(instance, window);

    WGPURequestAdapterOptions adapter_opts = {};
    adapter_opts.nextInChain = nullptr;
    adapter_opts.compatibleSurface = surface;
    WGPUAdapter adapter = requestAdapter(instance, &adapter_opts);

    std::cout << "WGPU adapter: " << adapter << std::endl;

    WGPUDeviceDescriptor device_desc = {};
    device_desc.nextInChain = nullptr;
    device_desc.label = "device_0";
    device_desc.requiredFeaturesCount = 0;
    device_desc.requiredLimits = nullptr;
    device_desc.defaultQueue.nextInChain = nullptr;
    device_desc.defaultQueue.label = "default_queue_0";
    WGPUDevice device = requestDevice(adapter, &device_desc);
    std::cout << "Device: " << device << std::endl;

    auto on_device_error = [](WGPUErrorType type, char const* message, void*) {
        std::cout << "Uncaptured device error: type " << type;
        if (message) std::cout << " (" << message << ")";
        std::cout << std::endl;
    };
    wgpuDeviceSetUncapturedErrorCallback(device, on_device_error, nullptr);

    WGPUSwapChainDescriptor swapchain_desc = {};
    swapchain_desc.nextInChain = nullptr;
    swapchain_desc.width = window_width;
    swapchain_desc.height = window_height;
    WGPUTextureFormat swapchain_format = wgpuSurfaceGetPreferredFormat(surface, adapter);
    swapchain_desc.format = swapchain_format;
    swapchain_desc.usage = WGPUTextureUsage_RenderAttachment;
    swapchain_desc.presentMode = WGPUPresentMode_Fifo;
    WGPUSwapChain swapchain = wgpuDeviceCreateSwapChain(device, surface, &swapchain_desc);
    std::cout << "Swapchain: " << swapchain << std::endl;

    WGPUQueue queue = wgpuDeviceGetQueue(device);

    auto on_queue_work_done = [](WGPUQueueWorkDoneStatus status, void*) {
        std::cout << "Queued work finished with status: " << status << std::endl;
    };
    wgpuQueueOnSubmittedWorkDone(queue, on_queue_work_done, nullptr);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        WGPUTextureView next_texture = wgpuSwapChainGetCurrentTextureView(swapchain);

        if (!next_texture) {
            std::cerr << "Cannot acquire next swap chain texture" << std::endl;
            break;
        }

        WGPUCommandEncoderDescriptor command_encoder_desc = {};
        command_encoder_desc.nextInChain = nullptr;
        command_encoder_desc.label = "command_encoder_0";
        WGPUCommandEncoder command_encoder =
            wgpuDeviceCreateCommandEncoder(device, &command_encoder_desc);

        WGPURenderPassDescriptor render_pass_desc = {};

        WGPURenderPassColorAttachment render_pass_color_attachment = {};
        render_pass_color_attachment.view = next_texture;
        render_pass_color_attachment.resolveTarget = nullptr;
        render_pass_color_attachment.loadOp = WGPULoadOp_Clear;
        render_pass_color_attachment.storeOp = WGPUStoreOp_Store;
        render_pass_color_attachment.clearValue = WGPUColor{0.1, 0.1, 0.1, 1.0};

        render_pass_desc.colorAttachmentCount = 1;
        render_pass_desc.colorAttachments = &render_pass_color_attachment;
        render_pass_desc.depthStencilAttachment = nullptr;
        render_pass_desc.timestampWriteCount = 0;
        render_pass_desc.timestampWrites = nullptr;
        render_pass_desc.nextInChain = nullptr;

        WGPURenderPassEncoder render_pass =
            wgpuCommandEncoderBeginRenderPass(command_encoder, &render_pass_desc);
        wgpuRenderPassEncoderEnd(render_pass);
        wgpuRenderPassEncoderRelease(render_pass);

        wgpuTextureViewRelease(next_texture);

        WGPUCommandBufferDescriptor command_buffer_desc = {};
        command_buffer_desc.nextInChain = nullptr;
        command_buffer_desc.label = "command_buffer_0";
        WGPUCommandBuffer command_buffer =
            wgpuCommandEncoderFinish(command_encoder, &command_buffer_desc);
        wgpuCommandEncoderRelease(command_encoder);
        wgpuQueueSubmit(queue, 1, &command_buffer);
        wgpuCommandBufferRelease(command_buffer);

        wgpuSwapChainPresent(swapchain);
    }

    wgpuSwapChainRelease(swapchain);
    wgpuQueueRelease(queue);
    wgpuSurfaceRelease(surface);
    wgpuAdapterRelease(adapter);
    wgpuDeviceRelease(device);
    wgpuInstanceRelease(instance);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}