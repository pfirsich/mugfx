#include <array>
#include <cassert>
#include <memory>
#include <optional>

#include "glad/include/glad/gl.h"

#include "../shared.hpp"

static const char* gl_error_string(GLenum error)
{
    switch (error) {
    case GL_NO_ERROR:
        return "GL_NO_ERROR";
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY";
    /*case GL_STACK_UNDERFLOW:
        return "GL_STACK_UNDERFLOW";
    case GL_STACK_OVERFLOW:
        return "GL_STACK_OVERFLOW";*/
    default:
        return "UNKNOWN";
    }
}

static std::optional<GLenum> gl_shader_type(mugfx_shader_stage stage)
{
    switch (stage) {
    case MUGFX_SHADER_STAGE_VERTEX:
        return GL_VERTEX_SHADER;
    case MUGFX_SHADER_STAGE_FRAGMENT:
        return GL_FRAGMENT_SHADER;
    default:
        return std::nullopt;
    }
}

static std::optional<GLenum> gl_pixel_format(mugfx_pixel_format format)
{
    switch (format) {
    case MUGFX_PIXEL_FORMAT_RGB8:
        return GL_RGB8;
    case MUGFX_PIXEL_FORMAT_RGBA8:
        return GL_RGBA8;
    case MUGFX_PIXEL_FORMAT_RGB16F:
        return GL_RGB16F;
    case MUGFX_PIXEL_FORMAT_RGBA16F:
        return GL_RGBA16F;
    case MUGFX_PIXEL_FORMAT_RGB32F:
        return GL_RGB32F;
    case MUGFX_PIXEL_FORMAT_RGBA32F:
        return GL_RGBA32F;
    case MUGFX_PIXEL_FORMAT_DEPTH24:
        return GL_DEPTH_COMPONENT24;
    case MUGFX_PIXEL_FORMAT_DEPTH32F:
        return GL_DEPTH_COMPONENT32F;
    case MUGFX_PIXEL_FORMAT_DEPTH24_STENCIL8:
        return GL_DEPTH24_STENCIL8;
    default:
        return std::nullopt;
    }
}

struct DataFormat {
    GLenum format;
    GLenum data_type;
};

static std::optional<DataFormat> gl_data_format(mugfx_pixel_format format)
{
    switch (format) {
    case MUGFX_PIXEL_FORMAT_RGB8:
        return DataFormat { GL_RGB, GL_UNSIGNED_BYTE };
    case MUGFX_PIXEL_FORMAT_RGBA8:
        return DataFormat { GL_RGBA, GL_UNSIGNED_BYTE };
    case MUGFX_PIXEL_FORMAT_RGB16F:
        return DataFormat { GL_RGB, GL_HALF_FLOAT };
    case MUGFX_PIXEL_FORMAT_RGBA16F:
        return DataFormat { GL_RGBA, GL_HALF_FLOAT };
    case MUGFX_PIXEL_FORMAT_RGB32F:
        return DataFormat { GL_RGB, GL_FLOAT };
    case MUGFX_PIXEL_FORMAT_RGBA32F:
        return DataFormat { GL_RGBA, GL_FLOAT };
    case MUGFX_PIXEL_FORMAT_DEPTH24:
        return DataFormat { GL_DEPTH_COMPONENT, GL_UNSIGNED_INT };
    case MUGFX_PIXEL_FORMAT_DEPTH32F:
        return DataFormat { GL_DEPTH_COMPONENT, GL_FLOAT };
    case MUGFX_PIXEL_FORMAT_DEPTH24_STENCIL8:
        return DataFormat { GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8 };
    default:
        return std::nullopt;
    }
}

static std::optional<GLenum> gl_wrap_mode(mugfx_texture_wrap_mode wrap)
{
    switch (wrap) {
    case MUGFX_TEXTURE_WRAP_REPEAT:
        return GL_REPEAT;
    case MUGFX_TEXTURE_WRAP_CLAMP_TO_EDGE:
        return GL_CLAMP_TO_EDGE;
    case MUGFX_TEXTURE_WRAP_MIRRORED_REPEAT:
        return GL_MIRRORED_REPEAT;
    default:
        return std::nullopt;
    }
}

static std::optional<GLenum> gl_min_filter(mugfx_texture_min_filter filter)
{
    switch (filter) {
    case MUGFX_TEXTURE_MIN_FILTER_NEAREST:
        return GL_NEAREST;
    case MUGFX_TEXTURE_MIN_FILTER_LINEAR:
        return GL_LINEAR;
    case MUGFX_TEXTURE_MIN_FILTER_NEAREST_MIPMAP_NEAREST:
        return GL_NEAREST_MIPMAP_NEAREST;
    case MUGFX_TEXTURE_MIN_FILTER_LINEAR_MIPMAP_NEAREST:
        return GL_LINEAR_MIPMAP_NEAREST;
    case MUGFX_TEXTURE_MIN_FILTER_NEAREST_MIPMAP_LINEAR:
        return GL_NEAREST_MIPMAP_LINEAR;
    case MUGFX_TEXTURE_MIN_FILTER_LINEAR_MIPMAP_LINEAR:
        return GL_LINEAR_MIPMAP_LINEAR;
    default:
        return std::nullopt;
    }
}

static std::optional<GLenum> gl_mag_filter(mugfx_texture_mag_filter filter)
{
    switch (filter) {
    case MUGFX_TEXTURE_MAG_FILTER_NEAREST:
        return GL_NEAREST;
    case MUGFX_TEXTURE_MAG_FILTER_LINEAR:
        return GL_LINEAR;
    default:
        return std::nullopt;
    }
}

static bool bind_texture(uint32_t unit, GLenum target, GLuint texture)
{
    // TODO: Save this per target!
    static std::array<GLuint, 64> current_texture_2d = {};
    if (target == GL_TEXTURE_2D) {
        if (unit >= current_texture_2d.size()) {
            log_error("Texture unit must be in [0, %lu]", current_texture_2d.size());
            return false;
        }
        if (texture != current_texture_2d[unit]) {
            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(target, texture);
            if (const auto error = glGetError()) {
                log_error("Error binding texture %d: %s", texture, gl_error_string(error));
                return false;
            }
            current_texture_2d[unit] = texture;
        }
    } else {
        log_error("Invalid texture target %d", target);
        return false;
    }
    return true;
}

static std::optional<GLenum> gl_buffer_target(mugfx_buffer_target target)
{
    switch (target) {
    case MUGFX_BUFFER_TARGET_ARRAY:
        return GL_ARRAY_BUFFER;
    case MUGFX_BUFFER_TARGET_INDEX:
        return GL_ELEMENT_ARRAY_BUFFER;
    case MUGFX_BUFFER_TARGET_UNIFORM:
        return GL_UNIFORM_BUFFER;
    default:
        return std::nullopt;
    }
}

static std::optional<GLenum> gl_buffer_usage(mugfx_buffer_usage_hint usage)
{
    switch (usage) {
    case MUGFX_BUFFER_USAGE_HINT_STATIC:
        return GL_STATIC_DRAW;
    case MUGFX_BUFFER_USAGE_HINT_DYNAMIC:
        return GL_DYNAMIC_DRAW;
    case MUGFX_BUFFER_USAGE_HINT_STREAM:
        return GL_STREAM_DRAW;
    default:
        return std::nullopt;
    }
}

static size_t get_buffer_target_index(GLenum target)
{
    switch (target) {
    case GL_ARRAY_BUFFER:
        return 0;
    case GL_ELEMENT_ARRAY_BUFFER:
        return 1;
    case GL_UNIFORM_BUFFER:
        return 2;
    default:
        return 0xFF;
    }
}

static bool bind_buffer(GLenum target, GLuint buffer)
{
    static std::array<GLuint, 3> current_buffers = {};
    auto& current_buffer = current_buffers.at(get_buffer_target_index(target));
    if (current_buffer != buffer) {
        glBindBuffer(target, buffer);
        if (const auto error = glGetError()) {
            log_error("Error in glBindBuffer: %s", gl_error_string(error));
            return false;
        }
        current_buffer = buffer;
    }
    return true;
}

static bool bind_buffer(GLenum target, GLuint buffer, uint32_t binding, mugfx_range range)
{
    if (range.length) {
        glBindBufferBase(target, binding, buffer);
    } else {
        glBindBufferRange(target, binding, buffer, range.offset, range.length);
    }
    if (const auto error = glGetError()) {
        log_error("Error in glBindBufferBase/Range: %s", gl_error_string(error));
        return false;
    }
    return true;
}

static std::optional<GLenum> gl_depth_func(mugfx_depth_func func)
{
    switch (func) {
    case MUGFX_DEPTH_FUNC_NEVER:
        return GL_NEVER;
    case MUGFX_DEPTH_FUNC_LESS:
        return GL_LESS;
    case MUGFX_DEPTH_FUNC_EQUAL:
        return GL_EQUAL;
    case MUGFX_DEPTH_FUNC_LEQUAL:
        return GL_LEQUAL;
    case MUGFX_DEPTH_FUNC_GREATER:
        return GL_GREATER;
    case MUGFX_DEPTH_FUNC_NOT_EQUAL:
        return GL_NOTEQUAL;
    case MUGFX_DEPTH_FUNC_GEQUAL:
        return GL_GEQUAL;
    case MUGFX_DEPTH_FUNC_ALWAYS:
        return GL_ALWAYS;
    default:
        return std::nullopt;
    }
}

struct WriteMask {
    bool r;
    bool g;
    bool b;
    bool a;
    bool depth;
};

static std::optional<WriteMask> gl_write_mask(mugfx_write_mask mask)
{
    if (mask == 0) {
        return std::nullopt;
    }
    if ((mask & MUGFX_WRITE_MASK_NONE) > 0 && mask != MUGFX_WRITE_MASK_NONE) {
        return std::nullopt;
    }
    return WriteMask {
        .r = (mask & MUGFX_WRITE_MASK_R) > 0,
        .g = (mask & MUGFX_WRITE_MASK_G) > 0,
        .b = (mask & MUGFX_WRITE_MASK_B) > 0,
        .a = (mask & MUGFX_WRITE_MASK_A) > 0,
        .depth = (mask & MUGFX_WRITE_MASK_DEPTH) > 0,
    };
}

static std::optional<GLenum> gl_cull_face_mode(mugfx_cull_face_mode mode)
{
    switch (mode) {
    case MUGFX_CULL_FACE_MODE_NONE:
        return GL_NONE;
    case MUGFX_CULL_FACE_MODE_FRONT:
        return GL_FRONT;
    case MUGFX_CULL_FACE_MODE_BACK:
        return GL_BACK;
    case MUGFX_CULL_FACE_MODE_FRONT_AND_BACK:
        return GL_FRONT_AND_BACK;
    default:
        return std::nullopt;
    }
}

static std::optional<GLenum> gl_blend_func(mugfx_blend_func func)
{
    switch (func) {
    case MUGFX_BLEND_FUNC_ZERO:
        return GL_ZERO;
    case MUGFX_BLEND_FUNC_ONE:
        return GL_ONE;
    case MUGFX_BLEND_FUNC_SRC_COLOR:
        return GL_SRC_COLOR;
    case MUGFX_BLEND_FUNC_ONE_MINUS_SRC_COLOR:
        return GL_ONE_MINUS_SRC_COLOR;
    case MUGFX_BLEND_FUNC_DST_COLOR:
        return GL_DST_COLOR;
    case MUGFX_BLEND_FUNC_ONE_MINUS_DST_COLOR:
        return GL_ONE_MINUS_DST_COLOR;
    case MUGFX_BLEND_FUNC_SRC_ALPHA:
        return GL_SRC_ALPHA;
    case MUGFX_BLEND_FUNC_ONE_MINUS_SRC_ALPHA:
        return GL_ONE_MINUS_SRC_ALPHA;
    case MUGFX_BLEND_FUNC_DST_ALPHA:
        return GL_DST_ALPHA;
    case MUGFX_BLEND_FUNC_ONE_MINUS_DST_ALPHA:
        return GL_ONE_MINUS_DST_ALPHA;
    default:
        return std::nullopt;
    }
}

static std::optional<GLenum> gl_polygon_mode(mugfx_polygon_mode mode)
{
    switch (mode) {
    case MUGFX_POLYGON_MODE_FILL:
        return GL_FILL;
    case MUGFX_POLYGON_MODE_LINE:
        return GL_LINE;
    case MUGFX_POLYGON_MODE_POINT:
        return GL_POINT;
    default:
        return std::nullopt;
    }
}

static std::optional<GLenum> gl_stencil_func(mugfx_stencil_func func)
{
    switch (func) {
    case MUGFX_STENCIL_FUNC_NEVER:
        return GL_NEVER;
    case MUGFX_STENCIL_FUNC_LESS:
        return GL_LESS;
    case MUGFX_STENCIL_FUNC_EQUAL:
        return GL_EQUAL;
    case MUGFX_STENCIL_FUNC_LEQUAL:
        return GL_LEQUAL;
    case MUGFX_STENCIL_FUNC_GREATER:
        return GL_GREATER;
    case MUGFX_STENCIL_FUNC_NOT_EQUAL:
        return GL_NOTEQUAL;
    case MUGFX_STENCIL_FUNC_GEQUAL:
        return GL_GEQUAL;
    case MUGFX_STENCIL_FUNC_ALWAYS:
        return GL_ALWAYS;
    default:
        return std::nullopt;
    }
}

static bool bind_shader(GLuint program)
{
    static GLuint current_program = 0;
    if (current_program != program) {
        glUseProgram(program);
        if (const auto error = glGetError()) {
            log_error("Error in glUseProgram: %s", gl_error_string(error));
            return false;
        }
        current_program = program;
    }
    return true;
}

struct IndexType {
    GLenum type;
    GLboolean normalized;
};

static std::optional<IndexType> gl_vertex_attribute_type(mugfx_vertex_attribute_type type)
{
    switch (type) {
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_F32:
        return IndexType { GL_FLOAT, GL_FALSE };
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_F16:
        return IndexType { GL_HALF_FLOAT, GL_FALSE };
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_U8_NORM:
        return IndexType { GL_UNSIGNED_BYTE, GL_TRUE };
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_U16_NORM:
        return IndexType { GL_UNSIGNED_SHORT, GL_TRUE };
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_I8_NORM:
        return IndexType { GL_BYTE, GL_TRUE };
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_I16_NORM:
        return IndexType { GL_SHORT, GL_TRUE };
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_U8:
        return IndexType { GL_UNSIGNED_BYTE, GL_FALSE };
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_U16:
        return IndexType { GL_UNSIGNED_SHORT, GL_FALSE };
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_I8:
        return IndexType { GL_BYTE, GL_FALSE };
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_I16:
        return IndexType { GL_SHORT, GL_FALSE };
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_I10_10_10_2_NORM:
        return IndexType { GL_INT_2_10_10_10_REV, GL_TRUE };
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_U10_10_10_2_NORM:
        return IndexType { GL_UNSIGNED_INT_2_10_10_10_REV, GL_TRUE };
    default:
        return std::nullopt;
    }
}

static size_t get_attribute_size(mugfx_vertex_attribute_type type, size_t components)
{
    assert(components >= 1 && components <= 4);
    switch (type) {
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_F32:
        return 4 * components;
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_F16:
        return 2 * components;
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_U8_NORM:
        return 1 * components;
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_U16_NORM:
        return 2 * components;
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_I8_NORM:
        return 1 * components;
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_I16_NORM:
        return 2 * components;
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_U8:
        return 1 * components;
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_U16:
        return 2 * components;
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_I8:
        return 1 * components;
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_I16:
        return 2 * components;
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_I10_10_10_2_NORM:
        assert(components == 4);
        return 4;
    case MUGFX_VERTEX_ATTRIBUTE_TYPE_U10_10_10_2_NORM:
        assert(components == 4);
        return 4;
    default:
        return type;
    }
}

static std::optional<GLenum> gl_draw_mode(mugfx_draw_mode mode)
{
    switch (mode) {
    case MUGFX_DRAW_MODE_TRIANGLES:
        return GL_TRIANGLES;
    case MUGFX_DRAW_MODE_TRIANGLE_STRIP:
        return GL_TRIANGLE_STRIP;
    case MUGFX_DRAW_MODE_LINES:
        return GL_LINES;
    case MUGFX_DRAW_MODE_LINE_STRIP:
        return GL_LINE_STRIP;
    default:
        return std::nullopt;
    }
}

static std::optional<GLenum> gl_index_type(mugfx_index_type type)
{
    switch (type) {
    case MUGFX_INDEX_TYPE_U8:
        return GL_UNSIGNED_BYTE;
    case MUGFX_INDEX_TYPE_U16:
        return GL_UNSIGNED_SHORT;
    case MUGFX_INDEX_TYPE_U32:
        return GL_UNSIGNED_INT;
    default:
        return std::nullopt;
    }
}

static std::optional<size_t> get_index_size(GLenum type)
{
    switch (type) {
    case GL_UNSIGNED_BYTE:
        return 1;
    case GL_UNSIGNED_SHORT:
        return 2;
    case GL_UNSIGNED_INT:
        return 4;
    default:
        return std::nullopt;
    }
}

static bool bind_vao(GLuint vao)
{
    static GLuint current_vao = 0;
    if (current_vao != vao) {
        glBindVertexArray(vao);
        if (const auto error = glGetError()) {
            log_error("Error in glBindVertexArray: %s", gl_error_string(error));
            return false;
        }
        current_vao = vao;
    }
    return true;
}

struct Shader {
    GLuint shader;
    std::array<mugfx_shader_binding, MUGFX_MAX_SHADER_BINDINGS> bindings;
};

struct Texture {
    GLenum target;
    GLuint texture;
    size_t width;
    size_t height;
};

struct Material {
    mugfx_shader_id vert_shader;
    mugfx_shader_id frag_shader;
    GLuint shader_program;
    GLenum depth_func;
    WriteMask write_mask;
    GLenum cull_face;
    GLenum src_blend;
    GLenum dst_blend;
    std::array<float, 4> blend_color;
    GLenum polygon_mode;
    bool stencil_enable;
    GLenum stencil_func;
    int stencil_ref;
    uint32_t stencil_mask;
    std::array<mugfx_shader_binding, MUGFX_MAX_SHADER_BINDINGS> frag_bindings;
    std::array<mugfx_shader_binding, MUGFX_MAX_SHADER_BINDINGS> vert_bindings;
};

struct Buffer {
    GLenum target;
    GLuint buffer;
    size_t size;
    GLenum usage;
};

struct UniformData {
    mugfx_uniform_data_usage_hint usage;
    mugfx_buffer_id buffer = {};
    mugfx_range buffer_range = {};
    std::byte* cpu_buffer = nullptr;
    bool cpu_buffer_owned = false;
    bool dirty = false;
};

struct Geometry {
    GLenum draw_mode;
    GLuint vao;
    GLenum index_type;
    GLsizei vertex_count;
    GLsizei index_count;
};

template <typename T>
static Pool<T>& get_pool(size_t size = 0)
{
    static Pool<T> pool(size);
    return pool;
}

struct Pass {
    mugfx_render_target_id target;
    bool in_pass = false;
};

static Pass& get_current_pass()
{
    static Pass pass;
    return pass;
}

EXPORT void mugfx_init(mugfx_init_params params)
{
    common_init(params);
    gladLoaderLoadGL();
    get_pool<Shader>(params.max_num_shaders);
    get_pool<Texture>(params.max_num_textures);
    get_pool<Material>(params.max_num_materials);
    get_pool<Buffer>(params.max_num_buffers);
    get_pool<UniformData>(params.max_num_uniforms);
    get_pool<Geometry>(params.max_num_geometries);
}

EXPORT void mugfx_shutdown()
{
    gladLoaderUnloadGL();
}

EXPORT mugfx_shader_id mugfx_shader_create(mugfx_shader_create_params params)
{
    default_init(params);

    const auto shader_type = gl_shader_type(params.stage);
    if (!shader_type) {
        log_error("Invalid shader stage %d", params.stage);
        return { 0 };
    }

    const GLuint shader = glCreateShader(*shader_type);
    if (shader == 0) {
        log_error("Failed to create shader object: %s", gl_error_string(glGetError()));
        return { 0 };
    }

    glShaderSource(shader, 1, &params.source, NULL);
    if (const auto error = glGetError()) {
        log_error("Error in glShaderSource: %s", gl_error_string(error));
        glDeleteShader(shader);
        return { 0 };
    }

    glCompileShader(shader);

    GLint compile_status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);

    GLint log_length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

    std::unique_ptr<char> info_log;
    if (log_length > 0) {
        info_log.reset(reinterpret_cast<char*>(allocate(log_length)));
        glGetShaderInfoLog(shader, log_length, NULL, info_log.get());
    }

    if (!compile_status) {
        log_error("Shader compilation failed: %s", info_log.get());
        glDeleteShader(shader);
        return { 0 };
    }

    if (info_log) {
        log_warn("Shader compilation log: %s", info_log.get());
    }

    Shader pool_shader {
        .shader = shader,
        .bindings = {},
    };

    std::memcpy(&pool_shader.bindings[0], &params.bindings[0],
        sizeof(mugfx_shader_binding) * MUGFX_MAX_SHADER_BINDINGS);

    const auto key = get_pool<Shader>().insert(std::move(pool_shader));
    return { key };
}

EXPORT mugfx_shader_binding mugfx_shader_get_binding(mugfx_shader_id shader_id, size_t idx)
{
    const auto shader = get_pool<Shader>().get(shader_id.id);
    if (!shader) {
        log_error("Shader ID %u does not exist", shader_id.id);
        return {};
    }
    if (idx >= MUGFX_MAX_SHADER_BINDINGS) {
        return { .type = MUGFX_SHADER_BINDING_TYPE_NONE };
    }
    return shader->bindings[idx];
}

EXPORT void mugfx_shader_destroy(mugfx_shader_id shader_id)
{
    const auto shader = get_pool<Shader>().get(shader_id.id);
    if (!shader) {
        log_error("Shader ID %u does not exist", shader_id.id);
        return;
    }

    glDeleteShader(shader->shader);
    if (const auto error = glGetError()) {
        log_error("Failed to delete shader %u: %s", shader_id.id, gl_error_string(error));
    }

    get_pool<Shader>().remove(shader_id.id);
}

EXPORT mugfx_texture_id mugfx_texture_create(mugfx_texture_create_params params)
{
    default_init(params);

    GLuint texture = 0;
    glGenTextures(1, &texture); // Apparently this can't fail

    auto error_return = [&]() {
        mugfx_texture_destroy({ texture });
        return mugfx_texture_id { 0 };
    };

    const auto target = GL_TEXTURE_2D;

    if (!bind_texture(0, target, texture)) {
        return error_return();
    }

    // wrapping
    const auto wrap_s = gl_wrap_mode(params.wrap_s);
    if (!wrap_s) {
        log_error("Invalid wrap mode %d", params.wrap_s);
        return error_return();
    }
    const auto wrap_t = gl_wrap_mode(params.wrap_t);
    if (!wrap_t) {
        log_error("Invalid wrap mode %d", params.wrap_t);
        return error_return();
    }

    glTexParameteri(target, GL_TEXTURE_WRAP_S, *wrap_s);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, *wrap_t);

    // Set texture filtering
    const auto min_filter = gl_min_filter(params.min_filter);
    if (!min_filter) {
        log_error("Invalid min filter: %d", params.min_filter);
        return error_return();
    }
    const auto mag_filter = gl_mag_filter(params.mag_filter);
    if (!mag_filter) {
        log_error("Invalid min filter: %d", params.min_filter);
        return error_return();
    }

    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, *min_filter);
    if (const auto error = glGetError()) {
        log_error("Error setting min filter: %s", gl_error_string(error));
        return error_return();
    }
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, *mag_filter);
    if (const auto error = glGetError()) {
        log_error("Error setting mag filter: %s", gl_error_string(error));
        return error_return();
    }

    // Set texture storage and upload data if available
    const auto internal_format = gl_pixel_format(params.format);
    if (!internal_format) {
        log_error("Invalid pixel format: %d", params.format);
        return error_return();
    }

    const auto data_format = gl_data_format(params.data_format);
    if (!data_format) {
        log_error("Invalid data format: %d", params.data_format);
        return error_return();
    }

    glTexImage2D(target, 0, *internal_format, params.width, params.height, 0, data_format->format,
        data_format->data_type, params.data.data);
    if (const auto error = glGetError()) {
        log_error("Error setting mag filter: %s", gl_error_string(error));
        return error_return();
    }

    if (params.generate_mipmaps) {
        glGenerateMipmap(target);
        if (const auto error = glGetError()) {
            log_error("Error generating mipmaps: %s", gl_error_string(error));
            return error_return();
        }
    }

    const auto key = get_pool<Texture>().insert(Texture {
        .target = target,
        .texture = texture,
        .width = params.width,
        .height = params.height,
    });
    return { key };
}

EXPORT void mugfx_texture_set_data(
    mugfx_texture_id texture, mugfx_slice data, mugfx_pixel_format data_format)
{
    const auto tex = get_pool<Texture>().get(texture.id);
    if (!tex) {
        log_error("Texture ID %u does not exist", texture.id);
        return;
    }

    if (!bind_texture(0, tex->target, tex->texture)) {
        return;
    }

    const auto df = gl_data_format(data_format);
    if (!df) {
        log_error("Invalid data format: %d", data_format);
        return;
    }

    glTexSubImage2D(
        tex->target, 0, 0, 0, tex->width, tex->height, df->format, df->data_type, data.data);
}

EXPORT void mugfx_texture_destroy(mugfx_texture_id texture)
{
    const auto tex = get_pool<Texture>().get(texture.id);
    if (!tex) {
        log_error("Texture ID %u does not exist", texture.id);
        return;
    }

    glDeleteTextures(1, &tex->texture);
    if (const auto error = glGetError()) {
        log_error("Error destroying texture ID %d: %s", texture.id, gl_error_string(error));
    }

    get_pool<Texture>().remove(texture.id);
}

EXPORT mugfx_material_id mugfx_material_create(mugfx_material_create_params params)
{
    default_init(params);

    const auto depth_func = gl_depth_func(params.depth_func);
    if (!depth_func) {
        log_error("Invalid depth func: %d", params.depth_func);
        return { 0 };
    }

    const auto write_mask = gl_write_mask(params.write_mask);
    if (!write_mask) {
        log_error("Invalid write mask: %d", params.write_mask);
        return { 0 };
    }

    const auto cull_face = gl_cull_face_mode(params.cull_face);
    if (!cull_face) {
        log_error("Invalid cull face mode: %d", params.cull_face);
        return { 0 };
    }

    const auto src_blend = gl_blend_func(params.src_blend);
    if (!src_blend) {
        log_error("Invalid src blend func: %d", params.src_blend);
        return { 0 };
    }

    const auto dst_blend = gl_blend_func(params.dst_blend);
    if (!dst_blend) {
        log_error("Invalid dst blend func: %d", params.dst_blend);
        return { 0 };
    }

    const auto polygon_mode = gl_polygon_mode(params.polygon_mode);
    if (!polygon_mode) {
        log_error("Invalid polygon mode: %d", params.polygon_mode);
        return { 0 };
    }

    const auto stencil_func = gl_stencil_func(params.stencil_func);
    if (!stencil_func) {
        log_error("Invalid stencil func: %d", params.stencil_func);
        return { 0 };
    }

    const auto prog = glCreateProgram();
    if (prog == 0) {
        log_error("Could not create shader program: %s", gl_error_string(glGetError()));
        return { 0 };
    }

    const auto vert = get_pool<Shader>().get(params.vert_shader.id);
    if (!vert) {
        log_error("Shader ID %u does not exist", params.vert_shader.id);
        return { 0 };
    }

    const auto frag = get_pool<Shader>().get(params.frag_shader.id);
    if (!frag) {
        log_error("Shader ID %u does not exist", params.frag_shader.id);
        return { 0 };
    }

    glAttachShader(prog, vert->shader);
    if (const auto error = glGetError()) {
        log_error("Error in glAttachShader: %s", gl_error_string(error));
        glDeleteProgram(prog);
        return { 0 };
    }

    glAttachShader(prog, frag->shader);
    if (const auto error = glGetError()) {
        log_error("Error in glAttachShader: %s", gl_error_string(error));
        glDeleteProgram(prog);
        return { 0 };
    }

    // Don't need to check GL errors because documented errors are only relevant if prog is not a
    // program
    glLinkProgram(prog);

    GLint log_length = 0;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_length);

    std::unique_ptr<char> info_log;
    if (log_length > 0) {
        info_log.reset(reinterpret_cast<char*>(allocate(log_length)));
        glGetProgramInfoLog(prog, log_length, nullptr, info_log.get());
    }

    GLint linkStatus = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE) {
        log_error("Linking shader failed: %s", info_log.get());
        glDeleteProgram(prog);
        return { 0 };
    }

    if (info_log) {
        log_warn("Shader link log: %s", info_log.get());
    }

    Material mat {
        .vert_shader = params.vert_shader,
        .frag_shader = params.frag_shader,
        .shader_program = prog,
        .depth_func = *depth_func,
        .write_mask = *write_mask,
        .cull_face = *cull_face,
        .src_blend = *src_blend,
        .dst_blend = *dst_blend,
        .blend_color = {},
        .polygon_mode = *polygon_mode,
        .stencil_enable = params.stencil_enable,
        .stencil_func = *stencil_func,
        .stencil_ref = params.stencil_ref,
        .stencil_mask = params.stencil_mask,
    };
    std::memcpy(mat.blend_color.data(), params.blend_color, 4 * sizeof(float));
    std::memcpy(mat.frag_bindings.data(), frag->bindings.data(),
        MUGFX_MAX_SHADER_BINDINGS * sizeof(mugfx_shader_binding));
    std::memcpy(mat.vert_bindings.data(), vert->bindings.data(),
        MUGFX_MAX_SHADER_BINDINGS * sizeof(mugfx_shader_binding));

    const auto key = get_pool<Material>().insert(std::move(mat));
    return { key };
}

EXPORT void mugfx_material_destroy(mugfx_material_id material)
{
    const auto mat = get_pool<Material>().get(material.id);
    if (!mat) {
        log_error("Material ID %u does not exist", material.id);
        return;
    }

    glDeleteProgram(mat->shader_program);
    if (const auto error = glGetError()) {
        log_error("Error destroying material ID %d: %s", material.id, gl_error_string(error));
    }

    get_pool<Material>().remove(material.id);
}

EXPORT mugfx_buffer_id mugfx_buffer_create(mugfx_buffer_create_params params)
{
    default_init(params);

    if (params.data.length == 0) {
        log_warn("Creating empty buffer");
    }

    const auto target = gl_buffer_target(params.target);
    if (!target) {
        log_error("Invalid buffer target: %d", params.target);
        return { 0 };
    }

    const auto usage = gl_buffer_usage(params.usage);
    if (!usage) {
        log_error("Invalid buffer usage: %d", params.usage);
        return { 0 };
    }

    GLuint buffer;
    // Errors: number of buffers is negative
    glGenBuffers(1, &buffer);

    // Errors: target is invalid, buffer is not a buffer
    bind_buffer(*target, buffer);
    glBufferData(*target, params.data.length, params.data.data, *usage);
    if (const auto error = glGetError()) {
        log_error("Error in glBufferData: %s", gl_error_string(error));
        glDeleteBuffers(1, &buffer);
        return { 0 };
    }

    const auto key = get_pool<Buffer>().insert(Buffer {
        .target = *target,
        .buffer = buffer,
        .size = params.data.length,
        .usage = *usage,
    });

    return { key };
}

EXPORT void mugfx_buffer_update(mugfx_buffer_id buffer, size_t offset, mugfx_slice data)
{
    const auto buf = get_pool<Buffer>().get(buffer.id);
    if (!buf) {
        log_error("Buffer ID %u does not exist", buffer.id);
        return;
    }

    if (!bind_buffer(buf->target, buf->buffer)) {
        return;
    }

    if (!data.data) {
        // Orphan buffer
        glBufferData(buf->target, buf->size, nullptr, buf->usage);
        if (const auto error = glGetError()) {
            log_error("Error in glBufferData: %s", gl_error_string(error));
        }
    } else {
        const auto length = std::min(offset + data.length, buf->size) - offset;
        glBufferSubData(buf->target, offset, length, data.data);
        if (const auto error = glGetError()) {
            log_error("Error in glBufferSubData: %s", gl_error_string(error));
        }
    }
}

EXPORT void mugfx_buffer_destroy(mugfx_buffer_id buffer)
{
    const auto buf = get_pool<Buffer>().get(buffer.id);
    if (!buf) {
        log_error("Buffer ID %u does not exist", buffer.id);
        return;
    }

    glDeleteBuffers(1, &buf->buffer);
    if (const auto error = glGetError()) {
        log_error("Error destroying buffer ID %d: %s", buffer.id, gl_error_string(error));
    }

    get_pool<Buffer>().remove(buffer.id);
}

static mugfx_buffer_usage_hint get_buffer_usage(mugfx_uniform_data_usage_hint usage)
{
    switch (usage) {
    case MUGFX_UNIFORM_DATA_USAGE_HINT_CONSTANT:
        return MUGFX_BUFFER_USAGE_HINT_STATIC;
    case MUGFX_UNIFORM_DATA_USAGE_HINT_FRAME:
        return MUGFX_BUFFER_USAGE_HINT_DYNAMIC;
    case MUGFX_UNIFORM_DATA_USAGE_HINT_DRAW:
        return MUGFX_BUFFER_USAGE_HINT_STREAM;
    default:
        return MUGFX_BUFFER_USAGE_HINT_DEFAULT;
    }
}

EXPORT mugfx_uniform_data_id mugfx_uniform_data_create(mugfx_uniform_data_create_params params)
{
    default_init(params);

    if (!params.size) {
        log_error("Uniform data size must be greater zero");
        return { 0 };
    }

    // TODO: Be much smarter about this
    const auto buffer_id = mugfx_buffer_create({
        .target = MUGFX_BUFFER_TARGET_UNIFORM,
        .usage = get_buffer_usage(params.usage_hint),
        .data = { .data = params.cpu_buffer, .length = params.size },
    });
    const mugfx_range buffer_range = { .offset = 0, .length = params.size };

    auto buffer = get_pool<Buffer>().get(buffer_id.id);
    if (!buffer) {
        log_error("Buffer ID %u does not exist", buffer_id.id);
        return { 0 };
    }

    std::byte* cpu_buffer = reinterpret_cast<std::byte*>(params.cpu_buffer);
    if (!cpu_buffer) {
        cpu_buffer = new std::byte[params.size];
        std::memset(cpu_buffer, 0, params.size);
    }

    UniformData ub {
        .usage = params.usage_hint,
        .buffer = buffer_id,
        .buffer_range = buffer_range,
        .cpu_buffer = cpu_buffer,
        .cpu_buffer_owned = params.cpu_buffer != nullptr,
    };

    const auto key = get_pool<UniformData>().insert(std::move(ub));
    return { key };
}

EXPORT void* mugfx_uniform_data_get_ptr(mugfx_uniform_data_id uniform_data_id)
{
    const auto udata = get_pool<UniformData>().get(uniform_data_id.id);
    if (!udata) {
        log_error("Uniform data ID %u does not exist", uniform_data_id.id);
        return nullptr;
    }
    udata->dirty = true;
    return udata->cpu_buffer;
}

EXPORT void mugfx_uniform_data_update(mugfx_uniform_data_id uniform_data_id)
{
    const auto udata = get_pool<UniformData>().get(uniform_data_id.id);
    if (!udata) {
        log_error("Uniform data ID %u does not exist", uniform_data_id.id);
        return;
    }
    udata->dirty = true;
}

EXPORT void mugfx_uniform_data_destroy(mugfx_uniform_data_id uniform_data_id)
{
    const auto udata = get_pool<UniformData>().get(uniform_data_id.id);
    if (!udata) {
        log_error("Uniform data ID %u does not exist", uniform_data_id.id);
        return;
    }
    if (udata->cpu_buffer_owned) {
        delete[] udata->cpu_buffer;
    }
    get_pool<UniformData>().remove(uniform_data_id.id);
}

static bool update_uniform_data(UniformData* udata, Buffer* buffer)
{
    // TODO: Make this faster
    if (!bind_buffer(GL_UNIFORM_BUFFER, buffer->buffer)) {
        return false;
    }
    glBufferSubData(GL_UNIFORM_BUFFER, udata->buffer_range.offset, udata->buffer_range.length,
        udata->cpu_buffer);
    if (const auto error = glGetError()) {
        log_error("Error in glBufferSubData: %s", gl_error_string(error));
        return false;
    }
    return true;
}

EXPORT mugfx_geometry_id mugfx_geometry_create(mugfx_geometry_create_params params)
{
    default_init(params);

    const auto draw_mode = gl_draw_mode(params.draw_mode);
    if (!draw_mode) {
        log_error("Invalid draw mode: %d", params.draw_mode);
        return { 0 };
    }

    Geometry geom {
        .draw_mode = *draw_mode,
        .vao = 0,
        .index_type = 0,
        .vertex_count = static_cast<GLsizei>(params.vertex_count),
        .index_count = static_cast<GLsizei>(params.index_count),
    };

    struct Attribute {
        GLuint location;
        GLint components;
        GLenum type;
        GLboolean normalized;
        size_t offset;
    };

    struct VertexBufferFormat {
        std::array<Attribute, MUGFX_MAX_VERTEX_ATTRIBUTES> attrs = {};
        size_t buffer_offset = 0;
        GLsizei stride = 0;
    };

    std::array<const Buffer*, MUGFX_MAX_VERTEX_BUFFERS> vbufs = {};
    std::array<VertexBufferFormat, MUGFX_MAX_VERTEX_BUFFERS> vfmt = {};
    for (size_t b = 0; b < MUGFX_MAX_VERTEX_BUFFERS; ++b) {
        const auto& buf = params.vertex_buffers[b];
        if (!buf.buffer.id) {
            break;
        }
        vbufs[b] = get_pool<Buffer>().get(buf.buffer.id);
        if (!vbufs[b]) {
            log_error("Vertex buffer ID %u does not exist", buf.buffer.id);
            return { 0 };
        }
        vfmt[b].buffer_offset = buf.buffer_offset;
        size_t offset = 0;
        for (size_t a = 0; a < MUGFX_MAX_VERTEX_ATTRIBUTES; ++a) {
            const auto& in_attr = buf.attributes[a];
            if (!in_attr.type) {
                break;
            }

            if (in_attr.components < 1 || in_attr.components > 4) {
                log_error("Vertex attribute components must be in [1, 4]");
                return { 0 };
            }
            if ((in_attr.type == MUGFX_VERTEX_ATTRIBUTE_TYPE_I10_10_10_2_NORM
                    || in_attr.type == MUGFX_VERTEX_ATTRIBUTE_TYPE_U10_10_10_2_NORM)
                && in_attr.components != 4) {
                log_error(
                    "Components must be 4 for vertex attribute of type U10_10_10_2 or I10_10_10_2");
                return { 0 };
            }
            const auto type = gl_vertex_attribute_type(in_attr.type);
            if (!type) {
                log_error("Invalid vertex attribute type: %d", in_attr.type);
                return { 0 };
            }
            auto& attr = vfmt[b].attrs[a];
            attr.location = static_cast<GLuint>(in_attr.location);
            attr.components = static_cast<GLint>(in_attr.components);
            attr.type = type->type;
            attr.normalized = type->normalized;
            attr.offset = in_attr.offset ? in_attr.offset : offset;
            offset = attr.offset + get_attribute_size(in_attr.type, in_attr.components);
        }
        vfmt[b].stride = buf.stride ? buf.stride : offset;

        // TODO: Check divisability
        const auto vertex_count = static_cast<GLsizei>(vbufs[b]->size / vfmt[b].stride);
        if (!geom.vertex_count) {
            geom.vertex_count = vertex_count;
        }
        if (geom.vertex_count > vertex_count) {
            log_error(
                "Geometry vertex_count (%u) exceeds vertex count of buffer with index %lu (%u)",
                geom.vertex_count, b, vertex_count);
            return { 0 };
        }
    }

    const Buffer* ibuf = nullptr;
    if (params.index_buffer.id) {
        const auto index_type = gl_index_type(params.index_type);
        if (!index_type) {
            log_error("Invalid index type: %d", params.index_type);
            return { 0 };
        }
        geom.index_type = *index_type;

        ibuf = get_pool<Buffer>().get(params.index_buffer.id);
        if (!ibuf) {
            log_error("Index buffer ID %u does not exist", params.index_buffer.id);
            return { 0 };
        }

        const auto index_size = get_index_size(geom.index_type);
        assert(index_size);

        const auto index_count = static_cast<GLsizei>(ibuf->size / *index_size);
        if (!geom.index_count) {
            geom.index_count = index_count;
        }
        if (geom.index_count > index_count) {
            log_error("Geometry index count (%u) exceeds size of index buffer (%u)",
                geom.index_count, index_count);
            return { 0 };
        }
    }

    bind_buffer(GL_ARRAY_BUFFER, 0);
    bind_buffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Errors: n is negative
    glGenVertexArrays(1, &geom.vao);

    // Errors: invalid vao
    glBindVertexArray(geom.vao);

    for (size_t b = 0; b < MUGFX_MAX_VERTEX_BUFFERS; ++b) {
        if (!vbufs[b]) {
            break;
        }
        if (!bind_buffer(GL_ARRAY_BUFFER, vbufs[b]->buffer)) {
            glDeleteVertexArrays(1, &geom.vao);
            return { 0 };
        }

        for (size_t a = 0; a < MUGFX_MAX_VERTEX_ATTRIBUTES; ++a) {
            if (!params.vertex_buffers[b].attributes[a].type) {
                break;
            }

            const auto& attr = vfmt[b].attrs[a];
            glEnableVertexAttribArray(attr.location);
            if (const auto error = glGetError()) {
                log_error("Error in glEnableVertexAttribArray: %s", gl_error_string(error));
                glDeleteVertexArrays(1, &geom.vao);
                return { 0 };
            }
            const auto offset
                = reinterpret_cast<const GLvoid*>(vfmt[b].buffer_offset + attr.offset);
            glVertexAttribPointer(
                attr.location, attr.components, attr.type, attr.normalized, vfmt[b].stride, offset);
            if (const auto error = glGetError()) {
                log_error("Error in glVertexAttribPointer(%d, %d, %d, %d, %d, %p): %s",
                    attr.location, attr.components, attr.type, attr.normalized, vfmt[b].stride,
                    offset, gl_error_string(error));
                glDeleteVertexArrays(1, &geom.vao);
                return { 0 };
            }
        }
    }

    if (ibuf) {
        assert(geom.index_type);
        if (!bind_buffer(GL_ELEMENT_ARRAY_BUFFER, ibuf->buffer)) {
            glDeleteVertexArrays(1, &geom.vao);
            return { 0 };
        }
    }

    glBindVertexArray(0);

    const auto key = get_pool<Geometry>().insert(std::move(geom));
    return { key };
}

EXPORT void mugfx_geometry_destroy(mugfx_geometry_id geometry)
{
    const auto geom = get_pool<Geometry>().get(geometry.id);
    if (!geom) {
        log_error("Geometry ID %u does not exist", geometry.id);
        return;
    }

    glDeleteVertexArrays(1, &geom->vao);
    if (const auto error = glGetError()) {
        log_error("Error in glDeleteVertexArrays: %s", gl_error_string(error));
    }

    get_pool<Geometry>().remove(geometry.id);
}

EXPORT mugfx_render_target_id mugfx_render_target_create(mugfx_render_target_create_params params)
{
    return { 0 };
}

EXPORT void mugfx_render_target_blit_to_render_target(
    mugfx_render_target_id src_target, mugfx_render_target_id dst_target)
{
}

EXPORT void mugfx_render_target_blit_to_texture(
    mugfx_render_target_id src_target, mugfx_texture_id dst_texture)
{
}

EXPORT void mugfx_render_target_destroy(mugfx_render_target_id target) { }

EXPORT void mugfx_render_target_bind(mugfx_render_target_id target) { }

EXPORT void mugfx_set_viewport(int x, int y, size_t width, size_t height)
{
    glViewport(x, y, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
}

EXPORT void mugfx_set_scissor(int x, int y, size_t width, size_t height) { }

EXPORT void mugfx_begin_frame() { }

EXPORT void mugfx_begin_pass(mugfx_render_target_id target)
{
    auto& pass = get_current_pass();
    if (pass.in_pass) {
        log_error("Cannot begin another pass");
        return;
    }
    pass.in_pass = true;
    pass.target = target;
}

void mugfx_clear(mugfx_clear_mask mask, mugfx_clear_values values)
{
    GLenum gl_mask = 0;
    if (mask & MUGFX_CLEAR_COLOR) {
        glClearColor(values.color[0], values.color[1], values.color[2], values.color[3]);
        gl_mask |= GL_COLOR_BUFFER_BIT;
    }
    if (mask & MUGFX_CLEAR_DEPTH) {
        glClearDepthf(values.depth);
        gl_mask |= GL_DEPTH_BUFFER_BIT;
    }
    if (mask & MUGFX_CLEAR_STENCIL) {
        glClearStencil(values.stencil);
        gl_mask |= GL_STENCIL_BUFFER_BIT;
    }
    glClear(gl_mask);
}

EXPORT void mugfx_draw(mugfx_material_id material, mugfx_geometry_id geometry,
    mugfx_draw_binding* bindings, size_t num_bindings)
{
    if (!get_current_pass().in_pass) {
        log_error("Cannot draw outside a pass");
        return;
    }

    const auto mat = get_pool<Material>().get(material.id);
    if (!mat) {
        log_error("Material ID %u does not exist", material.id);
        return;
    }

    const auto geom = get_pool<Geometry>().get(geometry.id);
    if (!geom) {
        log_error("Geometry ID %u does not exist", geometry.id);
        return;
    }

    if (!bind_shader(mat->shader_program)) {
        return;
    }

    for (size_t i = 0; i < num_bindings; ++i) {
        if (bindings[i].type == MUGFX_BINDING_TYPE_UNIFORM_DATA) {
            const auto udata = get_pool<UniformData>().get(bindings[i].uniform_data.id.id);
            if (!udata) {
                log_error("Uniform data ID %u does not exist", bindings[i].uniform_data.id.id);
                return;
            }

            const auto buffer = get_pool<Buffer>().get(udata->buffer.id);
            assert(buffer);
            if (!buffer) {
                log_error("Buffer ID %u does not exist", udata->buffer.id);
                return;
            }

            if (udata->dirty) {
                if (!update_uniform_data(udata, buffer)) {
                    return;
                }
            }

            if (!bind_buffer(buffer->target, buffer->buffer, bindings[i].uniform_data.binding,
                    udata->buffer_range)) {
                return;
            }
        } else if (bindings[i].type == MUGFX_BINDING_TYPE_TEXTURE) {
            const auto tex = get_pool<Texture>().get(bindings[i].texture.id.id);
            if (!tex) {
                log_error("Texture ID %u does not exist", bindings[i].texture.id.id);
                return;
            }
            if (!bind_texture(bindings[i].texture.binding, tex->target, tex->texture)) {
                return;
            }
        } else if (bindings[i].type == MUGFX_BINDING_TYPE_BUFFER) {
            const auto buffer = get_pool<Buffer>().get(bindings[i].buffer.id.id);
            if (!buffer) {
                log_error("Buffer ID %u does not exist", bindings[i].buffer.id.id);
                return;
            }
            if (!bind_buffer(buffer->target, buffer->buffer, bindings[i].uniform_data.binding,
                    bindings[i].buffer.range)) {
                return;
            }
        }
    }

    if (!bind_vao(geom->vao)) {
        return;
    }
    if (geom->index_type) {
        glDrawElements(geom->draw_mode, geom->index_count, geom->index_type, 0);
    } else {
        glDrawArrays(geom->draw_mode, 0, geom->vertex_count);
    }
    bind_vao(0);
}

EXPORT void mugfx_draw_instanced(mugfx_material_id material, mugfx_geometry_id geometry,
    mugfx_draw_binding* bindings, size_t num_bindings, size_t instance_count)
{
    log_error("mugfx_draw_instanced not implemented yet");
}

EXPORT void mugfx_flush() { }

EXPORT void mugfx_end_pass()
{
    auto& pass = get_current_pass();
    if (!pass.in_pass) {
        log_error("Cannot end a pass outside a pass");
        return;
    }
    pass.in_pass = false;
    mugfx_flush();
}

EXPORT void mugfx_end_frame()
{
    if (get_current_pass().in_pass) {
        log_error("Cannot end frame in a pass");
        return;
    }
    mugfx_flush();
}
