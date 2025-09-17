#include <GLFW/glfw3.h>
#include <X11/X.h>
#include "../../include/rocket/macros.hpp"
#include "rocket/runtime.hpp"

#ifdef ROCKETGE__Platform_Linux
#define RNATIVE__INCLUDE_WAYLAND
#define GLFW_EXPOSE_NATIVE_WAYLAND

#define RNATIVE__INCLUDE_X11
#define GLFW_EXPOSE_NATIVE_X11
#endif

#ifdef ROCKETGE__Platform_Windows
#define RNATIVE__INCLUDE_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include "native.hpp"
#include <GLFW/glfw3native.h>

namespace linux_backend {
    std::string get_glfw_window_platform_x11_or_wayland(GLFWwindow *window) {
#ifndef ROCKETGE__Platform_Linux
        rocket::log_error("[linux_backend] functions are only supported on linux", -1, "rnative::[linux_backend]", "fatal-to-function");
#else
        if (glfwPlatformSupported(GLFW_PLATFORM_X11)) {
            Window xid = glfwGetX11Window(window);
            if (xid) {
                return "x11";
            }
        }

        if (glfwPlatformSupported(GLFW_PLATFORM_WAYLAND)) {
            wl_surface *sf = glfwGetWaylandWindow(window);
            if (sf != nullptr) {
                return "wayland";
            }
        }

        rocket::log_error("failed to get a valid native windowing api handle", -1, "rnative::[linux_backend]", "fatal-to-function");
        return "did they add a new fking windowing api???";
#endif
    }
}

namespace windows_backend {
    void set_class_name(const wchar_t *app_id) {
#ifndef ROCKETGE__Platform_Windows
        rocket::log_error("[windows_backend] functions are only supported on windows", -1, "rnative::[windows_backend]", "fatal-to-function");
#else
        SetCurrentProcessExplicitAppUserModelID(app_id);
#endif
    }
}

namespace rnative {
    void wayland_set_window_icon(GLFWwindow *window, GLFWimage &image) {
#ifndef ROCKETGE__Platform_Linux
        rocket::log_error("wayland_set_window_icon is only supported on linux_wayland", -1, "rnative::wayland_set_window_icon", "fatal-to-function");
        return;
#else
        if (int platform = glfwGetPlatform(); platform != GLFW_PLATFORM_WAYLAND) {
            rocket::log_error("the current mode of glfw operation doesn't support PLATFORM_WAYLAND", -1, "rnative::wayland_set_window_icon", "fatal-to-function");
            return;
        }

        if (linux_backend::get_glfw_window_platform_x11_or_wayland(window) != "wayland") {
            rocket::log_error("this is not a wayland window (maybe x11 with xwayland?)", -1, "rnative::wayland_set_window_icon", "fatal-to-function");
            return;
        }

        wl_surface *surface = glfwGetWaylandWindow(window);
        if (surface == nullptr) {
            rocket::log_error("failed to get wayland surface", -1, "rnative::wayland_set_window_icon", "fatal-to-function");
            return;
        }

        rocket::log_error("not implemented", -1, "rnative::wayland_set_window_icon", "fatal-to-function");
#endif
    }

    void windows_set_window_class_name_prewincreate(const wchar_t *str) {
#ifndef ROCKETGE__Platform_Windows
        rocket::log_error("windows_set_window_class_name is only supported on windows", -1, "rnative::windows_set_window_class_name", "fatal-to-function");
        return;
#else
        if (int platform = glfwGetPlatform(); platform != GLFW_PLATFORM_WIN32) {
            rocket::log_error("this build of glfw doesn't support PLATFORM_WIN32", -1, "rnative::windows_set_window_class_name", "fatal-to-function");
            return;
        }

        windows_backend::set_class_name(str);
#endif
    }
}
