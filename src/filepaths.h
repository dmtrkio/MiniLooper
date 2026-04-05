#pragma once

#include <SDL3/SDL.h>
#include <filesystem>

#include "config.h"

namespace filepaths {
    inline std::filesystem::path prefPath()
    {
        void* raw = SDL_GetPrefPath(build::kOrganization, build::kProjectName);
        if (!raw) {
            throw std::runtime_error("SDL_GetPrefPath failed");
        }

        std::filesystem::path path(static_cast<const char*>(raw));
        SDL_free(raw);
        return path;
    }

    inline std::filesystem::path configPath()
    {
        const auto path = prefPath() / "config";
        std::filesystem::create_directories(path);
        return path;
    }

    inline std::filesystem::path savePath()
    {
        const auto path = prefPath() / "save";
        std::filesystem::create_directories(path);
        return path;
    }

    inline std::string settingsPath()
    {
        static constexpr auto kSettingsFileName = "settings.json";
        return (configPath() / kSettingsFileName).string();
    }

    inline const char* imguiIniPath()
    {
        static std::string path = (configPath() / "imgui.ini").string();
        return path.c_str();
    }
}