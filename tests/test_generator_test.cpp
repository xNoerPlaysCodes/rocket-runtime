#include "rocket/renderer.hpp"
#include "rocket/window.hpp"
#include <test_generator/test.hpp>

int main(int argc, char **argv) {
    rocket::renderer_2d *ren = nullptr;
    rocket::glfw_window_t *win = nullptr;
    return MakeTest(
        argc, argv, "Test Generator Test", 
        [ren, win]() -> bool {
            if (ren == nullptr || win == nullptr)
                return false;
            return true;
        },
        nullptr,
        nullptr,
        [&ren, &win](rocket::window_t *window, rocket::renderer_2d *renderer) {
            ren = renderer;
            win = window;
        }
    );
}
