#include <algorithm>
#include <iostream>
#include <numbers>
#include <cmath>

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

        looper.process(out, nFrames);

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
        //std::cout << "onStart()\n";
        looper.onStart();
    }

    void onStop() override
    {
        //std::cout << "onStop()\n";
        looper.onStop();
    }

    looper::Looper looper;
};

int main()
{
    auto& engine = audio::AudioEngine::getInstance();
    auto cb = std::make_shared<LooperCallback>();
    engine.setAudioCallback(cb);
    engine.setSampleRate(48000);
    engine.setBufferSize(64);
    engine.pickDevices();

    if (!engine.start()) {
        std::cerr << "Failed to start audio engine.\n";
        exit(EXIT_FAILURE);
    }

    if (!engine.isRunning()) {
        std::cerr << "Audio engine not running.\n";
        exit(EXIT_FAILURE);
    }

    std::cout << "Audio engine started\n";

    SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(800, 600, "MainLooper");
    SetTargetFPS(60);
    SetExitKey(KEY_ESCAPE);

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(DARKGRAY);

        DrawText("Quit[Escape] StartRecording[r] StopRecording[s] Clear[c]", 40, 100, 20, BLACK);

        const int x = GetScreenWidth() / 2;
        const int y = GetScreenHeight() / 2;
        const int radius = 60;
        const auto nFramesInLoop = cb->looper.getCurrentNumFrames();
        const auto loopPosition = cb->looper.getCurrentPosition();

        const Color outlineColor = BLACK;
        const Color emptyColor = GRAY;
        const Color filledColor = LIGHTGRAY;

        DrawCircle(x, y, radius + 3.0f, outlineColor);
        DrawCircle(x, y, radius, emptyColor);
        if (nFramesInLoop > 0) {
            constexpr auto startAngle = 270.f;
            const auto endAngle = 360.0f * (static_cast<float>(loopPosition) / static_cast<float>(nFramesInLoop)) + startAngle;
            DrawCircleSector({static_cast<float>(x),static_cast<float>(y)}, radius, startAngle, endAngle, 32, filledColor);
        }
        DrawCircle(x, y, radius * 0.4f, outlineColor);

        auto &looperMailbox = cb->looper.getCommandMailbox();

        if (IsKeyPressed(KEY_R)) {
            looperMailbox.tryPush(looper::LooperCommand::startRecording());
        } else if (IsKeyPressed(KEY_S)) {
            looperMailbox.tryPush(looper::LooperCommand::stopRecording());
        } else if (IsKeyPressed(KEY_C)) {
            looperMailbox.tryPush(looper::LooperCommand::clear());
        }

        EndDrawing();
    }

    CloseWindow();

    if (engine.stop())
        std::cout << "Audio engine stopped successfully.\n";

    return 0;
}
