#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
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

#ifdef ROCKETGE__Platform_Linux
namespace linux_backend {
    std::string get_glfw_window_platform_x11_or_wayland(GLFWwindow *window) {
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

        rocket::log("failed to get a valid native windowing api handle", "rnative", "[linux_backend]", "error");
        return "unknown";
    }

    void wayland_set_class_name(const char *str) {
        rocket::log("wayland_set_class_name is not implemented", "rnative", "[linux_backend]", "fixme");
    }

    void x11_set_class_name(const char *str) {
        glfwWindowHintString(GLFW_X11_CLASS_NAME, str);
        glfwWindowHintString(GLFW_X11_INSTANCE_NAME, str);
    }

    void exit_now(int code) {
        _exit(code);
    }
}
#endif

#ifdef ROCKETGE__Platform_Windows
namespace windows_backend {
    void set_class_name(const wchar_t *app_id) {
        SetCurrentProcessExplicitAppUserModelID(app_id);
    }

    void exit_now(int code) {
        ExitProcess(code);
    }

    void init() {
        timeBeginPeriod(1);
    }
}
#endif

namespace rnative {
    void wayland_set_window_icon(GLFWwindow *window, GLFWimage &image) {
#ifndef ROCKETGE__Platform_Linux
        rocket::log("wayland_set_window_icon is only supported on linux_wayland", "rnative", "wayland_set_window_icon", "error");
        return;
#else
        if (int platform = glfwGetPlatform(); platform != GLFW_PLATFORM_WAYLAND) {
            rocket::log("the current mode of glfw operation doesn't support PLATFORM_WAYLAND", "rnative", "wayland_set_window_icon", "error");
            return;
        }

        if (linux_backend::get_glfw_window_platform_x11_or_wayland(window) != "wayland") {
            rocket::log("this is not a wayland window (maybe x11 with xwayland?)", "rnative", "wayland_set_window_icon", "error");
            return;
        }

        wl_surface *surface = glfwGetWaylandWindow(window);

        if (surface == nullptr) {
            rocket::log("failed to get wayland surface", "rnative", "wayland_set_window_icon", "error");
            return;
        }

        rocket::log("not implemented", "rnative", "wayland_set_window_icon", "error");
#endif
    }

    void windows_set_window_class_name_prewincreate(const wchar_t *str) {
#ifndef ROCKETGE__Platform_Windows
        rocket::log("windows_set_window_class_name is only supported on windows", "rnative", "windows_set_window_class_name", "error");
        return;
#else
        if (int platform = glfwGetPlatform(); platform != GLFW_PLATFORM_WIN32) {
            rocket::log("this build of glfw doesn't support PLATFORM_WIN32", "rnative", "windows_set_window_class_name", "error");
            return;
        }

        windows_backend::set_class_name(str);
#endif
    }

    void linux_set_class_name(const char *str) {
#ifndef ROCKETGE__Platform_Linux
        rocket::log("linux_set_class_name is only supported on linux_any", "rnative", "linux_set_class_name", "error");
        return;
#else
        linux_backend::x11_set_class_name(str);
#endif
    }

    void exit_now(int code) {
#ifdef ROCKETGE__Platform_UnixCompatible
        linux_backend::exit_now(code);
#elifdef ROCKETGE__Platform_Windows
        windows_backend::exit_now(code);
#endif
    }

    void init() {
#ifdef ROCKETGE__Platform_Linux
#elifdef ROCKETGE__Platform_macOS
#elifdef ROCKETGE__Platform_Windows
        windows_backend::init();
#endif
    }
}
