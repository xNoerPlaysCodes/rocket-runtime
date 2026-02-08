#include <fstream>
#include <iostream>
#include <rocket/runtime.hpp>
#include <rocket/scripting.hpp>

// Do not include twice
#include <resources/rocket_pymodules.h>
#include <rocket/threads.hpp>
#include <string>
#include <thread>

PYBIND11_EMBEDDED_MODULE(rocket, m) {
    __bind(m);
}

namespace rocket::script {
    namespace py = pybind11;

    void init() {
        py::initialize_interpreter();
    }

    environment_t::environment_t() {
        this->globals["__builtins__"] = pybind11::module::import("builtins");
        py::module rocket = py::module::import("rocket");
        py::module sys = py::module::import("sys");
        sys.attr("modules")["rocket"] = rocket;
        this->globals["rocket"] = py::module::import("rocket");
    }

    bool environment_t::exec(const std::string &code) {
        py::gil_scoped_acquire gil;
        try {
            py::exec(code, this->globals, this->locals);
            return true;
        } catch (py::error_already_set &e) {
            std::string str = e.what();
            rocket::log("A scripting error occurred:" "\n" + str, "rocket::script::environment_t", "exec", "error");
            return false;
        }
    }

    bool environment_t::load_exec(const std::filesystem::path &path) {
        if (!std::filesystem::exists(path)) {
            rocket::log("Script does not exist", "rocket::script::environment_t", "exec", "error");
            return false;
        }

        std::ifstream f(path);
        if (!f.is_open()) {
            rocket::log("Script couldn't be opened", "rocket::Script::environment_t", "exec", "error");
            return false;
        }

        std::string line;
        std::string lines;
        while (std::getline(f, line)) {
            lines += line + "\n";
        }
        f.close();

        return this->exec(lines);
    }

    return_value_t environment_t::call(
        std::string fn,
        const std::vector<py::object> &args
    ) {
        return_value_t ret;
        py::gil_scoped_acquire gil;

        if (!locals.contains(fn)) {
            rocket::log("Function '" + fn + "' not found", "rocket::script::environment_t", "call", "error");
            ret.value = py::none();
            return ret;
        }

        try {
            py::object func = locals[fn.c_str()];

            if (args.empty()) {
                // zero arguments
                ret.value = func();  // call directly with no args
            } else {
                // convert vector to tuple
                py::tuple py_args(args.size());
                for (size_t i = 0; i < args.size(); ++i)
                    py_args[i] = args[i];

                ret.value = func(*py_args);  // unpack tuple into arguments
            }

        } catch (py::error_already_set &e) {
            rocket::log(std::string("A scripting error occurred:\n") + e.what(), "rocket::script::environment_t", "call", "error");
            ret.value = py::none();
        }

        return ret;
    }

    void initialize_file_watcher(std::function<void()> callback, std::filesystem::path file) {
        std::thread t = std::thread([file, &callback]() {
            auto last_write = std::filesystem::last_write_time(file);
            while (true) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                auto current_write = std::filesystem::last_write_time(file);
                if (current_write != last_write) {
                    last_write = current_write;
                    rocket::thread_t::schedule(callback);
                }
            }
        });

        t.detach();
    }

    void close() {
        py::finalize_interpreter();
    }
}
