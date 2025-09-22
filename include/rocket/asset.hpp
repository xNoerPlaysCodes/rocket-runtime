#ifndef ROCKETGE__ASSET_HPP
#define ROCKETGE__ASSET_HPP

#include "types.hpp"
#include "stb_truetype.h"
#include <AL/al.h>
#include <chrono>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <memory>
#include <GLFW/glfw3.h>

#define rGE__TEXTURE_CHANNEL_COUNT_RGBA     4
#define rGE__TEXTURE_CHANNEL_COUNT_RGB      3

#define rGE__FONT_DEFAULT                   nullptr
#define rGE__FONT_DEFAULT_MONOSPACED        std::shared_ptr<rocket::font_t>(reinterpret_cast<rocket::font_t*>(0x01))

namespace rocket {
    /// @brief Maps to OpenGL::Texture2D
    class texture_t {
    public:
        /// @brief Texture Format
        /// @modify Do not modify
        int channels;

        /// @brief Texture Raw Pixel Data
        /// @modify Do not modify (advanced)
        std::vector<uint8_t> data;
    public:
        /// @brief OpenGL Texture ID
        /// @modify Do not modify
        GLuint glid = 0;
        friend class asset_manager_t;
        friend class renderer_2d;
        friend class renderer_3d;
    public:
        /// @brief Texture Size
        /// @modify Do not modify
        vec2i_t size;
        /// @brief AssetID
        /// @modify Do not modify
        assetid_t id;
    public:
        texture_t();
    public:
        ~texture_t();
    };

    struct audio_context_t;

    class audio_t {
    private:
        std::shared_ptr<audio_context_t> context;
        ALuint *buffer = nullptr;
        ALuint source = 0;
        std::string path;

        friend class asset_manager_t;

        bool playing = false;
        std::thread finish_thread;
    public:
        /// @brief AssetID
        assetid_t id;
    private:
        /// @brief Set Audio Context
        /// @note Usually don't need to do this as
        /// @note asset manager does it
        void set_context(std::shared_ptr<audio_context_t> context);
    public:
        /// @brief Play an audio (async)
        /// @param vol Volume 0-100 [+100 is allowed]
        /// @param loop Loop audio
        /// @param on_finish Callback when audio finishes
        void play(float vol = 30.f, bool loop = false, std::function<void(audio_t *)> on_finish = nullptr);

        /// @brief If audio is already playing,
        /// @brief seeks the audio to the given time
        /// @brief since beginning
        void seek(std::chrono::milliseconds time);

        /// @brief Get time audio has played since beginning
        /// @note It is not cumulative of loops
        std::chrono::milliseconds get_time();
    public:
        audio_t();
    public:
        ~audio_t();
    };

    class text_t;
    class renderer_2d;

    class font_t {
    private:
        /// INNER
        GLuint glid = 0;
        rocket::vec2i_t sttex_size = { 512, 512 };
        stbtt_bakedchar cdata[96];
        std::vector<uint8_t> ttf_data;
        /// INNER
        float line_height;

        friend class renderer_2d;
        friend class asset_manager_t;
        friend class text_t;
    public:
        /// @brief The Font Size
        /// @modify Do not Modify
        float size;
    public:
        /// @brief AssetID
        assetid_t id;
    public:
        /// @brief Get Line Height
        float get_line_height() const;
    public:
        /// @brief Get the default font for a particular size
        /// @note Lazy-Loaded
        static std::shared_ptr<font_t> font_default(int fsize);

        /// @brief Get the default monospace font for a particular size
        /// @note Lazy-Loaded
        static std::shared_ptr<font_t> font_default_monospace(int fsize);
    public:
        font_t();
    public:
        void unload();
        ~font_t();
    };

    class text_t {
    private:
        float size = 0.f;
        rgb_color color;

        friend class renderer_2d;
    public:
        /// @brief The font to use
        std::shared_ptr<font_t> font;
        /// @brief Text
        std::string text;
    public:
        /// @brief Set text size
        /// @note Overriden by the font size
        void set_size(float size);
    public:
        /// @brief Get text size
        float get_size();
    public:
        /// @brief Get text measured dimensions
        vec2f_t measure();
    public:
        /// @brief Construct text
        /// @param text Text
        /// @param size Text Size
        /// @param color Text Color
        /// @param font Font, if nullptr then default will be used
        ///             if ptr == 0x01   then default monospaced font will be used
        text_t(std::string text, float size, rgb_color color, std::shared_ptr<font_t> font = rGE__FONT_DEFAULT);
    public:
        ~text_t();
    };

    enum class texture_color_format_t : int {
        auto_extract = 0,
        rgba = rGE__TEXTURE_CHANNEL_COUNT_RGBA,
        rgb = rGE__TEXTURE_CHANNEL_COUNT_RGB
    };

    class asset_manager_t {
    private:
        std::unordered_map<std::shared_ptr<texture_t>, std::chrono::time_point<std::chrono::high_resolution_clock>> textures;
        std::unordered_map<std::shared_ptr<audio_t>, std::chrono::time_point<std::chrono::high_resolution_clock>> audios;
        std::unordered_map<std::shared_ptr<font_t>, std::chrono::time_point<std::chrono::high_resolution_clock>> fonts;

        std::thread cleanup_thread;
        assetid_t current_id;

        bool cleanup_running;
        std::mutex asset_mutex;

        std::shared_ptr<audio_context_t> audio_context;

        std::chrono::seconds cleanup_interval;
    private:
        friend class std::thread;
        void cleanup();
    public:
        /// @brief Load a Texture2D from path
        assetid_t load_texture(std::string path, texture_color_format_t format = texture_color_format_t::auto_extract);
        /// @brief Load a Texture2D from memory
        assetid_t load_texture(std::vector<uint8_t> mem, texture_color_format_t format = texture_color_format_t::auto_extract);
        /// @brief Get a Texture2D from ID
        std::shared_ptr<texture_t> get_texture(assetid_t id);

        /// @brief Load an Audio from path
        assetid_t load_audio(std::string path);
        /// @brief Load an Audio from Memory
        assetid_t load_audio(std::vector<uint8_t> mem);
        /// @brief Get an Audio from ID
        std::shared_ptr<audio_t> get_audio(assetid_t id);

        /// @brief Load a Font
        assetid_t load_font(int size, std::string path);
        /// @brief Load a Font from memory
        assetid_t load_font(int fsize, std::vector<uint8_t> mem);
        /// @brief Get a Font from ID
        std::shared_ptr<font_t> get_font(assetid_t id);
    public:
        /// @brief Construct Asset Manager
        /// @param cleanup_interval How often to clean up unused assets
        /// @param-note If set to 0, cleanup is disabled
        asset_manager_t(std::chrono::seconds cleanup_interval = std::chrono::seconds(0));
    public:
        void close();
        ~asset_manager_t();
    };
}

#endif
