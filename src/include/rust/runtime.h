#pragma once

extern "C" {
    void rs_rocket_log(const char *log, const char *const class_file_library_source, const char *function_source, const char *level);
    void rs_rocket_exit(int status_code);
}
