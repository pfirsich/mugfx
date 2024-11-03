# mugfx (µgfx)

This is the smallest possible graphics library (hence "micro") that I can use as an abstraction layer on top of OpenGL 4 (which I know how to use), WebGL 2 (OpenGL ES 3.0) and possibly Vulkan in the future.

I heard a lot that after Vulkan is released there should be other high-level graphics APIs on top of it as alternative to OpenGL, but I don't see a lot of them (see alternatives below). I want something like that, so I am giving it a go myself.

Generally I try to keep more abstractions from OpenGL than from the newer APIs, because I want it to be simpler (e.g. `Texture` vs. `Image` + `Sampler`). Also you kind of need to read a book to understand how Vulkan works and if you want to use a graphics API abstraction library that supports Vulkan, you almost always need to understand Vulkan too, which imho defeats the point. This is intended for people that have rendered a few things here and there and know how to use OpenGL/WebGL. The modern graphics APIs allow a lot of control, but they have a very high complexity and OpenGL does not allow that much control, but is fairly simple. This is somewhat in the middle, but way closer to OpenGL. Essentially is a variation of OpenGL with a few tweaks to make it more compatible with modern graphics APIs.

I like to imagine that the header should be self-explanatory for someone that knows how to use OpenGL and roughly how rendering works. The big difference to OpenGL is that most data structure are immutable, so we can turn a lot of the functions that change properties into an argument to the `_create` function. This include sizes for textures and buffers.

To keep this small, this library also does no IO (apart from interfacing with a graphics library), i.e. it won't load any files or create windows or anything like that. The only dependency of this library is the graphics library you are compiling it for.

## Vulkan

I don't even if Vulkan is really necessary, because right now OpenGL ES works on Windows, Linux, WebGL 2, Mac/iOS (for now..), Android, Nintendo Switch and with [ANGLE](https://github.com/google/angle) or [Zink](https://docs.mesa3d.org/drivers/zink.html) you can run on Mac via Metal. But it's supposed to at least abstract OpenGL somewhat (I have used [glwrap](https://github.com/pfirsich/glwrap/) before) and it leaves the option to move to a more modern graphics API (like Vulkan) in the future.

I tried to design this API so it can work with Vulkan later, but I don't have much experience with Vulkan.
Vulkan is an interesting target, because its so close to the other APIs one might need to support (Direct3D 12, Metal, WebGPU, NVN, GNM) and it is lower overhead and is more flexible. In the future OpenGL will probably not just be on the way out, but actually out and I think Vulkan will stay relevant a bit longer. Lots of development effort nowadays is focused on Vulkan too. And [MoltenVK](https://github.com/KhronosGroup/MoltenVK) is probably faster and more robust than Zink/ANGLE.

## Alternatives

[sokol_gfx](https://github.com/floooh/sokol/blob/master/sokol_gfx.h): too low-level. I feel like I need to know the low-level APIs to use it.
[bgfx](https://github.com/bkaradzic/bgfx): very high level. No uniform buffers, own shading language (dialect to be fair), own file formats, much bigger and closer to a graphics engine.
SDL_gpu: very cool, but it's not ready and I can't wait.
[igl](https://github.com/facebook/igl): very cool, but very low-level (explicitly says so)

## To Do

* WebGL (Emscripten) example
* Figure out how to export symbols on Windows properly
* Vulkan backend
* Object Labels: 
    - https://www.khronos.org/opengl/wiki/Debug_Output#Scoping_messages
    - https://www.khronos.org/opengl/wiki/Debug_Output#Object_names
    - https://renderdoc.org/docs/how/how_annotate_capture.html