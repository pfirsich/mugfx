#include "shared.hpp"

#include <array>
#include <cstdio>

namespace mugfx {

static void* default_allocate(size_t size, void*)
{
    return std::malloc(size);
}

static void* default_reallocate(void* ptr, size_t, size_t new_size, void*)
{
    return std::realloc(ptr, new_size);
}

static void default_deallocate(void* ptr, size_t, void*)
{
    return std::free(ptr);
}

static void default_logging_callback(mugfx_severity severity, const char* msg)
{
    std::printf("[%s] %s\n", mugfx_severity_to_string(severity), msg);
}

static void default_panic_handler(const char*)
{
    std::abort();
}

struct SharedState {
    mugfx_allocator alloc;
    mugfx_logging_callback logging_callback;
    mugfx_panic_handler panic_handler;
};

static SharedState shared_state = {
    .alloc = {
        .allocate = default_allocate,
        .reallocate = default_reallocate,
        .deallocate = default_deallocate,
        .ctx = nullptr,
    },
    .logging_callback = default_logging_callback,
    .panic_handler = default_panic_handler,
};

void* allocate_raw(size_t size)
{
    return shared_state.alloc.allocate(size, shared_state.alloc.ctx);
}

void deallocate_raw(void* ptr, size_t size)
{
    shared_state.alloc.deallocate(ptr, size, shared_state.alloc.ctx);
}

EXPORT const char* mugfx_severity_to_string(mugfx_severity severity)
{
    switch (severity) {
    case MUGFX_SEVERITY_DEBUG:
        return "DEBUG";
    case MUGFX_SEVERITY_INFO:
        return "INFO";
    case MUGFX_SEVERITY_WARN:
        return "WARN";
    case MUGFX_SEVERITY_ERROR:
        return "ERROR";
    default:
        return "INVALID";
    }
}

void log(mugfx_severity severity, const char* msg)
{
    if (shared_state.logging_callback) {
        shared_state.logging_callback(severity, msg);
    }

    if (severity >= MUGFX_SEVERITY_ERROR && shared_state.panic_handler) {
        shared_state.panic_handler(msg);
    }
}

void log_fmt(mugfx_severity severity, const char* fmt, std::va_list va)
{
    static std::array<char, 1024> buf;
    std::vsnprintf(buf.data(), buf.size(), fmt, va);
    log(severity, buf.data());
}

void log_debug(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_fmt(MUGFX_SEVERITY_DEBUG, fmt, args);
    va_end(args);
}

void log_info(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_fmt(MUGFX_SEVERITY_INFO, fmt, args);
    va_end(args);
}

void log_warn(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_fmt(MUGFX_SEVERITY_WARN, fmt, args);
    va_end(args);
}

void log_error(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    log_fmt(MUGFX_SEVERITY_ERROR, fmt, args);
    va_end(args);
}

void common_init(mugfx_init_params& params)
{
    default_init(params);
    if (params.allocator) {
        shared_state.alloc = *params.allocator;
    }
    if (params.logging_callback) {
        shared_state.logging_callback = params.logging_callback;
    }
    if (params.panic_handler) {
        shared_state.panic_handler = params.panic_handler;
    }
}

template <typename T, typename U>
static void set_default(T& v, U default_value)
{
    if (v == T {}) {
        v = static_cast<T>(default_value);
    }
}

void default_init(mugfx_init_params& params)
{
    set_default(params.max_num_shaders, 64);
    set_default(params.max_num_textures, 128);
    set_default(params.max_num_uniforms, 1024);
    set_default(params.max_num_buffers, 1024);
    set_default(params.max_num_materials, 512);
    set_default(params.max_num_geometries, 1024);
    set_default(params.max_num_render_targets, 32);
    set_default(params.max_num_pipelines, 1024);
}

void default_init(mugfx_shader_create_params&) { }

void default_init(mugfx_texture_create_params& params)
{
    set_default(params.format, MUGFX_PIXEL_FORMAT_RGBA8);
    set_default(params.wrap_s, MUGFX_TEXTURE_WRAP_REPEAT);
    set_default(params.wrap_t, params.wrap_s);
    set_default(params.min_filter,
        params.generate_mipmaps ? MUGFX_TEXTURE_MIN_FILTER_LINEAR_MIPMAP_LINEAR
                                : MUGFX_TEXTURE_MIN_FILTER_LINEAR);
    set_default(params.mag_filter, MUGFX_TEXTURE_MAG_FILTER_LINEAR);
    set_default(params.data_format, params.format);
}

void default_init(mugfx_material_create_params& params)
{
    set_default(params.depth_func, MUGFX_DEPTH_FUNC_LEQUAL);
    set_default(params.write_mask, MUGFX_WRITE_MASK_RGBA | MUGFX_WRITE_MASK_DEPTH);
    set_default(params.cull_face, MUGFX_CULL_FACE_MODE_BACK);
    set_default(params.src_blend, MUGFX_BLEND_FUNC_ONE);
    set_default(params.dst_blend, MUGFX_BLEND_FUNC_ZERO);
#ifndef MUGFX_WEBGL
    set_default(params.polygon_mode, MUGFX_POLYGON_MODE_FILL);
#endif
    set_default(params.stencil_func, MUGFX_STENCIL_FUNC_ALWAYS);
}

void default_init(mugfx_buffer_create_params& params)
{
    set_default(params.target, MUGFX_BUFFER_TARGET_ARRAY);
    set_default(params.usage, MUGFX_BUFFER_USAGE_HINT_STATIC);
}

void default_init(mugfx_uniform_data_create_params& params)
{
    set_default(params.usage_hint, MUGFX_UNIFORM_DATA_USAGE_HINT_FRAME);
}

void default_init(mugfx_geometry_create_params& params)
{
    set_default(params.draw_mode, MUGFX_DRAW_MODE_TRIANGLES);
}

void default_init(mugfx_render_target_create_params& params)
{
    set_default(params.color_formats[0], MUGFX_PIXEL_FORMAT_RGBA8);
    set_default(params.depth_format, MUGFX_PIXEL_FORMAT_DEPTH24);
}

}