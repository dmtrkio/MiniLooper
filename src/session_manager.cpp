#include "session_manager.h"

#include <algorithm>

#include "SDL3/SDL.h"

#include "audio/wav_writer.h"
#include "filepaths.h"

static std::filesystem::path generateUniqueFileName(const std::filesystem::path& baseDir, const std::string& baseName = "Untitled")
{
    namespace fs = std::filesystem;
    fs::path candidate = baseDir / baseName;

    if (!fs::exists(candidate)) {
        return candidate.filename().string();
    }

    int counter = 1;
    while (true) {
        std::string newName = baseName + " " + std::to_string(counter);
        candidate = baseDir / newName;

        if (!fs::exists(candidate)) {
            return candidate.filename().string();
        }

        ++counter;
    }
}

SessionManager::SessionManager()
    : sessionsPath_(filepaths::defaultSaveDirPath()) {}

void SessionManager::setSessionsPath(const std::filesystem::path& sessionPath)
{
    sessionsPath_ = sessionPath;
}

void SessionManager::saveSessionToDisk(const looper::Looper& looper) const
{
    const auto session = looper.getSessionData();

    const auto currentSessionPath = sessionsPath_ / generateUniqueFileName(sessionsPath_);

    if (std::ranges::all_of(session->frameCounts,
                        session->frameCounts + looper.getNumLooperTracks(),
                        [](const unsigned frameCount) { return frameCount == 0; })) return;

    std::filesystem::create_directories(currentSessionPath);

    for (int i = 0; i < looper.getNumLooperTracks(); ++i) {
        float *data[2] = { session->leftBuffers[i], session->rightBuffers[i] };
        if (const auto framesToWrite = session->frameCounts[i]; framesToWrite > 0) {
            const auto filePath = currentSessionPath / std::format("looper_track_{}.wav", i);
            audio::WavWriter wavWriter(filePath, audio::AudioEngine::getInstance().getSampleRate(), 2);
            wavWriter.writeFrames(data, framesToWrite);
        }
    }
}