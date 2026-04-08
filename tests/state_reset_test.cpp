#include "../include/rocket/runtime.hpp"
#include "rocket/asset.hpp"
#include "rocket/io.hpp"
#include "rocket/renderer.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"
#include <iostream>

#include "rocket/macros.hpp"
#ifdef ROCKETGE__Platform_Android
#include <android/log.h>

#define LOG_TAG "RocketGE"

// Log levels: ANDROID_LOG_DEBUG, ANDROID_LOG_INFO, ANDROID_LOG_WARN, ANDROID_LOG_ERROR
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGI(...) (void)0
#define LOGE(...) (void)0
#define LOGD(...) (void)0
#endif


int main(int argc, char **argv) {
    rocket::init(argc, argv);
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }
    rocket::window_t window({1920, 1080}, "RocketGE - State Reset Test");
    rocket::renderer_2d r(&window, 60, {.show_splash = true});

    rocket::text_t text = {"Hello, Rocket World!", 48, rocket::rgb_color::black()};

    bool error = false;
    rocket::set_opengl_error_callback([&error, test_mode] (std::string, std::string, int, std::string, std::string) {
        error = true;
        std::exit(1);
    });

    int i = 0;
    while (window.is_running() && i < 20) {
        r.begin_frame();
        r.clear();
        {}
        r.end_frame();
        window.poll_events();
        ++i;
    }

    r.close();
    window.close();

    i = 0;
    {
        rocket::window_t w2 = {};
        rocket::renderer_2d r2 = &w2;
        while (w2.is_running() && (!test_mode || i < 2)) {
            r2.begin_frame();
            r2.clear();
            {
                r2.draw_rectangle({ {0,0}, {240,240} }, rocket::rgba_color::red());
            }
            r2.end_frame();
            w2.poll_events();

            ++i;
        }
    }

    return error ? 1 : 0;
}
#include "rocket/macros.hpp"
#ifdef ROCKETGE__Platform_Android
#include <android_native_app_glue.h>
#include <android/log.h>

extern "C" android_app *g_android_app = nullptr;

__attribute__((constructor)) static void on_library_load() {
    __android_log_print(ANDROID_LOG_INFO, "RocketGE", "Library loaded!");
}

extern "C" void android_main(android_app *app) {
    __android_log_print(ANDROID_LOG_INFO, "RocketGE", "android_main called!");
    // convert to fake argc/argv for rocket::init
    const char* argv[] = { "rocketge", nullptr };
    int argc = 1;

    LOGI("ANDROID_MAIN");

    g_android_app = app;
    app->onAppCmd = [](android_app* app, int32_t cmd) {
        __android_log_print(ANDROID_LOG_INFO, "RocketGE", "CMD: %d", cmd);
    };

    while (app->window == nullptr) {
        int events;
        android_poll_source* source;
        ALooper_pollOnce(100, nullptr, &events, (void**)&source);
        if (source) source->process(app, source);
        if (app->destroyRequested) return;
        LOGI("Waiting for window...");
    }
    
    main(argc, (char**)argv);
}
#endif
