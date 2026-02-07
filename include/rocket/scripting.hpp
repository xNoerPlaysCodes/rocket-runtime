#ifndef RocketGE__scripting_hpp
#define RocketGE__scripting_hpp

#include <pybind11/embed.h>
#include <string>

namespace rocket::script {
    struct environment_t {
        pybind11::dict globals;
        pybind11::dict locals;

        environment_t() {
            globals["__builtins__"] = pybind11::module::import("builtins");
        }

        bool exec(const std::string &code);
    };

    void init();
    void close();
}

#endif//RocketGE__scripting_hpp
