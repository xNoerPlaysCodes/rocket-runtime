#ifndef RocketGE__persistence_hpp
#define RocketGE__persistence_hpp

#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>
namespace rocket::storage {
    using variable_t = std::variant<
        std::nullptr_t,

        std::string,

        double, float,
        uint8_t, uint16_t,
        uint32_t, uint64_t,

        int8_t, int16_t,
        int32_t, int64_t,

        bool
    >;
    using data_t = std::unordered_map<std::string, variable_t>;

    void init(std::string name);
    data_t* load();
    void store(const std::string &name, const variable_t &value);
    variable_t& get(const std::string &name);
    void flush();
}

#endif//RocketGE__persistence_hpp
