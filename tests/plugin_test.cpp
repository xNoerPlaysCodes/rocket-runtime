#include "rocket/renderer.hpp"
#include "rocket/runtime.hpp"
#include "rocket/plugin/plugin.hpp"
#include "rocket/window.hpp"
#include <filesystem>
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
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }
    rocket::init(argc, argv);    
    rocket::set_log_level(rocket::log_level_t::info);
    if (test_mode) {
#ifdef ROCKETGE__Platform_Windows
        return !std::filesystem::exists("resources/test.plugin.win32");
#else
        return !std::filesystem::exists("resources/test.plugin");
#endif
    }
#ifdef ROCKETGE__Platform_Windows
    auto plugin = rocket::load_plugin("resources/test.plugin.win32");
#else
    auto plugin = rocket::load_plugin("resources/test.plugin");
#endif
    if (plugin != nullptr) {
        void (*my_test)() = reinterpret_cast<void(*)()>(plugin->get_function("my_test"));

        my_test();
    } else {
        rocket::log("you didn't let it load the plugin!", "main.cpp", "main", "warn");
    }

    rocket::window_t window = { { 1280, 720 }, "RocketGE - Plugin Test" };
    rocket::renderer_2d r(&window, 60, {.show_splash = !test_mode});

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
            // plugin renders first
            r.draw_rectangle({
                .pos = { 100, 300 },
                .size = { 100, 100 }
            }, rocket::rgba_color::red());
            // or use a plugin-specific drawing callback
        }
        r.end_frame();
        window.poll_events();
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
