#include "rocket/asset.hpp"
#include "rocket/renderer.hpp"
#include "rocket/rgl.hpp"
#include <chrono>
#include <rocket/runtime.hpp>
#include "rocket/types.hpp"
#include "rocket/window.hpp"
#include <rocket/threads.hpp>
#include <sstream>
#include <string>
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


int main(int argc, char **argv) {
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }
    rocket::init(argc, argv);
    rocket::window_t window({ 1280, 720 }, "RocketGE - Multithreaded Test", {
    });
    rocket::renderer_2d r(&window, 60, {
        .show_splash = !test_mode
    });

    rocket::asset_manager_t am;
    std::shared_ptr<rocket::texture_t> tx = nullptr;

    std::thread ([&] () {
        std::string thread_id = std::to_string(rocket::thread_t::get_thread_id());
        rocket::log("simulating real work...", "thread", thread_id, "info");
        int32_t foo = 0;
        while (foo != 2147483647) foo++;

        // Load the file from disk on any thread
        tx = am.get_texture(am.load_texture("resources/window_icon.jpg"));
        rocket::log("Loaded texture", "thread", thread_id, "info");

        // Load the texture on the main thread
        rocket::thread_t::schedule([&r, &tx]() {
            std::string thread_id = std::to_string(rocket::thread_t::get_thread_id());
            rocket::log("Readying texture...", "thread", thread_id, "info");
            r.make_ready_texture(tx);
            rocket::log("Ready texture", "thread", thread_id, "info");
        });
    }).detach();

    rocket::thread_t::run([]() {
        rocket::log("Hello, from a new thread", "thread", std::to_string(rocket::thread_t::get_thread_id()), "info");
    });

    std::chrono::time_point<std::chrono::high_resolution_clock> start = std::chrono::high_resolution_clock::now();

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        {       
            if (tx != nullptr && tx->is_ready()) {
                r.draw_texture(tx, { {250, 250}, {256,256} });
            }
            r.draw_rectangle({ 100, 100 }, { 256, 256 }, rocket::rgba_color::red());
            r.draw_fps(); // [FIXME] not rendering text when calling GL functions from another thread directly
        }
        r.end_frame();
        window.poll_events();
        auto end = std::chrono::high_resolution_clock::now();
        if (test_mode) {
            bool has_been_5_seconds = (end - start) >= std::chrono::seconds(5);
            if (has_been_5_seconds && tx == nullptr) {
                return 1;
            }

            if (tx != nullptr) return 0;
        }
    }

    window.close();
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
