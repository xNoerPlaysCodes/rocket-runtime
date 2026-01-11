#include <iostream>
#include <string>
#include <libquarrel/parser.hpp>
#include <algorithm>

namespace quarrel {
    parser_t::parser_t(int argc, char **argv, uint8_t prefixes_allowed) {
        this->argc = argc;
        this->argv = argv;
        this->prefixes_allowed = prefixes_allowed;
    }

    void parser_t::register_argument(std::string argument, bool value_req, uint8_t override_prefixes) {
        this->args.push_back({ argument, value_req, override_prefixes });
    }

    void parser_t::set_callback(argument_callback_t cb) {
        this->cb = cb;
    }

    bool parser_t::parse() {
        bool error = false;
        bool exit = false;
        for (int i = 1; i < argc; ++i) {
            int nexti = i + 1;
            std::string arg = argv[i];
            std::string prefix;

            std::string value = "";
            if (nexti != argc) {
                if (argv[nexti][0] != '-' && argv[nexti][0] != '/') {
                    value = argv[nexti];
                }
            }

            bool argument_detected = false;
            bool too_many_prefixes = false;

            if (prefixes_allowed & PREFIX_GNU && arg.starts_with("--")) {
                arg = arg.substr(2);
                prefix = "--";
                too_many_prefixes = argument_detected;
                argument_detected = true;
            } else if (prefixes_allowed & PREFIX_JAVA && arg.starts_with("-")) {
                arg = arg.substr(1);
                prefix = "-";
                too_many_prefixes = argument_detected;
                argument_detected = true;
            } else if (prefixes_allowed & PREFIX_WIN && arg.starts_with("/")) {
                arg = arg.substr(1);
                prefix = "/";
                too_many_prefixes = argument_detected;
                argument_detected = true;
            }

            if (too_many_prefixes) {
                exit = true;
                error = true;
            }

            {
                argument_t _arg = {arg, false, /* no need to set */};
                if (value.empty() && std::find(args.begin(), args.end(), _arg) != args.end()) {
                    exit = true;
                    error = true;
                }
            }

            if (!value.empty()) ++i;

            if ((arg.empty() && prefix == "--") || prefix.empty()) {
                break;
            }

            bool found = false;
            argument_t argument;
            for (auto &a : args) {
                if (a.arg == arg) {
                    found = true;
                    argument = a;
                }
            }

            if (found) {
                if (argument.prefixes != 255) {
                    int iprefix = 255;
                    if (prefix == "/") {
                        iprefix = PREFIX_WIN;
                    } else if (prefix == "--") {
                        iprefix = PREFIX_GNU;
                    } else if (prefix == "-") {
                        iprefix = PREFIX_JAVA;
                    }

                    if (!(argument.prefixes & iprefix)) {
                        continue;
                    }
                }

                if (!this->cb(argument.arg, argument.value_required ? value : "")) {
                    error = true;
                    exit = true;
                }
            }
        }

        return exit && error;
    }
}
