#include <array>
#include <cstdio>
#include <cstdlib>

#include <mugfx.h>

#include "window.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// #version directives will be inserted by mugfx
const auto vert_source = R"(
    layout (binding = 0, std140) uniform UConstant {
        mat4 projection;
    };

    layout (location = 0) in vec3 a_position;
    layout (location = 1) in vec2 a_texcoord;

    // No interface blocks in WebGL
    out vec2 vs_out_texcoord;

    void main() {
        vs_out_texcoord = a_texcoord;
        gl_Position = projection * vec4(a_position, 1.0);
    }
)";

struct UConstant {
    std::array<float, 16> projection;
};

const auto frag_source = R"(
    layout(binding = 0) uniform sampler2D u_base;

    in vec2 vs_out_texcoord;
    out vec4 frag_color;

    void main() {
        frag_color = texture(u_base, vs_out_texcoord);
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

    std::printf("Renderer: %s\n", mugfx_get_renderer_name());
    std::printf("Vendor: %s\n", mugfx_get_vendor_name());
    std::printf("API: %s\n", mugfx_get_api_version());

    const auto vert_shader = mugfx_shader_create({
        .stage = MUGFX_SHADER_STAGE_VERTEX,
        .source = vert_source,
        .bindings = {
            // Note this matches the binding layout specifier in the shader
            { .type = MUGFX_SHADER_BINDING_TYPE_UNIFORM, .binding = 0 },
        },
    });

    const auto frag_shader = mugfx_shader_create({
        .stage = MUGFX_SHADER_STAGE_FRAGMENT,
        .source = frag_source,
        .bindings = {
            // This also matches the binding layout specifier in the shader
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

    // This encapsulates your vertex layout and references the necessary buffers
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
            .uniform_data = { .binding = 0, .id = vs_uniform_data },
        },
        mugfx_draw_binding {
            .type = MUGFX_BINDING_TYPE_TEXTURE,
            .texture = { .binding = 0, .id = texture },
        },
    };

    mugfx_set_viewport(0, 0, 1024, 768);

#ifdef __EMSCRIPTEN__
    // Emscripten does not give you a mainloop, but you have to set a mainloop callback

    struct RenderContext {
        Window& window;
        mugfx_material_id material;
        mugfx_geometry_id geometry;
        mugfx_draw_binding* draw_bindings;
        size_t num_draw_bindings;
    };

    RenderContext ctx { window, material, geometry, bindings.data(), bindings.size() };

    auto main_loop = [](void* arg) {
        auto& ctx = *static_cast<RenderContext*>(arg);

        if (!ctx.window.poll_events()) {
            emscripten_cancel_main_loop();
            return;
        }

        mugfx_begin_frame();
        mugfx_begin_pass(MUGFX_RENDER_TARGET_BACKBUFFER);
        mugfx_clear(
            MUGFX_CLEAR_COLOR_DEPTH, { .color = { 0.0f, 0.0f, 0.0f, 1.0f }, .depth = 1.0f });
        mugfx_draw(ctx.material, ctx.geometry, ctx.draw_bindings, ctx.num_draw_bindings);
        mugfx_end_pass();
        mugfx_end_frame();
        ctx.window.swap();
    };

    emscripten_set_main_loop_arg(main_loop, &ctx, 0, 1);
#else
    while (window.poll_events()) {
        mugfx_begin_frame();
        mugfx_begin_pass(MUGFX_RENDER_TARGET_BACKBUFFER);
        mugfx_clear(
            MUGFX_CLEAR_COLOR_DEPTH, { .color = { 0.0f, 0.0f, 0.0f, 1.0f }, .depth = 1.0f });
        mugfx_draw(material, geometry, bindings.data(), bindings.size());
        mugfx_end_pass();
        mugfx_end_frame();
        window.swap();
    }
#endif

    mugfx_shutdown();
}
