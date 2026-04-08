#include "rocket/asset.hpp"
#include "rocket/renderer.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"
#include <chrono>
#include <iostream>
#include <rocket/runtime.hpp>
#include <thread>

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

int rocket_main(int argc, char **argv, rocket_arguments_t) {
    LOGI("RocketMain");
    rocket::init(argc, argv);
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }

    rocket::window_t window = { {1280, 720}, "RocketGE - Triangle & Drawcall Test" };
    rocket::renderer_2d r(&window, 60, {
        .show_splash = !test_mode
    });

    float max_while_60_fps_hit = 1;
    bool fin = false;

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {
            if (r.get_framecount() > static_cast<size_t>(r.get_fps() / 4)) {
                if (r.get_current_fps() > (r.get_fps() <= 5 ? 0 : r.get_fps() / 2) && !fin) {
                    for (int i = 0; i < max_while_60_fps_hit; ++i) {
                        r.draw_rectangle({ {256, 256}, {128, 128} }, rocket::rgba_color::red());
                    }
                    max_while_60_fps_hit *= 1.05;
                } else {
                    fin = true;
                }
            }

            rocket::text_t fps = { fin ? ("Max: " + std::to_string(static_cast<int>(max_while_60_fps_hit)) + " rectangles") : "Testing...", 48, rocket::rgb_color::black(), rGE__FONT_DEFAULT_MONOSPACED };
            r.draw_text(fps, r.get_viewport_size() / 2 - fps.measure() / 2);

            rocket::text_t text = { "Drawcalls: " + std::to_string(r.get_drawcalls()), 24, rocket::rgb_color::black(), rGE__FONT_DEFAULT_MONOSPACED };
            r.draw_text(text, { 10, 10 });
            r.draw_fps({ 10, 15 + 24 });
            
            if (test_mode && r.get_framecount() >= 10) {
                r.end_frame();
                window.poll_events();
                return 0;
            }
            if (fin && test_mode) {
                r.end_frame();
                window.poll_events();
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                return 0;
            };
        }
        r.end_frame();
        window.poll_events();
    }

    return 0;
}

DEFINE_PLATFORM_MAIN
