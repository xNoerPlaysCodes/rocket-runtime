#include <iostream>
#define GLAD_GL_IMPLEMENTATION
#include <lib/glad/glad.h>
#include <rocket/runtime.hpp>
#include <native.hpp>
bool BKEND_glad_init() {
    int ver = gladLoadGL(rnative::load_proc_address);

    printf("%d\n", ver);

    rocket::log("glad initialized", "glfnldr", "BKEND_init", "debug");
    return ver != 0;
}
