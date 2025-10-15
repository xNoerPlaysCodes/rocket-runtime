#include "../../include/rocket/window.hpp"
#include <GLFW/glfw3.h>
#include <codecvt>
#include <cstdlib>
#include <iostream>
#include <locale>
#include <sstream>
#include <string>
#include <vector>
#include "native.hpp"
#include "rocket/asset.hpp"
#include "rocket/io.hpp"
#include "rocket/macros.hpp"
#include "intl_macros.hpp"
#include "rocket/runtime.hpp"
#include "rocket/types.hpp"
#include "util.hpp"

namespace rocket {
    static bool glfw_initialized = false;

    io::keystate_t keys[GLFW_KEY_LAST + 1] = {};
    io::keystate_t buttons[GLFW_MOUSE_BUTTON_LAST + 1] = {};

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

    inline int glfwaltGetBoolean(bool b) {
        return static_cast<int>(b);
    }

    RGE_STATIC_FUNC_IMPL monitor_t monitor_t::with_cursor() {
        monitor_t m;
        GLFWmonitor *glfwm = glfwaltGetMonitorWithCursor();
        const GLFWvidmode *glfwvm = glfwGetVideoMode(glfwm);
        m.idx = -1;
        m.size = { glfwvm->width, glfwvm->height };
        m.redbits = glfwvm->redBits;
        m.greenbits = glfwvm->greenBits;
        m.bluebits = glfwvm->blueBits;
        m.refreshrate = glfwvm->refreshRate;
        return m;
    }

    RGE_STATIC_FUNC_IMPL int monitor_t::get_count() {
        int count;
        glfwGetMonitors(&count);
        return count;
    }

    RGE_STATIC_FUNC_IMPL monitor_t monitor_t::of(int idx) {
        if (idx < 0) {
            rocket::log_error("monitor index out of range (min is 0), using monitor with cursor", -1, "monitor_t::of", "fatal");
            return monitor_t::with_cursor();
        }
        int count;
        GLFWmonitor **monitors = glfwGetMonitors(&count);

        if (idx >= count) {
            rocket::log_error("monitor index out of range (max is " + std::to_string(count) + ")", -1, "monitor_t::of", "fatal");
            return {};
        }

        monitor_t m;
        m.idx = idx;
        const GLFWvidmode *glfwvm = glfwGetVideoMode(monitors[idx]);
        m.size = { glfwvm->width, glfwvm->height };
        m.redbits = glfwvm->redBits;
        m.greenbits = glfwvm->greenBits;
        m.bluebits = glfwvm->blueBits;
        m.refreshrate = glfwvm->refreshRate;
        return m;
    }

    void _cb_glfw_error_cb(int error, const char* description) {
        rocket::log_error(description, error, "GLFW::ErrorCallback", "warn");
    }

    RGE_STATIC_FUNC_IMPL void window_t::cpl_init() {
        if (!glfw_initialized) {
            glfwSetErrorCallback(_cb_glfw_error_cb);
            if (int glfwInit_exc = glfwInit(); !glfwInit_exc) {
                rocket::log_error("glfwInit() failed with exit code: " + std::to_string(glfwInit_exc), -1, "GLFW::glfwInit", "fatal");
                glfw_initialized = false;
                rocket::exit(1);
            }
            glfw_initialized = true;
        }
    }

    platform_t window_t::get_platform() {
        int glfw_platform = glfwGetPlatform();
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
            glfw_platform_str = "Cocoa";
            platform.type = platform_type_t::macos_cocoa;
            platform.os_name = "macOS";
        } else if (glfw_platform == GLFW_PLATFORM_WIN32) {
            glfw_platform_str = "Win32";
            platform.type = platform_type_t::windows;
            platform.os_name = "Windows";
        } else {
            rocket::log_error("platform unimplemented", -1, "window_t::get_platform", "fatal-to-function");
            return {};
        }

        platform.name = glfw_platform_str;
        platform.rge_name = std::string(ROCKETGE__PLATFORM);
        return platform;
    }

    RGE_STATIC_FUNC_IMPL void window_t::set_forced_platform(platform_type_t type) {
        if (glfw_initialized) {
            rocket::log_error("too late! glfw has already been initialized", -1, "window_t::set_forced_platform", "fatal-to-function");
            return;
        }

        if (type == platform_type_t::linux_wayland) {
#ifndef ROCKETGE__Platform_Linux
            rocket::log_error("type can only be platform_type_t::linux_wayland when on linux", -1, "window_t::set_forced_platform", "fatal-to-function");
#else
            glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WAYLAND);
#endif
        } else if (type == platform_type_t::linux_x11) {
#ifndef ROCKETGE__Platform_Linux
            rocket::log_error("type can only be platform_type_t::linux_x11 when on linux", -1, "window_t::set_forced_platform", "fatal-to-function");
#else
            glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
        }

        else if (type == platform_type_t::windows) {
#ifndef ROCKETGE__Platform_Windows
            rocket::log_error("type can only be platform_type_t::windows when on windows", -1, "window_t::set_forced_platform", "fatal-to-function");
#else
            glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_WIN32);
#endif
        } else if (type == platform_type_t::macos_cocoa) {
#ifndef ROCKETGE__Platform_macOS
            rocket::log_error("type can only be platform_type_t::macos_cocoa when on macOS", -1, "window_t::set_forced_platform", "fatal-to-function");
#else
            glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_COCOA);
#endif
        } else {
            rocket::log_error("platform is not valid!", -1, "window_t::set_forced_platform", "fatal-to-function");
        }
    }

    bool glfw_platform_is_wayland(platform_t platform) {
        return platform.type == platform_type_t::linux_wayland;
    }

    bool glfw_platform_is_x11(platform_t platform) {
        return platform.type == platform_type_t::linux_x11;
    }

    float get_max_context_gl_version() {
        glfwSetErrorCallback(nullptr);
        GLFWwindow *tg_win;
        int versions[][19] = {
            {4,6},
            {4,5},
            {4,4},
            {4,3},
            {4,2},
            {4,1},
            {4,0},

            {3,3},
            {3,2},
            {3,1},
            {3,0},

            {2,1},
            {2,0},

            {1,5},
            {1,4},
            {1,3},
            {1,2},
            {1,1},
            {1,0}
        };

        for (int i = 0; i < 19; i++) {
            rocket::log("Trying " + std::to_string(versions[i][0]) + "." + std::to_string(versions[i][1]), "RocketGE", "ContextCreator", "trace");
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, versions[i][0]);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, versions[i][1]);
            float ver = (float)versions[i][0] + (float)versions[i][1] / 10.f;
            if (ver >= 3.2) {
                glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            } else {
                glfwWindowHint(GLFW_OPENGL_PROFILE, 0);
            }
            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);

            tg_win = glfwCreateWindow(1, 1, "tmp", NULL, NULL);
            if (tg_win == nullptr) {
                if (versions[i][0] == 1 && versions[i][1] == 1) {
                    glfwSetErrorCallback(_cb_glfw_error_cb);
                    rocket::log_error("This graphics driver does not support the minimum OpenGL version of 1.1", 1, "RocketGE::ContextCreator", "fatal");
                    rocket::exit(0);
                    return 1.0f;
                }
            } else {
                glfwSetErrorCallback(_cb_glfw_error_cb);
                glfwDestroyWindow(tg_win);

                return ver;
            }
        }

        glfwSetErrorCallback(_cb_glfw_error_cb);
        return 4.6f;
    }

    window_t::window_t(const rocket::vec2i_t& size,
            const std::string& title,
            windowflags_t flags) {
        this->size = size;
        this->title = title;
        window_t::cpl_init();
        GLFWmonitor *monitor = glfwGetPrimaryMonitor();
        if (monitor == nullptr && flags.fullscreen) {
            rocket::log_error("failed to get primary monitor", 1, "GLFW::glfwGetPrimaryMonitor", "fatal");
            rocket::exit(1);
        }

        // override by cli args
        auto cliargs = util::get_clistate();
        if (cliargs.glversion == GL_VERSION_20) {
            flags.gl_version = rocket::vec2i_t(2, 0);
        } else if (cliargs.glversion == GL_VERSION_21) {
            flags.gl_version = rocket::vec2i_t(2, 1);
        } else if (cliargs.glversion == GL_VERSION_30) {
            flags.gl_version = rocket::vec2i_t(3, 0);
        } else if (cliargs.glversion == GL_VERSION_31) {
            flags.gl_version = rocket::vec2i_t(3, 1);
        } else if (cliargs.glversion == GL_VERSION_32) {
            flags.gl_version = rocket::vec2i_t(3, 2);
        } else if (cliargs.glversion == GL_VERSION_33) {
            flags.gl_version = rocket::vec2i_t(3, 3);
        } else if (cliargs.glversion == GL_VERSION_40) {
            flags.gl_version = rocket::vec2i_t(4, 0);
        } else if (cliargs.glversion == GL_VERSION_41) {
            flags.gl_version = rocket::vec2i_t(4, 1);
        } else if (cliargs.glversion == GL_VERSION_42) {
            flags.gl_version = rocket::vec2i_t(4, 2);
        } else if (cliargs.glversion == GL_VERSION_43) {
            flags.gl_version = rocket::vec2i_t(4, 3);
        } else if (cliargs.glversion == GL_VERSION_44) {
            flags.gl_version = rocket::vec2i_t(4, 4);
        } else if (cliargs.glversion == GL_VERSION_45) {
            flags.gl_version = rocket::vec2i_t(4, 5);
        } else if (cliargs.glversion == GL_VERSION_46) {
            flags.gl_version = rocket::vec2i_t(4, 6);
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, flags.gl_version.x);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, flags.gl_version.y);
        glfwWindowHint(GLFW_CONTEXT_RELEASE_BEHAVIOR, GLFW_ANY_RELEASE_BEHAVIOR);
        float glver = static_cast<float>(flags.gl_version.x) + static_cast<float>(0.1 * flags.gl_version.y);
        float max_gl_ver = get_max_context_gl_version();

        if (glver > max_gl_ver) {
            std::ostringstream ss;
            ss << std::setprecision(2) << max_gl_ver;
            if (max_gl_ver == 2 || max_gl_ver == 3 || max_gl_ver == 4) {
                ss << ".0";
            }
            rocket::log_error("This version of OpenGL is not supported by this graphics driver, using next available version: " + ss.str(), 1, "RocketGE::PreGL", "warning");
            glver = max_gl_ver;
        }

        if (float minglver = static_cast<float>(ROCKETGE__FEATURE_MIN_GL_VERSION_MAJOR) + static_cast<float>(0.1 * ROCKETGE__FEATURE_MIN_GL_VERSION_MINOR); glver < minglver) {
            std::ostringstream ss;
            ss << std::setprecision(2) << minglver;
            if (minglver == 2 || minglver == 3 || minglver == 4) {
                ss << ".0";
            }
            rocket::log_error("This version of OpenGL is not supported by this graphics driver, using minimum available version: " + ss.str(), 1, "RocketGE::PreGL", "warning");
            glver = static_cast<float>(ROCKETGE__FEATURE_MIN_GL_VERSION_MAJOR) + static_cast<float>(0.1 * ROCKETGE__FEATURE_MIN_GL_VERSION_MINOR);
        }

        if (max_gl_ver < 2.0f) {
            rocket::log_error("OpenGL Version 2.0 or higher must be used", 1, "RocketGE::PreGL", "fatal");
            rocket::exit(0);
        }

        if (glver < 3.0f) {
            rocket::log_error("OpenGL Version 3.0 or higher must be used for a modern environment", 1, "RocketGE::PreGL", "warning");
        }
        if (glver > 3.1f) {
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        }

        if (flags.gl_contextverifier) {
            if (glver >= 4.3f) {
                glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
            } else {
                rocket::log_error("OpenGL::ContextVerifier requires 4.3 or higher", -5, "RocketGE::PreGL", "warn");
            }
        }

        if (glver >= 3.0f) {
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
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
            rocket::log_error("[fixme] not implemented, windowflags_t::always_run", -1, "window_t::constructor", "fixme");
        }

        if (flags.opacity < 1.0f) {
            glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, glfwaltGetBoolean(true));
            glfwSetWindowOpacity(glfw_window, flags.opacity);
        }

        glfwWindowHint(GLFW_SCALE_TO_MONITOR, glfwaltGetBoolean(flags.hidpi));

        using GLFWenum = int;
        GLFWenum class_name = 0;
        if (glfw_platform_is_wayland(get_platform())) {
            class_name = GLFW_WAYLAND_APP_ID;
        } else if (glfw_platform_is_x11(get_platform())) {
            class_name = GLFW_X11_CLASS_NAME;
        }

        std::string class_name_or_id = flags.window_class_name.empty() || flags.window_class_name == ROCKETGE__PlatformSpecific_Linux_AppClassNameOrID ? ROCKETGE__PlatformSpecific_Linux_AppClassNameOrID : flags.window_class_name;

        if (class_name != 0) {
            glfwWindowHintString(class_name, class_name_or_id.c_str());
        }

#ifdef ROCKETGE__Platform_Windows
        auto to_widestr = [](std::string str) -> std::wstring {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            return converter.from_bytes(str);
        };

        bool is_custom_class_id = !flags.window_class_name.empty() && flags.window_class_name != ROCKETGE__PlatformSpecific_Linux_AppClassNameOrID;
        if (is_custom_class_id) {
            rocket::log_error("using an untested native API: may break", -1, "window_t::constructor", "warn");
            std::wstring wstr = to_widestr(flags.window_class_name);
            const wchar_t *cwstr = wstr.c_str();
            rnative::windows_set_window_class_name_prewincreate(cwstr);
        }
#endif
 
        if (!flags.fullscreen) {
            monitor = nullptr;
        }
        glfwWindowHint(GLFW_RESIZABLE, glfwaltGetBoolean(flags.resizable));
        // We set Swap Interval at poll_events() -> first_init
        this->flags = flags;
        glfwWindowHint(GLFW_SAMPLES, flags.msaa_samples);
        this->glfw_window = glfwCreateWindow(size.x, size.y, title.c_str(), nullptr, nullptr);
        if (this->glfw_window == nullptr) {
            rocket::log_error("failed to create window", -1, "window_t::constructor", "fatal");
            platform_t platform = this->get_platform();
            std::vector<std::string> log_messages = {
                "Error String: " + std::string("glfwCreateWindow resulted in a nullptr")
            };
            rocket::exit(1);
        }
        glfwMakeContextCurrent(glfw_window);

        monitor = glfwaltGetMonitorWithCursor();

        glfwSetFramebufferSizeCallback(this->glfw_window, [](GLFWwindow*, int width, int height) {
            glViewport(0, 0, width, height);
        });

        glfwSetCharModsCallback(this->glfw_window, [](GLFWwindow*, unsigned int codepoint, int /* mods */) {
            char c = static_cast<char>(codepoint);
            ::util::push_formatted_char_typed(c);
        });

        glfwSetScrollCallback(this->glfw_window, [](GLFWwindow*, double xoffset, double yoffset) {
            rocket::io::scroll_offset_event_t event;
            event.offset = { xoffset, yoffset };
            util::dispatch_event(event);
        });

        if (auto mode = glfwGetVideoMode(glfwaltGetMonitorWithCursor()); !glfw_platform_is_wayland(get_platform())) {
            glfwSetWindowPos(glfw_window, (mode->width - size.x) / 2, (mode->height - size.y) / 2);
        }
        glfwSetWindowUserPointer(glfw_window, this);
        glfwSetWindowSizeCallback(glfw_window, [](GLFWwindow* window, int width, int height) {
            window_t *w = static_cast<window_t*>(glfwGetWindowUserPointer(window));
            w->size = { width, height };
        });

        glfwSetWindowIconifyCallback(this->glfw_window, [](GLFWwindow*, int) {
            glfwWaitEvents();
        });

        rocket::log("Window created as [" + std::to_string(size.x) + "x" + std::to_string(size.y) + "] " + title, 
            "window_t", "constructor", 
            "info");
        auto platform = get_platform();
        std::string glfw_platform_str = platform.name;

        std::vector<std::string> logs = {
            "Engine Version: " ROCKETGE__VERSION,
            "Backend Windowing: GLFW",
            "Native Windowing: " + glfw_platform_str,
            "Engine Platform: " + platform.rge_name,
            "Rocket Libraries:",
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
            #ifdef ROCKETGE__BUILD_NETWORKING
                "- Networking: [ENABLED]",
            #else
                "- Networking: [DISABLED]",
            #endif
        };

        for (auto &l : logs) {
            rocket::log(l, "window_t", "constructor", "info");
        }
    }

    void window_t::set_window_state(window_state_t state) {
        glfwSetWindowAttrib(glfw_window, GLFW_FOCUSED,   state.focused);
        glfwSetWindowAttrib(glfw_window, GLFW_VISIBLE,   state.visible);
        glfwSetWindowAttrib(glfw_window, GLFW_ICONIFIED, state.iconified);
        glfwSetWindowAttrib(glfw_window, GLFW_MAXIMIZED, state.maximized);
        glfwSetWindowAttrib(glfw_window, GLFW_FLOATING,  state.floating);
    }

    void window_t::set_icon(std::shared_ptr<rocket::texture_t> texture) {
        if (texture->channels != rGE__TEXTURE_CHANNEL_COUNT_RGBA) {
            std::string channel_count = std::to_string(texture->channels);
            rocket::log_error("texture must be in RGBA format (try load in texture_color_format_t::rgba), channel count was: " + channel_count, -1, "window_t::set_icon", "fatal-to-function");
            return;
        }
        auto pxdata = texture->data.data();

        GLFWimage image;
        image.pixels = pxdata;
        image.width = texture->size.x;
        image.height = texture->size.y;

        if (int platform = glfwGetPlatform(); platform == GLFW_PLATFORM_WAYLAND) {
            rnative::wayland_set_window_icon(this->glfw_window, image);
            return;
        }
 
        if (get_platform().type == platform_type_t::macos_cocoa) {
            rocket::log_error("macOS::Cocoa does not support dynamically setting window icons", -1, "window_t::set_icon", "fatal-to-function");
            return;
        }

        glfwSetWindowIcon(this->glfw_window, 1, &image);
    }

    void window_t::set_cursor_mode(cursor_mode_t m) {
        this->mode = m;
        if (m == cursor_mode_t::normal) {
            glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); 
        } else if (m == cursor_mode_t::hidden) {
            glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
            if (glfw_platform_is_wayland(get_platform())) {
                glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                if (glfwRawMouseMotionSupported()) {
                    glfwSetInputMode(glfw_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
                } else {
                    rocket::log_error("GLFW_RAW_MOUSE_MOTION is not supported on wayland", 1, "window_t::set_cursor_mode", "warning");
                }
            }
        }
    }

    void window_t::set_cursor_position(const rocket::vec2d_t& pos) {
        if (glfw_platform_is_wayland(get_platform())) {
            rocket::log_error("setting cursor position is not supported on wayland", 1, "window_t::set_cursor_position", "warn");
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
        
        for (int i = static_cast<int>(io::mouse_button::first); i <= static_cast<int>(io::mouse_button::last); ++i) {
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
            event.position = {};
            glfwGetCursorPos(glfw_window, &event.position.x, &event.position.y);

            util::dispatch_event(event);
        }

        static double last_mouse_x{0}, last_mouse_y{0};
        double mouse_x{0}, mouse_y{0};
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

    window_state_t window_t::get_window_state() {
        window_state_t state;
        state.focused = glfwGetWindowAttrib(this->glfw_window, GLFW_FOCUSED);
        state.visible = glfwGetWindowAttrib(this->glfw_window, GLFW_VISIBLE);
        state.iconified = glfwGetWindowAttrib(this->glfw_window, GLFW_ICONIFIED);
        state.maximized = glfwGetWindowAttrib(this->glfw_window, GLFW_MAXIMIZED);
        state.floating = glfwGetWindowAttrib(this->glfw_window, GLFW_FLOATING);
        state.hovering = glfwGetWindowAttrib(this->glfw_window, GLFW_HOVERED);
        return state;
    }

    std::shared_ptr<texture_t> window_t::get_icon() {
        return icon;
    }

    cursor_mode_t window_t::get_cursor_mode() {
        return this->mode;
    }

    void window_t::close() {
        if (glfw_window == nullptr) {
            return;
        }
        glfwDestroyWindow(glfw_window);
        glfw_window = nullptr;

        if (glfw_initialized) {
            glfwTerminate();
            glfw_initialized = false;
        }

        std::string cxf = "close"; // does anyone know what cxf is?
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
