#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>
#include <webgpu/webgpu_glfw.h>
#include <iostream>

wgpu::Instance instance;
wgpu::Device device;
wgpu::RenderPipeline pipeline;
wgpu::SwapChain swapChain;

const uint32_t width = 512;
const uint32_t height = 512;

void SetupSwapChain(wgpu::Surface surface) {
    wgpu::SwapChainDescriptor scDesc{.usage = wgpu::TextureUsage::RenderAttachment,
                                     .format = wgpu::TextureFormat::BGRA8Unorm,
                                     .width = width,
                                     .height = height,
                                     .presentMode = wgpu::PresentMode::Fifo};
    swapChain = device.CreateSwapChain(surface, &scDesc);
}

void GetDevice(void (*callback)(wgpu::Device)) {
    instance.RequestAdapter(
        nullptr,
        [](WGPURequestAdapterStatus status, WGPUAdapter cAdapter, const char* message,
           void* userdata) {
            if (status != WGPURequestAdapterStatus_Success) {
                exit(0);
            }
            wgpu::Adapter adapter = wgpu::Adapter::Acquire(cAdapter);
            adapter.RequestDevice(
                nullptr,
                [](WGPURequestDeviceStatus status, WGPUDevice cDevice, const char* message,
                   void* userdata) {
                    wgpu::Device device = wgpu::Device::Acquire(cDevice);
                    device.SetUncapturedErrorCallback(
                        [](WGPUErrorType type, const char* message, void* userdata) {
                            std::cout << "Error: " << type << " - message: " << message;
                        },
                        nullptr);
                    reinterpret_cast<void (*)(wgpu::Device)>(userdata)(device);
                },
                userdata);
        },
        reinterpret_cast<void*>(callback));
}

const char shaderCode[] = R"(
struct VertexOutput {
    @builtin(position) Position : vec4f,
    @location(0) Color: vec3f
}

@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> VertexOutput {
    var output: VertexOutput;

    if (in_vertex_index == 0u) {
        output.Position = vec4<f32>(-0.5, -0.5, 0.0, 1.0);
        output.Color = vec3<f32>(1.0, 0.0, 0.0);
    } else if (in_vertex_index == 1u) {
        output.Position = vec4<f32>(0.5, -0.5, 0.0, 1.0);
        output.Color = vec3<f32>(0.0, 1.0, 0.0);
    } else {
        output.Position = vec4<f32>(0.0, 0.5, 0.0, 1.0);
        output.Color = vec3<f32>(0.0, 0.0, 1.0);
    }

    return output;
}

@fragment
fn fs_main(input: VertexOutput) -> @location(0) vec4f {
     return vec4<f32>(input.Color, 1.0);
}
)";

void CreateRenderPipeline() {
    wgpu::ShaderModuleWGSLDescriptor wgslDesc{};
    wgslDesc.code = shaderCode;

    wgpu::ShaderModuleDescriptor shaderModuleDescriptor{.nextInChain = &wgslDesc};
    wgpu::ShaderModule shaderModule = device.CreateShaderModule(&shaderModuleDescriptor);

    wgpu::ColorTargetState colorTargetState{.format = wgpu::TextureFormat::BGRA8Unorm};

    wgpu::FragmentState fragmentState{
        .module = shaderModule, .targetCount = 1, .targets = &colorTargetState};

    wgpu::RenderPipelineDescriptor descriptor{.vertex = {.module = shaderModule},
                                              .fragment = &fragmentState};
    pipeline = device.CreateRenderPipeline(&descriptor);
}

void Render() {
    wgpu::RenderPassColorAttachment attachment{.view = swapChain.GetCurrentTextureView(),
                                               .loadOp = wgpu::LoadOp::Clear,
                                               .storeOp = wgpu::StoreOp::Store};

    wgpu::RenderPassDescriptor renderpass{.colorAttachmentCount = 1,
                                          .colorAttachments = &attachment};

    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);
    pass.SetPipeline(pipeline);
    pass.Draw(3);
    pass.End();
    wgpu::CommandBuffer commands = encoder.Finish();
    device.GetQueue().Submit(1, &commands);
}

void InitGraphics(wgpu::Surface surface) {
    SetupSwapChain(surface);
    CreateRenderPipeline();
}

void Start() {
    if (!glfwInit()) {
        return;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(width, height, "webgpu-test", nullptr, nullptr);

    wgpu::Surface surface = wgpu::glfw::CreateSurfaceForWindow(instance, window);

    InitGraphics(surface);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        Render();
        swapChain.Present();
        instance.ProcessEvents();
    }
}

int main() {
    instance = wgpu::CreateInstance();
    GetDevice([](wgpu::Device dev) {
        device = dev;
        Start();
    });
}