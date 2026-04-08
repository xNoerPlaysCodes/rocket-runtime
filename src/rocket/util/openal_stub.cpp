#include <AL/al.h>
#include <AL/alc.h>

#include <array>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

struct ALCdevice {
    std::string name;
};

struct ALCcontext {
    ALCdevice *device;
};

namespace {
    struct buffer_state_t {
        ALenum format = 0;
        ALsizei size = 0;
        ALsizei freq = 0;
    };

    struct source_state_t {
        ALuint buffer = 0;
        ALboolean looping = AL_FALSE;
        ALfloat gain = 1.0f;
        ALint state = AL_STOPPED;
        ALint state_polls_remaining = 0;
        std::vector<ALuint> queued_buffers;
    };

    std::mutex g_openal_stub_mutex;
    std::unordered_map<ALuint, buffer_state_t> g_buffers;
    std::unordered_map<ALuint, source_state_t> g_sources;
    ALCcontext *g_current_context = nullptr;
    ALenum g_last_error = AL_NO_ERROR;
    ALuint g_next_buffer_id = 1;
    ALuint g_next_source_id = 1;

    constexpr std::array<const char*, 2> k_supported_extensions = {
        "ALC_ENUMERATION_EXT",
        "ALC_ENUMERATE_ALL_EXT",
    };

    constexpr char k_default_device_name[] = "RocketRuntime Stub Audio";
    constexpr char k_all_devices[] = "RocketRuntime Stub Audio\0\0";

    void set_last_error(ALenum error_code) {
        g_last_error = error_code;
    }

    source_state_t *find_source(ALuint source) {
        auto it = g_sources.find(source);
        if (it == g_sources.end()) {
            set_last_error(AL_INVALID_NAME);
            return nullptr;
        }
        return &it->second;
    }
}

extern "C" {
ALCdevice *ALC_APIENTRY alcOpenDevice(const ALCchar *devicename) ALC_API_NOEXCEPT {
    std::lock_guard<std::mutex> lock(g_openal_stub_mutex);
    auto *device = new ALCdevice;
    if (devicename != nullptr && devicename[0] != '\0') {
        device->name = devicename;
    } else {
        device->name = k_default_device_name;
    }
    return device;
}

ALCboolean ALC_APIENTRY alcCloseDevice(ALCdevice *device) ALC_API_NOEXCEPT {
    (void)device;
    return ALC_TRUE;
}

ALCcontext *ALC_APIENTRY alcCreateContext(ALCdevice *device, const ALCint *attrlist) ALC_API_NOEXCEPT {
    (void)attrlist;
    if (device == nullptr) {
        return nullptr;
    }

    auto *context = new ALCcontext;
    context->device = device;
    return context;
}

ALCboolean ALC_APIENTRY alcMakeContextCurrent(ALCcontext *context) ALC_API_NOEXCEPT {
    std::lock_guard<std::mutex> lock(g_openal_stub_mutex);
    g_current_context = context;
    return ALC_TRUE;
}

void ALC_APIENTRY alcDestroyContext(ALCcontext *context) ALC_API_NOEXCEPT {
    std::lock_guard<std::mutex> lock(g_openal_stub_mutex);
    if (g_current_context == context) {
        g_current_context = nullptr;
    }
    delete context;
}

ALCcontext *ALC_APIENTRY alcGetCurrentContext(void) ALC_API_NOEXCEPT {
    std::lock_guard<std::mutex> lock(g_openal_stub_mutex);
    return g_current_context;
}

ALCdevice *ALC_APIENTRY alcGetContextsDevice(ALCcontext *context) ALC_API_NOEXCEPT {
    if (context == nullptr) {
        return nullptr;
    }
    return context->device;
}

ALCboolean ALC_APIENTRY alcIsExtensionPresent(ALCdevice *device, const ALCchar *extname) ALC_API_NOEXCEPT {
    (void)device;
    if (extname == nullptr) {
        return ALC_FALSE;
    }

    for (const char *supported : k_supported_extensions) {
        if (std::string_view(extname) == supported) {
            return ALC_TRUE;
        }
    }

    return ALC_FALSE;
}

const ALCchar *ALC_APIENTRY alcGetString(ALCdevice *device, ALCenum param) ALC_API_NOEXCEPT {
    switch (param) {
        case ALC_DEVICE_SPECIFIER:
            if (device != nullptr && !device->name.empty()) {
                return device->name.c_str();
            }
            return k_default_device_name;
        case ALC_ALL_DEVICES_SPECIFIER:
            return k_all_devices;
        case ALC_DEFAULT_DEVICE_SPECIFIER:
        case ALC_DEFAULT_ALL_DEVICES_SPECIFIER:
            return k_default_device_name;
        case ALC_EXTENSIONS:
            return "ALC_ENUMERATION_EXT ALC_ENUMERATE_ALL_EXT";
        default:
            return "";
    }
}

void AL_APIENTRY alGenSources(ALsizei n, ALuint *sources) AL_API_NOEXCEPT {
    if (n < 0 || sources == nullptr) {
        set_last_error(AL_INVALID_VALUE);
        return;
    }

    std::lock_guard<std::mutex> lock(g_openal_stub_mutex);
    for (ALsizei i = 0; i < n; ++i) {
        const ALuint id = g_next_source_id++;
        g_sources.emplace(id, source_state_t{});
        sources[i] = id;
    }
}

void AL_APIENTRY alDeleteSources(ALsizei n, const ALuint *sources) AL_API_NOEXCEPT {
    if (n < 0 || sources == nullptr) {
        set_last_error(AL_INVALID_VALUE);
        return;
    }

    std::lock_guard<std::mutex> lock(g_openal_stub_mutex);
    for (ALsizei i = 0; i < n; ++i) {
        g_sources.erase(sources[i]);
    }
}

void AL_APIENTRY alSourcei(ALuint source, ALenum param, ALint value) AL_API_NOEXCEPT {
    std::lock_guard<std::mutex> lock(g_openal_stub_mutex);
    source_state_t *state = find_source(source);
    if (state == nullptr) {
        return;
    }

    switch (param) {
        case AL_BUFFER:
            state->buffer = static_cast<ALuint>(value);
            break;
        case AL_LOOPING:
            state->looping = value ? AL_TRUE : AL_FALSE;
            break;
        default:
            break;
    }
}

void AL_APIENTRY alSourcef(ALuint source, ALenum param, ALfloat value) AL_API_NOEXCEPT {
    std::lock_guard<std::mutex> lock(g_openal_stub_mutex);
    source_state_t *state = find_source(source);
    if (state == nullptr) {
        return;
    }

    if (param == AL_GAIN) {
        state->gain = value;
    }
}

void AL_APIENTRY alSourcePlay(ALuint source) AL_API_NOEXCEPT {
    std::lock_guard<std::mutex> lock(g_openal_stub_mutex);
    source_state_t *state = find_source(source);
    if (state == nullptr) {
        return;
    }

    state->state = AL_PLAYING;
    state->state_polls_remaining = (state->looping == AL_TRUE) ? 1000000 : 2;
}

void AL_APIENTRY alSourceStop(ALuint source) AL_API_NOEXCEPT {
    std::lock_guard<std::mutex> lock(g_openal_stub_mutex);
    source_state_t *state = find_source(source);
    if (state == nullptr) {
        return;
    }

    state->state = AL_STOPPED;
    state->state_polls_remaining = 0;
}

void AL_APIENTRY alGetSourcei(ALuint source, ALenum param, ALint *value) AL_API_NOEXCEPT {
    if (value == nullptr) {
        set_last_error(AL_INVALID_VALUE);
        return;
    }

    std::lock_guard<std::mutex> lock(g_openal_stub_mutex);
    source_state_t *state = find_source(source);
    if (state == nullptr) {
        *value = 0;
        return;
    }

    switch (param) {
        case AL_SOURCE_STATE:
            *value = state->state;
            if (state->state == AL_PLAYING && state->looping != AL_TRUE) {
                if (state->state_polls_remaining > 0) {
                    --state->state_polls_remaining;
                }
                if (state->state_polls_remaining == 0) {
                    state->state = AL_STOPPED;
                }
            }
            break;
        case AL_SEC_OFFSET:
            *value = (state->state == AL_PLAYING) ? 1 : 0;
            break;
        case AL_BUFFER:
            *value = static_cast<ALint>(state->buffer);
            break;
        default:
            *value = 0;
            break;
    }
}

void AL_APIENTRY alGenBuffers(ALsizei n, ALuint *buffers) AL_API_NOEXCEPT {
    if (n < 0 || buffers == nullptr) {
        set_last_error(AL_INVALID_VALUE);
        return;
    }

    std::lock_guard<std::mutex> lock(g_openal_stub_mutex);
    for (ALsizei i = 0; i < n; ++i) {
        const ALuint id = g_next_buffer_id++;
        g_buffers.emplace(id, buffer_state_t{});
        buffers[i] = id;
    }
}

void AL_APIENTRY alDeleteBuffers(ALsizei n, const ALuint *buffers) AL_API_NOEXCEPT {
    if (n < 0 || buffers == nullptr) {
        set_last_error(AL_INVALID_VALUE);
        return;
    }

    std::lock_guard<std::mutex> lock(g_openal_stub_mutex);
    for (ALsizei i = 0; i < n; ++i) {
        g_buffers.erase(buffers[i]);
    }
}

void AL_APIENTRY alBufferData(ALuint buffer, ALenum format, const ALvoid *data, ALsizei size, ALsizei freq) AL_API_NOEXCEPT {
    (void)data;

    std::lock_guard<std::mutex> lock(g_openal_stub_mutex);
    auto it = g_buffers.find(buffer);
    if (it == g_buffers.end()) {
        set_last_error(AL_INVALID_NAME);
        return;
    }

    it->second.format = format;
    it->second.size = size;
    it->second.freq = freq;
}

void AL_APIENTRY alListener3f(ALenum param, ALfloat value1, ALfloat value2, ALfloat value3) AL_API_NOEXCEPT {
    (void)param;
    (void)value1;
    (void)value2;
    (void)value3;
}

void AL_APIENTRY alListenerfv(ALenum param, const ALfloat *values) AL_API_NOEXCEPT {
    (void)param;
    (void)values;
}

void AL_APIENTRY alSourceQueueBuffers(ALuint source, ALsizei nb, const ALuint *buffers) AL_API_NOEXCEPT {
    if (nb < 0 || buffers == nullptr) {
        set_last_error(AL_INVALID_VALUE);
        return;
    }

    std::lock_guard<std::mutex> lock(g_openal_stub_mutex);
    source_state_t *state = find_source(source);
    if (state == nullptr) {
        return;
    }

    for (ALsizei i = 0; i < nb; ++i) {
        state->queued_buffers.push_back(buffers[i]);
        if (state->buffer == 0) {
            state->buffer = buffers[i];
        }
    }
}

ALenum AL_APIENTRY alGetError(void) AL_API_NOEXCEPT {
    std::lock_guard<std::mutex> lock(g_openal_stub_mutex);
    const ALenum error = g_last_error;
    g_last_error = AL_NO_ERROR;
    return error;
}
}
