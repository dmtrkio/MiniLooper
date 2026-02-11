#include <algorithm>
#include <iostream>
#include <numbers>

#include "raylib.h"

#include "audio/audio_engine.h"
#include "looper/looper.h"

class LooperCallback : public audio::AudioCallback
{
public:
    void onProcess(const float *const *in, float *const *out, unsigned int nFrames) override
    {
        const auto& engine = audio::AudioEngine::getInstance();
        const auto iChannels = engine.getNumInputChannels();
        const auto oChannels = engine.getNumOutputChannels();

        if (iChannels > 0 && iChannels == oChannels) {
            for (auto c{0u}; c < oChannels; ++c) {
                for (auto i{0u}; i < nFrames; ++i) {
                    out[c][i] = in[c][i];
                }
            }
        }

        //looper_.process(out, nFrames);

        /*const auto sr = static_cast<float>(engine.getSampleRate());
        constexpr auto twoPi = 2.0f * std::numbers::pi_v<float>;
        const float phaseIncr = twoPi * 440.0f / sr;
        static float osc{0};
        for (auto i{0u}; i < nFrames; ++i) {
            osc += phaseIncr;
            if (osc >= twoPi) osc -= twoPi;
            const float sine = std::sin(osc) * 0.03f;
            for (auto c{0u}; c < oChannels; ++c) {
                out[c][i] = sine;
            }
        }*/
    }

    void onStart() override
    {
        std::cout << "onStart()\n";
        looper_.onStart();
    }

    void onStop() override
    {
        std::cout << "onStop()\n";
        looper_.onStop();
    }

    looper::LooperMailbox& getCommandMailbox() { return looper_.getCommandMailbox(); }

private:
    looper::Looper looper_;
};

int main()
{
/*#ifdef WIN32
    std::cout << "Windows not implemented yet\n";
    return 1;
#endif*/

    auto& engine = audio::AudioEngine::getInstance();
    auto cb = std::make_shared<LooperCallback>();
    engine.setAudioCallback(cb);
    engine.setSampleRate(48000);
    engine.setBufferSize(64);

    if (!engine.start()) {
        std::cerr << "Failed to start audio stream.\n";
        return 1;
    }

    if (engine.isStreamRunning()) {
        std::cout << "Running audio stream\n";
    } else {
        std::cerr << "No audio stream running.\n";
    }

    SetTraceLogLevel(LOG_ERROR);
    InitWindow(800, 600, "MainLooper");
    SetTargetFPS(60);
    SetExitKey(KEY_ESCAPE);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(WHITE);

        DrawText("Quit[Escape] StartRecording[r] StopRecording[s] Clear[c]", 40, 100, 20, BLACK);

        if (IsKeyPressed(KEY_R)) {
            cb->getCommandMailbox().tryPush(looper::LooperCommand::startRecording());
        } else if (IsKeyPressed(KEY_S)) {
            cb->getCommandMailbox().tryPush(looper::LooperCommand::stopRecording());
        } else if (IsKeyPressed(KEY_C)) {
            cb->getCommandMailbox().tryPush(looper::LooperCommand::clear());
        }

        EndDrawing();
    }

    CloseWindow();

    if (engine.stop())
        std::cout << "Audio stream stopped successfully.\n";

    return 0;
}
