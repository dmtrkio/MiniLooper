#include "main_application.h"

#include <algorithm>
#include <iostream>
#include <format>

#include "imgui.h"

#include "audio/audio_engine.h"
#include "ui/volume_meter.h"

std::unique_ptr<midi::MidiEngine> makeMidiEngine(looper::Looper &looper)
{
    try {
        auto midiEngine = std::make_unique<midi::MidiEngine>([&looper](int, midi::MidiMessage msg) {
            looper.sendMidiMessage(msg);
        });

        std::cout << "Midi engine started" << std::endl;
        return midiEngine;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << "Failed to start midi engine. Proceeding without it.\n";
        return nullptr;
    }
}

MainApplication::MainApplication(const int argc, const char* const* argv)
    : midiEngine_(makeMidiEngine(looper_))
    , midiSettingsUi_(midiEngine_.get())
    , looperUi_(looper_)
    , mixerUi_(looper_)
{
    (void)argc;
    (void)argv;

    auto& audioEngine = audio::AudioEngine::getInstance();
    audioEngine.setSampleRate(48000);
    audioEngine.setBufferSize(64);
    audioEngine.rescanDevices();

    if (!audioEngine.start() || !audioEngine.isRunning()) {
        throw std::runtime_error("Failed to start audio engine.");
    }

    std::cout << "Audio engine started\n";

    auto &style = ImGui::GetStyle();
    constexpr float rounding = 4.0f;
    style.FrameRounding = rounding;
    style.WindowRounding = rounding;
    style.ChildRounding = rounding;
    style.PopupRounding = rounding;
}

MainApplication::~MainApplication()
{
    if (audio::AudioEngine::getInstance().stop())
        std::cout << "Audio engine stopped successfully.\n";
}

void MainApplication::onFrame()
{
    looper_.updateSnapshot();

    ImGui::BeginMainMenuBar();

    ImGui::MenuItem("Audio settings", nullptr, &audioSettingsUi_.opened);
    ImGui::MenuItem("MIDI settings", nullptr, &midiSettingsUi_.opened);
    ImGui::MenuItem("Looper", nullptr, &looperUi_.opened);
    ImGui::MenuItem("Meter", nullptr, &showVolumeMeter_);
    ImGui::MenuItem("Mixer", nullptr, &mixerUi_.opened);

    ImGui::EndMainMenuBar();

    if (showVolumeMeter_) {
        ImGui::Begin("Volume Meter", &showVolumeMeter_);
        const auto [leftDb, rightDb] = looper_.getLooperState().level;
        ui::volumeMeter(leftDb, rightDb);
        ImGui::End();
    }

    audioSettingsUi_.draw();
    midiSettingsUi_.draw();
    looperUi_.draw();
    mixerUi_.draw();
}