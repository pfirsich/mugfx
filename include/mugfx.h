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
    MUGFX_MAX_SHADER_UNIFORM_DESCRIPTORS = 8,
    MUGFX_MAX_SHADER_SAMPLERS = 16,
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
} mugfx_init_params;

void mugfx_init(mugfx_init_params params);
void mugfx_shutdown(); // Doesn't actually free all resources, but must be called

typedef struct {
    const void* data;
    size_t length;
} mugfx_slice;

typedef struct {
    size_t offset;
    size_t length;
} mugfx_range;

// Uniform Descriptor
typedef enum {
    MUGFX_UNIFORMS_SCOPE_DEFAULT = 0,
    MUGFX_UNIFORMS_SCOPE_CONSTANT,
    MUGFX_UNIFORMS_SCOPE_FRAME,
    MUGFX_UNIFORMS_SCOPE_DRAW,
} mugfx_uniforms_scope;

// For OpenGL std140 is used, for which you should prefer to use vec4 and mat4 only.
// vec3 will be stored as a vec4, mat2 as vec4[2] and mat3 as vec4[3], so they will work, but are
// wasteful.
typedef enum {
    MUGFX_UNIFORM_TYPE_DEFAULT = 0,
    MUGFX_UNIFORM_TYPE_FLOAT,
    MUGFX_UNIFORM_TYPE_VEC2,
    MUGFX_UNIFORM_TYPE_VEC3,
    MUGFX_UNIFORM_TYPE_VEC4,
    MUGFX_UNIFORM_TYPE_INT,
    MUGFX_UNIFORM_TYPE_IVEC2,
    MUGFX_UNIFORM_TYPE_IVEC3,
    MUGFX_UNIFORM_TYPE_IVEC4,
    MUGFX_UNIFORM_TYPE_UINT,
    MUGFX_UNIFORM_TYPE_UVEC2,
    MUGFX_UNIFORM_TYPE_UVEC3,
    MUGFX_UNIFORM_TYPE_UVEC4,
    MUGFX_UNIFORM_TYPE_MAT2,
    MUGFX_UNIFORM_TYPE_MAT3,
    MUGFX_UNIFORM_TYPE_MAT4,
    MUGFX_UNIFORM_TYPE_MAT2X3,
    MUGFX_UNIFORM_TYPE_MAT3X2,
    MUGFX_UNIFORM_TYPE_MAT2X4,
    MUGFX_UNIFORM_TYPE_MAT4X2,
    MUGFX_UNIFORM_TYPE_MAT3X4,
    MUGFX_UNIFORM_TYPE_MAT4X3,
} mugfx_uniform_type;

typedef struct {
    const char* name;
    mugfx_uniform_type type;
    size_t array_size;
    size_t offset;
} mugfx_uniform;

typedef struct {
    mugfx_uniforms_scope usage_hint; // default: FRAME
    mugfx_uniform uniforms[MUGFX_MAX_UNIFORMS];
    size_t size;
    uint32_t binding; // optional for OpenGL backends, mandatory for Vulkan
} mugfx_uniform_descriptor;

void mugfx_uniform_descriptor_calculate_layout(mugfx_uniform_descriptor* uniform_descriptor);

// Shader
typedef struct {
    uint32_t id;
} mugfx_shader_id;

typedef enum {
    MUGFX_SHADER_STAGE_DEFAULT = 0,
    MUGFX_SHADER_STAGE_VERTEX,
    MUGFX_SHADER_STAGE_FRAGMENT,
} mugfx_shader_stage;

typedef struct {
    const char* name;
    // This is the texture unit in OpenGL (like the binding layout qualifier) or the actual binding
    // number in Vulkan
    uint32_t binding;
} mugfx_shader_sampler;

typedef struct {
    mugfx_shader_stage stage;
    const char* source;
    const mugfx_uniform_descriptor* uniform_descriptors[MUGFX_MAX_SHADER_UNIFORM_DESCRIPTORS];
    mugfx_shader_sampler samplers[MUGFX_MAX_SHADER_SAMPLERS];
} mugfx_shader_create_params;

mugfx_shader_id mugfx_shader_create(mugfx_shader_create_params params);
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

typedef struct {
    // The descriptor pointer has to be the same as the one passed to mugfx_shader_create.
    const mugfx_uniform_descriptor* descriptor;
    mugfx_buffer_id buffer; // optional
    mugfx_range buffer_range; // optional
} mugfx_uniform_data_create_params;

// This is essentially a local buffer and a dirty flag, plus a reference to a pool of gpu uniform
// buffers (or a slice of one), also a copy of the uniform descriptor.
mugfx_uniform_data_id mugfx_uniform_data_create(mugfx_uniform_data_create_params params);
void mugfx_uniform_data_set_float(
    mugfx_uniform_data_id uniform_data, const char* name, mugfx_slice data);
void mugfx_uniform_data_set_texture(
    mugfx_uniform_data_id uniforms, const char* name, mugfx_texture_id texture);
void mugfx_uniform_data_destroy(mugfx_uniform_data_id uniforms);

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

mugfx_render_target_id mugfx_render_target_create(mugfx_render_target_create_params params);
void mugfx_render_target_blit_to_render_target(
    mugfx_render_target_id src_target, mugfx_render_target_id dst_target);
void mugfx_render_target_blit_to_texture(
    mugfx_render_target_id src_target, mugfx_texture_id dst_texture);
void mugfx_render_target_destroy(mugfx_render_target_id target);
void mugfx_render_target_bind(mugfx_render_target_id target);

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
            mugfx_uniform_data_id id;
        } uniform_data;

        struct {
            uint32_t binding;
            mugfx_buffer_id id;
        } buffer;

        struct {
            uint32_t binding;
            mugfx_texture_id id;
        } texture;
    };
} mugfx_draw_binding;

void mugfx_begin_frame();
void mugfx_draw(mugfx_material_id material, mugfx_geometry_id geometry,
    mugfx_draw_binding* bindings, size_t num_bindings);
void mugfx_draw_instanced(mugfx_material_id material, mugfx_geometry_id geometry,
    mugfx_draw_binding* bindings, size_t num_bindings, size_t instance_count);
void mugfx_flush();
void mugfx_end_frame();

#ifdef __cplusplus
}
#endif