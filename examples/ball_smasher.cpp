#include "rocket/asset.hpp"
#include "rocket/audio.hpp"
#include "rocket/io.hpp"
#include "rocket/renderer.hpp"
#include "rocket/rgl.hpp"
#include "rocket/shader.hpp"
#include "rocket/types.hpp"
#include "rocket/window.hpp"
#include "rocket/window_helpers.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <rocket/runtime.hpp>
#include <astro/astroui.hpp>
#include <unordered_map>

rocket_arguments_t g_system_arguments;

rocket::assetid_t g_arrow_left;
rocket::assetid_t g_arrow_right;
rocket::assetid_t g_hit_sound;

struct pair_hash {
    size_t operator()(const std::pair<int, int>& p) const {
        size_t h1 = std::hash<int>{}(p.first);
        size_t h2 = std::hash<int>{}(p.second);
        return h1 ^ (h2 << 32);  // simple combine
    }
};

bool g_game_ended = false;

std::unordered_map<std::pair<int, int>, bool, pair_hash> skip_positions;

rocket::vec2f_t ball = {
};

rocket::fbounding_box paddle = {
};

float paddle_x;

rocket::render_cache_t *cache = nullptr;

void draw_game(rocket::window_t &window, rocket::renderer_2d &ren, rocket::asset_manager_t &am, rocket::audio::sound_engine_t &sound_engine) {
    static auto last_vp_size = ren.get_viewport_size();
    const auto vp_size = ren.get_viewport_size();

    paddle.pos.x = paddle_x;

    static bool first_init = true;

    if (vp_size.x < paddle.size.x) return;

    rocket::vec2f_t box_size = { 40, 40 };

    static rocket::vec2f_t ball_velocity;

    if (first_init) {
        first_init = false;
        cache = ren.create_render_cache([box_size](rocket::renderer_2d *ren) {
            auto vp_size = ren->get_viewport_size();
            for (int y = 0; y < vp_size.y / (2*box_size.y); ++y) {
                for (int x = 0; x < vp_size.x / box_size.x; ++x) {
                    if (skip_positions.find({ x, y }) == skip_positions.end()) {
                        ren->draw_rectangle({ { x * box_size.x, y * box_size.y }, box_size }, rocket::rgba_color::green());
                        ren->draw_rectangle({ { x * box_size.x, y * box_size.y }, box_size }, rocket::rgba_color::black(), 0, 0, true);
                    }
                }
            }
        });

        int rand1 = rand() % 2;
        int rand2 = rand() % 2;

        if (rand1 == 0) {
            ball_velocity.x = -1;
        } else {
            ball_velocity.x = 1;
        }

        if (rand2 == 0) {
            ball_velocity.y = -1;
        } else {
            ball_velocity.y = 1;
        }
    }

    static auto last = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last);

    ren.draw_render_cache(cache, {0, 0}, vp_size);

    if (last_vp_size != vp_size)
        ren.invalidate_render_cache(cache);

    auto paddle_draw = paddle;
    paddle_draw.size.y = 4;
    ren.draw_rectangle(paddle_draw, rocket::rgba_color::red());

    paddle_x = std::clamp(paddle_x, 0.f, vp_size.x - paddle.size.x);

    constexpr float REF_WIDTH = 1280.f;
    constexpr float REF_HEIGHT = 720.f;

    // Scale factor relative to reference
    float scale_x = vp_size.x / REF_WIDTH;
    float scale_y = vp_size.y / REF_HEIGHT;
    float scale = (scale_x + scale_y) / 2.f;

    rocket::vec2f_t ctrl_size = { 144, 144 };
    rocket::vec2f_t ctrl1_pos = { 32, vp_size.y - ctrl_size.y - 32 };
    rocket::vec2f_t ctrl2_pos = { vp_size.x - ctrl_size.x - 32, vp_size.y - ctrl_size.y - 32 };
    int thickness1 = 4;
    int thickness2 = 4;
    rocket::fbounding_box mouse = { rocket::io::mouse_pos_f(), {1,1} };
    bool down = rocket::io::mouse_state(rocket::io::mouse_button::finger, rocket::io::keystate_t::make_down());

    if (
        (mouse.intersects({ctrl1_pos, ctrl_size}) && down)
        || rocket::io::key_down(rocket::io::keyboard_key::a)
    ) {
        paddle_x -= 10 * (scale) * 60.f * ren.get_delta_time();
        thickness1 = 0;
    }

    if (
        (mouse.intersects({ctrl2_pos, ctrl_size}) && down)
        || rocket::io::key_down(rocket::io::keyboard_key::d)
    ) {
        paddle_x += 10 * (scale) * 60.f * ren.get_delta_time();
        thickness2 = 0;
    }

    ren.draw_texture(am.get_texture(g_arrow_left), { ctrl1_pos, ctrl_size });
    ren.draw_circle({ ctrl1_pos.x + ctrl_size.x / 2 + 2, ctrl1_pos.y + ctrl_size.y / 2 }, (ctrl_size.x / 2) + 4, rocket::rgba_color::black(), thickness1);
    ren.draw_texture(am.get_texture(g_arrow_right), { ctrl2_pos, ctrl_size });
    ren.draw_circle({ ctrl2_pos.x + ctrl_size.x / 2 - 2, ctrl2_pos.y + ctrl_size.y / 2 }, (ctrl_size.x / 2) + 4, rocket::rgba_color::black(), thickness2);

    constexpr float radius = 12;
    ren.draw_circle(ball, radius, rocket::rgba_color::black());

    for (int y = 0; y < vp_size.y / (2*box_size.y); ++y) {
        for (int x = 0; x < vp_size.x / box_size.x; ++x) {
            if (skip_positions.find({ x, y }) == skip_positions.end()) {
                rocket::fbounding_box rect = { { x * box_size.x, y * box_size.y }, box_size };
                rocket::vec2f_t center = ball - radius;
                rocket::fbounding_box ball_sz = { center, { radius * 2, radius * 2 } };
                if (rect.intersects(ball_sz)) {
                    sound_engine.play(*am.get_sound(g_hit_sound), false, [](){}, 50.f);
                    skip_positions[{x, y}] = true;
                    ren.invalidate_render_cache(cache);

                    // Figure out which side was hit
                    float ball_cx = ball.x;
                    float ball_cy = ball.y;
                    float rect_cx = rect.pos.x + rect.size.x / 2;
                    float rect_cy = rect.pos.y + rect.size.y / 2;

                    float dx = ball_cx - rect_cx;
                    float dy = ball_cy - rect_cy;

                    // Whichever axis has less overlap = that's the side hit
                    if (std::abs(dx) > std::abs(dy)) {
                        ball_velocity.x = -ball_velocity.x;  // hit left or right side
                    } else {
                        ball_velocity.y = -ball_velocity.y;  // hit top or bottom
                    }
                }
            }
        }
    }

    static bool was_colliding_paddle = false;
    static bool was_colliding_top    = false;
    static bool was_colliding_left   = false;
    static bool was_colliding_right  = false;

    auto collide = [&](rocket::fbounding_box rect, bool& was_colliding) {
        rocket::vec2f_t center = ball - radius;
        rocket::fbounding_box ball_sz = { center, { radius * 2, radius * 2 } };

        if (rect.intersects(ball_sz)) {
            if (!was_colliding) {
                float dx = ball.x - (rect.pos.x + rect.size.x / 2);
                float dy = ball.y - (rect.pos.y + rect.size.y / 2);

                if (std::abs(dx) > std::abs(dy)) {
                    ball_velocity.x = -ball_velocity.x;
                } else {
                    ball_velocity.y = -ball_velocity.y;
                }
                was_colliding = true;
            }
        } else {
            was_colliding = false;
        }
    };

    auto collide_x = [&](rocket::fbounding_box rect, bool& was_colliding) {
        rocket::vec2f_t center = ball - radius;
        rocket::fbounding_box ball_sz = { center, { radius * 2, radius * 2 } };
        if (rect.intersects(ball_sz)) {
            if (!was_colliding) {
                ball_velocity.x = -ball_velocity.x;
                was_colliding = true;
            }
        } else {
            was_colliding = false;
        }
    };

    auto collide_y = [&](rocket::fbounding_box rect, bool& was_colliding) -> bool {
        rocket::vec2f_t center = ball - radius;
        rocket::fbounding_box ball_sz = { center, { radius * 2, radius * 2 } };
        if (rect.intersects(ball_sz)) {
            if (!was_colliding) {
                ball_velocity.y = -ball_velocity.y;
                was_colliding = true;
            }
            return true;
        } else {
            was_colliding = false;
        }

        return false;
    };

    rocket::fbounding_box scr_top = { { 0,-95 }, vp_size.x, 100 };
    rocket::fbounding_box scr_left = { { -95,0 }, 100, vp_size.y };
    rocket::fbounding_box scr_right = { { vp_size.x - 5,0 }, {100, vp_size.y} };

    ren.draw_rectangle(scr_top, rocket::rgba_color::black());
    ren.draw_rectangle(scr_left, rocket::rgba_color::black());
    ren.draw_rectangle(scr_right, rocket::rgba_color::black());

    int substeps = 16;
    float sub_dt = ren.get_delta_time() / substeps;

    if (elapsed >= std::chrono::seconds(3)) {
        for (int i = 0; i < substeps; i++) {
            ball = ball + ball_velocity * 60.f * sub_dt * 4;

            collide(paddle,       was_colliding_paddle);  // keep dx/dy for paddle
            collide_y(scr_top,    was_colliding_top);     // top/bottom = flip y
            collide_x(scr_left,   was_colliding_left);    // left/right = flip x
            collide_x(scr_right,  was_colliding_right);
        }
    }

    rocket::vec2f_t center = ball - radius;

    if (center.y + radius * 2 > vp_size.y) {
        g_game_ended = true;
    }

    last_vp_size = vp_size;
}

int rocket_main(int argc, char **argv, rocket_arguments_t args) {
    srand(time(nullptr));
    bool no_shader = false;
    rocket::register_argument("noshader", [&no_shader]() {
        no_shader = true;
    }, "Disable Post Processing Shaders");

    rocket::init(argc, argv);
    g_system_arguments = args;
    rocket::window_t window({1280, 720}, "rgeExample - Ball Smasher", {
        .msaa_samples = 4,
    });
    rocket::renderer_2d ren(&window, 60, {
        .show_splash = false
    });
    rocket::asset_manager_t am;

    g_arrow_left = am.load_texture(args.working_dir + "resources/arrow_left.png");
    g_arrow_right = am.load_texture(args.working_dir + "resources/arrow_right.png");
    g_hit_sound = am.load_sound(args.working_dir + "resources/hit.ogg");

    rgl::fbo_t fbo = rgl::create_fbo();
    rgl::use_fbo(fbo);
    rgl::gl_viewport({0,0}, ren.get_viewport_size());
    rgl::gl_clear(rgl::gl_color_buffer_bit | rgl::gl_depth_buffer_bit, { 0, 255, 0, 255 });
    rgl::reset_to_default_fbo();

    bool fullscreen_set = false;

    rocket::audio::sound_engine_t sound_engine(rocket::audio::device_t::get_default());

    astro::set_renderer(&ren);

    rocket::shader_t shader(rocket::shader_type::vert_frag, args.working_dir + "resources/postprocess.rlsl");
    while (window.is_running()) {
        static rocket::vec2f_t last_vp_size = ren.get_viewport_size();
        if (!fullscreen_set) {
            ren.begin_frame();
            ren.clear();
            {
                rocket::text_t text = { "Ensure you are in fullscreen for best experience", 40, rocket::rgb_color::black(), rGE__FONT_DEFAULT };
                auto center = ren.get_viewport_size() / 2;
                ren.draw_text(text, center - text.measure() / 2);

                astro::begin_ui();
                constexpr rocket::vec2f_t button_size = { 150, 75 };
                auto bpos = center - button_size / 2;
                bpos.y += button_size.y + 30;
                astro::button_t button = { "Done", bpos, button_size };
                astro::draw_info_t info = {
                    .color = { 128, 128, 128, 255 },
                    .text_color = rocket::rgb_color::black(),
                    .border = {
                        .width = 0,
                        .radius = 0.25,
                        .color = {},
                    }
                };
                if (button.is_clicking(true)) {
                    info.color = rocket::rgba_color::black();
                    info.text_color = rocket::rgb_color::white();
                    fullscreen_set = true;
                    auto vp_size = ren.get_viewport_size();
                    ball = {
                        .x = vp_size.x / 2,
                        .y = (vp_size.y / 2) + 41,
                    };
                    paddle = {
                        .pos = {
                            .x = vp_size.x / 2.f - 80.f,
                            .y = (vp_size.y / 2) + (vp_size.y * 0.30f)
                        },
                        .size = {
                            .x = 160,
                            .y = 160,
                        }
                    };
                    paddle_x = vp_size.x / 2 - 80.f;
                    skip_positions.clear();
                } else if (button.is_hovering()) {
                    info.color = { 64, 64, 64, 255 };
                    info.text_color = rocket::rgb_color::white();
                } else {
                    info.color = { 128, 128, 128, 255 };
                    info.text_color = rocket::rgb_color::black();
                }
                button.draw(info);
                astro::end_ui();
            }
            ren.end_frame();
            window.poll_events();
            continue;
        }
        window.poll_events();
        ren.clear();
        ren.begin_frame();
        rgl::use_fbo(fbo);
        rgl::gl_viewport({0,0}, ren.get_viewport_size());
        ren.clear();
        {
            if (g_game_ended) {
                rocket::text_t text = { "Game Over!", 64, rocket::rgb_color::black(), rGE__FONT_DEFAULT_MONOSPACED };
                auto center = ren.get_viewport_size() / 2;
                ren.draw_text(text, center - text.measure() / 2);

                astro::begin_ui();
                constexpr rocket::vec2f_t button_size = { 150, 75 };
                auto bpos = center - button_size / 2;
                bpos.y += button_size.y + 30;
                astro::button_t button = { "Restart", bpos, button_size };
                astro::draw_info_t info = {
                    .color = { 128, 128, 128, 255 },
                    .text_color = rocket::rgb_color::black(),
                    .border = {
                        .width = 0,
                        .radius = 0.25,
                        .color = {},
                    }
                };
                if (button.is_clicking(true)) {
                    info.color = rocket::rgba_color::black();
                    info.text_color = rocket::rgb_color::white();
                    g_game_ended = false;
                    auto vp_size = ren.get_viewport_size();
                    ball = {
                        .x = vp_size.x / 2,
                        .y = (vp_size.y / 2) + 41,
                    };
                    paddle = {
                        .pos = {
                            .x = vp_size.x / 2.f - 80.f,
                            .y = (vp_size.y / 2) + (vp_size.y * 0.30f)
                        },
                        .size = {
                            .x = 160,
                            .y = 160,
                        }
                    };
                    paddle_x = vp_size.x / 2 - 80.f;
                    skip_positions.clear();
                    if (cache != nullptr) ren.invalidate_render_cache(cache);
                } else if (button.is_hovering()) {
                    info.color = { 64, 64, 64, 255 };
                    info.text_color = rocket::rgb_color::white();
                } else {
                    info.color = { 128, 128, 128, 255 };
                    info.text_color = rocket::rgb_color::black();
                }
                button.draw(info);
                astro::end_ui();
            } else {
                draw_game(window,  ren, am, sound_engine);
            }
        }
        rgl::reset_to_default_fbo();
        if (!no_shader) {
            static float accumulator = 0.f;
            accumulator += ren.get_delta_time();

            shader.set_uniform("u_time", accumulator);
            shader.set_uniform("u_resolution", ren.get_viewport_size());
            shader.set_uniform("u_aberration", 0.005f);
            ren.draw_fbo(fbo, shader);
        } else {
            ren.draw_fbo(fbo, {0,0}, ren.get_viewport_size());
        }
        auto vp_size = ren.get_viewport_size();
        rocket::text_t text = { "Score: " + std::to_string(skip_positions.size()), 32, rocket::rgb_color::black() };
        auto pos = vp_size / 2 - text.measure() / 2;
        pos.y = 0.8 * vp_size.y;
        ren.draw_text(text, pos);
        ren.draw_fps();
        ren.end_frame();
        if (last_vp_size != ren.get_viewport_size()) {
            rgl::delete_fbo(fbo);
            fbo = rgl::create_fbo();
            rgl::use_fbo(fbo);
            rgl::gl_viewport({0,0}, ren.get_viewport_size());
            rgl::gl_clear(rgl::gl_color_buffer_bit | rgl::gl_depth_buffer_bit, { 0, 255, 0, 255 });
            rgl::reset_to_default_fbo();
        }
        last_vp_size = ren.get_viewport_size();
    }

    return 0;
}

DEFINE_PLATFORM_MAIN
