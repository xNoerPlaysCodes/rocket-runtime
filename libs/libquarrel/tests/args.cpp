#include <iostream>
#include <libquarrel/parser.hpp>
#include <string>

bool cb(std::string arg, std::string value) {
    std::cout << arg << "=" << value << '\n';
    return true;
}

int main(int argc, char **argv) {
    quarrel::parser_t parser(argc, argv);
    parser.register_argument("version");
    parser.register_argument("v", false, quarrel::PREFIX_JAVA);
    parser.set_callback(cb);
    if (!parser.parse()) {
        std::cerr << "shit\n";
    }
}
