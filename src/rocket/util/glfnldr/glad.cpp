#include <iostream>
#define GLAD_GL_IMPLEMENTATION
#include <lib/glad/glad.h>
#include <rocket/runtime.hpp>
#include <native.hpp>
bool BKEND_glad_init() {
    int ver = gladLoadGL(rnative::load_proc_address);
    rocket::log("glad initialized with exit code " + std::to_string(ver), "glfnldr", "BKEND_init", "debug");
    return ver != 0;
}
