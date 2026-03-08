#ifndef RGEDITOR__CODE_GENERATOR_HPP
#define RGEDITOR__CODE_GENERATOR_HPP

#include <string>
namespace rgeditor {
    struct generated_code_t {
        std::string hpp;
        std::string cpp;
    };

    generated_code_t *gen();
}

#endif//RGEDITOR__CODE_GENERATOR_HPP
