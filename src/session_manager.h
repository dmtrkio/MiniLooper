#pragma once

#include <filesystem>
#include <mutex>
#include <expected>
#include <string_view>
#include <optional>

#include "audio/audio_engine.h"
#include "looper/looper.h"

namespace ml {
    class SessionManager
    {
    public:
        SessionManager(audio::AudioEngine& audioEngine);

        [[nodiscard]] std::filesystem::path getSessionsPath() const;
        void setSessionsPath(const std::filesystem::path& sessionPath);

        [[nodiscard]] std::expected<void, std::string> saveCurrentSession(const looper::Looper& looper) const;

        [[nodiscard]] std::expected<void, std::string> loadSession(
            looper::Looper& looper,
            const std::filesystem::path& sessionPath
        );

        void openSessionsPathDialog();
        void openLoadSessionDialog();
        [[nodiscard]] std::expected<void, std::string> pollPendingSessionToLoad(looper::Looper& looper);

    private:
        struct MetaData
        {
            float bpm;
            int beatLengthInSamples;
            int sampleRate;
            NLOHMANN_DEFINE_TYPE_INTRUSIVE(MetaData, bpm, beatLengthInSamples, sampleRate)
        };

        struct TrackData
        {
            std::string filename;
            int length;
            int offset;
            NLOHMANN_DEFINE_TYPE_INTRUSIVE(TrackData, filename, length, offset)
        };

        static constexpr std::string_view sessionInfoFileName = "session_info.json";
        static constexpr std::string_view tracksSubDirName = "tracks";

        audio::AudioEngine& audioEngine_;

        mutable std::mutex sessionsPathMut_;
        std::filesystem::path sessionsPath_;

        mutable std::mutex pendingLoadMut_;
        std::optional<std::filesystem::path> pendingLoadPath_;
    };
}