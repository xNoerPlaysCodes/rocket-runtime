#pragma once

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

void rs_rocket_log(const char *log, const char *const class_file_library_source, const char *function_source, const char *level);
void rs_rocket_exit(int status_code);

struct RocketCliResult {
    bool noplugins;
    bool logall;
    bool debugoverlay;
    bool nosplash;
    bool notext;
    bool forcewayland;
    bool software_frame_timer;
    int32_t glversion;
    int32_t framerate;
    char viewport_size[64];
    bool viewport_size_set;
    
    bool should_exit;
    int32_t exit_code;
};

    
RocketCliResult parse_rocket_arguments(int argc, const char* const* argv); // rust will implement this function


#ifdef __cplusplus
}
#endif
