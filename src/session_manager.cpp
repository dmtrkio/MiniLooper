#include "session_manager.h"

#include <algorithm>
#include <chrono>

#include "SDL3/SDL.h"

#include "audio/wav_writer.h"
#include "filepaths.h"

static std::string makeTimestamp()
{
    using namespace std::chrono;

    const auto now = system_clock::now();
    auto z = current_zone();
    std::cout << z->name() << std::endl;
    return std::format("{:%Y-%m-%d_%H-%M-%S}", current_zone()->to_local(now));
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

    const auto currentSessionPath = sessionsPath_ / ("session_" + makeTimestamp());

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