#pragma once

#include <optional>
#include <string>
#include <iostream>
#include <fstream>

#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

inline bool saveJsonToFile(const std::string& filename, const json& j)
{
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Cound not open " << filename << std::endl;
            return false;
        }

        file << j.dump(4);
        return true;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

inline std::optional<json> loadJsonFromFile(const std::string& filename)
{
    try {
        std::ifstream file(filename);
        if (!file.is_open())
            return std::nullopt;
        return json::parse(file);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return std::nullopt;
    }
}