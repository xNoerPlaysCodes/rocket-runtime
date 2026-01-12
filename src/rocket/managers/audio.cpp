#include <cstring>
#include <iostream>
#include <memory>
#include <rocket/audio.hpp>
#include <thread>
#include "intl_macros.hpp"

#define ALC_DEVICE_NAME_DEFAULT nullptr
#include <rocket/runtime.hpp>

#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.h>

namespace rocket::audio {
    device_t default_dvc;
    bool dd_init = false;
    //   ^^ default_device

    RGE_STATIC_FUNC_IMPL device_t *device_t::get_default() {
        if (!dd_init) {
            dd_init = true;
            default_dvc.open();
        }

        return &default_dvc;
    }

    void device_t::open() {
        if (handle != nullptr) {
            return;
        }

        const char *dvc_name = ALC_DEVICE_NAME_DEFAULT;
        if (!this->name.empty()) {
            dvc_name = this->name.c_str();
        }

        this->handle = alcOpenDevice(dvc_name);

        if (this->name.empty()) {
            this->name = alcGetString(nullptr, ALC_DEVICE_SPECIFIER);
        }
    }

    source_t *fetch_source(std::array<rocket::audio::source_t, 32> &sources) {
        for (auto &source : sources) {
            if (!source.in_use) {
                source.in_use = true;
                return &source;
            }
        }

        return nullptr;
    }

    std::vector<device_t> get_devices() {
        std::vector<device_t> dvcl;

        if (!alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT")) {
            rocket::log_error("Device enumeration not supported by OpenAL implementation",
                              "OpenAL::alcIsExtensionPresent", "error");
            return dvcl;
        }

        // get the double-null terminated list of device names
        const ALCchar* devices = alcGetString(nullptr, ALC_ALL_DEVICES_SPECIFIER);
        if (!devices) return dvcl;

        // loop until we hit an empty string
        while (*devices) {
            device_t dvc;
            dvc.name = devices;
            dvc.capabilities = { capabilities_t::mono16, capabilities_t::stereo16 };
            dvc.open();  // you’re opening each device here — that’s fine if you really need it open

            dvcl.push_back(dvc);

            // move to next device name
            devices += std::strlen(devices) + 1;
        }

        return dvcl;
    }

    sound_engine_t::sound_engine_t(device_t *device) {
        this->device = device;
        rocket::log("Sound engine created with device: " + device->name, "sound_engine_t", "constructor", "info");
        this->ctx = alcCreateContext(this->device->handle, nullptr);
        alcMakeContextCurrent(this->ctx);

        for (auto &source : this->sources) {
            alGenSources(1, &source.source);
        }
    }

    void sound_engine_t::set_device(device_t *device) {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(this->ctx);

        this->device = device;
        this->ctx = alcCreateContext(this->device->handle, nullptr);
        alcMakeContextCurrent(this->ctx);
    }

    void listener_t::apply() {
        alListener3f(AL_POSITION, position.x, position.y, position.z);
        alListener3f(AL_VELOCITY, velocity.x, velocity.y, velocity.z);
        float orientation[6] = {
            forward.x, forward.y, forward.z,
            up.x, up.y, up.z
        };
        alListenerfv(AL_ORIENTATION, orientation);
    }

    void sound_engine_t::play(sound_t &sound, bool loop, sound_finish_callback_t cb) {
        source_t *source = fetch_source(this->sources);
        if (source == nullptr) {
            rocket::log_error("sound sources exhausted", "sound_engine_t::play", "error");
            return;
        }

        ALuint format = sound.buffer.format == format_t::mono16 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

        ALuint buffer;
        alGenBuffers(1, &buffer);

        alBufferData(buffer, format, sound.buffer.samples.data(),
                 sound.buffer.samples.size() * sizeof(int16_t), sound.buffer.sample_rate);
        alSourcei(source->source, AL_BUFFER, buffer);
        alSourcei(source->source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
        alSourcePlay(source->source);

        if (cb == nullptr) {
            source->in_use = false;
            return;
        }
        
        std::thread t = std::thread([source, cb]() {
            ALint state;
            alGetSourcei(source->source, AL_SOURCE_STATE, &state);
            while (state == AL_PLAYING) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                alGetSourcei(source->source, AL_SOURCE_STATE, &state);
            }

            source->in_use = false;
            cb();
        });

        t.detach();
    }

    std::shared_ptr<streaming_sound_t> sound_engine_t::stream(std::string file_path, bool loop, sound_finish_callback_t cb) {
        auto strsound = std::make_shared<streaming_sound_t>();
        strsound->file_path = file_path;
        strsound->loop = loop;
        strsound->cb = cb;

        this->streaming_sounds.push_back(strsound);

        return strsound;
    }

    // void streaming_sound_t::next_frame() {
    //     bool was_uninitialized = false;
    //     if (!this->vorbis) {
    //         was_uninitialized = true;
    //         int error;
    //         this->vorbis = stb_vorbis_open_filename(this->file_path.c_str(), &error, nullptr);
    //         if (!this->vorbis) {
    //             rocket::log_error("streaming sound failed: could not open audio file", "streaming_sound_t::next_frame", "error");
    //             return;
    //         }
    //     }
    //
    //     stb_vorbis_info info = stb_vorbis_get_info(vorbis);
    //     int channels = info.channels;
    //     int sample_rate = info.sample_rate;
    //
    //     if (was_uninitialized) {
    //         this->current_buffer_to_play.format = (channels == 1) ? format_t::mono16 : format_t::stereo16;
    //         this->current_buffer_to_play.sample_rate = sample_rate;
    //         this->samples_per_frame = 1024; // fine-tune?
    //     }
    //
    //     std::vector<short> temp_buffer(samples_per_frame * channels);
    //     int num_samples = stb_vorbis_get_samples_short_interleaved(vorbis, channels, temp_buffer.data(), samples_per_frame * channels);
    //
    //
    //     if (num_samples == 0) {
    //         // If we're not *actually* at EOF, just wait until more samples can be decoded next time
    //         if (stb_vorbis_get_sample_offset(vorbis) < stb_vorbis_stream_length_in_samples(vorbis)) {
    //             return; // not EOF yet
    //         }
    //
    //         // EOF reached — close decoder
    //         stb_vorbis_close(vorbis);
    //         vorbis = nullptr;
    //         return;
    //     }
    //
    //     temp_buffer.resize(num_samples * channels);
    //     this->current_buffer_to_play.samples = std::move(temp_buffer);
    //
    // }

    // static bool recursive = false;

    // void sound_engine_t::play_frame(std::shared_ptr<streaming_sound_t> ssound) {
    //     if (ssound->vorbis == nullptr) {
    //         if (recursive) {
    //             return;
    //         }
    //
    //         recursive = true;
    //         ssound->next_frame();
    //         this->play_frame(ssound);
    //         return;
    //     }
    //
    //     source_t *source = fetch_source(this->sources);
    //     if (source == nullptr) {
    //         rocket::log_error("sound sources exhausted", "sound_engine_t::play_frame", "error");
    //         return;
    //     }
    //
    //     recursive = false;
    //
    //     ALuint format = ssound->current_buffer_to_play.format == format_t::mono16 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
    //
    //     ALuint buffer;
    //     alGenBuffers(1, &buffer);
    //
    //     alBufferData(buffer, format, ssound->current_buffer_to_play.samples.data(), ssound->current_buffer_to_play.samples.size() * sizeof(int16_t), ssound->current_buffer_to_play.sample_rate);
    //     alSourcei(source->source, AL_BUFFER, buffer);
    //     alSourcei(source->source, AL_LOOPING, AL_FALSE);
    //     alSourcePlay(source->source);
    //
    //     source->in_use = false;
    // }

    void sound_engine_t::play(std::shared_ptr<streaming_sound_t> sound, bool loop, sound_finish_callback_t cb) {
        sound->loop = loop;
        sound->cb = cb;

        stb_vorbis *vorbis = stb_vorbis_open_filename(sound->file_path.c_str(), nullptr, nullptr);
        if (!vorbis) {
            rocket::log_error("streaming sound failed: could not open audio file", "sound_engine_t::play", "error");
            return;
        }

        sound->vorbis = vorbis;

        stb_vorbis_info info = stb_vorbis_get_info(vorbis);
        int channels = info.channels;
        int sample_rate = info.sample_rate;

        [[maybe_unused]] int samples = stb_vorbis_stream_length_in_samples(vorbis);

        int frames = 1024;

        std::vector<int16_t> buffer(frames * channels);

        int frames_decoded = stb_vorbis_get_frame_short_interleaved(vorbis, channels, buffer.data(), frames * channels);

        if (frames_decoded == 0) {
            // EOF reached
            stb_vorbis_close(vorbis);
            vorbis = nullptr;
        }
        // else if (frames_decoded < frames) {
        //     // File ended early (partial final frame chunk)
        //     buffer.resize(frames_decoded * channels);
        //     stb_vorbis_close(vorbis);
        //     vorbis = nullptr;
        // }

        sound->current_buffer_to_play.samples = std::move(buffer);
        sound->current_buffer_to_play.sample_rate = sample_rate;
        sound->current_buffer_to_play.format = channels == 1 ? audio::format_t::mono16 : audio::format_t::stereo16;

        {
            source_t *source = fetch_source(this->sources);
            sound->source = source;
            if (source == nullptr) {
                rocket::log_error("sound sources exhausted", "sound_engine_t::play", "error");
                return;
            }

            ALuint format = sound->current_buffer_to_play.format == format_t::mono16 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

            const int enqueued_buffers_len = 4;
            alGenBuffers(enqueued_buffers_len, sound->buffers.data());

            alBufferData(sound->buffers[0], format, sound->current_buffer_to_play.samples.data(),
                     sound->current_buffer_to_play.samples.size() * sizeof(int16_t), sound->current_buffer_to_play.sample_rate);
            alSourcei(source->source, AL_BUFFER, sound->buffers[0]);
            alSourcei(source->source, AL_LOOPING, AL_FALSE);
            alSourcePlay(source->source);

            if (cb == nullptr) {
                source->in_use = false;
                return;
            }
            
            std::thread t = std::thread([source, buffer, cb]() {
                ALint state;
                alGetSourcei(source->source, AL_SOURCE_STATE, &state);
                while (state == AL_PLAYING) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    alGetSourcei(source->source, AL_SOURCE_STATE, &state);
                }

                source->in_use = false;
                cb();
            });

            t.detach();
        }
    }

    void sound_engine_t::update_music_streams() {
        for (std::shared_ptr<streaming_sound_t> ssound : this->streaming_sounds) {
            if (!ssound->vorbis) continue;

            stb_vorbis_info info = stb_vorbis_get_info(ssound->vorbis);
            int channels = info.channels;
            int frames = 1024;

            std::vector<int16_t> buffer(frames * channels);

            int frames_decoded = stb_vorbis_get_samples_short_interleaved(ssound->vorbis, channels, buffer.data(), frames * channels);

            if (frames_decoded == 0) {
                stb_vorbis_close(ssound->vorbis);
                ssound->vorbis = nullptr;
                rocket::log("EOF reached at frame " + std::to_string(ssound->frames_loaded), "sound_engine_t", "update_music_streams", "trace");
                continue;
            }

            // Keep track of how many "chunks" (frames) we’ve streamed
            ssound->frames_loaded++;

            if (frames_decoded < frames) {
                // Partial final frame
                buffer.resize(frames_decoded * channels);
                stb_vorbis_close(ssound->vorbis);
                ssound->vorbis = nullptr;
                rocket::log("File ended early at chunk " + std::to_string(ssound->frames_loaded), "sound_engine_t", "update_music_streams", "trace");
            }

            ssound->current_buffer_to_play.samples = std::move(buffer);
            ssound->current_buffer_to_play.sample_rate = info.sample_rate;
            ssound->current_buffer_to_play.format = channels == 1 ? audio::format_t::mono16 : audio::format_t::stereo16;

            alSourceQueueBuffers(ssound->source->source, 1, ssound->buffers.data());
        }
    }

    sound_engine_t::~sound_engine_t() {
        alcMakeContextCurrent(nullptr);

        alcCloseDevice(this->device->handle);
        alcDestroyContext(this->ctx);
    }
}
