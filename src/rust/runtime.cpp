#include "rust/runtime.h"
#include "rocket/runtime.hpp"

extern "C" {
    void rs_rocket_log(const char *log, const char *const class_file_library_source, const char *function_source, const char *level) {
        rocket::log(log, class_file_library_source, function_source, level);
    }

    void rs_rocket_exit(int status_code) {
        rocket::exit(status_code);
    }
}
