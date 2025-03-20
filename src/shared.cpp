#include "shared.hpp"

#include <array>
#include <cstdio>

const char* mugfx_severity_to_string(mugfx_severity severity)
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

namespace {
mugfx_logging_callback& get_logging_callback()
{
    static mugfx_logging_callback callback = nullptr;
    return callback;
}

mugfx_panic_handler& get_panic_handler()
{
    static mugfx_panic_handler handler = nullptr;
    return handler;
}

void* default_allocate(size_t size, void*)
{
    return std::malloc(size);
}

void* default_reallocate(void* ptr, size_t, size_t new_size, void*)
{
    return std::realloc(ptr, new_size);
}

void default_deallocate(void* ptr, size_t, void*)
{
    return std::free(ptr);
}

mugfx_allocator* get_default_allocator()
{
    static mugfx_allocator allocator {
        .allocate = default_allocate,
        .reallocate = default_reallocate,
        .deallocate = default_deallocate,
        .ctx = nullptr,
    };
    return &allocator;
}

mugfx_allocator*& get_allocator()
{
    static mugfx_allocator* allocator = nullptr;
    return allocator;
}

template <typename T, typename U>
void set_default(T& v, U default_value)
{
    if (v == T {}) {
        v = static_cast<T>(default_value);
    }
}
}

void* allocate(size_t size)
{
    assert(get_allocator());
    return get_allocator()->allocate(size, get_allocator()->ctx);
}

void* reallocate(void* ptr, size_t old_size, size_t new_size)
{
    assert(get_allocator());
    return get_allocator()->reallocate(ptr, old_size, new_size, get_allocator()->ctx);
}

void deallocate(void* ptr, size_t size)
{
    assert(get_allocator());
    get_allocator()->deallocate(ptr, size, get_allocator()->ctx);
}

void log(mugfx_severity severity, const char* msg)
{
    const auto panic_handler = get_panic_handler();
    if (severity >= MUGFX_SEVERITY_ERROR && panic_handler) {
        panic_handler(msg);
        std::abort();
    }
    const auto logging_callback = get_logging_callback();
    if (logging_callback) {
        logging_callback(severity, msg);
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
    get_logging_callback() = params.logging_callback;
    get_panic_handler() = params.panic_handler;
    get_allocator() = params.allocator ? params.allocator : get_default_allocator();
}

void default_init(mugfx_init_params& params)
{
    if (!params.allocator) {
        params.allocator = get_default_allocator();
    }
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
    set_default(params.cull_face, MUGFX_CULL_FACE_MODE_NONE);
    set_default(params.src_blend, MUGFX_BLEND_FUNC_ONE);
    set_default(params.dst_blend, MUGFX_BLEND_FUNC_ZERO);
    set_default(params.polygon_mode, MUGFX_POLYGON_MODE_FILL);
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
