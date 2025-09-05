#include <memory>

struct Window {
public:
    static Window create(const char* title, size_t width, size_t height);

    Window();
    Window(Window&& other) = default;
    Window& operator=(Window&& other);
    ~Window();

    bool poll_events() const; // returns whether window is still open
    float get_time() const;
    void swap() const;

private:
    struct Impl;

    Window(std::unique_ptr<Impl> impl);

    std::unique_ptr<Impl> impl_;
};