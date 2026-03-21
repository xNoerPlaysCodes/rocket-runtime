#ifndef ROCKETGE__RNATIVE_HPP
#define ROCKETGE__RNATIVE_HPP

#include <rocket/macros.hpp>
#include <string>
#ifdef ROCKETGE__Platform_Linux
#include <GLFW/glfw3.h>
#endif
namespace rnative {
#ifdef ROCKETGE__Platform_Linux
    void wayland_set_window_icon(GLFWwindow *window, GLFWimage &image);
#endif
    /// @brief Pre window creation
    /// @param window Pass nullptr
    void windows_set_window_class_name_prewincreate(const wchar_t *str);
    /// @brief Linux Set AppID/WinClass
    void linux_set_class_name(const char *str);


    [[noreturn]] void exit_now(int code);
    void init();

    /// @brief On Posix Systems name may only be 15 bytes and 1 NULL long
    void set_thread_name(const char *name);

    typedef void (*proc_address_t)(void);

    void intrin_cpu_minfreq();

    proc_address_t load_proc_address(const char *name);
}

#ifdef RNATIVE__INCLUDE_WAYLAND

#include <wayland-client.hpp>
#include <wayland-server.hpp>
#include "rnative/xdg-toplevel-icon-v1-client-protocol.h"

#endif // RNATIVE__INCLUDE_WAYLAND

#ifdef RNATIVE__INCLUDE_X11

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#endif // RNATIVE__INCLUDE_X11

#if defined(RNATIVE__INCLUDE_X11) || defined(RNATIVE__INCLUDE_WAYLAND)

#include <linux/input.h>
#include <unistd.h>
#include <fcntl.h>
#include <GL/glx.h>
#include <dlfcn.h>

#endif // X11 or WAYLAND

#ifdef RNATIVE__INCLUDE_WINDOWS

#include <windows.h>
#include <timeapi.h>
#include <shobjidl.h>

#endif // RNATIVE__INCLUDE_WINDOWS

#if defined(ROCKETGE__Platform_UnixCompatible) || defined(ROCKETGE__Platform_Android)
#define native_gmtime(x, y) gmtime_r(x, y)
#define native_localtime(x, y) localtime_r(x, y)
#endif
#ifdef ROCKETGE__Platform_Windows
#define native_gmtime(x, y) gmtime_s(y, x)
#define native_localtime(x, y) localtime_s(y, x)
#endif

#endif // ROCKETGE__RNATIVE_HPP
