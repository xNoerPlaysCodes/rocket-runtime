#ifndef RGEDITOR__CODE_GENERATOR_HPP
#define RGEDITOR__CODE_GENERATOR_HPP

#include "rocket/asset.hpp"
#include "rocket/types.hpp"
#include <memory>
#include <string>
struct data_1_t {};
struct data_2_t {};

struct game_object_t {
    int id = -1;
    std::string name = "Newly Created Game Object";
    rocket::vec2f_t position;
    rocket::vec2f_t size;
    rocket::rgba_color color;
    float rotation   = 0;
    int draw_order   = 0;
    std::shared_ptr<rocket::texture_t> texture = nullptr;
    data_1_t *data_1 = nullptr;
    data_2_t *data_2 = nullptr;
};

namespace rgeditor {
    struct generated_code_t {
        std::string hpp;
        std::string cpp;
    };

    generated_code_t *gen(std::vector<game_object_t> objs);
}

#endif//RGEDITOR__CODE_GENERATOR_HPP
