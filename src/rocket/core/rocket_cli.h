#pragma once
#include <stdint.h>

// dis struct has to be the same in both rust and c++
extern "C" {
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
    // this function will be implemented by rust
    // what am i even saying
}