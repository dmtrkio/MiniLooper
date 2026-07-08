#pragma once

#include <string>
#include <string_view>
#include <expected>
#include <optional>

#include <nlohmann/json.hpp>

namespace ml {
    using json = nlohmann::ordered_json;

    [[nodiscard]] std::expected<void, std::string> saveJsonToFile(const std::string& filename, const json& j) noexcept;
    [[nodiscard]] std::expected<json, std::string> loadJsonFromFile(const std::string& filename) noexcept;

    [[nodiscard]] std::optional<json> findByKey(const json& j, std::string_view key) noexcept;

    template<typename T>
    [[nodiscard]] std::expected<T, std::string> parse(const json& j) noexcept
    {
        try {
            return j.get<T>();
        } catch (const std::exception& e) {
            return std::unexpected(e.what());
        }
    }
}