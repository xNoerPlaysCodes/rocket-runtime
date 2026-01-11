#include "../include/FontDefault.h"
#include "rocket/audio.hpp"
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
#include "rgl.hpp"

// should i add this?
/*
 *          // file name :: asset id
 * std::unordered_map<std::string, assetid_t> load_all_textures(std::string path, std::string extension, bool recursive) {
 *      texmap = std::unordered_map<std::string, assetid_t>();
 *      for (file_it : dir_it_rec) {
 *          file = file_it.file();
 *          if (extension == ".png") {
 *              id = load_texture(file);
 *              texmap[fs_absolute_path(file).string()] = id;
 *          }
 *      }
 *      
 *      return texmap;
 * }
*/

#include <AL/al.h>
#include <AL/alc.h>

template<typename T>
using asset_map_t = std::unordered_map<std::shared_ptr<T>, std::chrono::time_point<std::chrono::high_resolution_clock>>;

#include "util.hpp"

using namespace std::chrono_literals;

#include "../include/stb_image.h"
#define STB_VORBIS_HEADER_ONLY
#include "../include/stb_vorbis.h"

namespace rocket {
    bool texture_t::is_ready() {
        return this->glid != 0;
    }
    texture_t::texture_t() {}
    texture_t::~texture_t() {}

    audio_t::audio_t() {}
    
    void audio_t::play(float vol, bool loop, std::function<void(audio_t *)> on_finish) {
        if (this->playing) {
            rocket::log_error("audio is already playing", "openal", "error");
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
            rocket::log_error("failed to play audio: " + std::to_string(error), "openal", "error");
            return;
        }

        this->playing = true;

        if (on_finish == nullptr) {
            return;
        }

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
            rocket::log_error("seek failed: could not reopen audio", "openal", "error");
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
            this->cleanup_running = true;
            this->cleanup_thread = std::thread(&asset_manager_t::cleanup, this);
            this->cleanup_thread.detach();
        }
        this->current_id = 0;
    }

    assetid_t asset_manager_t::load_texture(std::string path, texture_color_format_t format) {
        assetid_t id = current_id++;
        std::shared_ptr<texture_t> texture = std::make_shared<texture_t>();
        texture->id = id;
        
        uint8_t *img_data = stbi_load(path.c_str(), &texture->size.x, &texture->size.y, &texture->channels, static_cast<int>(format));

        if (!img_data) {
            rocket::log_error("failed to load texture: " + path, "stb_image::stbi_load", "error");
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
            rocket::log_error("failed to load texture from memory, address: " + address.str(), "stb_image::stbi_load", "error");

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

    std::shared_ptr<texture_t> asset_manager_t::get_texture(assetid_t id) {
        for (auto &[k, v] : textures) {
            if (k->id == id) {
                return k;
            }
        }
        return nullptr;
    }

    static bool openal_initialized = false;

    void asset_manager_t::init_audio_ctx() {
        if (!openal_initialized) {
            ALCdevice *device = alcOpenDevice(nullptr); // Default device
            if (!device) {
                rocket::log_error("failed to open OpenAL device", "OpenAL::alcOpenDevice", "error");
                return;
            }

            ALCcontext *context = alcCreateContext(device, nullptr);
            if (!context || !alcMakeContextCurrent(context)) {
                rocket::log_error("failed to create OpenAL context", "OpenAL::alcCreateContext", "error");
                if (context) alcDestroyContext(context);
                alcCloseDevice(device);
                return;
            }

            openal_initialized = true;
            rocket::log("Initialized audio context", "asset_manager_t", "init_audio_ctx", "info");
        }
    }

    void destroy_audio_ctx(void) {
        ALCcontext *ctx = alcGetCurrentContext();
        ALCdevice *dvc = alcGetContextsDevice(ctx);
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(ctx);
        alcCloseDevice(dvc);
    }

    assetid_t asset_manager_t::load_sound(std::string path) {
        assetid_t id = current_id++;
        std::shared_ptr<audio::sound_t> sound = std::make_shared<audio::sound_t>();
        sound->id = id;

        int channels, samplerate;
        int16_t *output;
        int samples;

        stb_vorbis *vorbis = stb_vorbis_open_filename(path.c_str(), nullptr, nullptr);
        if (vorbis == nullptr) {
            rocket::log_error("failed to load sound file from path: " + path, "asset_manager_t::load_sound", "error");
            current_id--;
            return -1;
        }

        stb_vorbis_info info = stb_vorbis_get_info(vorbis);
        channels = info.channels;
        samplerate = info.sample_rate;
        samples = stb_vorbis_stream_length_in_samples(vorbis);

        output = new int16_t[samples * channels];

        int num_samples = stb_vorbis_get_samples_short_interleaved(vorbis, channels, output, samples * channels);
        stb_vorbis_close(vorbis);

        sound->buffer.samples = std::vector<int16_t>(output, output + num_samples * channels);
        sound->buffer.sample_rate = samplerate;
        sound->buffer.format = channels == 1 ? audio::format_t::mono16 : audio::format_t::stereo16;

        delete[] output;

        sounds.insert({sound, std::chrono::high_resolution_clock::now()});
        return id;
    }

    // [FIXME] add loading sound from memory
    assetid_t asset_manager_t::load_sound(std::vector<uint8_t> mem) {
        assetid_t id = current_id++;
        std::shared_ptr<audio::sound_t> sound = std::make_shared<audio::sound_t>();
        sound->id = id;

        rocket::log_error("not implemented", "asset_manager_t::load_sound", "fixme");

        sounds.insert({sound, std::chrono::high_resolution_clock::now()});
        return id;
    }
    
    assetid_t asset_manager_t::load_audio(std::string path) {
        init_audio_ctx();

        assetid_t id = current_id++;
        std::shared_ptr<audio_t> audio = std::make_shared<audio_t>();
        audio->id = id;

        audio->buffer = new ALuint;
        alGenBuffers(1, audio->buffer);

        ALenum error = alGetError();
        if (error != AL_NO_ERROR) {
            rocket::log_error("failed to generate OpenAL buffer", "OpenAL::alGenBuffers", "error");
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
            rocket::log_error("failed to load " + path, "stb_vorbis::stb_vorbis_open_filename", "error");
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
            rocket::log_error("failed to load audio properly: " + path, "OpenAL::alBufferData", "error");
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
        init_audio_ctx();

        assetid_t id = current_id++;
        std::shared_ptr<audio_t> audio = std::make_shared<audio_t>();
        audio->id = id;

        audio->buffer = new ALuint;
        alGenBuffers(1, audio->buffer);

        ALenum error = alGetError();
        if (error != AL_NO_ERROR) {
            rocket::log_error("failed to generate OpenAL buffer", "OpenAL::alGenBuffers", "error");
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
            rocket::log_error("failed to load audio from [memory]", "stb_vorbis::stb_vorbis_open_memory", "error");
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
            rocket::log_error("failed to load audio properly: [memory]", "OpenAL::alBufferData", "error");
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

    std::shared_ptr<audio::sound_t> asset_manager_t::get_sound(assetid_t id) {
        for (auto &[k, v] : sounds) {
            if (k->id == id) {
                return k;
            }
        }
        return nullptr;
    }

    static std::unordered_map<int, std::shared_ptr<font_t>> fonts_default;
    static std::unordered_map<int, std::shared_ptr<font_t>> fonts_monospaced;

    void asset_manager_t::__rst_fonts() {
        std::unordered_map<int, std::shared_ptr<font_t>>().swap(fonts_default);
        std::unordered_map<int, std::shared_ptr<font_t>>().swap(fonts_monospaced);
    }

    std::shared_ptr<font_t> font_t::font_default(int fsize) {
        if (fonts_default.find(fsize) == fonts_default.end()) {
            std::shared_ptr<font_t> font = std::make_shared<font_t>();
            font->ttf_data = std::vector<uint8_t>(rocket_font::FontDefault, rocket_font::FontDefault + rocket_font::FontDefault_len);
            font->id = -1;
            font->size = fsize;
            std::vector<unsigned char> bitmap(font->sttex_size.x * font->sttex_size.y);
            stbtt_BakeFontBitmap(rocket_font::FontDefault, 0, fsize, bitmap.data(), font->sttex_size.x, font->sttex_size.y, 32, 96, font->cdata);

            stbtt_fontinfo info;
            if (!stbtt_InitFont(&info, rocket_font::FontDefault, 0)) {
                rocket::log_error("failed to init font", "stbtt", "error");
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
            fonts_default[fsize] = font;
        }

        return fonts_default[fsize];
    }

    std::shared_ptr<font_t> font_t::font_default_monospace(int fsize) {
        if (fonts_monospaced.find(fsize) == fonts_monospaced.end()) {
            std::shared_ptr<font_t> font = std::make_shared<font_t>();
            font->ttf_data = std::vector<uint8_t>(rocket_font::FontDefault_Monospace_ttf, rocket_font::FontDefault_Monospace_ttf + rocket_font::FontDefault_Monospace_ttf_len);
            font->id = -1;
            font->size = fsize;
            std::vector<unsigned char> bitmap(font->sttex_size.x * font->sttex_size.y);
            stbtt_BakeFontBitmap(rocket_font::FontDefault_Monospace_ttf, 0, fsize, bitmap.data(), font->sttex_size.x, font->sttex_size.y, 32, 96, font->cdata);

            stbtt_fontinfo info;
            if (!stbtt_InitFont(&info, rocket_font::FontDefault_Monospace_ttf, 0)) {
                rocket::log_error("failed to init font", "stbtt", "error");
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
            fonts_monospaced[fsize] = font;
        }

        return fonts_monospaced[fsize];
    }

    assetid_t asset_manager_t::load_font(int fsize, std::vector<uint8_t> mem) {
        std::shared_ptr<font_t> font = std::make_shared<font_t>();
        font->id = current_id++;
        std::vector<unsigned char> bitmap(font->sttex_size.x * font->sttex_size.y);
        font->ttf_data = mem;
        stbtt_BakeFontBitmap(mem.data(), 0, fsize, bitmap.data(), font->sttex_size.x, font->sttex_size.y, 32, 96, font->cdata);

        stbtt_fontinfo info;
        if (!stbtt_InitFont(&info, rocket_font::FontDefault, 0)) {
            rocket::log_error("failed to init font", "stbtt", "error");
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
        if (!stbtt_InitFont(&info, ttf_buffer.data(), 0)) {
            rocket::log_error("failed to init font", "stbtt", "error");
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

    std::shared_ptr<font_t> asset_manager_t::get_font(assetid_t id) {
        for (auto &[k, v] : fonts) {
            if (k->id == id) {
                return k;
            }
        }
        return nullptr;
    }

    void asset_manager_t::cleanup() {
        this->__thread_cleanup_running = true;
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
                    }
                }
            }

            for (auto &tx : texture_removes) {
                glDeleteTextures(1, &tx->glid);
                tx->glid = 0;
                tx->data.clear();
            }

            {
                std::lock_guard<std::mutex> _(asset_mutex);
                for (auto &k : texture_removes) {
                    textures.erase(k);
                }
            }

            std::vector<std::shared_ptr<font_t>> font_removes;
            font_removes.reserve(fonts.size());

            {
                std::lock_guard<std::mutex> _(asset_mutex);
                for (auto &[k, v] : fonts) {
                    if (now - v > cleanup_interval) {
                        font_removes.push_back(k);
                    }
                }
            }

            for (auto &fnt : font_removes) {
                fnt->unload();
            }

            {
                std::lock_guard<std::mutex> _(asset_mutex);
                for (auto &k : font_removes) {
                    fonts.erase(k);
                }
            }

            std::vector<std::shared_ptr<audio_t>> audio_removes;
            audio_removes.reserve(audios.size());

            {
                std::lock_guard<std::mutex> _(asset_mutex);
                for (auto &[k, v] : audios) {
                    if (now - v > cleanup_interval) {
                        audio_removes.push_back(k);
                    }
                }
            }

            for (auto &a : audio_removes) {
                alDeleteBuffers(1, a->buffer);
                delete a->buffer;
                a->playing = false;
                alDeleteSources(1, &a->source);
                a->source = 0;
            }

            {
                std::lock_guard<std::mutex> _(asset_mutex);
                for (auto &k : audio_removes) {
                    audios.erase(k);
                }
            }

            std::vector<std::shared_ptr<audio::sound_t>> sound_removes;
            sound_removes.reserve(sounds.size());

            {
                std::lock_guard<std::mutex> _(asset_mutex);
                for (auto &[k, v] : sounds) {
                    if (now - v > cleanup_interval) {
                        sound_removes.push_back(k);
                    }
                }
            }

            for ([[maybe_unused]] auto &s : sound_removes) {
                // Ownership required
            }

            {
                std::lock_guard<std::mutex> _(asset_mutex);
                for (auto &k : sound_removes) {
                    sounds.erase(k);
                }
            }
        }

        this->__thread_cleanup_running = false;
    }

    void asset_manager_t::close() {
        if (this->cleanup_running) {
            cleanup_running = false;
            rocket::log("Waiting for cleanup thread...", "asset_manager_t", "close", "info");
            while (this->__thread_cleanup_running) {}
        }
        destroy_audio_ctx();
    }

    asset_manager_t::~asset_manager_t() {
        this->close();
    }
}
