#include "DeltaTime.hpp"

#include <glfw3.h>

void _DeltaTime::Update() {
    double current = glfwGetTime();

    delta_time = current - last;
    last = current;
}