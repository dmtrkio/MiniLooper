#include <iostream>
#include <thread>
#include <chrono>

#include "raylib.h"

#include "audio/audio_engine.h"
#include "looper/looper.h"

class LooperCallback : public audio::AudioCallback
{
public:
    void onProcess(const float **in, float **out, unsigned int nFrames) override
    {
        const auto& engine = audio::AudioEngine::getInstance();
        const auto iChannels = engine.getNumInputChannels();
        const auto oChannels = engine.getNumOutputChannels();

        if (iChannels > 0)
            for (auto c{0u}; c < oChannels; ++c)
                for (auto i{0u}; i < nFrames; ++i)
                    out[c][i] = in[0][i] * gain_;

        looper_.process(out, nFrames);
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
    float gain_{0.9f};
};

int main()
{
    auto& engine = audio::AudioEngine::getInstance();
    auto cb = std::make_shared<LooperCallback>();
    engine.setAudioCallback(cb);
    engine.setSampleRate(48000);
    engine.setBufferSize(64);

    if (!engine.startStream()) {
        std::cerr << "Failed to start audio stream.\n";
        return 1;
    }

    std::cout << "Running audio stream\n";

    InitWindow(800, 600, "MainLooper");
    SetTargetFPS(60);
    SetExitKey(KEY_ESCAPE);

    std::cout << "Quit[Escape] StartRecording[r] StopRecording[s] Clear[c]\n";

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

    engine.stopStream();
    std::cout << "Audio stream stopped successfully.\n";

    return 0;
}