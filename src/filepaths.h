#pragma once

#include <SDL3/SDL.h>
#include <filesystem>

#include "config.h"

namespace filepaths {
    inline std::filesystem::path homeDirPath()
    {
        const char* raw = SDL_GetUserFolder(SDL_FOLDER_HOME);
        if (!raw) {
            throw std::runtime_error("SDL_GetUserFolder failed");
        }

        std::filesystem::path path(raw);
        return path;
    }

    inline std::filesystem::path prefDirPath()
    {
        char* raw = SDL_GetPrefPath(build::kOrganization, build::kProjectName);
        if (!raw) {
            throw std::runtime_error("SDL_GetPrefPath failed");
        }

        std::filesystem::path path(raw);
        SDL_free(raw);
        return path;
    }

    inline std::filesystem::path configDirPath()
    {
        const auto path = prefDirPath() / "config";
        std::filesystem::create_directories(path);
        return path;
    }

    inline std::filesystem::path settingsPath()
    {
        static constexpr auto kSettingsFileName = "settings.json";
        return configDirPath() / kSettingsFileName;
    }

    inline const char* imguiIniPath()
    {
        static std::string path = (configDirPath() / "imgui.ini").string();
        return path.c_str();
    }

    inline std::filesystem::path defaultSaveDirPath()
    {
        auto path = homeDirPath() / build::kOrganization / build::kProjectName;
        std::filesystem::create_directories(path);
        return path;
    }
}