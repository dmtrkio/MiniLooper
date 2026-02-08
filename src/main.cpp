#include <iostream>
#include <thread>
#include <chrono>

#include "audio_engine.h"
#include "looper/looper.h"

class LooperCallback : public AudioCallback
{
public:
    void onProcess(const float **in, float **out, unsigned int nFrames) override
    {
        const auto& engine = AudioEngine::getInstance();
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
    auto& engine = AudioEngine::getInstance();
    auto cb = std::make_shared<LooperCallback>();
    engine.setAudioCallback(cb);
    engine.setSampleRate(48000);
    engine.setBufferSize(64);

    if (!engine.startStream()) {
        std::cerr << "Failed to start audio stream.\n";
        return 1;
    }

    std::cout << "Running audio stream\n";

    char c;
    while (true) {
        std::cout << "Quit[q] StartRecording[r] StopRecording[s] Clear[c]\n";
        std::cin >> c;

        if (c == 'q') {
            break;
        } else if (c == 'r') {
            cb->getCommandMailbox().tryPush(looper::LooperCommand::startRecording());
        } else if (c == 's') {
            cb->getCommandMailbox().tryPush(looper::LooperCommand::stopRecording());
        } else if (c == 'c') {
            cb->getCommandMailbox().tryPush(looper::LooperCommand::clear());
        }
    }

    engine.stopStream();
    std::cout << "Audio stream stopped successfully.\n";

    return 0;
}
