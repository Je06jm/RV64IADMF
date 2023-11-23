#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <string>
#include <cstdint>
#include <functional>
#include <memory>

#include "Keyboard.hpp"

class GLFWwindow;

class Window {
    GLFWwindow* window = nullptr;
    
    struct Callbacks;
    Callbacks* callbacks = nullptr;

public:
    const std::string title;
    const uint32_t width, height;

    Window(const std::string& title, uint32_t width, uint32_t height);
    ~Window();
    
    void Update();
    void SetShouldClose();
    bool ShouldClose() const;

    void SetKeyboardInputCallback(std::function<void(Key key, bool is_pressed)> key_callback);
    void SetMouseButtonInputCallback(std::function<void(uint32_t button, bool is_pressed)> mouse_button_callback);
    void SetMouseMovementInputCallback(std::function<void(double x, double y)> mouse_movement_callback);
};

#endif