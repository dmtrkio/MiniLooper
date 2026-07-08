#include "session_manager.h"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <iostream>

#include "json.h"
#include "audio/wav.h"
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
        std::scoped_lock lock(sessionsPathMut_);
        return sessionsPath_;
    }

    void SessionManager::setSessionsPath(const std::filesystem::path& sessionPath)
    {
        if (!std::filesystem::exists(sessionPath)) return;
        std::scoped_lock lock(sessionsPathMut_);
        sessionsPath_ = sessionPath;
    }

    std::expected<void, std::string> SessionManager::saveCurrentSession(const looper::Looper& looper) const
    {
        const auto session = looper.getSessionData();

        if (std::ranges::all_of(session->frameCounts, [](const auto fc) { return fc == 0; })) {
            return {};
        }

        const auto currentSessionPath = [&] {
            const auto path = getSessionsPath();
            return path / generateUniqueFileName(path);
        }();

        const auto tracksPath = currentSessionPath / tracksSubDirName;
        std::filesystem::create_directories(currentSessionPath);
        std::filesystem::create_directories(tracksPath);

        const auto sampleRate = audioEngine_.getSampleRate();

        const auto beatLengthInSamples = [&] {
            const auto& state = looper.getLooperState();
            assert(state.beatLength.has_value());
            return state.beatLength.value_or(1);
        }();

        const auto approxBpm = (sampleRate * 60.0f) / static_cast<float>(beatLengthInSamples);

        json j;

        j["metadata"] = MetaData{
            .bpm = approxBpm,
            .beatLengthInSamples = beatLengthInSamples,
            .sampleRate = static_cast<int>(sampleRate)
        };

        json &tracksJson = j["tracks"];

        for (int i = 0; i < looper.getNumLooperTracks(); ++i) {
            const float *data[2] = { session->leftBuffers[i], session->rightBuffers[i] };
            if (const auto framesToWrite = session->frameCounts[i]; framesToWrite > 0) {
                const auto trackName = std::format("track_{}", i + 1);
                const auto fileName = std::format("{}.wav", trackName);

                tracksJson[trackName] = TrackData{
                    .filename = fileName,
                    .length = framesToWrite,
                    .offset = looper.getTrackState(i).offset
                };

                const auto filePath = tracksPath / fileName;
                audio::WavWriter wavWriter(filePath, audioEngine_.getSampleRate(), 2);
                wavWriter.writeFrames(data, framesToWrite);
            }
        }

        j["settings"] = looper.getSettingsAsJson();

        const auto filePath = currentSessionPath / sessionInfoFileName;

        if (const auto r = saveJsonToFile(filePath.string(), j); !r) {
            return r;
        }

        std::cout << "Saved session at " << currentSessionPath << std::endl;
        return {};
    }

    std::expected<void, std::string> SessionManager::loadSession(
        looper::Looper& looper,
        const std::filesystem::path& sessionPath
    )
    {
        namespace fs = std::filesystem;

        if (!fs::exists(sessionPath)) {
            const auto err = std::format("Path {} does not exist", sessionPath.string());
            return std::unexpected(std::move(err));
        }

        const auto sessionInfoPath = sessionPath / sessionInfoFileName;

        if (!fs::is_regular_file(sessionInfoPath)) {
            const auto err = std::format("No {} file found in {}", sessionInfoFileName, sessionPath.string());
            return std::unexpected(std::move(err));
        }

        const auto jsonResult = loadJsonFromFile(sessionInfoPath.string());
        if (!jsonResult) {
            return std::unexpected(std::format(
                "Error loading session data from {}: {}",
                sessionInfoPath.string(),
                jsonResult.error()
            ));
        }

        const auto& j = *jsonResult;

        const auto metadataResult = [&] -> std::expected<MetaData, std::string> {
            if (const auto m = findByKey(j, "metadata"))
                return parse<MetaData>(*m);
            return std::unexpected("Missing metadata");
        }();

        if (!metadataResult) {
            return std::unexpected(metadataResult.error());
        }

        const auto& metadata = *metadataResult;

        std::vector<std::optional<std::pair<audio::WavData, TrackData>>> loadedAudio;
        loadedAudio.assign(looper.getNumLooperTracks(), std::nullopt);

        if (const auto tracksJson = findByKey(j, "tracks")) {
            const auto tracksDir = sessionPath / tracksSubDirName;

            for (int i = 1; i <= looper.getNumLooperTracks(); ++i) {
                const auto trackResult = findByKey(tracksJson, std::format("track_{}", i));
                if (!trackResult) continue;

                const auto trackData = parse<TrackData>(*trackResult);
                if (!trackData) {
                    return std::unexpected(std::format("Error loading track {}: {}", i, trackData.error()));
                }

                const auto wav = audio::readWavFile(tracksDir / trackData->filename);
                if (!wav) {
                    return std::unexpected(std::format("Error loading track {}: {}", i, wav.error()));
                }

                const bool valid = wav->nChannels == 2
                                && wav->sampleRate == metadata.sampleRate
                                && wav->frameCount == trackData->length;

                if (!valid) {
                    return std::unexpected(std::format("Error loading track {}: metadata mismatch", i));
                }

                loadedAudio[i - 1] = std::make_pair(std::move(*wav), *trackData);
            }
        }

        looper::LooperSession session;

        for (int i = 0; i < looper.getNumLooperTracks(); ++i) {
            if (auto o = loadedAudio[i]) {
                auto [audio, data] = std::move(*o);
                session.buffers[i][0] = std::move(audio.data[0]);
                session.buffers[i][1] = std::move(audio.data[1]);
                session.frameCounts[i] = audio.frameCount;
                session.offsets[i] = data.offset;
            }
        }

        session.framesInBeat = metadata.beatLengthInSamples;

        if (const auto success = audioEngine_.stop(); !success) {
            return std::unexpected(success.error());
        }

        audioEngine_.setSampleRate(metadata.sampleRate);

        looper.loadSession(std::move(session));

        if (const auto settingsJson = findByKey(j, "settings")) {
            looper.loadSettingsFromJson(*settingsJson);
        } 

        if (const auto success = audioEngine_.start(); !success) {
            return std::unexpected(success.error());
        }

        return {};
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

        SDL_ShowOpenFolderDialog(
            cb,
            this,
            nullptr,
            getSessionsPath().string().c_str(),
            false
        );
    }

    void SessionManager::openLoadSessionDialog()
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
            
            std::scoped_lock lock(sessionManager->pendingLoadMut_);
            sessionManager->pendingLoadPath_ = std::filesystem::path(*filelist);
        };

        SDL_ShowOpenFolderDialog(
            cb,
            this,
            nullptr,
            getSessionsPath().string().c_str(),
            false
        );
    }

    std::expected<void, std::string> SessionManager::pollPendingSessionToLoad(looper::Looper& looper)
    {
        std::optional<std::filesystem::path> pathToLoad;
        
        {
            std::scoped_lock lock(pendingLoadMut_);
            if (pendingLoadPath_.has_value()) {
                pathToLoad = std::move(pendingLoadPath_);
                pendingLoadPath_.reset();
            }
        }
        
        if (pathToLoad) {
            if (const auto r = loadSession(looper, *pathToLoad); !r) {
                const auto err = std::format("Failed to load session: {}", r.error());
                return std::unexpected(std::move(err));
            }
        }

        return {};
    }
}