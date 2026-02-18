#include <algorithm>
#include <iostream>
#include <cmath>

#include <raylib.h>
#include <raymath.h>

#include "audio/audio_engine.h"
#include "looper/looper.h"

constexpr Color outlineColor = BLACK;
constexpr Color emptyColor = GRAY;
constexpr Color filledColor = LIGHTGRAY;
constexpr Color clearedColor = outlineColor;
constexpr Color recordingColor = RED;
constexpr Color overdubbingColor = ORANGE;
constexpr Color playbackColor = LIME;

constexpr float outlineThickness = 4.0f;

void looperTrackWidget(float x, float y, float radius, looper::Looper &looper, int trackIndex)
{
    const auto [nFramesInLoop, loopPosition, state] = looper.getLooperState(trackIndex);

    const float widgetRadius = radius;

    const Vector2 origin = {x, y};

    DrawCircleV(origin, radius, outlineColor);
    radius -= outlineThickness;
    DrawCircleV(origin, radius, emptyColor);

    if (nFramesInLoop > 0) {
        constexpr auto startAngle = 270.f;
        const auto endAngle = 360.0f * (static_cast<float>(loopPosition) / static_cast<float>(nFramesInLoop)) + startAngle;
        DrawCircleSector(origin, radius, startAngle, endAngle, 32, filledColor);

        const auto angleRadians = endAngle * DEG2RAD;
        const Vector2 end = {
            x + std::cos(angleRadians) * radius,
            y + std::sin(angleRadians) * radius
        };
        DrawLineEx(origin, end, outlineThickness, outlineColor);
        const Vector2 up = origin + Vector2(0.0f, -1.0f) * radius;
        DrawLineEx(origin, up, outlineThickness, outlineColor);
    }

    auto indicatorRadius = radius * 0.5f;
    DrawCircleV(origin, indicatorRadius, outlineColor);
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

    DrawCircleV(origin, indicatorRadius, indicatorColor);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (Vector2Distance(GetMousePosition(), origin) < widgetRadius) {
            if (state != looper::LooperProcessor::State::RECORDING) {
                looper.startRecording(trackIndex);
            } else {
                looper.stopRecording(trackIndex);
            }
        }
    }
}

void looperWidget(float x, float y, float width, looper::Looper &looper)
{
    const auto nTracks = looper.getNumLooperTracks();
    if (nTracks < 1) return;

    const float padding = 5.0f;
    //const float diameter = width / static_cast<float>(nTracks) - padding * static_cast<float>(nTracks + 1);
    const float diameter = (width - padding) / static_cast<float>(nTracks) - padding;
    const float radius = diameter / 2.0f;

    const float rectX = x - width / 2.0f;
    const float rectH = diameter + padding * 2.0f;
    const float rectY = y - padding - radius;

    const auto roundness = 0.7f;
    const auto nSegments = 16;
    DrawRectangleRounded({rectX - outlineThickness, rectY - outlineThickness, width + outlineThickness * 2.0f, rectH + outlineThickness * 2.0f},
                         roundness, nSegments, BLACK);
    DrawRectangleRounded({rectX, rectY, width, rectH}, roundness, nSegments, WHITE);

    const float startX = rectX + padding + radius;
    const float startY = y;

    for (int i = 0; i < nTracks; ++i) {
        const float trackX = startX + (diameter + padding) * static_cast<float>(i);
        const float trackY = startY;
        looperTrackWidget(trackX, trackY, radius, looper, i);
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

        //DrawText("Quit[Escape] StartRecording[r] StopRecording[s] Clear[c]", 50, 100, 20, BLACK);

        const float x = static_cast<float>(GetScreenWidth()) / 2.0f;
        const float y = static_cast<float>(GetScreenHeight()) / 2.0f;

        looperWidget(x, y, 440.0f, looper);

        if (IsKeyPressed(KEY_R)) {
            looper.startRecording(0);
        } else if (IsKeyPressed(KEY_S)) {
            looper.stopRecording(0);
        } else if (IsKeyPressed(KEY_C)) {
            looper.clearAll();
        }

        EndDrawing();
    }

    CloseWindow();

    if (engine.stop())
        std::cout << "Audio engine stopped successfully.\n";

    return 0;
}
