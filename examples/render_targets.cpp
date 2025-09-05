#include <array>
#include <cstdio>
#include <cstdlib>

#include <mugfx.h>

#include "window.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// attribute-less fullscreen quad
static const char* fsq_vert = R"(
    const vec2 positions[4] = vec2[] (
        vec2(-1.0, -1.0),
        vec2( 1.0, -1.0),
        vec2(-1.0,  1.0),
        vec2( 1.0,  1.0)
    );

    out vec2 vs_out_uv;

    void main() {
        vec2 pos = positions[gl_VertexID];
        vs_out_uv = pos * 0.5 + 0.5;
        gl_Position = vec4(pos, 0.0, 1.0);
    }
)";

// PASS 1: animated checker pattern
static const char* checker_frag = R"(
    layout (binding = 0, std140) uniform UFrame {
        float time;
    };

    in vec2 vs_out_uv;
    out vec4 frag_color;

    float checker(vec2 uv, float n) {
        vec2 g = floor(uv * n + vec2(time, 0.0));
        return mod(g.x + g.y, 2.0);
    }

    void main() {
        float c = mix(0.15, 1.0, checker(vs_out_uv, 10.0));
        frag_color = vec4(c, c, c, 1.0);
    }
)";

// PASS 2: vignette
static const char* post_frag = R"(
    layout (binding = 0) uniform sampler2D u_scene;

    in vec2 vs_out_uv;
    out vec4 frag_color;

    void main() {
        vec3 col = texture(u_scene, vs_out_uv).rgb;
        float d = distance(vs_out_uv, vec2(0.5));
        float vign = smoothstep(0.5, 0.1, d); // darker toward edges
        frag_color = vec4(col * vign, 1.0);
    }
)";

struct UFrame {
    // std140-friendly 16 bytes
    float time;
    float _pad[3];
};

struct App {
    size_t win_w = 1024, win_h = 768;
    Window window;
    mugfx_shader_id checker_fs;
    mugfx_shader_id post_fs;
    mugfx_shader_id vs;
    mugfx_material_id post_mat;
    mugfx_material_id checker_mat;
    mugfx_geometry_id fs_quad;
    mugfx_render_target_id offscreen;
    mugfx_uniform_data_id uframe;

    void init()
    {
        window = Window::create("Render Targets", win_w, win_h);

        mugfx_init({
            .debug = true,
        });

        std::printf("Renderer: %s\n", mugfx_get_renderer_name());
        std::printf("Vendor:   %s\n", mugfx_get_vendor_name());
        std::printf("API:      %s\n", mugfx_get_api_version());

        // Shaders & materials
        vs = mugfx_shader_create({ .stage = MUGFX_SHADER_STAGE_VERTEX, .source = fsq_vert });

        checker_fs = mugfx_shader_create({
            .stage = MUGFX_SHADER_STAGE_FRAGMENT,
            .source = checker_frag,
            .bindings = { { .type = MUGFX_SHADER_BINDING_TYPE_UNIFORM, .binding = 0 }, },
        });
        checker_mat = mugfx_material_create({ .vert_shader = vs, .frag_shader = checker_fs });

        post_fs = mugfx_shader_create({
            .stage = MUGFX_SHADER_STAGE_FRAGMENT,
            .source = post_frag,
            .bindings = { { .type = MUGFX_SHADER_BINDING_TYPE_SAMPLER, .binding = 0 }, },
        });
        post_mat = mugfx_material_create({ .vert_shader = vs, .frag_shader = post_fs });

        fs_quad = mugfx_geometry_create({
            .draw_mode = MUGFX_DRAW_MODE_TRIANGLE_STRIP, .vertex_count = 4,
            // vertex_buffers left empty; no index buffer (attribute-less geometry)
        });

        uframe = mugfx_uniform_data_create({
            .usage_hint = MUGFX_UNIFORM_DATA_USAGE_HINT_FRAME,
            .size = sizeof(UFrame),
        });

        offscreen = mugfx_render_target_create({
            .width = win_w,
            .height = win_h,
            .color = {
                { MUGFX_PIXEL_FORMAT_RGBA8, true },
            },
            .depth = { MUGFX_PIXEL_FORMAT_DEPTH24, false },
        });
    }

    void shutdown()
    {
        mugfx_render_target_destroy(offscreen);
        mugfx_uniform_data_destroy(uframe);
        mugfx_geometry_destroy(fs_quad);
        mugfx_material_destroy(post_mat);
        mugfx_shader_destroy(post_fs);
        mugfx_material_destroy(checker_mat);
        mugfx_shader_destroy(checker_fs);
        mugfx_shader_destroy(vs);

        mugfx_shutdown();
    }

    void main_loop()
    {
        auto* uframe_data = (UFrame*)mugfx_uniform_data_get_ptr(uframe);
        uframe_data->time = window.get_time();
        mugfx_uniform_data_update(uframe);

        mugfx_begin_frame();

        // Pass 1: offscreen checker pattern
        mugfx_begin_pass(offscreen);
        mugfx_clear(MUGFX_CLEAR_COLOR_DEPTH, MUGFX_CLEAR_DEFAULT);
        std::array<mugfx_draw_binding, 1> pass1_bindings {
            mugfx_draw_binding {
                .type = MUGFX_BINDING_TYPE_UNIFORM_DATA,
                .uniform_data = { .binding = 0, .id = uframe },
            },
        };
        mugfx_draw(checker_mat, fs_quad, pass1_bindings.data(), pass1_bindings.size());
        mugfx_end_pass();

        // Pass 2: vignette
        mugfx_begin_pass(MUGFX_RENDER_TARGET_BACKBUFFER);
        mugfx_set_viewport(0, 0, win_w, win_h);
        mugfx_clear(MUGFX_CLEAR_COLOR_DEPTH, MUGFX_CLEAR_DEFAULT);
        const auto offscreen_tex = mugfx_render_target_get_color_texture(offscreen, 0);
        std::array<mugfx_draw_binding, 1> pass2_bindings {
            mugfx_draw_binding {
                .type = MUGFX_BINDING_TYPE_TEXTURE,
                .texture = { .binding = 0, .id = offscreen_tex },
            },
        };
        mugfx_draw(post_mat, fs_quad, pass2_bindings.data(), pass2_bindings.size());
        mugfx_end_pass();

        mugfx_end_frame();
        window.swap();
    }
};

int main()
{
    App app;
    app.init();

#ifdef __EMSCRIPTEN__
    auto main_loop = [](void* arg) {
        auto& app = *static_cast<App*>(arg);
        if (!app.window.poll_events()) {
            emscripten_cancel_main_loop();
            return;
        }
    };
    emscripten_set_main_loop_arg(main_loop, &app, 0, 1);
#else
    while (app.window.poll_events()) {
        app.main_loop();
    }
#endif

    app.shutdown();
    return 0;
}
