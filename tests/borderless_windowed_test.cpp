#include <rocket/runtime.hpp>
#include <string>

int main(int argc, char **argv) {
    if (argc >= 3 && std::string(argv[2]) == "--unit-test") {
        rocket::set_log_level(rocket::log_level_t::none);
        return 1;
    }
}
