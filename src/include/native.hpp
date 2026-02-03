#ifndef ROCKETGE__RNATIVE_HPP
#define ROCKETGE__RNATIVE_HPP

#include <GLFW/glfw3.h>
namespace rnative {
    void wayland_set_window_icon(GLFWwindow *window, GLFWimage &image);
    /// @brief Pre window creation
    /// @param window Pass nullptr
    void windows_set_window_class_name_prewincreate(const wchar_t *str);
    /// @brief Linux Set AppID/WinClass
    void linux_set_class_name(const char *str);

    void exit_now(int code);
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

#endif // X11 or WAYLAND

#ifdef RNATIVE__INCLUDE_WINDOWS

#include <windows.h>
#include <shobjidl.h>

#endif // RNATIVE__INCLUDE_WINDOWS

#endif // ROCKETGE__RNATIVE_HPP
