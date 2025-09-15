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

#if defined(__linux__) || defined(__linux)
#define ROCKETGE__Platform_Linux
#endif

#if defined(__APPLE__) || defined(__MACH__)
#define ROCKETGE__Platform_macOS
#endif

#if defined(__ANDROID__)
#define ROCKETGE__Platform_Android
#endif

#if defined(__EMSCRIPTEN__)
#define ROCKETGE__Platform_Web
#endif

// Groups
#if defined(__linux__) || defined(__linux) || defined(__APPLE__) || defined(__MACH__) || defined(__ANDROID__) || defined(__EMSCRIPTEN__)
#define ROCKETGE__Platform_Desktop
#endif

#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
#define ROCKETGE__Platform_Embedded
#endif

#if defined(__ANDROID__)
#define ROCKETGE__Platform_Mobile
#endif

#endif // ROCKETGE__MACROS_HPP
