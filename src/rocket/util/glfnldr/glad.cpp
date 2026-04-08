#include <iostream>
#define GLAD_GL_IMPLEMENTATION
#include <lib/glad/glad.h>
#include <rocket/runtime.hpp>
#include <native.hpp>
#ifdef ROCKETGE__Platform_Desktop
#include <GLFW/glfw3.h>
#endif
namespace glfnldr::glad {
    bool init() {
        int ver = 0;
#ifdef ROCKETGE__Platform_Desktop
        if (glfwGetCurrentContext() != nullptr) {
            ver = gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress));
        }
#endif
        if (ver == 0) {
            ver = gladLoadGL(rnative::load_proc_address);
        }
        rocket::log("glad initialized with exit code " + std::to_string(ver), "glfnldr::glad", "init", "debug");
        return ver != 0;
    }
}
