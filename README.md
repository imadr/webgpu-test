# webgpu-test

## Resources

- [Learn WebGPU (C++)](https://eliemichel.github.io/LearnWebGPU/index.html)
- [Learn Wgpu (Rust)](https://sotrh.github.io/learn-wgpu/#what-is-wgpu)
- [WebGPU Fundamentals (Javascript)](https://webgpufundamentals.org/)

## WebGPU implementations (backend)

- wgpu https://wgpu.rs/ https://github.com/gfx-rs/wgpu written in Rust
to use wgpu with C++ wgpu-native https://github.com/gfx-rs/wgpu-native

- Dawn https://dawn.googlesource.com/dawn

- emscripten

- WebGPU distribution https://github.com/eliemichel/WebGPU-distribution
cmake project, easily switch between difference implementations/backends



## Notes

- https://kvark.github.io/web/gpu/native/2020/05/03/point-of-webgpu-native.html
> If WebGPU on native becomes practical, it would be strictly superior to (target OpenGL only) and (target a graphics library like bgfx or sokol-gfx). It will be supported by an actual specification, a rich conformance test suite, big corporations, and tested to death by the use in browsers