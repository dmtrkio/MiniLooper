#include "json.h"

#include <iostream>
#include <fstream>

bool saveJsonToFile(const std::string& filename, const json& j)
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

std::optional<json> loadJsonFromFile(const std::string& filename)
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