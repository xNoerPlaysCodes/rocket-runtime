#include "../../include/rocket/window.hpp"
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include "rocket/io.hpp"
#include "rocket/runtime.hpp"
#include "rocket/types.hpp"
#include "util.hpp"

namespace rocket {
    bool glfw_initialized = false;

    io::keystate_t keys[GLFW_KEY_LAST + 1] = {};
    io::keystate_t buttons[GLFW_MOUSE_BUTTON_MIDDLE + 1] = {};

    double last_mouse_x, last_mouse_y;

    GLFWmonitor* glfwaltGetMonitorWithCursor() {
        int count;
        GLFWmonitor** monitors = glfwGetMonitors(&count);
        double cursor_x, cursor_y;
        glfwGetCursorPos(glfwGetCurrentContext(), &cursor_x, &cursor_y);

        for (int i = 0; i < count; ++i) {
            int x, y;
            glfwGetMonitorPos(monitors[i], &x, &y);

            const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);

            if (cursor_x >= x && cursor_x < x + mode->width &&
                cursor_y >= y && cursor_y < y + mode->height) {
                return monitors[i];
            }
        }

        // fallback
        return glfwGetPrimaryMonitor();
    }

    int glfwaltGetBoolean(bool b) {
        return static_cast<int>(b);
    }

    platform_t window_t::get_platform() {
        auto glfw_platform = glfwGetPlatform();
        platform_t platform;
        std::string glfw_platform_str = platform.name;
        if (glfw_platform == GLFW_PLATFORM_X11) {
            glfw_platform_str = "X11";
            platform.type = platform_type_t::linux_x11;
            platform.os_name = "Linux";
        } else if (glfw_platform == GLFW_PLATFORM_WAYLAND) {
            glfw_platform_str = "Wayland";
            platform.type = platform_type_t::linux_wayland;
            platform.os_name = "Linux";
        } else if (glfw_platform == GLFW_PLATFORM_COCOA) {
            glfw_platform_str = "MacOS (Cocoa)";
            platform.type = platform_type_t::macos_cocoa;
            platform.os_name = "macOS";
        } else if (glfw_platform == GLFW_PLATFORM_WIN32) {
            glfw_platform_str = "Windows";
            platform.type = platform_type_t::windows;
            platform.os_name = "Windows";
        }

        platform.name = glfw_platform_str;
        platform.rge_name = std::string(ROCKETGE__PLATFORM);
        return platform;
    }

    window_t::window_t(const rocket::vec2i_t& size,
            const std::string& title,
            windowflags_t flags) {
        this->size = size;
        this->title = title;
        if (!glfw_initialized) {
            glfwSetErrorCallback([](int error, const char* description) { rocket::log_error(description, error, "GLFW::ErrorCallback", "warn"); });
            glfwInit();
            glfw_initialized = true;
        }
        GLFWmonitor *monitor = glfwGetPrimaryMonitor();
        if (monitor == nullptr && flags.fullscreen) {
            rocket::log_error("failed to get primary monitor", 1, "GLFW", "fatal");
            std::exit(1);
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, flags.gl_version.x);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, flags.gl_version.y);
        float glver = static_cast<float>(flags.gl_version.x) + static_cast<float>(0.1 * flags.gl_version.y);
        if (glver < 3.0f) {
            rocket::log_error("OpenGL Version 3.0 or higher must be used for a modern environment", 1, "PreGL", "warning");
        }
        if (glver > 3.1f) {
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        }

        if (flags.gl_contextverifier) {
            if (glver >= 4.3f) {
                glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
            } else {
                rocket::log_error("OpenGL::ContextVerifier requires 4.3 or higher", -5, "OpenGL", "warn");
            }
        }

        if (glver >= 3.0f) {
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        }
        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);

        glfwWindowHint(GLFW_DECORATED, glfwaltGetBoolean(!flags.undecorated));
        glfwWindowHint(GLFW_VISIBLE, glfwaltGetBoolean(!flags.hidden));

        if (flags.minimized) {
            glfwIconifyWindow(glfw_window);
        }

        glfwWindowHint(GLFW_MAXIMIZED, glfwaltGetBoolean(flags.maximized));
        glfwWindowHint(GLFW_FOCUSED, glfwaltGetBoolean(!flags.unfocused));
        glfwWindowHint(GLFW_FLOATING, glfwaltGetBoolean(flags.topmost));
        if (flags.always_run) {
            rocket::log_error("Always Run is not implemented yet", -1, "window_t::constructor", "warn");
        }

        if (flags.opacity < 1.0f) {
            glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, glfwaltGetBoolean(true));
            glfwSetWindowOpacity(glfw_window, flags.opacity);
        }

        glfwWindowHint(GLFW_SCALE_TO_MONITOR, glfwaltGetBoolean(flags.hidpi));

        if (util::is_wayland()) {
            glfwWindowHintString(GLFW_WAYLAND_APP_ID, ROCKETGE__PlatformSpecific_Linux_AppClassNameOrID);
        } else if (util::is_x11()) {
            glfwWindowHintString(GLFW_X11_CLASS_NAME, ROCKETGE__PlatformSpecific_Linux_AppClassNameOrID);
        }
 
        if (!flags.fullscreen) {
            monitor = nullptr;
        }
        glfwWindowHint(GLFW_RESIZABLE, glfwaltGetBoolean(flags.resizable));
        // We set Swap Interval at polL_events() -> first_init
        this->flags = flags;
        glfwWindowHint(GLFW_SAMPLES, flags.msaa_samples);
        glfw_window = glfwCreateWindow(size.x, size.y, title.c_str(), nullptr, nullptr);
        glfwMakeContextCurrent(glfw_window);

        monitor = glfwaltGetMonitorWithCursor();

        glfwSetFramebufferSizeCallback(this->glfw_window, [](GLFWwindow* window, int width, int height) {
            glViewport(0, 0, width, height);
        });

        glfwSetCharModsCallback(this->glfw_window, [](GLFWwindow* window, unsigned int codepoint, int mods) {
            char c = static_cast<char>(codepoint);
            ::util::push_formatted_char_typed(c);
        });

        glfwSetScrollCallback(this->glfw_window, [](GLFWwindow* window, double xoffset, double yoffset) {
            rocket::io::scroll_offset_event_t event;
            event.offset = { xoffset, yoffset };
            util::dispatch_event(event);
        });

        int sx, sy;
        auto mode = glfwGetVideoMode(glfwaltGetMonitorWithCursor());
        sx = mode->width;
        sy = mode->height;
        if (!util::is_wayland()) {
            glfwSetWindowPos(glfw_window, (sx - size.x) / 2, (sy - size.y) / 2);
        }
        glfwSetWindowUserPointer(glfw_window, this);
        glfwSetWindowSizeCallback(glfw_window, [](GLFWwindow* window, int width, int height) {
            window_t *w = static_cast<window_t*>(glfwGetWindowUserPointer(window));
            w->size = { width, height };
        });

        glfwSetWindowIconifyCallback(this->glfw_window, [](GLFWwindow* window, int iconified) {
            glfwWaitEvents();
        });

        rocket::log("Window Initialized with size " + std::to_string(size.x) + "x" + std::to_string(size.y) + " and title " + title, 
            "window_t", "constructor", 
            "info");
        auto platform = get_platform();
        std::string glfw_platform_str = platform.name;
        rocket::log("CPL Windowing Library: GLFW", "window_t", "constructor", "info");
        rocket::log("Windowing API: " + glfw_platform_str, "window_t", "constructor", "info");
        rocket::log("RGE Platform: " + platform.rge_name, "window_t", "constructor", "info");

        std::vector<std::string> logs = {
            "Modules:",
            #ifdef ROCKETGE__BUILD_QUARK
                "- Quark: [ENABLED]",
            #else
                "- Quark: [DISABLED]",
            #endif
            #ifdef ROCKETGE__BUILD_ASTRO
                "- AstroUI: [ENABLED]",
            #else
                "- AstroUI: [DISABLED]",
            #endif
        };

        for (auto &l : logs) {
            rocket::log(l, "window_t", "constructor", "info");
        }
    }

    void window_t::set_cursor_mode(cursor_mode_t m) {
        this->mode = m;
        if (m == cursor_mode_t::normal) {
            glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); 
        } else if (m == cursor_mode_t::hidden) {
            glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            if (util::is_wayland()) {
                glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                if (glfwRawMouseMotionSupported()) {
                    glfwSetInputMode(glfw_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
                } else {
                    rocket::log_error("GLFW_RAW_MOUSE_MOTION is not supported on wayland", 1, "RocketRuntime", "warning");
                }
#ifdef RocketRuntime_DEBUG
                rocket::log_error("cursor mode 'GLFW_CURSOR_NORMAL' not supported on wayland so 'GLFW_CURSOR_DISABLED' has to be used", 1, "RocketRuntime", "warn");
#endif
            }
        }
    }

    void window_t::set_cursor_position(const rocket::vec2d_t& pos) {
        if (util::is_wayland()) {
            rocket::log_error("setting cursor position is not supported on wayland", 1, "RocketRuntime", "warn");
            return;
        }
        glfwSetCursorPos(glfw_window, pos.x, pos.y);
    }

    void window_t::set_size(const rocket::vec2i_t& size) {
        glfwSetWindowSize(glfw_window, size.x, size.y);
        this->size = size;
    }

    void window_t::set_title(const std::string& title) {
        glfwSetWindowTitle(glfw_window, title.c_str());
        this->title = title;
    }

    void window_t::register_on_close(std::function<void()> listener) {
        util::get_on_close_listeners().push_back(listener);
    }

    void window_t::poll_events() {
        static bool first_init = false;
        if (!first_init) {
            if (flags.vsync) {
                glfwSwapInterval(1);
            } else {
                glfwSwapInterval(0);
            }
            first_init = true;
        }
        glfwPollEvents();
        int w, h;
        glfwGetFramebufferSize(glfw_window, &w, &h);
        this->size = { w, h };

        for (int i = static_cast<int>(io::keyboard_key::first_key); i <= static_cast<int>(io::keyboard_key::last_key); ++i) {
            keys[i].previous = keys[i].current;
            keys[i].current = glfwGetKey(glfw_window, i) == GLFW_PRESS;

            io::key_event_t event;
            event.key = static_cast<io::keyboard_key>(i);
            event.state = keys[i];
            event.scancode = glfwGetKeyScancode(i);
            util::dispatch_event(event);
        }
        
        for (int i = static_cast<int>(io::mouse_button::left); i <= static_cast<int>(io::mouse_button::middle); ++i) {
            bool new_state = glfwGetMouseButton(glfw_window, i) == GLFW_PRESS;
            if (buttons[i].current != new_state) {
                buttons[i].previous = buttons[i].current;
                buttons[i].current = new_state;
            } else {
                buttons[i].previous = buttons[i].current;
            }

            io::mouse_event_t event;
            event.button = static_cast<io::mouse_button>(i);
            event.state = buttons[i];

            util::dispatch_event(event);
        }

        double mouse_x, mouse_y;
        glfwGetCursorPos(glfw_window, &mouse_x, &mouse_y);
        if (mouse_x != last_mouse_x || mouse_y != last_mouse_y) {
            io::mouse_move_event_t event;
            event.pos = { mouse_x, mouse_y };
            event.old_pos = { last_mouse_x, last_mouse_y };
            util::dispatch_event(event);
            last_mouse_x = mouse_x;
            last_mouse_y = mouse_y;
        }

        util::io_update_end_frame();
    }

    std::string window_t::get_title() const {
        return this->title;
    }

    rocket::vec2i_t window_t::get_size() const {
        return this->size;
    }

    rocket::vec2d_t window_t::get_cursor_position() {
        double x, y;
        glfwGetCursorPos(glfw_window, &x, &y);
        return { x, y };
    }

    bool window_t::is_running() const {
        return !glfwWindowShouldClose(glfw_window);
    }

    double window_t::get_time() const {
        return glfwGetTime();
    }

    cursor_mode_t window_t::get_cursor_mode() {
        return this->mode;
    }

    bool is_wayland() {
        return util::is_wayland();
    }

    bool destructor_called = false;

    void window_t::close() {
        if (glfw_window == nullptr) {
            // Silent Error, doesn't matter
            std::exit(0);
        }
        glfwDestroyWindow(glfw_window);
        glfw_window = nullptr;

        if (glfw_initialized) {
            glfwTerminate();
            glfw_initialized = false;
        }

        std::string cxf = "close";
        if (destructor_called) {
            cxf = "destructor";
        }
        rocket::log("Window closed", "window_t", cxf, "info");
    }

    window_t::~window_t() {
        destructor_called = true;
        close();
        destructor_called = false;
    }
}
