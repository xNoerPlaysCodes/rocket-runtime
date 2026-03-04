#ifndef RocketGE__scripting_hpp
#define RocketGE__scripting_hpp

#include <filesystem>
#include <functional>
#include <pybind11/embed.h>
#include <string>

namespace rocket::script {
    struct return_value_t {
        bool success = false;
        pybind11::object value;
    };

    struct environment_t {
        pybind11::dict globals;
        pybind11::dict locals;

        environment_t();

        bool exec(const std::string &code);
        bool load_exec(const std::filesystem::path &path);
        
        return_value_t call(std::string fn, const std::vector<pybind11::object> &args = {});
    };

    /// @brief Initializes the Python Interpreter
    void init();

    /// @brief Closes the Python Interpreter
    void close();

    /// @brief Initializes a file watcher using callback
    void initialize_file_watcher(std::function<void()> callback, std::filesystem::path file);
}

#endif//RocketGE__scripting_hpp
