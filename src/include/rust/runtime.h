#pragma once

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

void rs_rocket_log(const char *log, const char *const class_file_library_source, const char *function_source, const char *level);
void rs_rocket_exit(int status_code);

struct rs_RocketCliResult {
    bool noplugins;
    bool debugoverlay;
    bool nosplash;
    bool notext; 

    bool forcewayland;
    bool software_frame_timer;
    bool viewport_size_set;
    bool should_exit; 

    /// @brief GL Version Format
    ///       is: ((MAJOR * 10) + MINOR))
    int32_t glversion; 
    int32_t framerate; 
    int32_t exit_code;
    
    char viewport_size[64];
    char logfile[256];
};

    
rs_RocketCliResult rs_parse_rocket_arguments(int argc, const char* const* argv); // rust will implement this function


#ifdef __cplusplus
}
#endif
