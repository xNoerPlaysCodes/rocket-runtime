#ifndef ROCKETGE__AUDIO_HPP
#define ROCKETGE__AUDIO_HPP

#include "rocket/types.hpp"
#include <AL/alc.h>
#include <AL/al.h>
#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>

struct stb_vorbis;

namespace rocket::audio {
    enum class capabilities_t {
        mono16 = 0, stereo16
    };

    enum class format_t {
        mono16 = 0, stereo16
    };

    class device_t {
    private:
        ALCdevice *handle = nullptr;
        friend class sound_engine_t;
    public:
        std::string name;
        std::vector<capabilities_t> capabilities;
    public:
        static device_t *get_default();

        void open();
    };

    std::vector<device_t> get_devices();

    struct buffer_t {
    private:
        [[maybe_unused]]
        ALuint handle = 0;
    public:
        format_t format;
        std::vector<int16_t> samples;
        int sample_rate;
    };

    struct listener_t {
        vec3f_t position;
        vec3f_t velocity;
        vec3f_t forward;
        vec3f_t up;

        void apply();
    };

    struct sound_t {
    private:
        [[maybe_unused]]
        ALuint handle = 0;
        [[maybe_unused]]
        bool flat_2d = true;
    public:
        assetid_t id = -1;
    public:
        buffer_t buffer;
    };

    struct source_t {
        bool in_use = false;
        ALuint source = 0;
    };

    using sound_finish_callback_t = std::function<void()>;

    struct streaming_sound_t {
    private:
        stb_vorbis *vorbis = nullptr;
        std::array<ALuint, 4> buffers;
        friend class sound_engine_t;
    public:
        bool loop = false;
        sound_finish_callback_t cb;
        std::string file_path;
        int frames_loaded = 0;
        int samples_per_frame = 0;
        buffer_t current_buffer_to_play;
        source_t *source;
    };

    class sound_engine_t {
    private:
        device_t *device = nullptr;
        ALCcontext *ctx;
        std::array<source_t, 32> sources = {{}};
        std::vector<std::shared_ptr<streaming_sound_t>> streaming_sounds;
    public:
        void set_device(device_t *device);
    public:
        void play(sound_t &sound, bool loop = false, sound_finish_callback_t = nullptr);
        void play(std::shared_ptr<streaming_sound_t> sound, bool loop = false, sound_finish_callback_t = nullptr);
        std::shared_ptr<streaming_sound_t> stream(std::string file_path, bool loop = false, sound_finish_callback_t = nullptr);

        void update_music_streams();
    public:
        sound_engine_t(device_t *device);
    public:
        ~sound_engine_t();
    };
}

#endif//ROCKETGE__AUDIO_HPP
