#ifndef ROCKETGE__MACROS_HPP
#define ROCKETGE__MACROS_HPP

// #define ROCKETGE__NOT_IMPLEMENTED __attribute__((unavailable("Not Implemented")))
#define ROCKETGE__NOT_IMPLEMENTED
// #define ROCKETGE__DEPRECATED [[deprecated]]
#define ROCKETGE__DEPRECATED

#define ROCKETGE__PlatformSpecific_Linux_AppClassNameOrID "rge-game"

#ifdef _WIN32
#define ROCKETGE__Platform_Windows
#endif

#if (defined(__linux__) || defined(__linux)) && !defined(__ANDROID__)
#define ROCKETGE__Platform_Linux
#define ROCKETGE__RAW_PLATFORM_STRING "Linux"
#endif

#if defined(__APPLE__) || defined(__MACH__)
#define ROCKETGE__Platform_macOS
#define ROCKETGE__RAW_PLATFORM_STRING "Mach"
#endif

#if defined(ROCKETGE__Platform_Linux) || defined(ROCKETGE__Platform_macOS)
#define ROCKETGE__Platform_UnixCompatible
#endif

#if defined(__ANDROID__)
#define ROCKETGE__Platform_Android
#define ROCKETGE__RAW_PLATFORM_STRING "Android"
#endif

#if defined(__EMSCRIPTEN__)
#define ROCKETGE__Platform_Web
#define ROCKETGE__RAW_PLATFORM_STRING "Emscripten"
#endif

#if defined(__x86_64__) || defined(_M_X64)
#define ROCKETGE__Architecture_x64
#endif

#if defined(__i386__) || defined(_M_IX86)
#define ROCKETGE__Architecture_x86
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
#define ROCKETGE__Architecture_arm64
#endif

#if defined(__arm__) || defined(_M_ARM)
#define ROCKETGE__Architecture_arm32
#endif

// Groups
#if defined(ROCKETGE__Platform_Linux) || defined(ROCKETGE__Platform_macOS) || defined(ROCKETGE__Platform_Windows)
#define ROCKETGE__Platform_Desktop
#endif

#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
#define ROCKETGE__Platform_Embedded
#endif

#if defined(__ANDROID__)
#define ROCKETGE__Platform_Mobile
#endif

#endif // ROCKETGE__MACROS_HPP
