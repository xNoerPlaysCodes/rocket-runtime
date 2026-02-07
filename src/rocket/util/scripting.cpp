#include <rocket/runtime.hpp>
#include <rocket/scripting.hpp>

// Do not include twice
#include <resources/rocket_pymodules.h>

PYBIND11_EMBEDDED_MODULE(rocket, m) {
    __bind(m);
}

namespace rocket::script {
    namespace py = pybind11;

    void init() {
        py::initialize_interpreter();
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

    void close() {
        py::finalize_interpreter();
    }
}
