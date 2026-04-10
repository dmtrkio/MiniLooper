#pragma once

#include <filesystem>
#include <thread>

#include "looper/looper.h"

class SessionManager
{
public:
    SessionManager();

    std::filesystem::path getSessionsPath() const;
    void setSessionsPath(const std::filesystem::path& sessionPath);
    void openSessionsPathDialog();
    void saveCurrentSessionToDisk(const looper::Looper& looper) const;

private:
    mutable std::mutex mut_;
    std::filesystem::path sessionsPath_;
};