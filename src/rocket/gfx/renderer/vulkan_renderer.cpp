#include "rocket/renderer.hpp"
#include <rocket/types.hpp>

namespace rocket {
    vulkan_renderer_2d::vulkan_renderer_2d(window_backend_i *, int, renderer_flags_t)
    {
    }
    
    vulkan_renderer_2d::gfx_chk_result vulkan_renderer_2d::check_graphics_settings(rocket::vec2f_t, rocket::vec2f_t) {
        return gfx_chk_result::drawable;
    }

    void vulkan_renderer_2d::draw_circle(rocket::vec2f_t pos, float radius, rocket::rgba_color color, int thickness) {}

    void vulkan_renderer_2d::draw_polygon(rocket::vec2f_t pos, float radius, rocket::rgba_color color, int segments, float rotation) {}

    void vulkan_renderer_2d::draw_pixel(rocket::vec2f_t pos, rocket::rgba_color color) {}

    void vulkan_renderer_2d::draw_rectangle(rocket::vec2f_t pos, rocket::vec2f_t size, rocket::rgba_color color, float rotation, float roundedness, bool lines) {}

    void vulkan_renderer_2d::begin_frame() {}

    void vulkan_renderer_2d::show_splash() {}

    void vulkan_renderer_2d::begin_render_mode(render_mode_t mode) {
    }

    render_cache_t* vulkan_renderer_2d::create_render_cache(std::function<void(renderer_2d_i*)> cb) {
        return nullptr;
    }

    void vulkan_renderer_2d::invalidate_render_cache(render_cache_t *c) {}

    void vulkan_renderer_2d::begin_render_cache(render_cache_t *c) {}

    void vulkan_renderer_2d::draw_render_cache(render_cache_t *c, rocket::vec2f_t pos, rocket::vec2f_t sz) {
    }
    
    void vulkan_renderer_2d::draw_render_cache(render_cache_t *c, rocket::fbounding_box bbox) {
    }

    void vulkan_renderer_2d::end_render_cache(render_cache_t *c) {
    }

    void vulkan_renderer_2d::destroy_render_cache(render_cache_t *&c) {
    }

    void vulkan_renderer_2d::clear(rocket::rgba_color color) {
    }

    void vulkan_renderer_2d::make_ready_texture(std::shared_ptr<rocket::texture_t> texture) {}

    void vulkan_renderer_2d::draw_texture(std::shared_ptr<rocket::texture_t> texture, rocket::fbounding_box rect, float rotation, float roundedness) {
    }

    void vulkan_renderer_2d::draw_atlas_texture(
        std::shared_ptr<rocket::texture_t> atlas,
        rocket::fbounding_box rect,              // quad position & size on screen
        rocket::vec2f_t sprite_pos_in_atlas,     // top-left pixel of sprite inside atlas
        rocket::vec2f_t sprite_size_in_atlas,    // width/height of the sprite inside atlas
        float rotation,
        float roundedness
    ) {
    }

    void vulkan_renderer_2d::draw_rectangle(rocket::fbounding_box rect, rocket::rgba_color color, float rotation, float roundedness, bool lines) {
    }
   
    void vulkan_renderer_2d::draw_text(const rocket::text_t& text_, rocket::vec2f_t position) {
    }

    void vulkan_renderer_2d::draw_shader(shader_t shader) {
    }

    void vulkan_renderer_2d::set_wireframe(bool x) {
    }

    void vulkan_renderer_2d::set_vsync(bool x) {
    }

    void vulkan_renderer_2d::set_fps(int fps) {
    }

    void vulkan_renderer_2d::set_graphics_settings(graphics_settings_t settings) {
    }

    void vulkan_renderer_2d::set_viewport_size(vec2f_t size) {
    }

    void vulkan_renderer_2d::set_viewport_offset(vec2f_t offset) {
    }

    void vulkan_renderer_2d::set_camera(camera_2d *cam) {
    }

    void vulkan_renderer_2d::draw_fps(vec2f_t pos) {
    }

    std::vector<rgba_color> vulkan_renderer_2d::get_framebuffer() {
        return {};
    }

    void vulkan_renderer_2d::push_framebuffer(const std::vector<rgba_color> &framebuffer) {
    }

    vec2f_t vulkan_renderer_2d::get_viewport_size() {
        return {};
    }

    void vulkan_renderer_2d::end_render_mode(render_mode_t mode) {
    }

    bool vulkan_renderer_2d::has_frame_began() {
        return this->frame_started;
    }

    bool vulkan_renderer_2d::has_frame_ended() {
        return !this->frame_started;
    }

    void vulkan_renderer_2d::end_frame() {
    }

    double vulkan_renderer_2d::get_delta_time() {
        return delta_time;
    }

    int vulkan_renderer_2d::get_fps() {
        return fps;
    }

    uint64_t vulkan_renderer_2d::get_framecount() {
        return this->frame_counter;
    }

    bool vulkan_renderer_2d::get_vsync() {
        return vsync;
    }

    bool vulkan_renderer_2d::get_wireframe() {
        return wireframe;
    }

    float vulkan_renderer_2d::get_current_fps() {
        return rgl::get_draw_metrics().avg_fps;
    }

    int vulkan_renderer_2d::get_drawcalls() {
        return rgl::get_frame_metrics().drawcalls;
    }

    rgl::draw_metrics_t vulkan_renderer_2d::get_draw_metrics() {
        return rgl::get_draw_metrics();
    }

    const graphics_settings_t &vulkan_renderer_2d::get_graphics_settings() {
        return this->graphics_settings;
    }

    api_object_t vulkan_renderer_2d::get_framebuffer_texture() {
        // TODO sometime else
        return {};
    }

    camera_2d* vulkan_renderer_2d::get_camera() {
        return nullptr;
    }

    glm::mat4 vulkan_renderer_2d::get_camera_matrix() {
        return {}; // FIXME return 
    };

    void vulkan_renderer_2d::begin_scissor_mode(rocket::fbounding_box rect) {
    }

    void vulkan_renderer_2d::begin_scissor_mode(rocket::vec2f_t pos, rocket::vec2f_t size) {
    }

    void vulkan_renderer_2d::begin_scissor_mode(float x, float y, float sx, float sy) {
    }

    void vulkan_renderer_2d::end_scissor_mode() {
    }

    void vulkan_renderer_2d::close() {
    }

    vulkan_renderer_2d::~vulkan_renderer_2d() {
        close();
    }
}

