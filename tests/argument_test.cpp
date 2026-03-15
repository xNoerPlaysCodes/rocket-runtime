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
    rocket::register_argument("do-something-impressive", []() {
        std::cout << "okay but how\n";
    }, "Do something really impressive");
    rocket::register_argument("print-smth", [](std::string value) {
        std::cout << value << '\n';
    }, "Print Something", "str");
    rocket::init(argc, argv);
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }
    rocket::window_t window({1920, 1080}, "RocketGE - Argument Test");
    rocket::renderer_2d r(&window, 60, {.show_splash = !test_mode});

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
        }
        r.end_frame();
        window.poll_events();

        if (test_mode) return 0;
    }

    r.close();
    window.close();
}

#include "rocket/macros.hpp"
#ifdef ROCKETGE__Platform_Android
#include <android_native_app_glue.h>
#include <android/log.h>

extern "C" android_app *g_android_app = nullptr;

void android_main(android_app *app) {
    // convert to fake argc/argv for rocket::init
    const char* argv[] = { "rocketge", nullptr };
    int argc = 1;

    g_android_app = app;
    
    main(argc, (char**)argv);
}
#endif
