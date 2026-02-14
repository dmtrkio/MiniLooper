#include <algorithm>
#include <iostream>
#include <numbers>
#include <cmath>

#include <raylib.h>
#include <raymath.h>

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

void looperIndicator(int x, int y, float radius, unsigned int nFramesInLoop, unsigned int loopPosition)
{

    const Color outlineColor = BLACK;
    const Color emptyColor = GRAY;
    const Color filledColor = LIGHTGRAY;
    const float outlineThickness = 3.0f;

    const Vector2 origin = {static_cast<float>(x), static_cast<float>(y)};

    DrawCircle(x, y, radius + outlineThickness, outlineColor);
    DrawCircle(x, y, radius, emptyColor);
    if (nFramesInLoop > 0) {
        constexpr auto startAngle = 270.f;
        const auto endAngle = 360.0f * (static_cast<float>(loopPosition) / static_cast<float>(nFramesInLoop)) + startAngle;
        DrawCircleSector(origin, radius, startAngle, endAngle, 32, filledColor);

        const auto angleRadians = endAngle * DEG2RAD;
        const Vector2 end = {
            static_cast<float>(x) + std::cos(angleRadians) * radius,
            static_cast<float>(y) + std::sin(angleRadians) * radius
        };
        DrawLineEx(origin, end, outlineThickness, outlineColor);
        const Vector2 up = origin + Vector2(0.0f, -1.0f) * radius;
        DrawLineEx(origin, up, outlineThickness, outlineColor);
    }
    DrawCircle(x, y, radius * 0.5f, outlineColor);
}

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

        DrawText("Quit[Escape] StartRecording[r] StopRecording[s] Clear[c]", 50, 100, 20, BLACK);

        const int x = GetScreenWidth() / 2;
        const int y = GetScreenHeight() / 2;
        const float radius = 60.0f;
        const auto nFramesInLoop = cb->looper.getCurrentNumFrames();
        const auto loopPosition = cb->looper.getCurrentPosition();
        looperIndicator(x, y, radius, nFramesInLoop, loopPosition);

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
