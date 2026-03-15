#include "rocket/asset.hpp"
#include "rocket/io.hpp"
#include "rocket/renderer.hpp"
#include <rocket/window.hpp>
#include <rocket/runtime.hpp>
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


std::string bool_to_str(bool b) {
    return b ? "true" : "false";
}

int main(int argc, char **argv) {
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }
    rocket::init(argc, argv);
    rocket::window_t window = { { 1280, 720 }, "RocketGE - IO Test" };
    rocket::renderer_2d r(&window, 60, {.show_splash = !test_mode});

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
            std::vector<std::string> strs = {};
            strs.reserve(8);
            if (rocket::io::mouse_down(rocket::io::mouse_button::left)) {
                strs.push_back("LMB");
            } if (rocket::io::mouse_down(rocket::io::mouse_button::right)) {
                strs.push_back("RMB");
            } if (rocket::io::mouse_down(rocket::io::mouse_button::middle)) {
                strs.push_back("MMB");
            } if (rocket::io::mouse_down(rocket::io::mouse_button::button_4)) {
                strs.push_back("B4");
            } if (rocket::io::mouse_down(rocket::io::mouse_button::button_5)) {
                strs.push_back("B5");
            } if (rocket::io::mouse_down(rocket::io::mouse_button::button_6)) {
                strs.push_back("B6");
            } if (rocket::io::mouse_down(rocket::io::mouse_button::button_7)) {
                strs.push_back("B7");
            } if (rocket::io::mouse_down(rocket::io::mouse_button::button_8)) {
                strs.push_back("B8");
            }

            rocket::text_t t = { "", 32, rocket::rgb_color::black(), rGE__FONT_DEFAULT_MONOSPACED };
            for (auto &s : strs) {
                t.text += s + " ";
            }
            if (t.text.empty()) {
                t.text = "Press any mouse key to test";
            }
            r.draw_text(t, r.get_viewport_size() / 2.f - t.measure() / 2.f);
            r.draw_fps();
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
