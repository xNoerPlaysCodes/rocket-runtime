#include "code_generator.hpp"
#include <cctype>
#include <string>

std::string get_variable_safe_path_name(std::string path) {
    std::string gen = "_GenVar_";
    gen.reserve(path.size());
    for (auto &c : path) {
        if (std::isalnum(c) || c == '_')
            gen += c;
    }
    return gen;
}

namespace rgeditor {
    generated_code_t *gen(std::vector<game_object_t> objs) {
        generated_code_t *cd = new generated_code_t;
        cd->hpp = R"(#ifndef RGEditor__GeneratedCode_hpp
#define RGEditor__GeneratedCode_hpp

namespace rgeditor_generated_code {
    void draw(rocket::renderer_2d *ren, rocket::glfw_window_t *win);
}

#endif//RGEditor__GeneratedCode_hpp
)";
        cd->cpp = R"(#include "rge_generated_code.hpp"
#include <rocket/runtime.hpp>

int main(int argc, char **argv) {
    using namespace rgeditor_generated_code;
    rocket::glfw_window_t window = { {1280, 720}, "RGame" };
    rocket::renderer_2d r2d = { &window, 60 };
    
    while (window.is_running()) {
        r2d.begin_frame();
        r2d.clear();
        {
            draw(&r2d, &window);
        }
        r2d.end_frame();
        window.poll_events();
    }
}
        )";

        cd->cpp += "\nvoid rgeditor_generated_code::draw(rocket::renderer_2d *ren, rocket::glfw_window_t *win) {\n";
        cd->cpp += "    using namespace rgeditor_generated_code;\n";
        cd->cpp += "    static bool first_draw = true;\n";
        cd->cpp += "    static rocket::asset_manager_t am;\n";
        for (auto &obj : objs) {
            if (obj.texture != nullptr) {
                cd->cpp += "        static std::shared_ptr<rocket::texture_t> " + get_variable_safe_path_name(obj.texture->path) + " = nullptr;\n";
            }
        }
        cd->cpp += "    if (first_draw) {\n";
        for (auto &obj : objs) {
            if (obj.texture != nullptr) {
                cd->cpp += std::string("        ") + get_variable_safe_path_name(obj.texture->path) + " = " + "am.get_texture(am.load_texture(\"" + obj.texture->path + "\"));\n";
            }
        }
        cd->cpp += "        first_draw = false;\n";
        cd->cpp += "    }\n";
        std::sort(objs.begin(), objs.end(), [](const game_object_t &a, const game_object_t &b) {
            return a.draw_order < b.draw_order;
        });

        std::ostringstream cpp;

        for (auto &obj : objs) {
            cpp << "    // " << obj.name << "\n";
            if (obj.texture) {
                cpp << "    ren->draw_texture("
                    <<  get_variable_safe_path_name(obj.texture->path) + ", "
                    << "{{ " << obj.position.x << ".f, " << obj.position.y << ".f }, "
                    << "{ "  << obj.size.x     << ".f, " << obj.size.y     << ".f }}, "
                    << obj.rotation << ".f);\n";
            } else {
                cpp << "    ren->draw_rectangle("
                    << "{ " << obj.position.x << ".f, " << obj.position.y << ".f }, "
                    << "{ " << obj.size.x     << ".f, " << obj.size.y     << ".f }, "
                    << "{ " << (int)obj.color.x << ", " << (int)obj.color.y
                    << ", " << (int)obj.color.z << ", " << (int)obj.color.w << " }, "
                    << obj.rotation << ".f);\n";
            }
        }
        cd->cpp += cpp.str();
        cd->cpp += "}\n";
        return cd;
    }
}
