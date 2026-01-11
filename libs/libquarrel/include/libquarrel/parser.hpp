#ifndef libQUARREL__arg_hpp
#define libQUARREL__arg_hpp

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
namespace quarrel {
    constexpr uint8_t PREFIX_GNU  = 0b10000000;
    constexpr uint8_t PREFIX_JAVA = 0b01000000;
    constexpr uint8_t PREFIX_WIN  = 0b00100000;

    using argument_callback_t = std::function<bool(std::string arg, std::string val)>;

    struct argument_t {
        std::string arg;
        bool value_required;
        uint8_t prefixes;

        bool operator==(argument_t &other) {
            return arg == other.arg && value_required == other.value_required;
        }

        bool operator==(argument_t other) {
            return arg == other.arg && value_required == other.value_required;
        }
    };

    class parser_t {
    private:
        int argc;
        char **argv;
        uint8_t prefixes_allowed;
        argument_callback_t cb;

        std::vector<argument_t> args;
    public:
        void register_argument(std::string arg, bool value_required = false, uint8_t override_prefixes = 255);
        void set_callback(argument_callback_t cb);
        [[nodiscard]] bool parse();
    public:
        parser_t(int argc, char **argv, uint8_t prefixes_allowed = PREFIX_GNU | PREFIX_WIN | PREFIX_JAVA);
    };
}

#endif//libQUARREL__arg_hpp
