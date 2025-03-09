#include <array>
#include <cstdio>
#include <cstdlib>

#include <mugfx.h>

#include "window.hpp"

const auto vert_source = R"(
    #version 420 core

    layout (binding = 0, std140) uniform UConstant {
        mat4 projection;
    };

    layout (location = 0) in vec3 a_position;
    layout (location = 1) in vec2 a_texcoord;

    out VsOut {
        vec2 texcoord;
    } vs_out;

    void main() {
        vs_out.texcoord = a_texcoord;
        gl_Position = projection * vec4(a_position, 1.0);
    }
)";

struct UConstant {
    std::array<float, 16> projection;
};

const auto frag_source = R"(
    #version 420 core

    layout(binding = 0) uniform sampler2D u_base;

    in VsOut {
        vec2 texcoord;
    } vs_out;

    out vec4 frag_color;

    void main() {
        frag_color = texture(u_base, vs_out.texcoord);
    }
)";

void logger(mugfx_severity severity, const char* msg)
{
    std::printf("[%s] %s\n", mugfx_severity_to_string(severity), msg);
}

void panic_handler(const char* msg)
{
    logger(MUGFX_SEVERITY_ERROR, msg);
    std::abort();
}

int main()
{
    auto window = Window::create("Hello Triangle", 1024, 768);

    mugfx_init({
        .logging_callback = logger,
        .panic_handler = panic_handler,
        .debug = true,
    });

    const auto vert_shader = mugfx_shader_create({
        .stage = MUGFX_SHADER_STAGE_VERTEX,
        .source = vert_source,
        .bindings = {
            { .type = MUGFX_SHADER_BINDING_TYPE_UNIFORM, .binding = 0 },
        },
    });

    const auto frag_shader = mugfx_shader_create({
        .stage = MUGFX_SHADER_STAGE_FRAGMENT,
        .source = frag_source,
        .bindings = {
            { .type = MUGFX_SHADER_BINDING_TYPE_SAMPLER, .binding = 0 },
        },
    });

    // 4 pixels: white, red, green, blue
    const std::array<uint32_t, 4> texture_data
        = { 0xFF'FF'FF'FFul, 0x00'00'FF'FFul, 0x00'FF'00'FFul, 0xFF'00'00'FFul };
    const auto texture = mugfx_texture_create({
        .width = 2,
        .height = 2,
        .data = { texture_data.data(), texture_data.size() * sizeof(texture_data[0]) },
    });

    const auto material = mugfx_material_create({
        .vert_shader = vert_shader,
        .frag_shader = frag_shader,
    });

    struct Vertex {
        float position[3];
        uint16_t texcoord[2];
    };

    std::array<Vertex, 3> vertices = {
        Vertex { { -1.0f, -1.0f, 0.0f }, { 0x0000, 0x0000 } },
        Vertex { { 1.0f, -1.0f, 0.0f }, { 0xffff, 0x0000 } },
        Vertex { { 1.0f, 1.0f, 0.0f }, { 0xffff, 0xffff } },
    };
    const auto vertex_buffer = mugfx_buffer_create({
        .target = MUGFX_BUFFER_TARGET_ARRAY,
        .data = { vertices.data(), vertices.size() * sizeof(Vertex) },
    });

    std::array<uint16_t, 3> indices = { 0, 1, 2 };
    const auto index_buffer = mugfx_buffer_create({
        .target = MUGFX_BUFFER_TARGET_INDEX,
        .data = { indices.data(), indices.size() * sizeof(indices[0]) },
    });

    const auto geometry = mugfx_geometry_create({
        .vertex_buffers = {
            {
                .buffer = vertex_buffer,
                .attributes = {
                    {.location = 0, .components = 3, .type = MUGFX_VERTEX_ATTRIBUTE_TYPE_F32}, // position
                    {.location = 1, .components = 2, .type = MUGFX_VERTEX_ATTRIBUTE_TYPE_U16_NORM}, // texcoord
                },
            },
        },
        .index_buffer = index_buffer,
        .index_type = MUGFX_INDEX_TYPE_U16,
    });

    const auto vs_uniform_data = mugfx_uniform_data_create({ .size = sizeof(UConstant) });
    auto ubuf = (UConstant*)mugfx_uniform_data_get_ptr(vs_uniform_data);
    ubuf->projection = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f };

    std::array<mugfx_draw_binding, 2> bindings {
        mugfx_draw_binding {
            .type = MUGFX_BINDING_TYPE_UNIFORM_DATA,
            .uniform_data = { .id = vs_uniform_data },
        },
        mugfx_draw_binding {
            .type = MUGFX_BINDING_TYPE_TEXTURE,
            .texture = { .binding = 0, .id = texture },
        },
    };

    mugfx_set_viewport(0, 0, 1024, 768);

    while (window.poll_events()) {
        mugfx_begin_frame();
        mugfx_begin_pass(MUGFX_RENDER_TARGET_BACKBUFFER);
        mugfx_draw(material, geometry, bindings.data(), bindings.size());
        mugfx_end_pass();
        mugfx_end_frame();
        window.swap();
    }

    mugfx_shutdown();
}
