#ifndef ROCKETGE__RNATIVE_HPP
#define ROCKETGE__RNATIVE_HPP

#include <GLFW/glfw3.h>
namespace rnative {
    void wayland_set_window_icon(GLFWwindow *window, GLFWimage &image);
}

#ifdef RNATIVE__INCLUDE_WAYLAND

#include <wayland-client.hpp>
#include <wayland-server.hpp>
#include "rnative/xdg-toplevel-icon-v1-client-protocol.h"

#endif // RNATIVE__INCLUDE_WAYLAND

#endif // ROCKETGE__RNATIVE_HPP
