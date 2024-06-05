#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>

#include <cstdio>

#include "dawn/native/DawnNative.h"

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

struct WebGpuRenderer {
    std::unique_ptr<dawn::native::Instance> instance;
    wgpu::BackendType backendType = wgpu::BackendType::Vulkan;
    wgpu::AdapterType adapterType = wgpu::AdapterType::Unknown;
    std::vector<std::string> enableToggles;
    std::vector<std::string> disableToggles;
    wgpu::SwapChain swapChain;

    wgpu::Device device;
    wgpu::RenderPipeline pipeline;

    void init(GLFWwindow* window, uint32_t width, uint32_t height) {
        WGPUInstanceDescriptor instanceDescriptor{};
        instanceDescriptor.features.timedWaitAnyEnable = true;
        instance = std::make_unique<dawn::native::Instance>(&instanceDescriptor);
        wgpu::RequestAdapterOptions options = {};
        options.backendType = backendType;
        auto adapters = instance->EnumerateAdapters(&options);
        wgpu::DawnAdapterPropertiesPowerPreference power_props{};
        wgpu::AdapterProperties adapterProperties{};
        adapterProperties.nextInChain = &power_props;
        auto isAdapterType = [this, &adapterProperties](const auto& adapter) -> bool {
            if (adapterType == wgpu::AdapterType::Unknown) {
                return true;
            }
            adapter.GetProperties(&adapterProperties);
            return adapterProperties.adapterType == adapterType;
        };
        auto preferredAdapter = std::find_if(adapters.begin(), adapters.end(), isAdapterType);
        if (preferredAdapter == adapters.end()) {
            fprintf(stderr, "Failed to find an adapter! Please try another adapter type.\n");
            device = wgpu::Device();
            return;
        }
        std::vector<const char*> enableToggleNames;
        std::vector<const char*> disabledToggleNames;
        for (const std::string& toggle : enableToggles) {
            enableToggleNames.push_back(toggle.c_str());
        }
        for (const std::string& toggle : disableToggles) {
            disabledToggleNames.push_back(toggle.c_str());
        }
        WGPUDawnTogglesDescriptor toggles;
        toggles.chain.sType = WGPUSType_DawnTogglesDescriptor;
        toggles.chain.next = nullptr;
        toggles.enabledToggles = enableToggleNames.data();
        toggles.enabledToggleCount = enableToggleNames.size();
        toggles.disabledToggles = disabledToggleNames.data();
        toggles.disabledToggleCount = disabledToggleNames.size();
        WGPUDeviceDescriptor deviceDesc = {};
        deviceDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&toggles);
        WGPUDevice backendDevice = preferredAdapter->CreateDevice(&deviceDesc);
        DawnProcTable backendProcs = dawn::native::GetProcs();
        auto surfaceChainedDesc = wgpu::glfw::SetupWindowAndGetSurfaceDescriptor(window);
        WGPUSurfaceDescriptor surfaceDesc;
        surfaceDesc.label = "surface";
        surfaceDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(surfaceChainedDesc.get());
        WGPUSurface surface = backendProcs.instanceCreateSurface(instance->Get(), &surfaceDesc);
        WGPUSwapChainDescriptor swapChainDesc = {};
        swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;
        swapChainDesc.format = static_cast<WGPUTextureFormat>(wgpu::TextureFormat::BGRA8Unorm);
        swapChainDesc.width = width;
        swapChainDesc.height = height;
        swapChainDesc.presentMode = WGPUPresentMode_Mailbox;
        WGPUSwapChain backendSwapChain =
            backendProcs.deviceCreateSwapChain(backendDevice, surface, &swapChainDesc);

        WGPUDevice cDevice = nullptr;
        DawnProcTable procs;

        procs = backendProcs;
        cDevice = backendDevice;
        swapChain = wgpu::SwapChain::Acquire(backendSwapChain);

        device = wgpu::Device::Acquire(cDevice);

        /////////////
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

    void draw() {
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

        swapChain.Present();
        dawn::native::InstanceProcessEvents(instance->Get());
    }
};

int main() {
    WebGpuRenderer renderer;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    const uint32_t width = 512;
    const uint32_t height = 512;
    GLFWwindow* window = glfwCreateWindow(width, height, "webgpu-test", nullptr, nullptr);

    renderer.init(window, width, height);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        renderer.draw();
    }
}