#include "Window.hpp"

int main() {
    Window window("RV32IMF", 800, 600);

    while (!window.ShouldClose()) {
        window.Update();
    }
}