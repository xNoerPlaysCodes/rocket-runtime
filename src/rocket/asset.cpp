#include "../include/FontDefault.h"
#include "../../include/rocket/asset.hpp"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
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

#include "../include/stb_image.h"
#include "../include/stb_vorbis.h"

namespace rocket {
    bool openal_initialized;
    texture_t::texture_t() {}
    texture_t::~texture_t() {}

    audio_t::audio_t() {}
    
    void audio_t::play(float vol, bool loop, std::function<void(audio_t *)> on_finish) {
        if (this->playing) {
            rocket::log_error("audio is already playing", 1, "openal", "fatal-to-function");
            return;
        }

        // Delete previous source if still exists
        if (source != 0) {
            alDeleteSources(1, &source);
            source = 0;
        }

        alGenSources(1, &source);

        // Remove percentage
        vol = vol / 100.f;
        alSourcei(source, AL_BUFFER, *buffer);
        alSourcef(source, AL_GAIN, vol);

        alSourcei(source, AL_LOOPING, AL_FALSE);
        if (loop) {
            alSourcei(source, AL_LOOPING, AL_TRUE);
        }

        alSourcePlay(source);

        ALenum error = alGetError();
        if (error != AL_NO_ERROR) {
            rocket::log_error("failed to play audio: " + std::to_string(error), 1, "openal", "fatal-to-function");
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
            rocket::log_error("seek failed: could not reopen audio", 1, "openal", "fatal-to-function");
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

        if (cleanup_interval.count() != 0) {
            this->cleanup_thread = std::thread(&asset_manager_t::cleanup, this);
        }
        this->current_id = 0;
        this->cleanup_running = true;
    }

    assetid_t asset_manager_t::load_texture(std::string path, texture_color_format_t format) {
        assetid_t id = current_id++;
        std::shared_ptr<texture_t> texture = std::make_shared<texture_t>();
        texture->id = id;
        
        uint8_t *img_data = stbi_load(path.c_str(), &texture->size.x, &texture->size.y, &texture->channels, static_cast<int>(format));

        if (!img_data) {
            rocket::log_error("failed to load texture: " + path, 1, "stb_image::stbi_load", "fatal-to-function");
            current_id--;
            return -1;
        }
        
        if (format != texture_color_format_t::auto_extract) {
            texture->channels = static_cast<int>(format);
        }
        texture->data.assign(img_data, img_data + texture->size.x * texture->size.y * texture->channels); 

        textures.insert({texture, std::chrono::high_resolution_clock::now()});

        return id;
    }

    assetid_t asset_manager_t::load_texture(std::vector<uint8_t> data, texture_color_format_t format) {
        assetid_t id = current_id++;
        std::shared_ptr<texture_t> texture = std::make_shared<texture_t>();
        texture->id = id;

        uint8_t *file_data = data.data();
        size_t len = data.size();

        uint8_t *img_data = stbi_load_from_memory(file_data, len, &texture->size.x, &texture->size.y, &texture->channels, static_cast<int>(format));
        if (img_data == nullptr) {
            std::stringstream address;
            address << file_data << '\n';
            rocket::log_error("failed to load texture from memory, address: " + address.str(), 1, "stb_image::stbi_load", "fatal-to-function");

            current_id--;
            return -1;
        }
        if (format != texture_color_format_t::auto_extract) {
            texture->channels = static_cast<int>(format);
        }
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
                rocket::log_error("failed to open OpenAL device", 1, "OpenAL::alcOpenDevice", "fatal-to-function");
                return -1;
            }

            ALCcontext *context = alcCreateContext(device, nullptr);
            if (!context || !alcMakeContextCurrent(context)) {
                rocket::log_error("failed to create OpenAL context", 1, "OpenAL::alcCreateContext", "fatal-to-function");
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
            rocket::log_error("failed to generate OpenAL buffer", 1, "OpenAL::alGenBuffers", "fatal-to-function");
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
            rocket::log_error("failed to load " + path, 1, "stb_vorbis::stb_vorbis_open_filename", "fatal-to-function");
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
            rocket::log_error("failed to load audio properly: " + path, 1, "OpenAL::alBufferData", "fatal-to-function");
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

    assetid_t asset_manager_t::load_audio(std::vector<uint8_t> mem) {
        if (!openal_initialized) {
            ALCdevice *device = alcOpenDevice(nullptr); // Default device
            if (!device) {
                rocket::log_error("failed to open OpenAL device", 1, "OpenAL::alcOpenDevice", "fatal-to-function");
                return -1;
            }

            ALCcontext *context = alcCreateContext(device, nullptr);
            if (!context || !alcMakeContextCurrent(context)) {
                rocket::log_error("failed to create OpenAL context", 1, "OpenAL::alcCreateContext", "fatal-to-function");
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
            rocket::log_error("failed to generate OpenAL buffer", 1, "OpenAL::alGenBuffers", "fatal-to-function");
            delete audio->buffer;
            audio->buffer = nullptr;
            current_id--;
            return -1;
        }

        int channels, samplerate;
        short *output;
        int samples;

        stb_vorbis* vorbis = stb_vorbis_open_memory(mem.data(), mem.size(), nullptr, nullptr);
        if (!vorbis) {
            rocket::log_error("failed to load audio from [memory]", 1, "stb_vorbis::stb_vorbis_open_memory", "fatal-to-function");
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
            rocket::log_error("failed to load audio properly: [memory]", 1, "OpenAL::alBufferData", "fatal-to-function");
            alDeleteBuffers(1, audio->buffer);
            delete audio->buffer;
            audio->buffer = nullptr;
            current_id--;
            return -1;
        }

        audio->path = "[memory]";
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

    std::shared_ptr<font_t> font_t::font_default(int fsize) {
        static std::unordered_map<int, std::shared_ptr<font_t>> fonts;
        if (fonts.find(fsize) == fonts.end()) {
            std::shared_ptr<font_t> font = std::make_shared<font_t>();
            font->ttf_data = std::vector<uint8_t>(rocket_font::FontDefault, rocket_font::FontDefault + rocket_font::FontDefault_len);
            font->id = -1;
            font->size = fsize;
            std::vector<unsigned char> bitmap(font->sttex_size.x * font->sttex_size.y);
            stbtt_BakeFontBitmap(rocket_font::FontDefault, 0, fsize, bitmap.data(), font->sttex_size.x, font->sttex_size.y, 32, 96, font->cdata);

            stbtt_fontinfo info;
            if (!stbtt_InitFont(&info, rocket_font::FontDefault, 0)) {
                rocket::log_error("failed to init font", 1, "stbtt", "fatal-to-function");
                return nullptr;
            }
            int ascent, descent, line_gap;
            stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);

            // Convert from font units to pixels
            float scale = stbtt_ScaleForPixelHeight(&info, fsize);
            float line_height = (ascent - descent + line_gap) * scale;

            // Generate OpenGL texture
            glGenTextures(1, &font->glid);
            glBindTexture(GL_TEXTURE_2D, font->glid);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, font->sttex_size.x, font->sttex_size.y, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap.data());

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            font->line_height = line_height;
            fonts[fsize] = font;
        }

        return fonts[fsize];
    }

    assetid_t asset_manager_t::load_font(int fsize, std::vector<uint8_t> mem) {
        std::shared_ptr<font_t> font = std::make_shared<font_t>();
        font->id = current_id++;
        std::vector<unsigned char> bitmap(font->sttex_size.x * font->sttex_size.y);
        font->ttf_data = mem;
        stbtt_BakeFontBitmap(mem.data(), 0, fsize, bitmap.data(), font->sttex_size.x, font->sttex_size.y, 32, 96, font->cdata);

        stbtt_fontinfo info;
        if (!stbtt_InitFont(&info, rocket_font::FontDefault, 0)) {
            rocket::log_error("failed to init font", 1, "stbtt", "fatal-to-function");
            current_id--;
            return -1;
        }
        int ascent, descent, line_gap;
        stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);

        // Convert from font units to pixels
        float scale = stbtt_ScaleForPixelHeight(&info, fsize);
        float line_height = (ascent - descent + line_gap) * scale;

        // Generate OpenGL texture
        glGenTextures(1, &font->glid);
        glBindTexture(GL_TEXTURE_2D, font->glid);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, font->sttex_size.x, font->sttex_size.y, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap.data());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        font->line_height = line_height;

        fonts.insert({font, std::chrono::high_resolution_clock::now()});
        return font->id;
    }

    assetid_t asset_manager_t::load_font(int fsize, std::string path) {
        std::shared_ptr<font_t> font = std::make_shared<font_t>();
        font->id = current_id++;
        FILE* f = fopen(path.c_str(), "rb");
        if (!f) return false;

        fseek(f, 0, SEEK_END);
        int size = ftell(f);
        fseek(f, 0, SEEK_SET);

        std::vector<unsigned char> ttf_buffer(size);
        fread(ttf_buffer.data(), 1, size, f);
        fclose(f);

        font->ttf_data = ttf_buffer;

        std::vector<unsigned char> bitmap(font->sttex_size.x * font->sttex_size.y);
        stbtt_BakeFontBitmap(ttf_buffer.data(), 0, fsize, bitmap.data(), font->sttex_size.x, font->sttex_size.y, 32, 96, font->cdata);

        stbtt_fontinfo info;
        int ascent, descent, line_gap;
        stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);

        // Convert from font units to pixels
        float scale = stbtt_ScaleForPixelHeight(&info, fsize);
        float line_height = (ascent - descent + line_gap) * scale;

        // Generate OpenGL texture
        glGenTextures(1, &font->glid);
        glBindTexture(GL_TEXTURE_2D, font->glid);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, font->sttex_size.x, font->sttex_size.y, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap.data());

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        font->line_height = line_height;
        fonts.insert({font, std::chrono::high_resolution_clock::now()});
        return font->id;
    }

    std::shared_ptr<font_t> asset_manager_t::get_font(assetid_t id) {
        for (auto &[k, v] : fonts) {
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
