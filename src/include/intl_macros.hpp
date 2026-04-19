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

#define r_LOG_Impl(...) __android_log_print(ANDROID_LOG_INFO, "RocketGE", __VA_ARGS__)
#define r_LOG(X) r_LOG_Impl(X)
#else
#define LOGI(...) (void)0
#define LOGE(...) (void)0
#define LOGD(...) (void)0
#define r_LOG(X) rocket::log(X, std::string(r::class_or_file(__fsig, __FILE__)), __func__, "fatal")
#endif

#ifdef ROCKETGE__DEBUG_BUILD
#define r_assert(condition) \
    do { \
        if (!(condition)) { \
            constexpr std::string_view __fsig = r_FuncSig; \
            r_LOG("Assertion Failed: " r_Stringify(condition)); \
            rnative::exit_now(1); \
        } \
    } while (0)
#else
#define r_assert(x) ((void)sizeof(x))
#endif

#define r_quicklog(msg) rocket::log("QuickLog Message: ", std::string(r::class_or_file(r_FuncSig, __FILE__)), __func__, "debug")

#ifdef ROCKETGE__DEBUG_BUILD
#define r_debug_if(condition) if ((condition))
#else
#define r_debug_if(condition) if (false && ((condition) && false))
#endif

#endif // INTL__MACROS_HPP
