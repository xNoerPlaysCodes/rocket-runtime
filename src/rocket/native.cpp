#include <GLFW/glfw3.h>
#include "../../include/rocket/macros.hpp"
#include "rocket/runtime.hpp"

#ifdef ROCKETGE__Platform_Linux
#define RNATIVE__INCLUDE_WAYLAND
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif

#include "native.hpp"
#include <GLFW/glfw3native.h>

namespace rnative {
    void wayland_set_window_icon(GLFWwindow *window, GLFWimage &image) {
#ifndef ROCKETGE__Platform_Linux
        rocket::log_error("wayland_set_window_icon is only supported on linux_wayland", -1, "rnative::wayland_set_window_icon", "fatal-to-function");
        return;
#else

        int platform = glfwGetPlatform();
        if (platform != GLFW_PLATFORM_WAYLAND) {
            rocket::log_error("this build of glfw doesn't support PLATFORM_WAYLAND", -1, "rnative::wayland_set_window_icon", "fatal-to-function");
            return;
        }

        wl_surface *surface = glfwGetWaylandWindow(window);
        if (surface == nullptr) {
            rocket::log_error("failed to get wayland surface", -1, "rnative::wayland_set_window_icon", "fatal-to-function");
            return;
        }

        rocket::log_error("wayland_set_window_icon is not implemented yet", -1, "rnative::wayland_set_window_icon", "fatal-to-function");
#endif
    }
}
