#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void rs_rocket_log(const char *log, const char *const class_file_library_source, const char *function_source, const char *level);
void rs_rocket_exit(int status_code);

#ifdef __cplusplus
}
#endif
