#include "session_manager.h"

#include <algorithm>
#include <cassert>
#include <iostream>

#include "json.h"
#include "audio/wav_writer.h"
#include "filepaths.h"

namespace ml {
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

    SessionManager::SessionManager(audio::AudioEngine& audioEngine)
        : audioEngine_(audioEngine)
        , sessionsPath_(filepaths::defaultSaveDirPath())
    {}

    std::filesystem::path SessionManager::getSessionsPath() const
    {
        std::unique_lock lock(mut_);
        return sessionsPath_;
    }

    void SessionManager::setSessionsPath(const std::filesystem::path& sessionPath)
    {
        if (!std::filesystem::exists(sessionPath)) return;
        std::unique_lock lock(mut_);
        sessionsPath_ = sessionPath;
    }

    void SessionManager::openSessionsPathDialog()
    {
        const SDL_DialogFileCallback cb = [](void *userdata, const char * const *filelist, int) {
            if (!filelist || !userdata) {
                std::cerr << "SDL_DialogFileCallback error" << std::endl;
                return;
            }

            if (!*filelist) {
                std::cerr << "SDL_DialogFileCallback no path selected" << std::endl;
                return;
            }

            const auto sessionManager = static_cast<SessionManager*>(userdata);
            sessionManager->setSessionsPath(*filelist);
        };

        SDL_ShowOpenFolderDialog(cb, this, nullptr, sessionsPath_.string().c_str(), false);
    }

    void SessionManager::saveCurrentSessionToDisk(const looper::Looper& looper) const
    {
        const auto session = looper.getSessionData();

        if (std::ranges::all_of(session->frameCounts, [](const auto fc) { return fc == 0; })) {
            return;
        }

        const auto currentSessionPath = [&] {
            const auto path = getSessionsPath();
            return path / generateUniqueFileName(path);
        }();

        std::filesystem::create_directories(currentSessionPath);

        const auto approxBpm = [&] {
            const auto& state = looper.getLooperState();
            assert(state.approxBPM.has_value());
            return state.approxBPM.value_or(0.0f);
        }();

        json j;
        auto &metadata = j["metadata"];
        metadata["bpm"] = approxBpm;
        metadata["sampleRate"] = audioEngine_.getSampleRate();

        std::vector<std::string> loopList;
        loopList.reserve(looper.getNumLooperTracks());

        for (int i = 0; i < looper.getNumLooperTracks(); ++i) {
            const float *data[2] = { session->leftBuffers[i], session->rightBuffers[i] };
            if (const auto framesToWrite = session->frameCounts[i]; framesToWrite > 0) {
                const auto fileName = std::format("loop_{}.wav", i);
                loopList.emplace_back(fileName);

                const auto filePath = currentSessionPath / fileName;
                audio::WavWriter wavWriter(filePath, audioEngine_.getSampleRate(), 2);
                wavWriter.writeFrames(data, framesToWrite);
            }
        }

        j["loops"] = loopList;
        const auto filePath = currentSessionPath / "session_info.json";
        saveJsonToFile(filePath.string(), j);

        std::cout << "Saved session at " << currentSessionPath << std::endl;
    }
}