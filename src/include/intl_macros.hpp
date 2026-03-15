#ifndef INTL__MACROS_HPP
#define INTL__MACROS_HPP

// @brief A semantic helper for static function implementations
#define r_static

#include <string_view>
#include <rocket/runtime.hpp>
#include <native.hpp>

#if defined(_MSC_VER)
#define r_FuncSig __FUNCSIG__
#elif defined(__clang__) || defined(__GNUC__)
#define r_FuncSig __PRETTY_FUNCTION__
#else
#define r_FuncSig __func__
#endif

namespace r {
    constexpr std::string_view class_or_file(std::string_view sig, std::string_view file)
    {
        auto end = sig.find('(');
        auto before = sig.substr(0, end);

        auto last_space = before.rfind(' ');
        auto name = before.substr(last_space + 1);

        auto pos = name.rfind("::");
        if (pos == std::string_view::npos)
            return file;

        return name.substr(0, pos); // remove function name
    }
}

#define r_Stringify(x) #x

#ifdef ROCKETGE__Platform_Android
#include <android/log.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES3/gl32.h>

#define LOG_TAG "RocketGE"

extern android_app* g_android_app;

// Log levels: ANDROID_LOG_DEBUG, ANDROID_LOG_INFO, ANDROID_LOG_WARN, ANDROID_LOG_ERROR
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define r_LOG(X) LOGI(X)
#else
#define LOGI(...) (void)0
#define LOGE(...) (void)0
#define LOGD(...) (void)0
#define r_LOG(X) rocket::log(X, std::string(r::class_or_file(fn, __FILE__)), __func__, "fatal")
#endif

#ifdef ROCKETGE__DEBUG_BUILD
#define r_assert(x) \
    do { \
        if (!(x)) { \
            constexpr std::string_view fn = r_FuncSig; \
            r_LOG("Assertion Failed: " r_Stringify(x)); \
            rnative::exit_now(1); \
        } \
    } while (0)
#else
#define r_assert(x) ((void)sizeof(x))
#endif

#endif // INTL__MACROS_HPP
