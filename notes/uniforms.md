# Uniforms

Currently I only have uniform buffers. I also had a uniform_data abstraction, which turned out to be fairly useless. In theory you would want different strategies for handling uniform data depending on size and update frequency. I had this TODO in the past:
```
* Uniform Optimizations:
  - `CONSTANT`: OpenGL: Single shared, large uniform buffer bound once, Vulkan: Specialization Constants
  - `FRAME`: Pool multiple together and triple buffer them (one being written by CPU, one waiting for GPU, one being read by GPU)
  - `DRAW`: OpenGL: glUniform, Vulkan: Push Constants
```
When I noticed some slow-downs where GPU draw time was quite high and I thought I should finally implement this fancier uniform handling, but it turned out that most of what I had planned there is not even good.

Afaik the "actual fast path" on modern desktop GL is persistently mapped buffers. Unfortunately those are not supported on WebGL and in [ung](https://github.com/pfirsich/ung) I really want to keep WebGL2 compatibility and I'm just a guy, so I really do not want to keep two different paths for desktop/web (and test both constantly etc.). So I have to restrict myself to the options I would have in WebGL2, which is.. not that much I think?

I tested this in a small game with about 150 draw calls per frame with a small-ish uniform block:
```
layout (binding = 3, std140) uniform UngTransform {
    mat4 model;
    mat4 model_inv;
    mat4 model_view;
    mat4 model_view_projection;
};
```

I used a static scene, let the game warm-up for 20 seconds and then averaged the gpu draw times (measured from first draw to just before swap with a GL query).

I compared the following approaches:

1. Use a single uniform buffer, update it before every draw and then draw.
2. Use one large uniform buffer as an arena, add data for every draw (with glBufferSubData), then draw. Reset offset at the start of each frame.
3. Use a ring of uniform buffers.

I tested on Steam Deck (AMD), on my T480 with Intel UHD Graphics 620 and on my Nvidia 2070 Super.
These are the results:

1. deck=0.50ms, intel=5.25ms, nv=0.26ms
2. deck=0.72ms, intel=5.19ms, nv=0.53ms
3. deck=0.53ms, intel=5.34ms, nv=0.27ms

Perplexingly, shockingly, flabberghastingly the very stupid approach of hammering the same UBO and forcing implicit synchronization for every draw is the best on all three machines I tested (or is on-par with another approach, that is the best). Everything is similarly shit on Intel, so whatever. I am sure this naive approach is so bad and so common at the same time (it's the simplest thing you can implement, I would say), that drivers likely detect this and have a fast path for it. Relying on some driver fast path for "idiot mode" is not good, but the ring buffer of UBOs seems wasteful (I am sure there is some overhead for every UBO) and you need another size constant for the pool size. So I went with the simple, silly, unintuitively fast approach. I removed all the stuff in mugfx I don't actually need, so it does not rot.

I think one thing I should probably have tried is doing a version of 2, but buffering all the draw calls and doing one large glBufferSubData and then kick of a bunch of draws. This induces less implicit sync, but the sync is worse, because more data is transferred. It would have been too much work to refactor everything to buffer the draw calls and there is a good argument it might not have been faster, so I didn't test it.

I still think a common abstraction over all the different ways to upload uniforms could be useful, especially since you would handle uniforms very differently on Vulkan compared to OpenGL and because Vulkan has push constants. But I decided that until I actually want to implement a Vulkan backend, I will just not bother and keep using the buffers. For future reference I want to add something like this (I hate the name and every other named I considered thus far):

```c++
// Uniform Arena
// This is intended as a common abstraction that abstracts over push constants on Vulkan,
// persistently mapped buffers on desktop-GL and single UBOs on WebGL.
typedef enum {
    MUGFX_UNIFORM_ARENA_USAGE_DEFAULT = 0,
    MUGFX_UNIFORM_ARENA_USAGE_FRAME,
    MUGFX_UNIFORM_ARENA_USAGE_PASS,
    MUGFX_UNIFORM_ARENA_USAGE_DRAW,
} mugfx_uniform_arena_usage;

typedef struct {
    size_t block_size; // payload size, doesn't have to include block alignment
    size_t block_count; // maximum number of uploads per frame
    mugfx_uniform_arena_usage usage;
    const char* debug_label;
} mugfx_uniform_arena_create_params;

typedef struct {
    uint32_t id;
} mugfx_uniform_arena_id;

mugfx_uniform_arena_id mugfx_uniform_arena_create(mugfx_uniform_arena_create_params params);
void mugfx_uniform_arena_destroy(mugfx_uniform_arena_id arena);

typedef struct {
    mugfx_uniform_arena_id arena;
    uint32_t ref;
} mugfx_uniform_arena_ref;

// Call this between mugfx_begin_frame and mugfx_end_frame.
// The returned mugfx_uniform_id is only valid until the next mugfx_end_frame.
// data.length must be <= size.
// If the arena is exhausted, {0} is returned (mugfx_draw will assert it is valid).
mugfx_uniform_arena_ref mugfx_uniform_arena_push(mugfx_uniform_arena_id arena, mugfx_slice data);
```

I used this for testing, in case I ever want to look it up again:

```c++
#pragma once

#include <mugfx.h>

#include "types.hpp"

namespace ung {

struct UniformPoolRef {
    mugfx_buffer_id buffer;
    mugfx_range range;
};

enum class UniformUpdateFreq { Constant, Frame, Pass, Draw };

static constexpr std::array<mugfx_buffer_usage_hint, 4> freq_usage_map = {
    MUGFX_BUFFER_USAGE_HINT_STATIC,
    MUGFX_BUFFER_USAGE_HINT_DYNAMIC,
    MUGFX_BUFFER_USAGE_HINT_DYNAMIC,
    MUGFX_BUFFER_USAGE_HINT_STREAM,
};

struct UniformPool_SingleUbo {
    mugfx_buffer_id buffer;

    void init(size_t block_size, size_t /*updates_per_frame*/, UniformUpdateFreq freq,
        const char* debug_label)
    {
        buffer = mugfx_buffer_create({
            .target = MUGFX_BUFFER_TARGET_UNIFORM,
            .usage = freq_usage_map.at((size_t)(int)freq),
            .data = { nullptr, block_size },
            .debug_label = debug_label,
        });
    }

    void reset() { }

    UniformPoolRef push(mugfx_slice data)
    {
        // mugfx_buffer_update(buffer, 0, {}); // orphan
        mugfx_buffer_update(buffer, 0, data);
        return { buffer, {} };
    }
};

constexpr size_t align_up(size_t n, size_t alignment)
{
    const auto r = n % alignment;
    return r > 0 ? n - r + alignment : n;
}

constexpr size_t ubo_align(size_t n)
{
    // actually: GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, but no way to query it in mugfx right now
    // in practice this is 256 very often and I think Vulkan requires this to be equal or less than
    // that so 256 is probably safe.
    return align_up(n, 256);
}

struct UniformPool_SingleUboSuballoc {
    mugfx_buffer_id buffer;
    size_t block_size;
    size_t index;

    void init(size_t block_size_, size_t updates_per_frame, UniformUpdateFreq freq,
        const char* debug_label)
    {
        block_size = block_size_;
        const auto size = ubo_align(block_size) * updates_per_frame;
        buffer = mugfx_buffer_create({
            .target = MUGFX_BUFFER_TARGET_UNIFORM,
            .usage = freq_usage_map.at((size_t)(int)freq),
            .data = { nullptr, size },
            .debug_label = debug_label,
        });
    }

    void reset()
    {
        mugfx_buffer_update(buffer, 0, {}); // orphan
        index = 0;
    }

    UniformPoolRef push(mugfx_slice data)
    {
        const auto offset = (index++) * ubo_align(block_size);
        mugfx_buffer_update(buffer, offset, data);
        return { buffer, { offset, block_size } };
    }
};

struct UniformPool_UboPool {
    Array<mugfx_buffer_id> buffers;
    size_t index;

    void init(size_t block_size, size_t updates_per_frame, UniformUpdateFreq freq,
        const char* debug_label)
    {
        buffers.init((u32)updates_per_frame);
        for (u32 i = 0; i < buffers.size; ++i) {
            buffers[i] = mugfx_buffer_create({
                .target = MUGFX_BUFFER_TARGET_UNIFORM,
                .usage = freq_usage_map.at((size_t)(int)freq),
                .data = { nullptr, block_size },
                .debug_label = debug_label,
            });
        }
    }

    void reset() { index = 0; }

    UniformPoolRef push(mugfx_slice data)
    {
        const auto buffer = buffers[(u32)(index++)];
        // nvidia: orphan makes no difference
        // mugfx_buffer_update(buffer, 0, {}); // orphan
        mugfx_buffer_update(buffer, 0, data);
        return { buffer, {} };
    }
};

using UniformPool = UniformPool_SingleUbo;

}
```