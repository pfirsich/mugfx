#include "window.hpp"

#include <cstdio>

#include <SDL.h>

struct Window::Impl {
    bool init(const char* title, size_t width, size_t height)
    {
        if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
            std::printf("Could not initialize SDL2\n");
            return false;
        }

#ifdef MUGFX_WEBGL
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

        SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 0);

#if !defined(NDEBUG) && !defined(MUGFX_WEBGL)
        int contextFlags = 0;
        SDL_GL_GetAttribute(SDL_GL_CONTEXT_FLAGS, &contextFlags);
        contextFlags |= SDL_GL_CONTEXT_DEBUG_FLAG;
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, contextFlags);
#endif

        // Window
        uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
        window = SDL_CreateWindow(
            title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, flags);
        if (!window) {
            std::printf("Could not create window\n");
            return false;
        }

        // Context
        ctx = SDL_GL_CreateContext(window);
        if (!ctx) {
            std::printf("Could not create context\n");
            return false;
        }

        return true;
    }

    ~Impl()
    {
        if (ctx) {
            SDL_GL_DeleteContext(ctx);
        }
        if (window) {
            SDL_DestroyWindow(window);
        }
    }

    SDL_Window* window = nullptr;
    SDL_GLContext ctx = nullptr;
};

Window Window::create(const char* title, size_t width, size_t height)
{
    auto impl = std::make_unique<Impl>();
    if (!impl->init(title, width, height)) { // already logged an error
        std::abort();
    }
    return Window(std::move(impl));
}

Window::Window() = default;

Window& Window::operator=(Window&& other) = default;

Window::~Window() = default;

bool Window::poll_events() const
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            return false;
        }
    }
    return true;
}

float Window::get_time() const
{
    static const auto start = SDL_GetPerformanceCounter();
    const auto now = SDL_GetPerformanceCounter();
    return (float)(now - start) / (float)SDL_GetPerformanceFrequency();
}

void Window::swap() const
{
    SDL_GL_SwapWindow(impl_->window);
}

Window::Window(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) { }
