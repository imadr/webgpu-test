# webgpu-test

## Resources

- [Learn WebGPU (C++)](https://eliemichel.github.io/LearnWebGPU/index.html)
- [Learn Wgpu (Rust)](https://sotrh.github.io/learn-wgpu/#what-is-wgpu)
- [WebGPU Fundamentals (Javascript)](https://webgpufundamentals.org/)

## WebGPU implementations (backend)

- wgpu https://wgpu.rs/ https://github.com/gfx-rs/wgpu written in Rust
to use wgpu with C++ wgpu-native https://github.com/gfx-rs/wgpu-native

- Dawn https://dawn.googlesource.com/dawn

- emscripten : backend provided by web browser

- **WebGPU distribution** https://github.com/eliemichel/WebGPU-distribution
cmake project, easily switch between difference implementations/backends
Use WEBGPU_BACKEND param in cmake to change backend

-> Dawn isn't working, opened issue https://github.com/eliemichel/WebGPU-distribution/issues/17
**!! Can't breakpoint debug with wgpu !**

- + **WebGPU-Cpp** wrapper https://github.com/eliemichel/WebGPU-Cpp
to get "nice" c++ features like namespace and default constructors
example :
```
// C style
WGPUInstanceDescriptor desc = {};
desc.nextInChain = nullptr;
WGPUInstance instance = wgpuCreateInstance(&desc);
```
vs
```
// C++ style
wgpu::InstanceDescriptor desc = {};
wgpu::Instance instance = wgpu::createInstance(&desc);
```

## Shader compilers

- Tint https://dawn.googlesource.com/tint
- Naga https://github.com/gfx-rs/wgpu/tree/trunk/naga

## Setup

- Unzip https://eliemichel.github.io/LearnWebGPU/_downloads/823ae747c9d201d7bfefb17fb832d853/glfw-3.3.8-light.zip in ext
- Unzip https://github.com/eliemichel/glfw3webgpu/releases/download/v1.1.0/glfw3webgpu-v1.1.0.zip in ext
- clone https://github.com/eliemichel/WebGPU-distribution.git in ext

- WebGPU distribution works seamlessly with wgpu, [dawn is broken on main branch ](https://github.com/eliemichel/WebGPU-distribution/issues/15)

## Notes

- https://kvark.github.io/web/gpu/native/2020/05/03/point-of-webgpu-native.html
> If WebGPU on native becomes practical, it would be strictly superior to (target OpenGL only) and (target a graphics library like bgfx or sokol-gfx). It will be supported by an actual specification, a rich conformance test suite, big corporations, and tested to death by the use in browsers