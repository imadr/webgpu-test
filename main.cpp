#include <GLFW/glfw3.h>
#include <webgpu/webgpu_cpp.h>
#include <webgpu/webgpu_glfw.h>

#include <cstdio>
#include <iostream>

#include "dawn/native/DawnNative.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define __STDC_LIB_EXT1__
#include "stb_image_write.h"

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
    wgpu::TextureView targetTextureView;
    wgpu::Texture targetTexture;
    wgpu::BufferDescriptor bufferDesc;
    wgpu::Buffer buffer;

    uint32_t m_width;
    uint32_t m_height;

    void init(GLFWwindow* window, uint32_t width, uint32_t height) {
        m_width = width;
        m_height = height;
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

        WGPUDeviceDescriptor deviceDesc = {};
        WGPUDevice backendDevice = preferredAdapter->CreateDevice(&deviceDesc);

        // DawnProcTable backendProcs = dawn::native::GetProcs();
        // auto surfaceChainedDesc = wgpu::glfw::SetupWindowAndGetSurfaceDescriptor(window);
        // WGPUSurfaceDescriptor surfaceDesc;
        // surfaceDesc.label = "surface";
        // surfaceDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(surfaceChainedDesc.get());
        // WGPUSurface surface = backendProcs.instanceCreateSurface(instance->Get(), &surfaceDesc);
        // WGPUSwapChainDescriptor swapChainDesc = {};
        // swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;
        // swapChainDesc.format = static_cast<WGPUTextureFormat>(wgpu::TextureFormat::BGRA8Unorm);
        // swapChainDesc.width = width;
        // swapChainDesc.height = height;
        // swapChainDesc.presentMode = WGPUPresentMode_Mailbox;
        // WGPUSwapChain backendSwapChain =
        //     backendProcs.deviceCreateSwapChain(backendDevice, surface, &swapChainDesc);
        // swapChain = wgpu::SwapChain::Acquire(backendSwapChain);

        WGPUDevice cDevice = nullptr;

        cDevice = backendDevice;

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

        wgpu::TextureDescriptor targetTextureDesc;
        targetTextureDesc.label = "Render target";
        targetTextureDesc.dimension = wgpu::TextureDimension::e2D;
        targetTextureDesc.size = {width, height, 1};
        targetTextureDesc.format = wgpu::TextureFormat::RGBA8UnormSrgb;
        targetTextureDesc.mipLevelCount = 1;
        targetTextureDesc.sampleCount = 1;
        targetTextureDesc.usage =
            wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopySrc;
        targetTextureDesc.viewFormats = nullptr;
        targetTextureDesc.viewFormatCount = 0;
        targetTexture = device.CreateTexture(&targetTextureDesc);

        wgpu::TextureViewDescriptor targetTextureViewDesc;
        targetTextureViewDesc.label = "Render texture view";
        targetTextureViewDesc.baseArrayLayer = 0;
        targetTextureViewDesc.arrayLayerCount = 1;
        targetTextureViewDesc.baseMipLevel = 0;
        targetTextureViewDesc.mipLevelCount = 1;
        targetTextureViewDesc.aspect = wgpu::TextureAspect::All;
        targetTextureView = targetTexture.CreateView(&targetTextureViewDesc);

        bufferDesc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
        bufferDesc.mappedAtCreation = false;
        bufferDesc.size = width * height * 4;
        buffer = device.CreateBuffer(&bufferDesc);
    }

    void draw() {
        wgpu::RenderPassColorAttachment attachment{//.view = swapChain.GetCurrentTextureView(),
                                                   .view = targetTextureView,
                                                   .loadOp = wgpu::LoadOp::Clear,
                                                   .storeOp = wgpu::StoreOp::Store,
                                                   .clearValue = wgpu::Color{0.5, 0.5, 0.5, 1.0}};

        wgpu::RenderPassDescriptor renderpass{.colorAttachmentCount = 1,
                                              .colorAttachments = &attachment};

        wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
        wgpu::RenderPassEncoder pass = encoder.BeginRenderPass(&renderpass);
        pass.SetPipeline(pipeline);
        pass.Draw(3);
        pass.End();
        wgpu::CommandBuffer commands = encoder.Finish();
        device.GetQueue().Submit(1, &commands);

        //////////////////////
        // swapChain.Present();
        ///////////////////////

        encoder = device.CreateCommandEncoder();
        wgpu::ImageCopyTexture source;
        source.texture = targetTexture;

        wgpu::ImageCopyBuffer destination;
        destination.buffer = buffer;
        destination.layout.bytesPerRow = 4 * m_width;
        destination.layout.offset = 0;
        destination.layout.rowsPerImage = m_height;
        wgpu::Extent3D copyExtent = {m_width, m_height, 1};
        encoder.CopyTextureToBuffer(&source, &destination, &copyExtent);

        commands = encoder.Finish();
        device.GetQueue().Submit(1, &commands);

        /////////////////////

        struct UserData {
            wgpu::BufferDescriptor* bufferDesc;
            wgpu::Buffer* buffer;
            int width;
            int height;
        };

        UserData userData = {&bufferDesc, &buffer, m_width, m_height};

        buffer.MapAsync(
            wgpu::MapMode::Read, 0, bufferDesc.size,
            [](WGPUBufferMapAsyncStatus status, void* userdata_) {
                UserData* userData = (UserData*)userdata_;
                if (status == WGPUBufferMapAsyncStatus_Success) {
                    wgpu::Buffer* buffer = static_cast<wgpu::Buffer*>(userData->buffer);
                    wgpu::BufferDescriptor* bufferDesc =
                        static_cast<wgpu::BufferDescriptor*>(userData->bufferDesc);

                    unsigned char* pixelData =
                        (unsigned char*)buffer->GetConstMappedRange(0, bufferDesc->size);

                    if (pixelData == NULL) {
                        return;
                    }
                    std::vector<char> vec(pixelData, pixelData + bufferDesc->size);

                    int bytesPerRow = 4 * userData->width;
                    int success = stbi_write_png("test_output_buffer.png", (int)userData->width,
                                                 (int)userData->height, 4, pixelData, bytesPerRow);

                    buffer->Unmap();
                } else {
                    std::cerr << "Error: Failed to map buffer to CPU memory. Error code: " << status
                              << std::endl;
                }
            },
            (void*)(&userData));

        device.Tick();

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