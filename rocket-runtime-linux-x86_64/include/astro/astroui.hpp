#ifndef ASTRO__AstroUI__HPP
#define ASTRO__AstroUI__HPP

#include "../rocket/runtime.hpp"
#include "../rocket/renderer.hpp"
#include <memory>

namespace astro {
    void set_renderer(rocket::renderer_2d *renderer);

    /// @brief Does NOT call rocket::renderer_2d::begin_frame()
    void begin_ui();

    /// @brief Sets font for all future drawcalls
    void set_font(int size, std::shared_ptr<rocket::font_t> font);

    struct border_t {
        float width = 0;
        float radius = 0;
        rocket::rgba_color color = { 0, 0, 0, 0 };
    };

    struct draw_info_t {
        rocket::rgba_color color;
        rocket::rgb_color text_color;
        border_t border;
    };

    class button_t {
    public:
        std::string text = "";
        float text_offset = 0;
        rocket::vec2f_t pos = { 0, 0 };
        rocket::vec2f_t size = { 0, 0 };

        bool last_frame_was_held = true;
    public:
        void draw(draw_info_t &info);
    public:
        bool is_hovering();
        bool is_clicking(bool hold = false, rocket::io::mouse_button btn = rocket::io::mouse_button::left);
        bool is_released(rocket::io::mouse_button btn = rocket::io::mouse_button::left);
    public:
        button_t();
        button_t(std::string text, rocket::vec2f_t pos, rocket::vec2f_t size);
    public:
        ~button_t();
    };

    enum class field_type_t {
        text,
        number,
        special_character,
    };

    class input_field_t {
    public:
        std::string text = "";
        std::string placeholder = "";
        rocket::vec2f_t pos = { 0, 0 };
        rocket::vec2f_t size = { 0, 0 };

        bool last_frame_was_held = true;
    public:
        void draw(draw_info_t &info);
    public:
        void update_field_io(std::vector<field_type_t> types = { field_type_t::text, field_type_t::number, field_type_t::special_character });
        void update_field_io(std::vector<int> printable_ascii_codes_allowed);
    public:
        bool is_hovering();
        bool is_clicking(bool hold = false, rocket::io::mouse_button button = rocket::io::mouse_button::left);
        bool is_released(rocket::io::mouse_button button = rocket::io::mouse_button::left);

        bool is_submit();
    public:
        input_field_t();
        input_field_t(std::string placeholder, rocket::vec2f_t pos, rocket::vec2f_t size);
    public:
        ~input_field_t();
    };

    class dialog_t {
    private:
        std::string text = "";
        rocket::vec2f_t pos = { 0, 0 };
        rocket::vec2f_t tpos = { 0, 0 };
        rocket::vec2f_t size = { 0, 0 };

        button_t* button_left = nullptr;
        button_t* button_right = nullptr;
    private:
        void draw_self(draw_info_t &info);
    public:
        void draw(draw_info_t &info);
        void draw(draw_info_t &info, draw_info_t &btnleftinfo);
        void draw(draw_info_t &info, draw_info_t &btnleftinfo, draw_info_t &btnrightinfo);
    public:
        /// @brief text only dialog
        dialog_t(std::string text, rocket::vec2f_t tpos, rocket::vec2f_t pos, rocket::vec2f_t size);
        
        /// @brief single-button dialog
        dialog_t(std::string text, rocket::vec2f_t tpos, rocket::vec2f_t pos, rocket::vec2f_t size, button_t* button);

        /// @brief double-button dialog
        dialog_t(std::string text, rocket::vec2f_t tpos, rocket::vec2f_t pos, rocket::vec2f_t size, button_t* button_left, button_t* button_right);
    public:
        ~dialog_t();
    };

    /// @brief Does NOT call rocket::renderer_2d::end_frame()
    void end_ui();
}

#endif // ASTRO__AstroUI__HPP
