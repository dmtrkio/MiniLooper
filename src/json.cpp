#include "json.h"

#include <fstream>

namespace ml {
    std::expected<void, std::string> saveJsonToFile(const std::string& filename, const json& j) noexcept
    {
        try {
            std::ofstream file(filename);
            if (!file.is_open()) {
                const auto err = std::format("Failed to open file for writing {}", filename);
                return std::unexpected(std::move(err));
            }

            file << j.dump(4);
            return {};
        } catch (const std::exception& e) {
            return std::unexpected(e.what());
        }
    }

    std::expected<json, std::string> loadJsonFromFile(const std::string& filename) noexcept
    {
        try {
            std::ifstream file(filename);
            if (!file.is_open()) {
                const auto err = std::format("Failed to open file for reading {}", filename);
                return std::unexpected(std::move(err));
            } else {
                return json::parse(file);
            }
        } catch (const std::exception& e) {
            return std::unexpected(e.what());
        }
    }

    std::optional<json> findByKey(const json& j, std::string_view key) noexcept
    {
        if (const auto it = j.find(key); it != j.end()) {
            return *it;
        } else {
            return std::nullopt;
        }
    }
}