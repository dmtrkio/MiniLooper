#pragma once

#include <filesystem>

#include "looper/looper.h"

class SessionManager
{
public:
    SessionManager();

    void setSessionsPath(const std::filesystem::path& sessionPath);

    void saveSessionToDisk(const looper::Looper& looper) const;

private:
    std::filesystem::path sessionsPath_;
};