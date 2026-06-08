#pragma once

#include <filesystem>
#include <mutex>

#include "audio/audio_engine.h"
#include "looper/looper.h"

class SessionManager
{
public:
    SessionManager(audio::AudioEngine& audioEngine);

    std::filesystem::path getSessionsPath() const;
    void setSessionsPath(const std::filesystem::path& sessionPath);
    void openSessionsPathDialog();
    void saveCurrentSessionToDisk(const looper::Looper& looper) const;

private:
    audio::AudioEngine& audioEngine_;
    mutable std::mutex mut_;
    std::filesystem::path sessionsPath_;
};