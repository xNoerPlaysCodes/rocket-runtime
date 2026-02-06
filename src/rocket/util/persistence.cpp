#include <cfloat>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <rocket/persistence.hpp>
#include <rocket/runtime.hpp>
#include <nlohmann/json.hpp>
// FIXME crossplatform

namespace nm = nlohmann;
namespace rocket::storage {
    std::filesystem::path get_data_storage_path() {
        std::filesystem::path path;
        if (char *home = getenv("USERPROFILE"); home != nullptr) {
            path = std::filesystem::path(home) / "AppData" / "Roaming";
        } else if (char *home = getenv("HOME"); home != nullptr) {
            path = std::filesystem::path(home) / ".config";
        }

        return path;
    }

    std::filesystem::path data_path;
    std::filesystem::path vars_path;

    data_t data;
    nm::json j;

    variable_t json_to_variant(const nm::json &v) {
        if (v.is_null()) {
            return nullptr;
        } 
        else if (v.is_boolean()) {
            return v.get<bool>();
        } 
        else if (v.is_string()) {
            return v.get<std::string>();
        } 
        else if (v.is_number_integer()) {
            int64_t n = v.get<int64_t>();

            if (n >= 0) {
                if (n <= UINT8_MAX) return static_cast<uint8_t>(n);
                if (n <= UINT16_MAX) return static_cast<uint16_t>(n);
                if (n <= UINT32_MAX) return static_cast<uint32_t>(n);
                return static_cast<uint64_t>(n);
            } else {
                if (n >= INT8_MIN && n <= INT8_MAX) return static_cast<int8_t>(n);
                if (n >= INT16_MIN && n <= INT16_MAX) return static_cast<int16_t>(n);
                if (n >= INT32_MIN && n <= INT32_MAX) return static_cast<int32_t>(n);
                return static_cast<int64_t>(n);
            }
        } 
        else if (v.is_number_float()) {
            double f = v.get<double>();
            if (f >= -FLT_MAX && f <= FLT_MAX) return static_cast<float>(f);
            return f;
        }

        throw std::runtime_error("Unsupported JSON type for variant");
    }

    void init(std::string name) {
        if (name.empty()) {
            rocket::log("Name may not be empty", "rocket::storage", "init", "error");
            return;
        }
        data_path = get_data_storage_path() / ("RocketGE_" + name);
        vars_path = data_path / "rocket-runtime-persistent-storage.dat";
        if (!std::filesystem::exists(data_path)) {
            std::filesystem::create_directories(data_path);
        }

        if (!std::filesystem::exists(vars_path)) {
            std::ofstream _ = vars_path;
            _.close();
        }
        {
            std::ifstream f = vars_path;
            if (!f.is_open()) {
                rocket::log("Persistent Storage couldn't be opened", "rocket::storage", "init", "error");
                return;
            }

            std::vector<uint8_t> buffer((std::istreambuf_iterator<char>(f)),
                std::istreambuf_iterator<char>());

            rocket::log("Persistent storage opened", "rocket::storage", "init", "debug");

            if (buffer.size() == 0) return;

            j = nm::json::from_cbor(buffer);

            f.close();

            for (auto &[k, v] : j.items()) {
                variable_t var = json_to_variant(v);

                std::visit([&](auto&& x){
                    data[k] = x;
                }, var);
            }
        }
    }

    data_t* load() {
        return &data;
    }

    void store(const std::string &name, const variable_t &value) {
        data[name] = value;
    }

    variable_t& get(const std::string &name) {
        return data[name];
    }

    nm::json to_json() {
        for (auto& [k, v] : data) {
            j[k] = std::visit([](auto&& value) -> nm::json {
                return value;
            }, v);
        }

        return j;
    }

    void flush() {
        nm::json j = to_json();
        std::vector<uint8_t> bytes = nm::json::to_cbor(j);
        std::ofstream f(vars_path, std::ios::binary);
        f.write(reinterpret_cast<char*>(bytes.data()), bytes.size());
        f.close();
    }
}
