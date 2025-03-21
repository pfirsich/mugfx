# mugfx (µgfx)

This is the smallest possible graphics library (hence "micro") that I can use as an abstraction layer on top of OpenGL 4 (which I know how to use), WebGL 2 (OpenGL ES 3.0) and possibly Vulkan in the future.

I heard a lot that after Vulkan would be released there should be other high-level graphics APIs on top of it as alternative to OpenGL, but I don't see a lot of them (see alternatives below). I want something like that, so I am giving it a go myself.

You kind of need to read a book to understand how Vulkan works and if you want to use a graphics API abstraction library that supports Vulkan, you almost always need to understand Vulkan too, which imho defeats the point of the library. This is intended for people that have rendered a few things here and there and know how to use OpenGL/WebGL. The modern graphics APIs allow a lot of control, but they have a very high complexity and OpenGL does not allow that much control, but is fairly simple. This is somewhat in the middle, but way closer to OpenGL. Essentially it is a variation of OpenGL with a few tweaks to make it more compatible with modern graphics APIs.

To be clear, I designed this library as an intermediate step to possibly using Vulkan some day. Don't use.

To keep it small, this library also does no IO (apart from interfacing with a graphics library), i.e. it won't load any files or create windows or anything like that. The only dependency of this library is the graphics library you are compiling it for.

## Documentation

### Overview

I like to imagine that the header should be self-explanatory for someone that knows how to use OpenGL and roughly how rendering works. The big difference to OpenGL is that most data structure are immutable, so we can turn a lot of the functions that change properties into an argument to the `*_create` functions. This include sizes for textures and buffers. Also most of OpenGL's global state is encapsulated by objects in mugfx. The following explanations are assuming an OpenGL background and are intended at devs that know how to use OpenGL, but not a modern rendering API.

Have a look at [hello_triangle.cpp](examples/hello_triangle.cpp) to get a rough idea of what you need to do to get something on the screen.

### General

Of course you need to initialize and shutdown the library with `mugfx_init` and `mugfx_shutdown`. Apart from uniform data all CPU-side dynamic allocations will happen in `mugfx_init`. If you need other limits (`MUGFX_MAX_*`), you can just change them and recompile the library.

Every object in mugfx is identified with an id that forms a generational index. An id of 0 is always an invalid object id and, if returned, indicates an error or, if used, generates an error. Inside parameter structs it indicates that no object is referenced.

### Shaders & Uniforms

Shaders are annoying to do properly cross-platform, because they simply are not. In OpenGL you can just load GLSL and reflect that GLSL (enumerate all the uniforms and their type info), but in Vulkan you have to load an intermediate SPIR-V representation and provide reflected uniform information you retrieved externally. Additionally the old-school OpenGL way of setting uniforms is setting them by name and retrieving uniform locations requires a handle to a shader. In Vulkan you set uniforms through uniform buffers only, which may be bound to multiple pipelines (i.e. independent of shader objects).

Therefore finding a common abstraction is a pain. I had multiple iterations of the uniform handling API and they all stank. I still want it to be possible for mugfx to use glUniform in case that proves to be more performant in some cases (which you can easily find online). Therefore I added a high-level uniform data abstraction that has a hint that describes how the uniform data is used to optimize the storage for these uniforms. But the fundamental uniform storage CPU-side is simply a struct, i.e. maps to a uniform buffer. Every uniform block should map to a uniform data object. To get rid of the names I require binding layout specifier support (OpenGL 4.2). I might re-evaluate this in the future. If uniforms have to be set using glUniform, mugfx will reflect the uniforms from the shader program and extract the data from the CPU-side copies of the uniform buffers.

So now you have to set binding locations explicitly in the shader through binding layout specifiers and also specify the used binding points during shader creation. Then during drawing you can pass resources to be bound to these binding locations. This does not require any uniform names.

Additionally it is recommended to use uniform buffers in WebGL 2 for best performance.

### Uniform Data

These wrap a uniform buffer, a cpu side buffer of the uniform data and a dirty flag.

The uniform data must be in std140 layout, because that is the most platform-independent. Ideally you should use a common subset of std140 and std430. This means you should only use scalars, two and four dimensional vectors and mat4. Sort your fields from largest to smallest.

In std140 layout can be quite wasteful as vec3 takes up as much space as vec4 and mat3 as much as mat4. This is another reason these types should be avoided.

### Textures

This is very similar to OpenGL, but it compared to Vulkan these encapsulate a texture and a sampler.

### Materials

These contain most of the fixed function global state in OpenGL and for Vulkan they partially wrap a pipeline. Vulkan pipelines are actually created dynamically for material/geometry pairs.

### Rendering

All `mugfx_draw_*` and `mugfx_clear` calls need to happen between `mugfx_begin_pass` and `mugfx_end_pass`, both of which need to be between `mugfx_begin_frame` and `mugfx_end_frame`. You can have multiple passes per frame.

Every resources has to be explicitly passed to `mugfx_draw`. The shaders are part of the material and all geometry related data is part of the geometry object. Other buffers, like textures and uniforms, need to be passed through a `mugfx_draw_binding`. You need to specify the binding point of a resources in the shaders and specify the used binding points when the shader is created. Then during drawing you should bind resources to the specified binding points.

Draw and clear calls are not actually sumitted to the GPU unless you call `mugfx_flush`, which is implied by ending a pass or a frame.

## Vulkan

I don't even if Vulkan is really necessary, because right now OpenGL ES works on Windows, Linux, WebGL 2, Mac/iOS (for now..), Android, Nintendo Switch and with [ANGLE](https://github.com/google/angle) or [Zink](https://docs.mesa3d.org/drivers/zink.html) you can run on Mac via Metal. But it's supposed to at least abstract OpenGL somewhat (I have used [glwrap](https://github.com/pfirsich/glwrap/) before) and it leaves the option to move to a more modern graphics API (like Vulkan) in the future.

I tried to design this API so it can work with Vulkan later, but I don't have much experience with Vulkan.
Vulkan is an interesting target, because its so close to the other APIs one might need to support (Direct3D 12, Metal, WebGPU, NVN, GNM) and it is lower overhead and is more flexible. In the future OpenGL will probably not just be on the way out, but actually out and I think Vulkan will stay relevant a bit longer. Lots of development effort nowadays is focused on Vulkan too. And [MoltenVK](https://github.com/KhronosGroup/MoltenVK) is probably faster and more robust than Zink/ANGLE.

## Building

You shouldn't need any dependencies beyond CMake, build-essentials and a C++20 compiler. Then do the usual dance:

```bash
cmake -B build/ .
cmake --build build/
```

### Emscripten

Make sure Emscripten is installed and it's in your PATH (note to self: use `source "emsdk/emsdk_env.fish"`) then do a slightly different dance:

```bash
emcmake cmake -B build_webgl/ . # I recommend a separate build directory
cmake --build build_webgl/
```

## Integration

Just `add_subdirectory` this project with CMake and link against the `mugfx` target. If you want to see an example on how to use this with Emscripten, look at [hello_triangle](examples/CMakeLists.txt).

## Alternatives

* [sokol_gfx](https://github.com/floooh/sokol/blob/master/sokol_gfx.h): too low-level. I feel like I need to know the low-level APIs to use it.
* [bgfx](https://github.com/bkaradzic/bgfx): very high level. No uniform buffers, own shading language (dialect to be fair), own file formats, much bigger and closer to a graphics engine.
* SDL_gpu: very cool, but it's not ready and I can't wait.
* [igl](https://github.com/facebook/igl): very cool, but very low-level (explicitly says so)

## To Do

* WebGL (Emscripten) example
* Instanced Rendering
* Render Targets
* Uniform Optimizations:
  - `CONSTANT`: OpenGL: Single shared, large uniform buffer bound once, Vulkan: Specialization Constants
  - `FRAME`: Pool multiple together and triple buffer them (one being written by CPU, one waiting for GPU, one being read by GPU)
  - `DRAW`: OpenGL: glUniform, Vulkan: Push Constants
* Figure out how to export symbols on Windows properly
* Disable checks when debug is not set
* Consider OpenGL 3.3 support (only thing missing is binding layout qualifier) - add optional `name` to `mugfx_shader_binding` to map uniform blocks/samplers to bindings.
* Shader Storage Buffers
* Compute Shaders
* Queries?
* Vulkan backend
* Object Labels:
    - https://www.khronos.org/opengl/wiki/Debug_Output#Scoping_messages
    - https://www.khronos.org/opengl/wiki/Debug_Output#Object_names
    - https://renderdoc.org/docs/how/how_annotate_capture.html
