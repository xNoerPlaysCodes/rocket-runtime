#include "rocket/asset.hpp"
#include "rocket/io.hpp"
#include "rocket/renderer.hpp"
#include "rocket/runtime.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"
#include <cstring>
#include <iostream>
#include <string>

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
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }
    rocket::init(argc, argv);
    rocket::window_t window = { {1280, 720}, "RocketGE - IO Simulation Test" };
    rocket::renderer_2d r(&window, 60, {.show_splash = !test_mode});
    rocket::text_t instructions = {
        "Press the Arrow Keys for WASD",
        24, rocket::rgb_color::black()
    };

    rocket::io::add_listener([](rocket::io::key_event_t e) {
        if (!e.state.down()) {
            return;
        }
    });

    while (window.is_running()) {
        r.begin_frame();
        r.clear();

        if (rocket::io::key_down(rocket::io::keyboard_key::up)) {
            rocket::io::simulate(rocket::io::keyboard_key::w, rocket::io::keystate_t::make_down());
        }

        if (rocket::io::key_down(rocket::io::keyboard_key::down)) {
            rocket::io::simulate(rocket::io::keyboard_key::s, rocket::io::keystate_t::make_down());
        }

        if (rocket::io::key_down(rocket::io::keyboard_key::left)) {
            rocket::io::simulate(rocket::io::keyboard_key::a, rocket::io::keystate_t::make_down());
        }

        if (rocket::io::key_down(rocket::io::keyboard_key::right)) {
            rocket::io::simulate(rocket::io::keyboard_key::d, rocket::io::keystate_t::make_down());
        }

        if (rocket::io::key_down(rocket::io::keyboard_key::w)) {
            rocket::log("Key: [W]", "main.cpp", "main", "info");
        }

        if (rocket::io::key_down(rocket::io::keyboard_key::s)) {
            rocket::log("Key: [S]", "main.cpp", "main", "info");
        }

        if (rocket::io::key_down(rocket::io::keyboard_key::a)) {
            rocket::log("Key: [A]", "main.cpp", "main", "info");
        }

        if (rocket::io::key_down(rocket::io::keyboard_key::d)) {
            rocket::log("Key: [D]", "main.cpp", "main", "info");
        }

        r.end_frame();
        window.poll_events();
        if (test_mode) return 0;
    }
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
