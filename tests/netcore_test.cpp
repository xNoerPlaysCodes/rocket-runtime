#include <iostream>
#include <rocket/runtime.hpp>
#include <rocket/net/core.hpp>
#include <string>

std::string bool_str(bool b) {
    return b ? "true" : "false";
}

int main() {
#ifdef ROCKETGE__BUILD_NETWORKING
    rocket::net::init();
    std::cout << bool_str(rocket::net::check_connection_status()) << '\n';
#else
    std::cout << "RocketGE was built without networking support" << '\n';
#endif
}
