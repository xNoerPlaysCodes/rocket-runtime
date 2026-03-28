#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <exception>
#include "../../include/rocket/macros.hpp"
#include "rocket/runtime.hpp"

#ifdef ROCKETGE__Platform_Desktop

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
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#endif

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

    void *get_proc_address(const char *name) {
        void *ptr = (void*) glXGetProcAddress((const GLubyte*) name);
        if (ptr != nullptr) return ptr;

        // Legacy path
        static void *libGL = dlopen("libGL.so.1", RTLD_LAZY | RTLD_GLOBAL);
        if (libGL == nullptr) return nullptr;
        return dlsym(libGL, name);
    }
}
#endif

#ifdef ROCKETGE__Platform_UnixCompatible
namespace unix_backend {
    [[noreturn]] void exit_now(int code) {
        _exit(code);
    }

    void set_thread_name(const char *_name) {
        char name[16];
        strncpy(name, _name, 15);
        name[15] = '\0';
        pthread_setname_np(pthread_self(), name);
    }
}
#endif

#ifdef ROCKETGE__Platform_Windows
namespace windows_backend {
    void set_class_name(const wchar_t *app_id) {
        SetCurrentProcessExplicitAppUserModelID(app_id);
    }

    [[noreturn]] void exit_now(int code) {
        ExitProcess(code);
    }

    void init() {
        // Set scheduler time period to 1ms
        // its 15.625ms normally
        timeBeginPeriod(1);
    }

    void set_thread_name(const char *name) {
        std::wstring wide(name, name + strlen(name));
        HANDLE thread = GetCurrentThread();
        HRESULT hr = SetThreadDescription(thread, wide.c_str());
        if (FAILED(hr)) {
            rocket::log("SetThreadDescription failed", "rnative", "[windows_backend]", "error");
        }
    }

    void *get_proc_address(const char *name) {
        void *ptr = (void*) wglGetProcAddress(name);
        if (ptr != nullptr) return ptr;

        // Legacy path
        static HMODULE module = LoadLibraryA("opengl32.dll");
        if (module == nullptr) return nullptr;
        return (void*) GetProcAddress(module, name);
    }
}
#endif

namespace rnative {
#ifdef ROCKETGE__Platform_Desktop
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
#endif

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
        unix_backend::exit_now(code);
#elifdef ROCKETGE__Platform_Windows
        windows_backend::exit_now(code);
#else
        (void) code;
        __builtin_trap();
#endif
    }

    void init() {
#ifdef ROCKETGE__Platform_Linux
#elifdef ROCKETGE__Platform_macOS
#elifdef ROCKETGE__Platform_Windows
        windows_backend::init();
#endif
    }

    void set_thread_name(const char *name) {
#ifdef ROCKETGE__Platform_UnixCompatible
        unix_backend::set_thread_name(name);
#elifdef ROCKETGE__Platform_Windows
        windows_backend::set_thread_name(name);
#endif
    }

#ifdef ROCKETGE__Platform_Desktop
    proc_address_t load_proc_address(const char *name) {
#ifdef ROCKETGE__Platform_Linux
        return reinterpret_cast<proc_address_t>(linux_backend::get_proc_address(name));
#elifdef ROCKETGE__Platform_Windows
        return reinterpret_cast<proc_address_t>(windows_backend::get_proc_address(name));
#else
        return nullptr;
#endif
    }
#endif
}
