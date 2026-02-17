#include <algorithm>
#include <iostream>
#include <cmath>

#include <raylib.h>
#include <raymath.h>

#include "audio/audio_engine.h"
#include "looper/looper.h"

void looperWidget(int x, int y, float radius, looper::Looper &looper)
{
    const auto [nFramesInLoop, loopPosition, state] = looper.getLooperState();

    const Color outlineColor = BLACK;
    const Color emptyColor = GRAY;
    const Color filledColor = LIGHTGRAY;
    const Color clearedColor = outlineColor;
    const Color recordingColor = RED;
    const Color overdubbingColor = ORANGE;
    const Color playbackColor = LIME;

    const float outlineThickness = 4.0f;

    const float widgetRadius = radius;

    const Vector2 origin = {static_cast<float>(x), static_cast<float>(y)};

    DrawCircle(x, y, radius, outlineColor);
    radius -= outlineThickness;
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

    auto indicatorRadius = radius * 0.5f;
    DrawCircle(x, y, indicatorRadius, outlineColor);
    indicatorRadius -= outlineThickness;

    Color indicatorColor = clearedColor;
    if (state == looper::LooperProcessor::State::RECORDING) {
        if (nFramesInLoop > 0) {
            indicatorColor = overdubbingColor;
        } else {
            indicatorColor = recordingColor;
        }
    } else if (state == looper::LooperProcessor::State::PLAYBACK) {
        indicatorColor = playbackColor;
    }

    DrawCircle(x, y, indicatorRadius, indicatorColor);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (Vector2Distance(GetMousePosition(), origin) < widgetRadius) {
            if (state != looper::LooperProcessor::State::RECORDING) {
                looper.startRecording();
            } else {
                looper.stopRecording();
            }
        }
    }
}

int main()
{
    looper::Looper looper;

    auto& engine = audio::AudioEngine::getInstance();
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

        looperWidget(x, y, radius, looper);

        if (IsKeyPressed(KEY_R)) {
            looper.startRecording();
        } else if (IsKeyPressed(KEY_S)) {
            looper.stopRecording();
        } else if (IsKeyPressed(KEY_C)) {
            looper.clear();
        }

        EndDrawing();
    }

    CloseWindow();

    if (engine.stop())
        std::cout << "Audio engine stopped successfully.\n";

    return 0;
}
