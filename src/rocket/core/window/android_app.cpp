#include <util.hpp>
#define SDL_AUDIO_DRIVER_DUMMY 1
#include "rocket/window.hpp"
#include "window.hpp"
#include <rocket/io.hpp>
#include <rocket/modularity/window_backend.hpp>
#include <rocket/runtime.hpp>
#include <internal_types.hpp>
#include <rocket/window_helpers.hpp>
#include <intl_macros.hpp>
#include "rocket/macros.hpp"
#include "intl_macros.hpp"

#ifdef ROCKETGE__Platform_Android
#include <android/log.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>
#include <android/input.h>
#include <EGL/egl.h>
#include <GLES3/gl32.h>

#define LOG_TAG "RocketGE"

extern android_app *g_android_app;
rocket::android_app_impl_t *g_android_app_impl = nullptr;

#include <EGL/egl.h>
#include <GLES3/gl32.h>

#endif

#ifndef EGL_GL_COLORSPACE_KHR
#define EGL_GL_COLORSPACE_KHR           0x309D
#endif
#ifndef EGL_GL_COLORSPACE_SRGB_KHR
#define EGL_GL_COLORSPACE_SRGB_KHR      0x3089
#endif

namespace rocket {
    void window::android_cpl_init() {
    }

    platform_t android_app_t::get_platform() const {
        platform_t platform;
        platform.name = "ANativeWindow";
        platform.rge_name = std::string("Android::Embedded");
        platform.os_name = "Android";
        platform.type = platform_type_t::android;
        return platform;
    }

    void android_app_t::set_vsync(bool vsync) {
#ifdef ROCKETGE__Platform_Android
        eglSwapInterval(this->impl->display, vsync ? 1 : 0);
#endif
    }

    android_app_t::android_app_t(const rocket::vec2i_t &size, const std::string &title, windowflags_t flags) {
        this->handle = new native_window_t;
        this->handle->backend = window_backend_t::android;
        this->wbi_impl = new window_backend_i_impl_t;
        this->wbi_impl->obj = this;
        this->impl = new android_app_impl_t;
        
        native_window_t::set_instance(this->handle);

        this->size = size;
        this->title = title;
        this->flags = flags;
#ifdef ROCKETGE__Platform_Android
        ANativeWindow* native_window = g_android_app->window;

        this->impl->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        eglInitialize(this->impl->display, nullptr, nullptr);

        EGLint attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
            EGL_RED_SIZE,        8,
            EGL_GREEN_SIZE,      8,
            EGL_BLUE_SIZE,       8,
            EGL_ALPHA_SIZE,      8,
            EGL_NONE
        };

        EGLConfig config;
        EGLint numConfigs;
        eglChooseConfig(this->impl->display, attribs, &config, 1, &numConfigs);
        

        EGLint contextAttribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, 3,
            EGL_NONE
        };

        this->impl->context = eglCreateContext(this->impl->display, config, EGL_NO_CONTEXT, contextAttribs);
        g_android_app_impl = this->impl;

        // sRGB surface if supported
        const char* egl_extensions = eglQueryString(this->impl->display, EGL_EXTENSIONS);
        bool has_colorspace = egl_extensions && strstr(egl_extensions, "EGL_KHR_gl_colorspace");

        if (has_colorspace) {
            EGLint srgb_attribs[] = {
                EGL_GL_COLORSPACE_KHR, EGL_GL_COLORSPACE_SRGB_KHR,
                EGL_NONE
            };
            this->impl->surface = eglCreateWindowSurface(this->impl->display, config, native_window, srgb_attribs);
            
        } else {
            this->impl->surface = eglCreateWindowSurface(this->impl->display, config, native_window, nullptr); 
        }

        eglMakeCurrent(this->impl->display, this->impl->surface, this->impl->surface, this->impl->context);

        SDL_Init(SDL_INIT_EVENTS | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC);
        this->running = true;

        g_android_app->onInputEvent = [](android_app* app, AInputEvent* event) -> int32_t {
            int32_t type = AInputEvent_getType(event);

            if (type == AINPUT_EVENT_TYPE_MOTION) {
                int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
                float x = AMotionEvent_getX(event, 0);
                float y = AMotionEvent_getY(event, 0);
                float nx = x;
                float ny = y;

            if (action == AMOTION_EVENT_ACTION_DOWN) {
                util::set_last_touch_state(rocket::io::keystate_t::make_down());
                util::set_last_touch_pos({x, y});
            } else if (action == AMOTION_EVENT_ACTION_UP) {
                util::set_last_touch_state(rocket::io::keystate_t::make_released());
                util::set_last_touch_pos({x, y});
            } else if (action == AMOTION_EVENT_ACTION_MOVE) {
                util::set_last_touch_pos({x, y});
            }
                return 1;

            } else if (type == AINPUT_EVENT_TYPE_KEY) {
                int32_t action = AKeyEvent_getAction(event);
                int32_t keycode = AKeyEvent_getKeyCode(event);
                rocket::io::key_event_t key_event;
                key_event.scancode = keycode;
                key_event.state = (action == AKEY_EVENT_ACTION_DOWN)
                    ? rocket::io::keystate_t::make_down()
                    : rocket::io::keystate_t::make_released();
                util::dispatch_event(key_event);
                return 1;
            }

            return 0;
        };

        this->impl->config = config;

        g_android_app->onAppCmd = [](android_app *app, int32_t cmd) {
            if (!g_android_app_impl || !g_android_app_impl->obj)
                return;

            auto window = g_android_app_impl->obj;

            switch (cmd) {
                case APP_CMD_INIT_WINDOW:
                    if (app->window)
                        window->create_surface();
                    break;

                case APP_CMD_TERM_WINDOW:
                    window->destroy_surface();
                    break;

                case APP_CMD_GAINED_FOCUS:
                    window->resume_surface();
                    break;

                case APP_CMD_LOST_FOCUS:
                    window->pause_surface();
                    break;
            }
        };

#endif

        rocket::log("Window created as [" + std::to_string(size.x) + "x" + std::to_string(size.y) + "]: " + title, 
            "glfw_window_t", "constructor", 
            "info");
        auto platform = get_platform();
        std::string windowing_platform = platform.name;

        std::vector<std::string> logs = {
            "RocketGE v" ROCKETGE__VERSION,
            "Windowing: Android + " + windowing_platform,
            "Platform: " + platform.rge_name,
        };

        for (auto &l : logs) {
            rocket::log(l, "glfw_window_t", "constructor", "info");
        }
        this->poll_events();
    }

    void android_app_t::set_window_state(window_state_t state) const {
    }

    void android_app_t::set_icon(std::shared_ptr<rocket::texture_t> texture) {
    }

    void android_app_t::set_cursor_mode(cursor_mode_t m) {
    }

    void android_app_t::set_cursor_position(const rocket::vec2d_t& pos) const {
    }

    void android_app_t::swap_buffers() const {
#ifdef ROCKETGE__Platform_Android
        if (this->impl->surface != EGL_NO_SURFACE)
            eglSwapBuffers(this->impl->display, this->impl->surface);
#endif
    }

    void android_app_t::set_size(const rocket::vec2i_t& size) {
        this->size = size;
    }

    void android_app_t::set_title(const std::string& title) {
        this->title = title;
    }

    void android_app_t::register_on_close(std::function<void()> listener) {
    }

    void android_app_t::poll_events() {
#ifdef ROCKETGE__Platform_Android
        EGLint w, h;
        eglQuerySurface(this->impl->display, this->impl->surface, EGL_WIDTH, &w);
        eglQuerySurface(this->impl->display, this->impl->surface, EGL_HEIGHT, &h);
        if (w != this->size.x || h != this->size.y) {
            this->size = { w, h };
        }

        if (g_android_app) {
            int events;
            android_poll_source* source;
            while (ALooper_pollOnce(0, nullptr, &events, (void**)&source) >= 0) {
                if (source) source->process(g_android_app, source);
                if (g_android_app->destroyRequested) {
                    this->running = false;
                    return;
                }
            }
        }
#endif

        util::io_update_end_frame();
    }

    std::string android_app_t::get_title() const {
        return this->title;
    }

    rocket::vec2i_t android_app_t::get_size() const {
        return this->size;
    }

    rocket::vec2d_t android_app_t::get_cursor_position() const {
        return rocket::io::mouse_pos();
    }

    bool android_app_t::is_running() const {
        return this->running;
    }

    double android_app_t::get_time() const {
        return static_cast<double>(SDL_GetTicks64()) / 1000.;
    }

    window_state_t android_app_t::get_window_state() const {
        return {};
    }

    std::shared_ptr<texture_t> android_app_t::get_icon() const {
        return nullptr;
    }

    cursor_mode_t android_app_t::get_cursor_mode() const {
        return {};
    }

    void android_app_t::close() {
        rocket::log("Window closed", "android_app_t", "destructor", "info");

        if (this->handle != nullptr) {
            delete this->handle;
            this->handle = nullptr;
        }

        if (this->wbi_impl != nullptr) {
            delete this->wbi_impl;
            this->wbi_impl = nullptr;
        }
    }

    android_app_t::~android_app_t() {
        destructor_called = true;
        close();
        destructor_called = false;
    }

    void android_app_t::destroy_surface() {
#ifdef ROCKETGE__Platform_Android
        if (!this->impl->surface)
            return;

        if (this->impl->display != EGL_NO_DISPLAY) {
            if (!eglMakeCurrent(this->impl->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT)) {
                int err = eglGetError();
                rocket::log("eglMakeCurrent failed: " + std::to_string(err), "android_app_t", "destroy_surface", "error");
            }

            if (!eglDestroySurface(this->impl->display, this->impl->surface)) {
                int err = eglGetError();
                rocket::log("eglDestroySurface failed: " + std::to_string(err), "android_app_t", "destroy_surface", "error");
            }
        }

        this->impl->surface = EGL_NO_SURFACE;

        if (g_android_app->window) {
            ANativeWindow_release(g_android_app->window);
            g_android_app->window = nullptr;
        }
#endif
    }

    void android_app_t::create_surface() {
#ifdef ROCKETGE__Platform_Android
        ANativeWindow* native_window = g_android_app->window;
        if (!native_window) return;

        const char* egl_extensions =
            eglQueryString(this->impl->display, EGL_EXTENSIONS);

        bool has_colorspace =
            egl_extensions && strstr(egl_extensions, "EGL_KHR_gl_colorspace");

        if (has_colorspace) {
            EGLint srgb_attribs[] = {
                EGL_GL_COLORSPACE_KHR,
                EGL_GL_COLORSPACE_SRGB_KHR,
                EGL_NONE
            };

            this->impl->surface = eglCreateWindowSurface(
                this->impl->display,
                this->impl->config,
                native_window,
                srgb_attribs
            );
        } else {
            this->impl->surface = eglCreateWindowSurface(
                this->impl->display,
                this->impl->config,
                native_window,
                nullptr
            );
        }

        eglMakeCurrent(
            this->impl->display,
            this->impl->surface,
            this->impl->surface,
            this->impl->context
        );
#endif
    }

    void android_app_t::pause_surface() {
        // this->running = false;
    }

    void android_app_t::resume_surface() {
        // this->running = true;
    }

#ifdef ROCKETGE__Platform_Android
    r_static monitor_t monitor_t::with_cursor() {
        return monitor_t::of(0);
    }

    r_static int monitor_t::get_count() {
        int count = 1;
        return count;
    }

    r_static monitor_t monitor_t::of(int idx) {
        monitor_t m;
        ANativeWindow *w = g_android_app->window;
        int32_t width = ANativeWindow_getWidth(w);
        int32_t height= ANativeWindow_getHeight(w);

        m.size = { width, height };
        m.bluebits = 8;
        m.redbits = 8;
        m.greenbits = 8;
        m.refreshrate = 60; // TODO Get from Choreographer Callback?
        return m;
    }
#endif
}
