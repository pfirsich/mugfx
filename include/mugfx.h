#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// General
enum {
    MUGFX_MAX_VERTEX_BUFFERS = 8,
    MUGFX_MAX_VERTEX_ATTRIBUTES = 8,
    MUGFX_MAX_UNIFORMS = 32,
    MUGFX_MAX_MATERIAL_UNIFORMS = 8,
    MUGFX_MAX_COLOR_FORMATS = 8,
    MUGFX_MAX_SHADER_BINDINGS = 16,
};

typedef enum {
    MUGFX_SEVERITY_DEFAULT = 0,
    MUGFX_SEVERITY_DEBUG,
    MUGFX_SEVERITY_INFO,
    MUGFX_SEVERITY_WARN,
    MUGFX_SEVERITY_ERROR,
} mugfx_severity;

typedef void (*mugfx_logging_callback)(mugfx_severity severity, const char* message);
const char* mugfx_severity_to_string(mugfx_severity severity);

typedef void (*mugfx_panic_handler)(const char* msg);

// https://nullprogram.com/blog/2023/12/17/
typedef void* (*mugfx_allocator_allocate)(size_t size, void* ctx);
typedef void* (*mugfx_allocator_reallocate)(void* ptr, size_t old_size, size_t new_size, void* ctx);
typedef void (*mugfx_allocator_deallocate)(void* ptr, size_t size, void* ctx);

typedef struct {
    mugfx_allocator_allocate allocate;
    mugfx_allocator_reallocate reallocate;
    mugfx_allocator_deallocate deallocate;
    void* ctx;
} mugfx_allocator;

typedef struct {
    mugfx_logging_callback logging_callback;
    mugfx_panic_handler panic_handler; // if set, mugfx will panic on error
    mugfx_allocator* allocator; // default will use malloc
    size_t max_num_shaders; // default: 64
    size_t max_num_textures; // default: 128
    size_t max_num_uniforms; // default: 1024
    size_t max_num_buffers; // default: 1024
    size_t max_num_materials; // default: 512
    size_t max_num_geometries; // default: 1024
    size_t max_num_render_targets; // default: 32
    size_t max_num_pipelines; // default: 1024
#ifdef MUGFX_OPENGL
    // We don't need anything here. Just create a context and make it current
#elif MUGFX_VULKAN
    const void* device; // VkDevice
    const void* swapchain; // VkSwapchainKHR
#endif
    bool debug; // do error checking and panic if something is wrong
} mugfx_init_params;

void mugfx_init(mugfx_init_params params);

// Doesn't actually free all memory but destroys all resources and must be called
void mugfx_shutdown();

// The strings returned here are owned by mugfx
const char* mugfx_get_renderer_name();
const char* mugfx_get_vendor_name();
const char* mugfx_get_api_name(); // "OpenGL", "WebGL" or "Vulkan"
const char* mugfx_get_api_version();

typedef struct {
    const void* data;
    size_t length;
} mugfx_slice;

#define MUGFX_SLICE_OBJ(obj)                                                                       \
    (mugfx_slice)                                                                                  \
    {                                                                                              \
        .data = &obj, .length = sizeof(obj)                                                        \
    }

typedef struct {
    size_t offset;
    size_t length;
} mugfx_range;

// Shader
typedef struct {
    uint32_t id;
} mugfx_shader_id;

typedef enum {
    MUGFX_SHADER_STAGE_DEFAULT = 0,
    MUGFX_SHADER_STAGE_VERTEX,
    MUGFX_SHADER_STAGE_FRAGMENT,
} mugfx_shader_stage;

typedef enum {
    MUGFX_SHADER_BINDING_TYPE_NONE = 0,
    MUGFX_SHADER_BINDING_TYPE_UNIFORM,
    MUGFX_SHADER_BINDING_TYPE_SAMPLER,
} mugfx_shader_binding_type;

typedef struct {
    mugfx_shader_binding_type type;
    // OpenGL:For a sampler, binding is the texture unit (equal to the binding layout qualifier in
    // the shader). The binding number is ignored if glUniform is used.
    // Vulkan: the actual binding number. The set number is derived automatically.
    uint32_t binding;
} mugfx_shader_binding;

typedef struct {
    mugfx_shader_stage stage;
    const char* source; // GLSL or SPIR-V
    mugfx_shader_binding bindings[MUGFX_MAX_SHADER_BINDINGS];
} mugfx_shader_create_params;

mugfx_shader_id mugfx_shader_create(mugfx_shader_create_params params);
mugfx_shader_binding mugfx_shader_get_binding(mugfx_shader_id shader, size_t idx);
void mugfx_shader_destroy(mugfx_shader_id shader);

// Texture
typedef struct {
    uint32_t id;
} mugfx_texture_id;

typedef enum {
    MUGFX_PIXEL_FORMAT_DEFAULT = 0,
    MUGFX_PIXEL_FORMAT_RGB8,
    MUGFX_PIXEL_FORMAT_RGBA8,
    MUGFX_PIXEL_FORMAT_RGB16F,
    MUGFX_PIXEL_FORMAT_RGBA16F,
    MUGFX_PIXEL_FORMAT_RGB32F,
    MUGFX_PIXEL_FORMAT_RGBA32F,
    MUGFX_PIXEL_FORMAT_DEPTH24,
    MUGFX_PIXEL_FORMAT_DEPTH32F,
    MUGFX_PIXEL_FORMAT_DEPTH24_STENCIL8,
} mugfx_pixel_format;

typedef enum {
    MUGFX_TEXTURE_WRAP_DEFAULT = 0,
    MUGFX_TEXTURE_WRAP_REPEAT,
    MUGFX_TEXTURE_WRAP_CLAMP_TO_EDGE,
    MUGFX_TEXTURE_WRAP_MIRRORED_REPEAT,
} mugfx_texture_wrap_mode;

typedef enum {
    MUGFX_TEXTURE_MIN_FILTER_DEFAULT = 0,
    MUGFX_TEXTURE_MIN_FILTER_NEAREST,
    MUGFX_TEXTURE_MIN_FILTER_LINEAR,
    MUGFX_TEXTURE_MIN_FILTER_NEAREST_MIPMAP_NEAREST,
    MUGFX_TEXTURE_MIN_FILTER_LINEAR_MIPMAP_NEAREST,
    MUGFX_TEXTURE_MIN_FILTER_NEAREST_MIPMAP_LINEAR,
    MUGFX_TEXTURE_MIN_FILTER_LINEAR_MIPMAP_LINEAR,
} mugfx_texture_min_filter;

typedef enum {
    MUGFX_TEXTURE_MAG_FILTER_DEFAULT = 0,
    MUGFX_TEXTURE_MAG_FILTER_NEAREST,
    MUGFX_TEXTURE_MAG_FILTER_LINEAR,
} mugfx_texture_mag_filter;

typedef struct {
    size_t width;
    size_t height;
    // int depth; // default: 0 (2D texture) // TODO: 3D Textures, Cubemaps
    mugfx_pixel_format format; // default: RGBA8
    mugfx_texture_wrap_mode wrap_s; // default: REPEAT
    mugfx_texture_wrap_mode wrap_t; // default: wrap_s
    // mugfx_texture_wrap_mode wrap_r; // default: wrap_s
    mugfx_texture_min_filter min_filter; // default: mipmaps ? linear : linear_mipmap_linear
    mugfx_texture_mag_filter mag_filter; // default: linear
    bool generate_mipmaps;
    mugfx_slice data; // maybe optionally be set at creation
    mugfx_pixel_format data_format; // default: format
} mugfx_texture_create_params;

mugfx_texture_id mugfx_texture_create(mugfx_texture_create_params params);
void mugfx_texture_set_data(
    mugfx_texture_id texture, mugfx_slice data, mugfx_pixel_format data_format);
void mugfx_texture_destroy(mugfx_texture_id texture);

// Material (maps to Pipeline in Vulkan)
typedef struct {
    uint32_t id;
} mugfx_material_id;

typedef enum {
    MUGFX_DEPTH_FUNC_DEFAULT = 0,
    MUGFX_DEPTH_FUNC_NEVER,
    MUGFX_DEPTH_FUNC_LESS,
    MUGFX_DEPTH_FUNC_EQUAL,
    MUGFX_DEPTH_FUNC_LEQUAL,
    MUGFX_DEPTH_FUNC_GREATER,
    MUGFX_DEPTH_FUNC_NOT_EQUAL,
    MUGFX_DEPTH_FUNC_GEQUAL,
    MUGFX_DEPTH_FUNC_ALWAYS,
} mugfx_depth_func;

typedef enum {
    MUGFX_WRITE_MASK_DEFAULT = 0,
    MUGFX_WRITE_MASK_NONE = 1, // bit weird, I know
    MUGFX_WRITE_MASK_R = 2,
    MUGFX_WRITE_MASK_G = 4,
    MUGFX_WRITE_MASK_B = 8,
    MUGFX_WRITE_MASK_A = 16,
    MUGFX_WRITE_MASK_RGBA = 30,
    MUGFX_WRITE_MASK_DEPTH = 32,
} mugfx_write_mask;

typedef enum {
    MUGFX_CULL_FACE_MODE_DEFAULT = 0,
    MUGFX_CULL_FACE_MODE_NONE,
    MUGFX_CULL_FACE_MODE_FRONT,
    MUGFX_CULL_FACE_MODE_BACK,
    MUGFX_CULL_FACE_MODE_FRONT_AND_BACK,
} mugfx_cull_face_mode;

typedef enum {
    MUGFX_BLEND_FUNC_DEFAULT = 0,
    MUGFX_BLEND_FUNC_ZERO,
    MUGFX_BLEND_FUNC_ONE,
    MUGFX_BLEND_FUNC_SRC_COLOR,
    MUGFX_BLEND_FUNC_ONE_MINUS_SRC_COLOR,
    MUGFX_BLEND_FUNC_DST_COLOR,
    MUGFX_BLEND_FUNC_ONE_MINUS_DST_COLOR,
    MUGFX_BLEND_FUNC_SRC_ALPHA,
    MUGFX_BLEND_FUNC_ONE_MINUS_SRC_ALPHA,
    MUGFX_BLEND_FUNC_DST_ALPHA,
    MUGFX_BLEND_FUNC_ONE_MINUS_DST_ALPHA,
} mugfx_blend_func;

typedef enum {
    MUGFX_POLYGON_MODE_DEFAULT = 0,
    MUGFX_POLYGON_MODE_FILL,
    MUGFX_POLYGON_MODE_LINE,
    MUGFX_POLYGON_MODE_POINT,
} mugfx_polygon_mode;

typedef enum {
    MUGFX_STENCIL_FUNC_DEFAULT = 0,
    MUGFX_STENCIL_FUNC_NEVER,
    MUGFX_STENCIL_FUNC_LESS,
    MUGFX_STENCIL_FUNC_EQUAL,
    MUGFX_STENCIL_FUNC_LEQUAL,
    MUGFX_STENCIL_FUNC_GREATER,
    MUGFX_STENCIL_FUNC_NOT_EQUAL,
    MUGFX_STENCIL_FUNC_GEQUAL,
    MUGFX_STENCIL_FUNC_ALWAYS,
} mugfx_stencil_func;

typedef struct {
    mugfx_shader_id vert_shader;
    mugfx_shader_id frag_shader;
    mugfx_depth_func depth_func; // default: LEQUAL
    mugfx_write_mask write_mask; // default: RGBA | DEPTH
    mugfx_cull_face_mode cull_face; // default: NONE
    mugfx_blend_func src_blend; // default: ONE
    mugfx_blend_func dst_blend; // default: ZERO
    float blend_color[4];
    mugfx_polygon_mode polygon_mode; // default: FILL
    bool stencil_enable;
    mugfx_stencil_func stencil_func; // default: ALWAYS
    int stencil_ref;
    uint32_t stencil_mask;
} mugfx_material_create_params;

mugfx_material_id mugfx_material_create(mugfx_material_create_params params);
void mugfx_material_destroy(mugfx_material_id material);

// Graphics Buffer
typedef struct {
    uint32_t id;
} mugfx_buffer_id;

typedef enum {
    MUGFX_BUFFER_TARGET_DEFAULT = 0,
    MUGFX_BUFFER_TARGET_ARRAY,
    MUGFX_BUFFER_TARGET_INDEX,
    MUGFX_BUFFER_TARGET_UNIFORM,
} mugfx_buffer_target;

typedef enum {
    MUGFX_BUFFER_USAGE_HINT_DEFAULT = 0,
    MUGFX_BUFFER_USAGE_HINT_STATIC,
    MUGFX_BUFFER_USAGE_HINT_DYNAMIC,
    MUGFX_BUFFER_USAGE_HINT_STREAM,
} mugfx_buffer_usage_hint;

typedef struct {
    mugfx_buffer_target target; // default: ARRAY
    mugfx_buffer_usage_hint usage; // default: STATIC
    mugfx_slice data; // optional initial data, length must be set
} mugfx_buffer_create_params;

mugfx_buffer_id mugfx_buffer_create(mugfx_buffer_create_params params);
// You can pass NULL for data to orphan a buffer (offset and data.length will be ignored).
void mugfx_buffer_update(mugfx_buffer_id buf, size_t offset, mugfx_slice data);
void mugfx_buffer_destroy(mugfx_buffer_id buf);

// Uniform Data
typedef struct {
    uint32_t id;
} mugfx_uniform_data_id;

// This gives a hint on what this data is used for. I.e. once per frame, or once per draw
typedef enum {
    MUGFX_UNIFORM_DATA_USAGE_HINT_DEFAULT = 0,
    MUGFX_UNIFORM_DATA_USAGE_HINT_CONSTANT, // e.g. screen dimensions, camera projection, ...
    MUGFX_UNIFORM_DATA_USAGE_HINT_FRAME, // e.g. view matrix, time, lights, ...
    MUGFX_UNIFORM_DATA_USAGE_HINT_DRAW, // e.g. model matrix, skinning, some material params, ...
} mugfx_uniform_data_usage_hint;

typedef struct {
    mugfx_uniform_data_usage_hint usage_hint; // default: FRAME
    size_t size;
    // cpu_buffer must live as long as the uniform buffer itself, if given.
    // It's length must be >= size
    void* cpu_buffer; // optional, will be allocated otherwise
} mugfx_uniform_data_create_params;

// This is essentially a cpu buffer and a dirty flag, plus a reference to a pool of gpu uniform
// buffers (or a slice of one).
mugfx_uniform_data_id mugfx_uniform_data_create(mugfx_uniform_data_create_params params);
// This function marks the uniform data as dirty, assuming this pointer is used to modify it
void* mugfx_uniform_data_get_ptr(mugfx_uniform_data_id uniform_data);
// If you passed cpu_buffer, this will mark the data dirty and schedule an update of the GPU buffer
void mugfx_uniform_data_update(mugfx_uniform_data_id uniform_data);
void mugfx_uniform_data_destroy(mugfx_uniform_data_id uniform_data);

// Geometry
typedef struct {
    uint32_t id;
} mugfx_geometry_id;

typedef enum {
    MUGFX_VERTEX_ATTRIBUTE_TYPE_DEFAULT = 0,
    MUGFX_VERTEX_ATTRIBUTE_TYPE_F32,
    MUGFX_VERTEX_ATTRIBUTE_TYPE_F16,
    MUGFX_VERTEX_ATTRIBUTE_TYPE_U8_NORM,
    MUGFX_VERTEX_ATTRIBUTE_TYPE_U16_NORM,
    MUGFX_VERTEX_ATTRIBUTE_TYPE_I8_NORM,
    MUGFX_VERTEX_ATTRIBUTE_TYPE_I16_NORM,
    MUGFX_VERTEX_ATTRIBUTE_TYPE_U8,
    MUGFX_VERTEX_ATTRIBUTE_TYPE_U16,
    MUGFX_VERTEX_ATTRIBUTE_TYPE_I8,
    MUGFX_VERTEX_ATTRIBUTE_TYPE_I16,
    MUGFX_VERTEX_ATTRIBUTE_TYPE_I10_10_10_2_NORM,
    MUGFX_VERTEX_ATTRIBUTE_TYPE_U10_10_10_2_NORM,
} mugfx_vertex_attribute_type;

// Try to align each attribute to at least 4 bytes
typedef struct {
    size_t location;
    size_t components;
    mugfx_vertex_attribute_type type;
    size_t offset;
} mugfx_vertex_attribute;

typedef struct {
    mugfx_buffer_id buffer;
    size_t buffer_offset;
    size_t stride;
    mugfx_vertex_attribute attributes[MUGFX_MAX_VERTEX_ATTRIBUTES];
} mugfx_vertex_buffer;

typedef enum {
    MUGFX_DRAW_MODE_DEFAULT = 0,
    MUGFX_DRAW_MODE_TRIANGLES,
    MUGFX_DRAW_MODE_TRIANGLE_STRIP,
    MUGFX_DRAW_MODE_LINES,
    MUGFX_DRAW_MODE_LINE_STRIP,
} mugfx_draw_mode;

typedef enum {
    MUGFX_INDEX_TYPE_DEFAULT = 0,
    MUGFX_INDEX_TYPE_U8,
    MUGFX_INDEX_TYPE_U16,
    MUGFX_INDEX_TYPE_U32,
} mugfx_index_type;

typedef struct {
    mugfx_draw_mode draw_mode; // default: TRIANGLES
    mugfx_vertex_buffer vertex_buffers[MUGFX_MAX_VERTEX_BUFFERS];
    mugfx_buffer_id index_buffer; // optional
    mugfx_index_type index_type; // mandatory if index_buffer is given
    size_t index_buffer_offset;
    size_t vertex_count;
    size_t index_count;
} mugfx_geometry_create_params;

// This represents the vertex input state of the pipeline
mugfx_geometry_id mugfx_geometry_create(mugfx_geometry_create_params params);
void mugfx_geometry_destroy(mugfx_geometry_id geometry);

// Render Target
typedef struct {
    uint32_t id;
} mugfx_render_target_id;

typedef struct {
    size_t width;
    size_t height;
    mugfx_pixel_format color_formats[MUGFX_MAX_COLOR_FORMATS]; // default is {RGBA8}
    mugfx_pixel_format depth_format; // default is DEPTH24
    size_t samples;
} mugfx_render_target_create_params;

#define MUGFX_RENDER_TARGET_BACKBUFFER                                                             \
    (mugfx_render_target_id)                                                                       \
    {                                                                                              \
        0                                                                                          \
    }

mugfx_render_target_id mugfx_render_target_create(mugfx_render_target_create_params params);
void mugfx_render_target_blit_to_render_target(
    mugfx_render_target_id src_target, mugfx_render_target_id dst_target);
void mugfx_render_target_blit_to_texture(
    mugfx_render_target_id src_target, mugfx_texture_id dst_texture);
void mugfx_render_target_destroy(mugfx_render_target_id target);

// Dynamic Pipeline State
void mugfx_set_viewport(int x, int y, size_t width, size_t height);
void mugfx_set_scissor(int x, int y, size_t width, size_t height);

// Drawing
typedef enum {
    MUGFX_BINDING_TYPE_DEFAULT = 0,
    MUGFX_BINDING_TYPE_UNIFORM_DATA,
    MUGFX_BINDING_TYPE_TEXTURE,
    MUGFX_BINDING_TYPE_BUFFER,
} mugfx_binding_type;

typedef struct {
    mugfx_binding_type type;
    union {
        struct {
            uint32_t binding;
            mugfx_uniform_data_id id;
        } uniform_data;

        struct {
            uint32_t binding;
            mugfx_buffer_id id;
            mugfx_range range;
        } buffer;

        struct {
            uint32_t binding;
            mugfx_texture_id id;
        } texture;
    };
} mugfx_draw_binding;

typedef enum {
    MUGFX_CLEAR_COLOR = 1 << 0,
    MUGFX_CLEAR_DEPTH = 1 << 1,
    MUGFX_CLEAR_STENCIL = 1 << 2,
    MUGFX_CLEAR_COLOR_DEPTH = MUGFX_CLEAR_COLOR | MUGFX_CLEAR_DEPTH,
    MUGFX_CLEAR_DEPTH_STENCIL = MUGFX_CLEAR_DEPTH | MUGFX_CLEAR_STENCIL,
    MUGFX_CLEAR_ALL = MUGFX_CLEAR_COLOR | MUGFX_CLEAR_DEPTH | MUGFX_CLEAR_STENCIL
} mugfx_clear_mask;

typedef struct {
    float color[4]; // default: {0.0f, 0.0f, 0.0f, 0.0f}
    float depth; // default: 1.0f
    int stencil; // default: 0
} mugfx_clear_values;

void mugfx_begin_frame();
void mugfx_begin_pass(mugfx_render_target_id target);

void mugfx_clear(mugfx_clear_mask mask, mugfx_clear_values values);

void mugfx_draw(mugfx_material_id material, mugfx_geometry_id geometry,
    mugfx_draw_binding* bindings, size_t num_bindings);
void mugfx_draw_instanced(mugfx_material_id material, mugfx_geometry_id geometry,
    mugfx_draw_binding* bindings, size_t num_bindings, size_t instance_count);

void mugfx_flush();
void mugfx_end_pass(); // flushes
void mugfx_end_frame(); // flushes

#ifdef __cplusplus
}
#endif
