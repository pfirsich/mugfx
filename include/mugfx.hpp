#pragma once

#include <array>
#include <memory>
#include <span>
#include <string_view>
#include <utility>

#include "mugfx.h"

namespace mugfx {
enum class Severity { Debug, Info, Warn, Error, Critical };
std::string_view to_string(Severity severity);

using LoggingCallback = void(Severity, std::string_view message);
using PanicHandler = void(std::string_view message);

struct Allocator {
    ~Allocator() = default;
    virtual void* allocate(size_t size) = 0;
    virtual void* rellocate(void* ptr, size_t old_size, size_t new_size) = 0;
    virtual void free(void* ptr, size_t size) = 0;
};

struct InitParams {
    LoggingCallback* logging_callback = nullptr;
    PanicHandler* panic_handler = nullptr;
    Allocator* allocator = nullptr;
    size_t max_num_shaders = 64;
    size_t max_num_textures = 128;
    size_t max_num_uniforms = 1024;
    size_t max_num_buffers = 1024;
    size_t max_num_materials = 512;
    size_t max_num_geometries = 1024;
    size_t max_num_render_targets = 32;
    size_t max_num_pipelines = 1024;
#ifdef MUGFX_OPENGL
    // We don't need anything here. Just create a context and make it current
#elif MUGFX_VULKAN
    const void* device; // VkDevice
    const void* swapchain; // VkSwapchainKHR
#elif MUGFX_WEBGPU
    const void* device; // GPUDevice
#endif
};

void init(const InitParams& params);

struct Id {
    uint32_t id = 0;
    bool valid() const { return id != 0; }
    explicit operator bool() const { return valid(); }
    uint32_t release() { return std::exchange(id, 0); }
};

// Uniform Descriptor
enum class UniformsScope { Constant, Frame, Draw };

enum class UniformType {
    Invalid = 0,
    Float,
    Vec2,
    Vec3,
    Vec4,
    Int,
    Ivec2,
    Ivec3,
    Ivec4,
    Uint,
    Uvec2,
    Uvec3,
    Uvec4,
    Mat2,
    Mat3,
    Mat4,
    Mat2x3,
    Mat3x2,
    Mat2x4,
    Mat4x2,
    Mat3x4,
    Mat4x3,
};

// Shader
enum class ShaderStage {
    Vertex = MUGFX_SHADER_STAGE_VERTEX,
    Fragment = MUGFX_SHADER_STAGE_FRAGMENT,
};

struct Shader : public Id {
    ~Shader() { mugfx_shader_destroy({ id }); }

    static Shader create(ShaderStage stage, const char* source)
    {
        return { mugfx_shader_create(static_cast<mugfx_shader_stage>(stage), source) };
    }
};

struct Texture : public Id {
    struct Params {
        size_t width;
        size_t height;
        size_t depth = 0;
    };

    ~Texture() { mugfx_texture_destroy({ id }); }

    void set_data(std::span<const uint8_t> data, PixelFormat format);

    static Texture create(const Params& params) { }
};

struct Geometry : public Id {
    struct Params { };
};

}