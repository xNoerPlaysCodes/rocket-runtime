#include "../include/rocket/asset.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <iostream>

#include <AL/al.h>
#include <AL/alc.h>

template<typename T>
using asset_map_t = std::unordered_map<std::shared_ptr<T>, std::chrono::time_point<std::chrono::high_resolution_clock>>;

#define ROCKETRUNTIME_LogAssetRemoval(message) std::cout << message << std::endl;
#ifdef ROCKETRUNTIME_DEBUG
#undef ROCKETRUNTIME_LogAssetRemoval(message)
#define ROCKETRUNTIME_LogAssetRemoval(message)
#endif

#include "util.hpp"

using namespace std::chrono_literals;

#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"

#define STB_VORBIS_IMPLEMENTATION
#include "include/stb_vorbis.h"

namespace rocket {
    bool openal_initialized;
    texture_t::texture_t() {}
    texture_t::~texture_t() {}

    audio_t::audio_t() {}
    
    void audio_t::play(float vol, bool loop, std::function<void(audio_t *)> on_finish) {
        if (this->playing) {
            std::cout << util::format_error("audio is already playing", 1, "openal", "fatal-to-function") << std::endl;
            return;
        }

        // Delete previous source if still exists
        if (source != 0) {
            alDeleteSources(1, &source);
            source = 0;
        }

        alGenSources(1, &source);

        vol = std::clamp(vol, 0.f, 100.f) / 100.f;
        alSourcei(source, AL_BUFFER, *buffer);
        alSourcef(source, AL_GAIN, vol);

        alSourcei(source, AL_LOOPING, AL_FALSE);
        if (loop) {
            alSourcei(source, AL_LOOPING, AL_TRUE);
        }

        alSourcePlay(source);

        ALenum error = alGetError();
        if (error != AL_NO_ERROR) {
            std::cout << util::format_error("failed to play audio: " + std::to_string(error), 1, "openal", "fatal-to-function") << std::endl;
            return;
        }

        if (on_finish == nullptr) {
            return;
        }

        this->playing = true;

        // Wait until the audio actually starts playing
        ALint state;
        do {
            alGetSourcei(source, AL_SOURCE_STATE, &state);
            std::this_thread::sleep_for(16ms);
        } while (state != AL_PLAYING);

        // Start monitoring thread
        finish_thread = std::thread([this, on_finish, loop]() {
            ALint state;
            do {
                alGetSourcei(this->source, AL_SOURCE_STATE, &state);
                std::this_thread::sleep_for(16ms);
            } while (state == AL_PLAYING);

            if (!loop) {
                this->playing = false;
                on_finish(this);
            }
        });

        finish_thread.detach();
    }

    void audio_t::seek(std::chrono::milliseconds time) {
        if (!this->buffer) return;

        // Stop current playback if needed
        if (this->source) {
            alSourceStop(this->source);
        }

        // Re-decode using stb_vorbis
        stb_vorbis* vorbis = stb_vorbis_open_filename(this->path.c_str(), nullptr, nullptr);
        if (!vorbis) {
            std::cout << util::format_error("seek failed: could not reopen audio", 1, "openal", "fatal-to-function") << std::endl;
            return;
        }

        stb_vorbis_info info = stb_vorbis_get_info(vorbis);
        int channels = info.channels;
        int sample_rate = info.sample_rate;

        // Calculate sample index to seek to
        int sample_index = static_cast<int>((time.count() / 1000.0) * sample_rate);

        // Seek
        stb_vorbis_seek(vorbis, sample_index);

        // Read remaining samples
        int max_samples = stb_vorbis_stream_length_in_samples(vorbis) - sample_index;
        short* samples = new short[max_samples * channels];
        int actual_samples = stb_vorbis_get_samples_short_interleaved(vorbis, channels, samples, max_samples * channels);
        stb_vorbis_close(vorbis);

        // Rebuffer data
        ALenum format = (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
        alBufferData(*buffer, format, samples, actual_samples * sizeof(short) * channels, sample_rate);
        delete[] samples;

        // Play again from new position
        alSourcei(this->source, AL_BUFFER, *buffer);
        alSourcePlay(this->source);
    }

    std::chrono::milliseconds audio_t::get_time() {
        if (!this->playing) return 0ms;
        ALint position;
        alGetSourcei(this->source, AL_SEC_OFFSET, &position);
        return std::chrono::milliseconds(static_cast<int>(position) * 1000);
    }
    
    audio_t::~audio_t() {
        if (source != 0) {
            alDeleteSources(1, &source);
        }
        if (buffer) {
            alDeleteBuffers(1, buffer);
            delete buffer;
        }
    }


    struct audio_context_t {
        ALCdevice *device;
        ALCcontext *context;
    };

    asset_manager_t::asset_manager_t(std::chrono::seconds cleanup_interval) {
        this->cleanup_interval = cleanup_interval;

        this->cleanup_thread = std::thread(&asset_manager_t::cleanup, this);
        this->current_id = 0;
        this->cleanup_running = true;
    }

    assetid_t asset_manager_t::load_texture(std::string path) {
        assetid_t id = current_id++;
        std::shared_ptr<texture_t> texture = std::make_shared<texture_t>();
        texture->id = id;
        
        uint8_t *img_data = stbi_load(path.c_str(), &texture->size.x, &texture->size.y, &texture->channels, 0);

        if (!img_data) {
            std::cout << util::format_error("failed to load texture: " + path, 1, "stb_image", "fatal-to-function") << std::endl;
            current_id--;
            return -1;
        }
        
        texture->data.assign(img_data, img_data + texture->size.x * texture->size.y * texture->channels);

        textures.insert({texture, std::chrono::high_resolution_clock::now()});

        return id;
    }

    std::shared_ptr<texture_t> asset_manager_t::get_texture(assetid_t id) {
        for (auto &[k, v] : textures) {
            if (k->id == id) {
                return k;
            }
        }
        return nullptr;
    }

    
    assetid_t asset_manager_t::load_audio(std::string path) {
        if (!openal_initialized) {
            ALCdevice *device = alcOpenDevice(nullptr); // Default device
            if (!device) {
                std::cout << util::format_error("failed to open OpenAL device", 1, "openal", "fatal-to-function") << std::endl;
                return -1;
            }

            ALCcontext *context = alcCreateContext(device, nullptr);
            if (!context || !alcMakeContextCurrent(context)) {
                std::cout << util::format_error("failed to create OpenAL context", 1, "openal", "fatal-to-function") << std::endl;
                if (context) alcDestroyContext(context);
                alcCloseDevice(device);
                return - 1;
            }

            openal_initialized = true;
        }

        assetid_t id = current_id++;
        std::shared_ptr<audio_t> audio = std::make_shared<audio_t>();
        audio->id = id;

        audio->buffer = new ALuint;
        alGenBuffers(1, audio->buffer);

        ALenum error = alGetError();
        if (error != AL_NO_ERROR) {
            std::cout << util::format_error("failed to generate OpenAL buffer", 1, "openal", "fatal-to-function") << std::endl;
            delete audio->buffer;
            audio->buffer = nullptr;
            current_id--;
            return -1;
        }

        int channels, samplerate;
        short *output;
        int samples;

        stb_vorbis* vorbis = stb_vorbis_open_filename(path.c_str(), nullptr, nullptr);
        if (!vorbis) {
            std::cout << util::format_error("failed to load " + path, 1, "stb_vorbis", "fatal-to-function") << std::endl;
            delete audio->buffer;
            audio->buffer = nullptr;
            current_id--;
            return -1;
        }

        stb_vorbis_info info = stb_vorbis_get_info(vorbis);
        channels = info.channels;
        samplerate = info.sample_rate;

        samples = stb_vorbis_stream_length_in_samples(vorbis);
        output = new short[samples * channels];

        int numSamples = stb_vorbis_get_samples_short_interleaved(vorbis, channels, output, samples * channels);
        stb_vorbis_close(vorbis);

        ALenum format = (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

        alBufferData(*audio->buffer, format, output, numSamples * sizeof(short) * channels, samplerate);
        delete[] output;

        error = alGetError();
        if (error != AL_NO_ERROR) {
            std::cout << util::format_error("failed to load audio properly: " + path, 1, "openal", "fatal-to-function") << std::endl;
            alDeleteBuffers(1, audio->buffer);
            delete audio->buffer;
            audio->buffer = nullptr;
            current_id--;
            return -1;
        }

        audio->path = path;
        audios.insert({audio, std::chrono::high_resolution_clock::now()});
        return id;
    }

    std::shared_ptr<audio_t> asset_manager_t::get_audio(assetid_t id) {
        for (auto &[k, v] : audios) {
            if (k->id == id) {
                return k;
            }
        }
        return nullptr;
    }

    void asset_manager_t::cleanup() {
        std::this_thread::sleep_for(3s);
        while (cleanup_running && cleanup_interval.count() != 0) {
            std::this_thread::sleep_for(1s);

            std::chrono::time_point<std::chrono::high_resolution_clock> now = std::chrono::high_resolution_clock::now();

            std::vector<std::shared_ptr<texture_t>> texture_removes;
            texture_removes.reserve(textures.size());

            {
                std::lock_guard<std::mutex> _(asset_mutex);
                for (auto &[k, v] : textures) {
                    if (now - v > cleanup_interval) {
                        texture_removes.push_back(k);
                        ROCKETRUNTIME_LogAssetRemoval("Texture removed with id: " << k->id);
                    }
                }

                for (auto &k : texture_removes) {
                    textures.erase(k);
                }
            }
        }
    }

    void asset_manager_t::close() {
        cleanup_running = false;
    }

    asset_manager_t::~asset_manager_t() {
        this->close();
    }
}
