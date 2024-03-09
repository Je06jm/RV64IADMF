#include "DeltaTime.hpp"

#include "OpenGL.hpp"

void _DeltaTime::Update() {
    double current = glfwGetTime();

    delta_time = current - last;
    last = current;
}