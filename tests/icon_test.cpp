#include "../include/rocket/runtime.hpp"
#include "rocket/asset.hpp"
#include "rocket/renderer.hpp"
#include "rocket/rgl.hpp"
#include "rocket/window.hpp"
#include "rocket/macros.hpp"

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
#ifdef ROCKETGE__Platform_Linux
    // --------------------------------------
    // wayland doesn't support window icons!
    // --------------------------------------
    // wayland also needs a valid framebuffer
    // before showing the window
    // --------------------------------------
    rocket::window::set_forced_platform(rocket::platform_type_t::linux_x11);
#endif
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }
    rocket::init(argc, argv);
    rocket::window_t window = { { 1280, 720 }, "RocketGE - Window Icon Test"};
    rocket::renderer_2d r = { &window, 60, { .show_splash = !test_mode } };
    rocket::asset_manager_t asset_manager;
    rocket::assetid_t texture = asset_manager.load_texture("resources/window_icon.jpg", rocket::texture_color_format_t::rgba);
    window.set_icon(asset_manager.get_texture(texture));

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        r.end_frame();
        window.poll_events();
        if (test_mode) {
            return 0;
        }
    }

    window.close();
    return 0;
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
