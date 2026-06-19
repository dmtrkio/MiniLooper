#pragma once

#include <optional>
#include <string>

#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

bool saveJsonToFile(const std::string& filename, const json& j);
std::optional<json> loadJsonFromFile(const std::string& filename);