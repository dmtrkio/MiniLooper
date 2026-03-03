#include "main_application.h"

#include <iostream>

#include "audio/audio_engine.h"

MainApplication::MainApplication(const int argc, const char* const* argv)
{
    (void)argc;
    (void)argv;

    try {
        midiEngine_ = std::make_unique<midi::MidiEngine>([this](int, midi::MidiMessage msg) {
            this->looper_.sendMidiMessage(msg);
        });
        std::cout << "Midi engine started" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << "Failed to start midi engine. Proceeding without it.\n";
    }

    auto& audioEngine = audio::AudioEngine::getInstance();
    audioEngine.setSampleRate(48000);
    audioEngine.setBufferSize(64);
    audioEngine.pickDevices();

    if (!audioEngine.start() || !audioEngine.isRunning()) {
        throw std::runtime_error("Failed to start audio engine.");
    }

    std::cout << "Audio engine started\n";
}

MainApplication::~MainApplication()
{
    if (audio::AudioEngine::getInstance().stop())
        std::cout << "Audio engine stopped successfully.\n";
}

void MainApplication::onFrame()
{

}