#pragma once

#include <rocket/runtime.hpp>
#include <iostream>
#include <rocket/types.hpp>
#include <string>

int MakeTest(
        int argc, char **argv,
        std::string test_name, 
        std::function<bool()> test_condition,
        std::function<void()> frame_callback = nullptr,
        std::function<void()> pre_win_create = nullptr,
        std::function<void(rocket::window_t *win, rocket::renderer_2d *ren)> post_win_create = nullptr,
        rocket::vec2i_t size = { 1280, 720 }
) {
    rocket::init(argc, argv);
    bool test_mode = false;
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        test_mode = true;
    }

    if (pre_win_create != nullptr) pre_win_create();
    rocket::window_t window = { size, "RocketGE - " + test_name };
    rocket::renderer_2d r = { &window, 60, { .show_splash = !test_mode } };
    if (post_win_create != nullptr) post_win_create(&window, &r);

    while (window.is_running()) {
        r.begin_frame();
        r.clear();
        if (frame_callback != nullptr)
            frame_callback();
        r.end_frame();
        window.poll_events();

        if (test_mode)
            return test_condition();
    }

    return 0;
}
