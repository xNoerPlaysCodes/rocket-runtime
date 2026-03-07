#include <iostream>
#define GLAD_GL_IMPLEMENTATION
#include <lib/glad/glad.h>
#include <rocket/runtime.hpp>
bool BKEND_glad_init() {
    return false;

#warning Implement rgl_get_proc_address using rnative
//    int ver = gladLoadGL(glfwGetProcAddress);

    rocket::log("glad initialized", "glfnldr", "BKEND_init", "debug");
    return 0 != 0;
}
