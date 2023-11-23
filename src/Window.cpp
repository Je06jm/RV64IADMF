#include "Window.hpp"
#include "OpenGL.hpp"

#include <stdexcept>
#include <memory>
#include <unordered_map>

uint32_t glfw_users = 0;

struct Window::Callbacks {
    std::function<void(Key key, bool is_pressed)> key_callback;
    std::function<void(uint32_t button, bool is_pressed)> mouse_button_callback;
    std::function<void(double x, double y)> mouse_movement_callback;
};

Window::Window(const std::string& title, uint32_t width, uint32_t height) : title{title}, width{width}, height{height} {
    if (glfw_users == 0) {
        glfwInit();
    }

    glfw_users++;

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    
    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);


    if (window == nullptr) {
        throw std::runtime_error("Could not create OpenGL 3.3 Window");
    }

    {
        auto monitor = glfwGetPrimaryMonitor();
        auto mode = glfwGetVideoMode(monitor);

        auto pos_x = mode->width / 2 - width / 2;
        auto pos_y = mode->height / 2 - height / 2;

        glfwSetWindowPos(window, pos_x, pos_y);
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGL(glfwGetProcAddress)) {
        throw std::runtime_error("Could not load GLAD2");
    }

    {
        int frame_width = 0;
        int frame_height = 0;
        glfwGetFramebufferSize(window, &frame_width, &frame_height);
        glViewport(0, 0, frame_width, frame_height);
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    static const std::unordered_map<int, Key> key_translation = {
        {GLFW_KEY_GRAVE_ACCENT, KEY_GRAVE_ACCENT},
        {GLFW_KEY_1, KEY_ONE},
        {GLFW_KEY_2, KEY_TWO},
        {GLFW_KEY_3, KEY_THREE},
        {GLFW_KEY_4, KEY_FOUR},
        {GLFW_KEY_5, KEY_FIVE},
        {GLFW_KEY_6, KEY_SIX},
        {GLFW_KEY_7, KEY_SEVEN},
        {GLFW_KEY_8, KEY_EIGHT},
        {GLFW_KEY_9, KEY_NINE},
        {GLFW_KEY_0, KEY_ZERO},
        {GLFW_KEY_MINUS, KEY_MINUS},
        {GLFW_KEY_EQUAL, KEY_EQUALS},
        {GLFW_KEY_BACKSPACE, KEY_BACKSPACE},
        {GLFW_KEY_TAB, KEY_TAB},
        {GLFW_KEY_Q, KEY_Q},
        {GLFW_KEY_W, KEY_W},
        {GLFW_KEY_E, KEY_E},
        {GLFW_KEY_R, KEY_R},
        {GLFW_KEY_T, KEY_T},
        {GLFW_KEY_Y, KEY_Y},
        {GLFW_KEY_U, KEY_U},
        {GLFW_KEY_I, KEY_I},
        {GLFW_KEY_O, KEY_O},
        {GLFW_KEY_P, KEY_P},
        {GLFW_KEY_LEFT_BRACKET, KEY_LEFT_BRACKET},
        {GLFW_KEY_RIGHT_BRACKET, KEY_RIGHT_BRACKET},
        {GLFW_KEY_BACKSLASH, KEY_BACK_SLASH},
        {GLFW_KEY_CAPS_LOCK, KEY_CAPS_LOCK},
        {GLFW_KEY_A, KEY_A},
        {GLFW_KEY_S, KEY_S},
        {GLFW_KEY_D, KEY_D},
        {GLFW_KEY_F, KEY_F},
        {GLFW_KEY_G, KEY_G},
        {GLFW_KEY_H, KEY_H},
        {GLFW_KEY_J, KEY_J},
        {GLFW_KEY_K, KEY_K},
        {GLFW_KEY_L, KEY_L},
        {GLFW_KEY_SEMICOLON, KEY_SEMICOLON},
        {GLFW_KEY_APOSTROPHE, KEY_QUOTE},
        {GLFW_KEY_ENTER, KEY_ENTER},
        {GLFW_KEY_LEFT_SHIFT, KEY_LEFT_SHIFT},
        {GLFW_KEY_Z, KEY_Z},
        {GLFW_KEY_X, KEY_X},
        {GLFW_KEY_C, KEY_C},
        {GLFW_KEY_V, KEY_V},
        {GLFW_KEY_B, KEY_B},
        {GLFW_KEY_N, KEY_N},
        {GLFW_KEY_M, KEY_M},
        {GLFW_KEY_COMMA, KEY_COMMA},
        {GLFW_KEY_PERIOD, KEY_PERIOD},
        {GLFW_KEY_SLASH, KEY_FORWARD_SLASH},
        {GLFW_KEY_RIGHT_SHIFT, KEY_RIGHT_SHIFT},
        {GLFW_KEY_LEFT_CONTROL, KEY_LEFT_CTRL},
        {GLFW_KEY_LEFT_ALT, KEY_LEFT_ALT},
        {GLFW_KEY_SPACE, KEY_SPACE},
        {GLFW_KEY_RIGHT_ALT, KEY_RIGHT_ALT},
        {GLFW_KEY_RIGHT_CONTROL, KEY_RIGHT_CTRL},
        {GLFW_KEY_UP, KEY_UP_ARROW},
        {GLFW_KEY_LEFT, KEY_LEFT_ARROW},
        {GLFW_KEY_DOWN, KEY_DOWN_ARROW},
        {GLFW_KEY_RIGHT, KEY_RIGHT_ARROW},
        {GLFW_KEY_INSERT, KEY_INSERT},
        {GLFW_KEY_HOME, KEY_HOME},
        {GLFW_KEY_PAGE_UP, KEY_PAGE_UP},
        {GLFW_KEY_DELETE, KEY_DELETE},
        {GLFW_KEY_END, KEY_END},
        {GLFW_KEY_PAGE_DOWN, KEY_PAGE_DOWN}
    };

    callbacks = new Callbacks;
    callbacks->key_callback = [](Key, bool) {};
    callbacks->mouse_button_callback = [](uint32_t, bool) {};
    callbacks->mouse_movement_callback = [](double, double) {};

    glfwSetWindowUserPointer(window, callbacks);

    glfwSetKeyCallback(window, [](GLFWwindow* window, int key, int, int action, int) {
        if (key_translation.find(key) != key_translation.end() && action != GLFW_REPEAT) {
            Callbacks* callbacks = reinterpret_cast<Callbacks*>(glfwGetWindowUserPointer(window));
            if (callbacks != nullptr) {
                callbacks->key_callback(key_translation.at(key), action == GLFW_PRESS);
            }
        }
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int) {
        Callbacks* callbacks = reinterpret_cast<Callbacks*>(glfwGetWindowUserPointer(window));
        if (callbacks != nullptr && action != GLFW_REPEAT) {
            callbacks->mouse_button_callback(button, action == GLFW_PRESS);
        }
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow* window, double x, double y) {
        Callbacks* callbacks = reinterpret_cast<Callbacks*>(glfwGetWindowUserPointer(window));
        if (callbacks != nullptr) {
            callbacks->mouse_movement_callback(x, y);
        }
    });
}

Window::~Window() {
    glfwDestroyWindow(window);
    delete callbacks;
    
    glfw_users--;

    if (glfw_users == 0) {
        glfwTerminate();
    }
}

void Window::Update() {
    glfwPollEvents();
    glfwSwapBuffers(window);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::SetShouldClose() {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

bool Window::ShouldClose() const {
    return glfwWindowShouldClose(window) == GLFW_TRUE;
}

void Window::SetKeyboardInputCallback(std::function<void(Key key, bool is_pressed)> key_callback) {
    callbacks->key_callback = key_callback;
}

void Window::SetMouseButtonInputCallback(std::function<void(uint32_t button, bool is_pressed)> mouse_button_callback) {
    callbacks->mouse_button_callback = mouse_button_callback;
}

void Window::SetMouseMovementInputCallback(std::function<void(double x, double y)> mouse_movement_callback) {
    callbacks->mouse_movement_callback = mouse_movement_callback;
}