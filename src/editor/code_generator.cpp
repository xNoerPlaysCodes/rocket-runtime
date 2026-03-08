#include "code_generator.hpp"

namespace rgeditor {
    generated_code_t *gen() {
        generated_code_t *cd = new generated_code_t;
        cd->hpp = R"(#ifndef RGEditor__GeneratedCode_hpp
#define RGEditor__GeneratedCode_hpp

namespace rgeditor_generated_code {
    void draw();
}

#endif//RGEditor__GeneratedCode_hpp
)";
        cd->cpp = R"(#include "rge_generated_code.hpp"
#include <rocket/runtime.hpp>

int main(int argc, char **argv) {
    using namespace rgeditor_generated_code;
    rocket::glfw_window_t window = { {1280, 720}, "RGame" };
    rocket::renderer_2d r2d = { &window, 60 };
    
    while (window.is_running()) {
        r2d.begin_frame();
        r2d.clear();
        {
            draw();
        }
        r2d.end_frame();
        window.poll_events();
    }
}
        )";
        return cd;
    }
}
