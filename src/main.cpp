#include <algorithm>
#include <iostream>
#include <cmath>

#include <libremidi/libremidi.hpp>

#if defined(_WIN32) && __has_include(<winrt/base.h>)
  #include <winrt/base.h>
#endif

#include <raylib.h>
#include <raymath.h>

#include "audio/audio_engine.h"
#include "looper/looper.h"

inline std::ostream& operator<<(std::ostream& s, const libremidi::message& message)
{
    auto nBytes = message.size();
    s << "[ ";
    for (auto i = 0U; i < nBytes; i++)
        s << std::hex << (int)message[i] << std::dec << " ";
    s << "]";
    if (nBytes > 0)
        s << " ; stamp = " << message.timestamp;
    return s;
}

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
#if defined(_WIN32) && __has_include(<winrt/base.h>)
    // Necessary for using WinUWP and WinMIDI, must be done as early as possible in your main()
    winrt::init_apartment();
#endif

    for (const auto& api : libremidi::available_apis()) {
        std::cout << "API name: " << libremidi::get_api_name(api) << std::endl;
        libremidi::observer midi{{.track_hardware = true, .track_virtual = true}, libremidi::observer_configuration_for(api)};

        std::cout << "Available midi input devices: " << std::endl;
        for (const auto& port : midi.get_input_ports()) {
            std::cout << std::endl;
            //std::cout << "API: " << libremidi::get_api_name(port.api) << std::endl;
            std::cout << "Port: " << port.port << std::endl;
            std::cout << "Manufacturer: " << port.manufacturer << std::endl;
            std::cout << "Product: " << port.product << std::endl;
            std::cout << "Serial: " << port.serial << std::endl;
            std::cout << "Device name: " << port.device_name << std::endl;
        }
        std::cout << std::endl;
    }

    libremidi::midi_in midi_in{{.on_message = [](const libremidi::message& message) {
        std::cout << message << std::endl;
    }}};

    libremidi::observer observer;
    if (auto err = midi_in.open_port(observer.get_input_ports()[0]); err != stdx::error{})
        err.throw_exception();

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
