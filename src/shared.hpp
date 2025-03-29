#pragma once

#include <array>
#include <cassert>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string_view>
#include <utility>

#include "mugfx.h"

namespace mugfx {

#define EXPORT extern "C"

void* allocate_raw(size_t size);
void deallocate_raw(void* ptr, size_t size);

template <typename T>
T* allocate(size_t num)
{
    auto ptr = reinterpret_cast<T*>(allocate_raw(sizeof(T) * num));
    for (size_t i = 0; i < num; ++i) {
        new (ptr + i) T {};
    }
    return ptr;
}

template <typename T>
void deallocate(T* ptr, size_t num)
{
    for (size_t i = 0; i < num; ++i) {
        ptr[i].~T();
    }
    deallocate_raw(ptr, sizeof(T) * num);
}

#ifdef __GNUC__
#define PRINTFLIKE(n, m) __attribute__((format(printf, n, m)))
#else
#define PRINTFLIKE(n, m)
#endif

void log(mugfx_severity severity, const char* msg);
void log_fmt(mugfx_severity severity, const char* fmt, std::va_list va);
void log_debug(const char* fmt, ...) PRINTFLIKE(1, 2);
void log_info(const char* fmt, ...) PRINTFLIKE(1, 2);
void log_warn(const char* fmt, ...) PRINTFLIKE(1, 2);
void log_error(const char* fmt, ...) PRINTFLIKE(1, 2);

void common_init(mugfx_init_params& params);
void default_init(mugfx_init_params& params);
void default_init(mugfx_shader_create_params& params);
void default_init(mugfx_texture_create_params& params);
void default_init(mugfx_material_create_params& params);
void default_init(mugfx_buffer_create_params& params);
void default_init(mugfx_uniform_data_create_params& params);
void default_init(mugfx_geometry_create_params& params);
void default_init(mugfx_render_target_create_params& params);

template <typename T>
struct Pool {
    static_assert(sizeof(T) >= sizeof(uint16_t));

    void init(size_t capacity)
    {
        assert(capacity > 0 && capacity < 0xffff);
        data_ = reinterpret_cast<T*>(allocate_raw(sizeof(T) * capacity));
        ids_ = allocate<Id>(capacity);
        capacity_ = capacity;
        for (size_t i = 0; i < capacity; ++i) {
            store_free_list(i, i + 1);
            // We invalidate on removal and we want to start with generation 1, so we init with 1
            ids_[i] = Id { EmptyIndex, 1 };
        }
    }

    ~Pool()
    {
        for (size_t i = 0; i < capacity_; ++i) {
            if (ids_[i].idx != EmptyIndex) {
                destroy_value(i);
            } else {
                destroy_free_list(i);
            }
        }
        deallocate_raw(data_, sizeof(T) * capacity_);
        deallocate(ids_, capacity_);
    }

    uint32_t insert(T&& v)
    {
        const auto idx = free_list_head_;
        assert(idx < capacity_);
        assert(ids_[idx].idx == EmptyIndex);
        free_list_head_ = get_free_list(idx);
        destroy_free_list(idx);
        store_value(idx, std::move(v));
        ids_[idx].idx = idx; // mark not empty
        return ids_[idx].combine();
    }

    uint32_t get_key(size_t idx)
    {
        return idx < capacity_ && ids_[idx].idx == idx ? ids_[idx].combine() : 0;
    }

    bool contains(uint32_t key)
    {
        const auto id = Id(key);
        return id.idx < capacity_ && ids_[id.idx].gen == id.gen;
    }

    bool remove(uint32_t key)
    {
        if (!contains(key)) {
            return false;
        }
        const auto idx = Id(key).idx;
        destroy_value(idx);
        store_free_list(idx, free_list_head_);
        free_list_head_ = idx;
        ids_[idx].idx = EmptyIndex;
        return true;
    }

    T* get(uint32_t key) { return contains(key) ? data_ + Id(key).idx : nullptr; }

    size_t capacity() const { return capacity_; }

private:
    static constexpr size_t EmptyIndex = 0xFFFF;

    struct Id {
        uint16_t idx;
        uint16_t gen;

        Id() : idx(0), gen(0) { }
        Id(uint32_t id) : idx(id & 0xFFFF), gen(id >> 16) { }
        Id(uint16_t idx, uint16_t gen) : idx(idx), gen(gen) { }
        uint32_t combine() const { return gen << 16 | idx; }
    };

    void store_free_list(size_t idx, uint16_t value) { new (data_ + idx) uint16_t { value }; }
    uint16_t get_free_list(size_t idx) const { return *reinterpret_cast<uint16_t*>(data_ + idx); }
    void destroy_free_list(size_t idx) { reinterpret_cast<uint16_t*>(data_ + idx)->~uint16_t(); }
    void store_value(size_t idx, T&& v) { new (data_ + idx) T { std::move(v) }; }
    void destroy_value(size_t idx) { (data_ + idx)->~T(); }

    T* data_ = nullptr;
    Id* ids_ = nullptr;
    size_t capacity_ = 0;
    size_t free_list_head_ = 0;
};

template <size_t Size = 128>
class StackString {
public:
    static std::optional<StackString> create(const char* src, size_t len)
    {
        if (len >= Size) {
            return std::nullopt;
        }
        return StackString(src, len);
    }

    static std::optional<StackString> create(const char* src)
    {
        return create(src, src ? std::strlen(src) : 0);
    }

    StackString() = default;

    StackString& operator=(const StackString& other)
    {
        size_ = other.size_;
        std::memcpy(data_.data(), other.data_.data(), other.size_);
        return *this;
    }

    bool operator==(const StackString& other) const { return view() == other.view(); }

    bool operator==(std::string_view other) const { return view() == other; }

    std::string_view view() const { return std::string_view(data_.data(), size_); }

    const char* c_str() const { return data_.data(); }

    bool empty() const { return size_ == 0; }

    size_t size() const { return size_; }

private:
    StackString(const char* src, size_t len) : size_(len)
    {
        assert(len < Size); // `<` because of null-terminator
        assert(src || len == 0);
        if (src) {
            std::memcpy(data_.data(), src, len);
        }
        data_[len] = '\0';
    }

    std::array<char, Size> data_;
    size_t size_ = 0;
};

}