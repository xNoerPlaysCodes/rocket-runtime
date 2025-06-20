#ifndef ROCKETGE__ASSET_HPP
#define ROCKETGE__ASSET_HPP

#include "types.hpp"
#include <AL/al.h>
#include <chrono>
#include <functional>
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

        GLuint glid = 0;
        friend class asset_manager_t;
        friend class renderer_2d;
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

    class asset_manager_t {
    private:
        std::unordered_map<std::shared_ptr<texture_t>, std::chrono::time_point<std::chrono::high_resolution_clock>> textures;
        std::unordered_map<std::shared_ptr<audio_t>, std::chrono::time_point<std::chrono::high_resolution_clock>> audios;

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
    public:
        asset_manager_t(std::chrono::seconds cleanup_interval = std::chrono::seconds(0));
    public:
        void close();
        ~asset_manager_t();
    };
}

#endif
