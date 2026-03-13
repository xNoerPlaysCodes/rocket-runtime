#ifndef INTL__MACROS_HPP
#define INTL__MACROS_HPP

// @brief A semantic helper for static function implementations
#define r_static

#include <string_view>
#include <rocket/runtime.hpp>
#include <exception>

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

#ifdef ROCKETGE__DEBUG_BUILD
#define r_assert(x) \
    do { \
        if (!(x)) { \
            constexpr std::string_view fn = r_FuncSig; \
            rocket::log("Assertion Failed: " r_Stringify(x), std::string(r::class_or_file(fn, __FILE__)), __func__, "fatal"); \
            std::terminate(); \
        } \
    } while (0)
#else
#define r_assert(x) ((void)sizeof(x))
#endif

#endif // INTL__MACROS_HPP
