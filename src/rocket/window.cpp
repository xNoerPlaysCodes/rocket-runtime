#include "../../include/rocket/window.hpp"
#include <GLFW/glfw3.h>
#include <cstdlib>
#include <iostream>
#include <vector>
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

    void window_t::glfw_set_platform(const char *platform) {
        setenv("GLFW_PLATFORM", platform, 1);
    }

    window_t::window_t(const rocket::vec2i_t& size,
            const std::string& title,
            windowflags_t flags) {
        this->size = size;
        this->title = title;
        if (!glfw_initialized) {
            glfwSetErrorCallback([](int error, const char* description) { std::cout << util::format_error(description, error, "glfw", "warning"); });
            glfwInit();
            glfw_initialized = true;
        }
        GLFWmonitor *monitor = glfwGetPrimaryMonitor();
        if (monitor == nullptr && flags.fullscreen) {
            std::cout << util::format_error("failed to get primary monitor", 1, "GLFW", "fatal");
            std::exit(1);
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, flags.gl_version.x);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, flags.gl_version.y);
        float glver = static_cast<float>(flags.gl_version.x) + static_cast<float>(0.1 * flags.gl_version.y);
        if (glver < 3.0f) {
            rocket::log_error("OpenGL Version 3.0 or higher must be used", 1, "PreGL", "fatal");
            std::exit(7);
        }
        if (glver > 3.1f) {
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        }

        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);

        if (util::is_wayland()) {
            glfwWindowHintString(GLFW_WAYLAND_APP_ID, "rocketge-game");
        }
 
        if (!flags.fullscreen) {
            monitor = nullptr;
        }
        if (flags.resizable) {
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        } else {
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        }
        if (flags.vsync) {
            glfwSwapInterval(1);
        }
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

        rocket::log("GLFW window Initialized with size " + std::to_string(size.x) + "x" + std::to_string(size.y) + " and title " + title, 
            "window_t", "constructor", 
            "info");

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
                    std::cout << util::format_error("GLFW_RAW_MOUSE_MOTION is not supported on wayland", 1, "RocketRuntime", "warning");
                }
#ifdef RocketRuntime_DEBUG
                std::cout << util::format_error("cursor mode 'GLFW_CURSOR_NORMAL' not supported on wayland so 'GLFW_CURSOR_DISABLED' has to be used", 1, "RocketRuntime", "warn");
#endif
            }
        }
    }

    void window_t::set_cursor_position(const rocket::vec2d_t& pos) {
        if (util::is_wayland()) {
            std::cout << util::format_error("setting cursor position is not supported on wayland", 1, "RocketRuntime", "warn");
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
            util::dispatch_event(event);
        }
        
        for (int i = static_cast<int>(io::mouse_button::left); i <= static_cast<int>(io::mouse_button::middle); ++i) {
            bool new_state = glfwGetMouseButton(glfw_window, i) == GLFW_PRESS;
            if (buttons[i].current != new_state) {
                buttons[i].previous = buttons[i].current;
                buttons[i].current = new_state;

                // Dispatch only on change
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
            event.pos.x = mouse_x;
            event.pos.y = mouse_y;
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
        std::cerr << util::format_log("GLFW window closed", "window_t", cxf, "info");
    }

    window_t::~window_t() {
        destructor_called = true;
        close();
        destructor_called = false;
    }
}
