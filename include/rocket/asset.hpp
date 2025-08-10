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

namespace rocket {
    class texture_t {
    private:
        std::vector<uint8_t> data;
        int channels;
    public:
        GLuint glid = 0;
        friend class asset_manager_t;
        friend class renderer_2d;
        friend class renderer_3d;
    public:
        vec2i_t size;
        assetid_t id;
    public:
        texture_t();
    public:
        ~texture_t();
    };

    struct audio_context_t;

    class audio_t {
    private:
        assetid_t id;
        std::shared_ptr<audio_context_t> context;
        ALuint *buffer = nullptr;
        ALuint source = 0;
        std::string path;

        friend class asset_manager_t;

        bool playing = false;
        std::thread finish_thread;
    private:
        void set_context(std::shared_ptr<audio_context_t> context);
    public:
        /// @brief play an audio (async)
        /// @note vol is between 0, 100
        void play(float vol = 30.f, bool loop = false, std::function<void(audio_t *)> on_finish = nullptr);

        /// @brief if playing seeks to time
        void seek(std::chrono::milliseconds time);

        /// @brief get time it has played till
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
        assetid_t id;
        /// INNER
        GLuint glid = 0;
        vec2i_t sttex_size = { 512, 512 };
        stbtt_bakedchar cdata[96];
        std::vector<uint8_t> ttf_data;
        /// INNER
        float size;

        float line_height;

        friend class renderer_2d;
        friend class asset_manager_t;
        friend class text_t;
    public:
        float get_line_height() const;
    public:
        static std::shared_ptr<font_t> font_default(int fsize);
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
        std::shared_ptr<font_t> font;
        std::string text;
    public:
        void set_size(float size);
    public:
        float get_size();
    public:
        vec2f_t measure();
    public:
        /// @note if font is nullptr then internal default font will be used
        text_t(std::string text, float size, rgb_color color, std::shared_ptr<font_t> font = nullptr);
    public:
        ~text_t();
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
        assetid_t load_texture(std::string path);
        std::shared_ptr<texture_t> get_texture(assetid_t id);

        assetid_t load_audio(std::string path);
        std::shared_ptr<audio_t> get_audio(assetid_t id);

        assetid_t load_font(int size, std::string path);
        assetid_t load_font(int fsize, std::vector<uint8_t> mem);
        std::shared_ptr<font_t> get_font(assetid_t id);
    public:
        asset_manager_t(std::chrono::seconds cleanup_interval = std::chrono::seconds(0));
    public:
        void close();
        ~asset_manager_t();
    };
}

#endif
