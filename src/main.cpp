#include <algorithm>
#include <iostream>
#include <cmath>

#include <raylib.h>
#include <raymath.h>

#include "timer.h"
#include "audio/audio_engine.h"
#include "looper/looper.h"
#include "midi/foot_switch.h"
#include "midi/midi.h"

constexpr Color backgroundColor = {50, 50, 60, 255};
constexpr Color outlineColor = BLACK;
constexpr Color emptyColor = DARKGRAY;
constexpr Color filledColor = LIGHTGRAY;
constexpr Color clearedColor = outlineColor;
constexpr Color recordingColor = {220, 60, 50, 255};
constexpr Color overdubbingColor = {200, 160, 190, 255};
constexpr Color playbackColor = {130, 170, 200, 255};
constexpr Color pausedColor = {90, 105, 140, 255};
constexpr Color looperWidgetColor = {155, 150, 160, 255};

constexpr float outlineThickness = 3.6f;

void looperTrackWidget(float x, float y, float radius, looper::Looper &looper, int trackIndex)
{
    const auto [nFramesInLoop, loopPosition, state] = looper.getTrackState(trackIndex);

    const float widgetRadius = radius;

    const Vector2 origin = {x, y};

    DrawCircleV(origin, radius, outlineColor);
    radius -= outlineThickness;
    DrawCircleV(origin, radius, emptyColor);

    if (nFramesInLoop > 0) {
        const auto relativePosition = static_cast<float>(loopPosition) / static_cast<float>(nFramesInLoop);
        constexpr auto startAngle = 270.f;
        const auto endAngle = 360.0f * relativePosition + startAngle;
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
    if (state == looper::State::RECORDING) {
        if (nFramesInLoop > 0) {
            indicatorColor = overdubbingColor;
        } else {
            indicatorColor = recordingColor;
        }
    } else if (state == looper::State::PLAYBACK) {
        indicatorColor = playbackColor;
    } else if (state == looper::State::PAUSED) {
        indicatorColor = pausedColor;
    }

    DrawCircleV(origin, indicatorRadius, indicatorColor);

    if (Vector2Distance(GetMousePosition(), origin) < widgetRadius) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (state != looper::State::RECORDING) {
                looper.startRecording(trackIndex);
            } else {
                looper.stopRecording(trackIndex);
            }
        } else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            if (state == looper::State::PAUSED) {
                looper.resume(trackIndex);
            } else {
                looper.pause(trackIndex);
            }
        }
    }
}

void looperWidget(float x, float y, float width, looper::Looper &looper)
{
    looper.updateSnapshot();
    const auto nTracks = looper.getNumLooperTracks();
    if (nTracks < 1) return;

    const float padding = 5.0f;
    const float diameter = (width - padding) / static_cast<float>(nTracks) - padding;
    const float radius = diameter / 2.0f;

    const float rectX = x - width / 2.0f;
    const float rectH = diameter + padding * 2.0f;
    const float rectY = y - padding - radius;

    const auto roundness = 0.7f;
    const auto nSegments = 16;
    DrawRectangleRounded({rectX - outlineThickness, rectY - outlineThickness, width + outlineThickness * 2.0f, rectH + outlineThickness * 2.0f},
                         roundness, nSegments, outlineColor);
    DrawRectangleRounded({rectX, rectY, width, rectH}, roundness, nSegments, looperWidgetColor);

    const float startX = rectX + padding + radius;
    const float startY = y;

    for (int i = 0; i < nTracks; ++i) {
        const float trackX = startX + (diameter + padding) * static_cast<float>(i);
        const float trackY = startY;
        looperTrackWidget(trackX, trackY, radius, looper, i);
    }
}

void looperInput(looper::Looper &looper)
{
    const auto nTracks = looper.getNumLooperTracks();

    for (int i = 0; i < nTracks; ++i) {
        const auto looperState = looper.getTrackState(i);
        if (IsKeyPressed(KEY_ONE + i)) {
            if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)) {
                if (looperState.state == looper::State::PAUSED) {
                    looper.resume(i);
                } else {
                    looper.pause(i);
                }
            } else {
                if (looperState.state != looper::State::RECORDING) {
                    looper.startRecording(i);
                } else {
                    looper.stopRecording(i);
                }
            }
        }
    }

    if (IsKeyPressed(KEY_C)) {
        looper.clearAll();
    }
}

int main()
{
    midi::MidiQueue midiQueue{64};
    std::unique_ptr<midi::MidiEngine> midiEngine;

    try {
        midiEngine = std::make_unique<midi::MidiEngine>([&](int, midi::MidiMessage msg) {
            midiQueue.tryPush(msg);
        });
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << "Failed to start midi engine. Proceeding without it.\n";
    }

    std::cout << "Midi engine started" << std::endl;

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

    midi::FootSwitch footSwitch;
    Timer pressTimer;
    pressTimer.onTimeout = [&]() {
        const int trackIndex = 0;
        const auto looperState = looper.getTrackState(trackIndex);
        if (looperState.state != looper::State::RECORDING) {
            looper.startRecording(trackIndex);
        } else {
            looper.stopRecording(trackIndex);
        }

        std::cout << "Foot switch pressed\n";
    };
    pressTimer.isOneShot = true;
    pressTimer.timeoutSecs = 0.4f;

    while (!WindowShouldClose()) {
        pressTimer.tick(GetFrameTime());
        midiQueue.consumeAll([&](const midi::MidiMessage& msg) {
            if (footSwitch.update(msg)) {
                if (pressTimer.isRunning()) {
                    pressTimer.stop();

                    looper.clearAll();

                    std::cout << "Foot switch double pressed\n";
                } else {
                    pressTimer.start();
                }
            }
        });

        BeginDrawing();
        ClearBackground(backgroundColor);

        const float x = static_cast<float>(GetScreenWidth()) / 2.0f;
        const float y = static_cast<float>(GetScreenHeight()) / 2.0f;

        looperWidget(x, y, 440.0f, looper);
        looperInput(looper);

        EndDrawing();
    }

    CloseWindow();

    if (engine.stop())
        std::cout << "Audio engine stopped successfully.\n";

    return 0;
}
