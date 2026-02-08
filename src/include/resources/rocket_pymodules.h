#include "rocket/threads.hpp"
#include <rocket/renderer.hpp>
#include <rocket/runtime.hpp>
#include <rocket/types.hpp>
#include <string>
#include <util.hpp>

#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
#include <pybind11/functional.h>

namespace py = pybind11;

std::string get_macro_value(std::string value) {
#define RETMAC(val) if (value == val) return val;
    RETMAC("ROCKETGE__MAJOR_VERSION");
    RETMAC("ROCKETGE__MINOR_VERSION");
    RETMAC("ROCKETGE__BUILD");
    RETMAC("ROCKETGE__VERSION");
    RETMAC("ROCKETGE__PLATFORM");
    RETMAC("ROCKETGE__FEATURE_GL_LOADER");
    return "UNKNOWN_MACRO";
}

template<typename T>
void __bind_vec2(py::module_ &m, const char* name) {
    using Vec = rocket::vec2<T>;

    py::class_<Vec>(m, name)
        .def(py::init<>())
        .def(py::init<T, T>())
        .def_readwrite("x", &Vec::x)
        .def_readwrite("y", &Vec::y)

        .def(py::self + py::self)
        .def(py::self - py::self)
        .def(py::self * T())
        .def(py::self == py::self);
}

int __devtest();
void __bind(py::module_ &m) {
    m.doc() = "The RocketGE Python Scripting API";

    m.def("__devtest", &__devtest, "(Dev Builds only) Test");
    m.def("get_macro_value", &get_macro_value, "Returns the RocketGE macro");
    m.def("log", &rocket::log, "Log a message using RocketLogger");
    m.def("get_renderer2d", &util::get_global_renderer_2d, "Get a user-exposed renderer2d (May be None)", py::return_value_policy::reference);
    m.def("schedule_gl", &rocket::thread_t::schedule, "Run OpenGL code on the main-thread at frame-end");
    m.def("schedule_now", [](std::function<void()> fn) {
        rocket::thread_t::run([fn = std::move(fn)]() {
            py::gil_scoped_acquire _;
            fn();
        });
    }, "Runs on a new thread NOW (No OpenGL calls)");

    py::class_<rocket::rgba_color>(m, "rgba_color")
        .def(py::init<>())
        .def(py::init<uint8_t, uint8_t, uint8_t, uint8_t>())
        .def_readwrite("x", &rocket::rgba_color::x)
        .def_readwrite("y", &rocket::rgba_color::y)
        .def_readwrite("z", &rocket::rgba_color::z)
        .def_readwrite("w", &rocket::rgba_color::w);

    __bind_vec2<float>(m, "vec2f");
    __bind_vec2<int>(m, "vec2i");
    __bind_vec2<double>(m, "vec2d");

    py::class_<rocket::fbounding_box>(m, "fbounding_box")
        .def(py::init<>())
        .def(py::init<rocket::vec2<float>, rocket::vec2<float>>())
        .def("intersects", &rocket::fbounding_box::intersects)
        .def_readwrite("pos", &rocket::fbounding_box::pos)
        .def_readwrite("size", &rocket::fbounding_box::size);

    py::class_<rocket::renderer_2d>(m, "renderer_2d")
        .def("clear", &rocket::renderer_2d::clear)
        .def("begin_frame", &rocket::renderer_2d::begin_frame)
        .def("end_frame", &rocket::renderer_2d::end_frame)
        .def("draw_rectangle", 
                py::overload_cast<rocket::fbounding_box, 
                rocket::rgba_color, 
                float, 
                float, 
                bool>(&rocket::renderer_2d::draw_rectangle),
                py::arg("rect"),
                py::arg("color") = rocket::rgba_color(0, 0, 0, 255),
                py::arg("rotation") = 0.f,
                py::arg("roundedness") = 0.f,
                py::arg("lines") = false)
        .def("draw_rectangle", py::overload_cast<rocket::vec2f_t, 
                rocket::vec2f_t, 
                rocket::rgba_color, 
                float, 
                float, 
                bool>(&rocket::renderer_2d::draw_rectangle),
                py::arg("pos"),
                py::arg("size"),
                py::arg("color") = rocket::rgba_color(0, 0, 0, 255),
                py::arg("rotation") = 0.f,
                py::arg("roundedness") = 0.f,
                py::arg("lines") = false)
        .def("draw_circle", &rocket::renderer_2d::draw_circle,
                py::arg("pos"),
                py::arg("radius"),
                py::arg("color") = rocket::rgba_color(0, 0, 0, 255),
                py::arg("thickness") = 0)
        .def("draw_polygon", &rocket::renderer_2d::draw_polygon,
                py::arg("pos"),
                py::arg("radius"),
                py::arg("color") = rocket::rgba_color(0, 0, 0, 255),
                py::arg("sides") = 3,
                py::arg("rotation") = 0.f);
}

int __devtest() {
    return 0;
}

