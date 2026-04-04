#ifndef ROCKETGE__RUNTIME_HPP
#define ROCKETGE__RUNTIME_HPP

#include "rocket/macros.hpp"
#include <filesystem>
#include <string>
#ifdef ROCKETGE__Platform_Windows
#define GL_STATIC_DRAW 0x88E4
#include <windows.h>
#endif

#ifndef ROCKETGE__RUNTIME_SKIP_HEADER_INCLUSION
#include "renderer.hpp"
#include "asset.hpp"
#include "io.hpp"
#include "types.hpp"
#include "window.hpp"
#include "shader.hpp"
#include "constants.hpp"
#endif

#include <functional>

#define ROCKETGE__MAJOR_VERSION  3
#define ROCKETGE__MINOR_VERSION  0
#define ROCKETGE__BUILD          "beta"
#define ROCKETGE__STRX_(x)       #x
#define ROCKETGE__STRX(x)        ROCKETGE__STRX_(x)
#define ROCKETGE__VERSION        ROCKETGE__STRX(ROCKETGE__MAJOR_VERSION) "." ROCKETGE__STRX(ROCKETGE__MINOR_VERSION) "-" ROCKETGE__BUILD

#define ROCKETGE__FEATURE_MAX_GL_VERSION_MAJOR 4
#define ROCKETGE__FEATURE_MAX_GL_VERSION_MINOR 6

#define ROCKETGE__FEATURE_RECOMMENDED_GL_VERSION_MAJOR 4
#define ROCKETGE__FEATURE_RECOMMENDED_GL_VERSION_MINOR 1

/// @brief The minimum OpenGL version required for
///     operation
#define ROCKETGE__FEATURE_MIN_GL_VERSION_MAJOR 3
#define ROCKETGE__FEATURE_MIN_GL_VERSION_MINOR 3

/// @brief The max RLSL version supported
///                ^^^^ -> RocketShaderLanguage
#define ROCKETGE__FEATURE_MAX_RLSL_VERSION_MAJOR 1
#define ROCKETGE__FEATURE_MAX_RLSL_VERSION_MINOR 3

#define ROCKETGE__FEATURE_GL_LOADER "GLfnldr"

#define ROCKETGE__FEATURE_SHADER_SUPPORT_VERT_FRAG
// #define ROCKETGE__FEATURE_SHADER_SUPPORT_COMPUTE_SHADER

namespace rocket {
    /// @brief Log Level
    enum class log_level_t : int {
        all = 0,
        trace = 1,
        debug = 2,
        info = 3,
        warn = 4,
        error = 5,
        fatal = 6,
        none = 100
    };

    enum class logger_state_t : int {
        flush_always = 0,
        flush_never,
    };

    /// @brief Sets minimum log level to be printed to the console
    /// @brief or callback
    void set_log_level(log_level_t level);

    /// @brief Converts log level to string
    /// @note lowercase
    std::string log_level_to_str(log_level_t level);

    /// @brief Log Callback
    /// @param std::string log_message
    /// @param std::string class, file, or library source
    /// @param std::string function_source
    /// @param std::string level
    using log_callback_t = std::function<void(
            std::string log_message, 
            std::string class_file_library_source, 
            std::string function_source,
            std::string level
    )>;

    /// @brief OpenGL::ContextVerifier Error Callback
    /// @param std::string type
    /// @param std::string severity
    /// @param int id
    /// @param std::string message
    /// @param std::string source
    using gl_error_callback_t = std::function<void(
            std::string type,
            std::string severity,
            int id,
            std::string message,
            std::string source
    )>;

    /// @brief Exit Callback
    /// @note You can choose not to exit at all
    /// @param int status_code
    using exit_callback_t = std::function<void(
            int status_code
    )>;

    /// @brief Sets log callback
    void set_log_callback(log_callback_t);

    /// @brief Set exit callback
    void set_exit_callback(exit_callback_t);

    /// @brief Log using RocketLogger or callback
    /// @note Thread Safe
    void log(const std::string &log, const std::string &class_file_library_source, const std::string &function_source, const std::string &level);

    /// @brief Flushes the output buffer for RocketLogger
    void logger_flush();

    /// @brief Push Logger State
    void logger_push(logger_state_t state);

    /// @brief Pop Logger State
    void logger_pop(logger_state_t state);

    /// @brief Exit using RocketExit or callback
    /// @note Treat as if it does NOT exit
    /// @note Overridable
    void exit(int status_code = 1);

    /// @brief Set OpenGL Error callback
    void set_opengl_error_callback(gl_error_callback_t);

    /// @brief Get OpenGL Error callback
    gl_error_callback_t get_opengl_error_callback();

    /// @brief Registers an argument with callback with NO value
    /// @note Must be called before init
    void register_argument(std::string arg, std::function<void()> cb, std::string description);

    /// @brief Registers an argument with callback with a value
    /// @note Must be called before init
    void register_argument(std::string arg, std::function<void(std::string value)> cb, std::string description, std::string value_type);

    /// @brief Registers a library to be attributed on CLI flag --version
    void register_libattribution(std::string lib, std::string license);

    /// @brief Initializes Rocket Runtime
    void init(std::vector<std::string> args = {});
    /// @brief Initializes Rocket Runtime
    void init(int argc = 0, char **argv = nullptr);

    /// @brief Set the file output (if any) for RocketLogger
    void set_logger_file_output(const std::filesystem::path &path);
}

/// @brief Rocket Main Arguments
struct rocket_arguments_t {
    /// @brief Working Directory of Application
    /// @note Guranteed to end with '/'
    std::string working_dir = "./";
    /// @brief Platform-Specific Main Function
    std::string platform_main;
};

/// @brief Rocket Main function
/// @note Platform-Independent Main
/// @note Look at docs on how to use
int rocket_main(int argc, char **argv, rocket_arguments_t);

/// @brief Internal
/// @note Do Not Define
void __rocket_premain(int argc, char **argv);

#ifdef ROCKETGE__Platform_Android
#include <android_native_app_glue.h>
#include <android/asset_manager.h>
#include <android/log.h>
#include <android/native_activity.h>
#include <android/window.h>
#include <fstream>
#include <jni.h>
/**
 * @def ROCKETGE__ANDROID_NOTRUEFULLSCREEN
 * @brief Define this macro to disable rendering behind the display notch/cutout on Android.
 *
 * By default, RocketGE renders in true fullscreen mode, extending into the notch/cutout area.
 * Defining this macro before including this header will instead leave a black bar where the
 * notch is, ensuring no game content is obscured by the cutout.
 *
 * Usage:
 * @code
 * #define ROCKETGE__ANDROID_NOTRUEFULLSCREEN
 * #include <rocket/runtime.hpp>
 * @endcode
 */
#ifndef ROCKETGE__ANDROID_NOTRUEFULLSCREEN
#define ROCKETGE__INT_JNICUTOUTLOGIC \
    jmethodID getAttributes = env->GetMethodID(windowClass, "getAttributes", "()Landroid/view/WindowManager$LayoutParams;"); \
    jobject attrs = env->CallObjectMethod(window, getAttributes); \
    jclass attrsClass = env->GetObjectClass(attrs); \
    jfieldID cutoutField = env->GetFieldID(attrsClass, "layoutInDisplayCutoutMode", "I"); \
    env->SetIntField(attrs, cutoutField, 3); \
    jmethodID setAttributes = env->GetMethodID(windowClass, "setAttributes", "(Landroid/view/WindowManager$LayoutParams;)V"); \
    env->CallVoidMethod(window, setAttributes, attrs);
#else
#define ROCKETGE__INT_JNICUTOUTLOGIC
#endif
#define DEFINE_PLATFORM_MAIN \
    android_app *g_android_app = nullptr; \
    extern "C" void RocketGE_hide_navigation(android_app* app) { \
        JNIEnv* env; \
        app->activity->vm->AttachCurrentThread(&env, nullptr); \
        jobject activity = app->activity->clazz; \
        jclass activityClass = env->GetObjectClass(activity); \
        jmethodID getWindow = env->GetMethodID(activityClass, "getWindow", "()Landroid/view/Window;"); \
        jobject window = env->CallObjectMethod(activity, getWindow); \
        jclass windowClass = env->GetObjectClass(window); \
        jmethodID getInsetsController = env->GetMethodID(windowClass, "getInsetsController", "()Landroid/view/WindowInsetsController;"); \
        jobject controller = env->CallObjectMethod(window, getInsetsController); \
        jclass controllerClass = env->GetObjectClass(controller); \
        jmethodID hide = env->GetMethodID(controllerClass, "hide", "(I)V"); \
        env->CallVoidMethod(controller, hide, 0x02); \
        jmethodID setBehavior = env->GetMethodID(controllerClass, "setSystemBarsBehavior", "(I)V"); \
        env->CallVoidMethod(controller, setBehavior, 2); \
        \
        ROCKETGE__INT_JNICUTOUTLOGIC \
        app->activity->vm->DetachCurrentThread(); \
    } \
    extern "C" void android_main(android_app *app) { \
        const char *argv[] = { "RocketGE_EmbeddedArgv0", "--debug-overlay", "--", nullptr }; \
        int argc = (sizeof(argv) / sizeof(argv[0])) - 1; \
        __rocket_premain(argc, (char**) argv); \
        g_android_app = app; \
        app->onAppCmd = [](android_app *app, int32_t cmd) {}; \
        ANativeActivity_setWindowFlags(app->activity, \
            AWINDOW_FLAG_LAYOUT_IN_SCREEN | AWINDOW_FLAG_LAYOUT_NO_LIMITS, 0); \
        RocketGE_hide_navigation(app); \
        while (app->window == nullptr) { \
            int events; \
            android_poll_source *src; \
            ALooper_pollOnce(100, nullptr, &events, (void**)&src); \
            if (src != nullptr) src->process(app, src); \
            if (app->destroyRequested) return; \
        } \
        AAssetManager* mgr = app->activity->assetManager; \
        std::string internal = std::string(app->activity->internalDataPath) + "/"; \
        std::filesystem::create_directories(internal + "resources"); \
        AAssetDir* dir = AAssetManager_openDir(mgr, "resources"); \
        const char* filename; \
        while ((filename = AAssetDir_getNextFileName(dir)) != nullptr) { \
            std::string src_path = std::string("resources/") + filename; \
            std::string dst_path = internal + "resources/" + filename; \
            AAsset* asset = AAssetManager_open(mgr, src_path.c_str(), AASSET_MODE_BUFFER); \
            if (!asset) continue; \
            size_t size = AAsset_getLength(asset); \
            std::vector<uint8_t> buf(size); \
            AAsset_read(asset, buf.data(), size); \
            AAsset_close(asset); \
            std::ofstream f(dst_path, std::ios::binary); \
            f.write((char*)buf.data(), size); \
        } \
        AAssetDir_close(dir); \
        rocket_main(argc, (char**) argv, { \
            .working_dir = std::string(g_android_app->activity->internalDataPath) + "/", \
            .platform_main = "android_main" \
        }); \
    }
#elifdef ROCKETGE__Platform_Windows
#define DEFINE_PLATFORM_MAIN \
    int main(int argc, char **argv) { \
        __rocket_premain(argc, argv); \
        return rocket_main(argc, argv, { \
            .platform_main = "main" \
        }); \
    } \
    int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) { \
        __rocket_premain(0, nullptr); \
        return rocket_main(0, nullptr, { \
            .platform_main = "WinMain" \
        }); \
    }
#else
#define DEFINE_PLATFORM_MAIN \
    int main(int argc, char **argv) { \
        __rocket_premain(argc, argv); \
        return rocket_main(argc, argv, { \
            .platform_main = "main" \
        }); \
    }
#endif
#endif
