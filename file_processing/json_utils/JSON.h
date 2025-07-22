#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include "json.hpp"

class JSON {
public:
    JSON() = default;
    ~JSON() = default;

    explicit JSON(const nlohmann::json& j) {
        if (j.is_object()) {
            for (auto& [key, value] : j.items()) {
                content[key] = JSON(value);
            }
        } else {
            data = j;
        }
    }

    static JSON fromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Unable to open file: " + filename);
        }

        nlohmann::json j;
        file >> j;
        return JSON(j);
    }

    JSON& operator[](const std::string& key) {
        return content[key];
    }

    template<typename T>
    T get() const {
        return data.get<T>();
    }

    bool contains(const std::string& key) const {
        return content.find(key) != content.end();
    }

private:
    std::unordered_map<std::string, JSON> content;
    nlohmann::json data;
};
