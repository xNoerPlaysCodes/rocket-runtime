#include <iostream>
#define GLAD_GL_IMPLEMENTATION
#include <lib/glad/glad.h>
#include <rocket/runtime.hpp>
bool BKEND_glad_init() {
    return false;

    int ver = gladLoadGL(glfwGetProcAddress);

    rocket::log("glad initialized", "glfnldr", "BKEND_init", "debug");
    return ver != 0;
}
