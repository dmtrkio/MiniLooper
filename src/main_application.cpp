#include "main_application.h"

#include <iostream>
#include <format>

#include "imgui.h"

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
        midiIsOn = true;
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
    looper_.updateSnapshot();

    for (auto trackIndex = 0; trackIndex < looper_.getNumLooperTracks(); ++trackIndex) {
        const auto key = ImGuiKey_1 + trackIndex;

        if (ImGui::Shortcut(key, ImGuiInputFlags_RouteGlobal)) {
            toggleRec(trackIndex);
        }

        if (ImGui::Shortcut(key | ImGuiMod_Shift, ImGuiInputFlags_RouteGlobal)) {
            togglePlay(trackIndex);
        }
    }

    if (ImGui::Shortcut(ImGuiKey_C, ImGuiInputFlags_RouteGlobal)) {
        looper_.clearAll();
    }

    looperUi();
}

void MainApplication::looperUi()
{
    const auto nTracks = looper_.getNumLooperTracks();

    ImGui::Begin("MiniLooper");

    for (auto i{0}; i < nTracks; ++i) {
        const auto trackIndex = i;
        const auto label = std::format("Track {}", trackIndex);
        ImGui::PushID(label.c_str());
        ImGui::BeginGroup();

        ImGui::Text("%s", label.c_str());
        trackUi(trackIndex);

        ImGui::EndGroup();
        ImGui::PopID();
        ImGui::Separator();
    }

    if (ImGui::Button("Clear All")) {
        looper_.clearAll();
    }

    ImGui::End();
}

void MainApplication::trackUi(int trackIndex)
{
    auto &track = looper_.getTrackState(trackIndex);

    ImGui::Text("State: %s", looper::stateToStr(track.state));

    const auto progress = (track.nFrames > 0) ? (static_cast<float>(track.position) / static_cast<float>(track.nFrames)) : 0.0f;
    ImGui::ProgressBar(progress);

    if (ImGui::Button((track.state == looper::State::Recording) ? "Stop" : "Record")) {
        toggleRec(trackIndex);
    }

    ImGui::SameLine();

    if (ImGui::Button((track.state == looper::State::Paused) ? "Resume" : "Pause")) {
        togglePlay(trackIndex);
    }

    ImGui::SameLine();

    if (track.state != looper::State::Cleared) {
        if (ImGui::Button("Clear")) {
            looper_.clear(trackIndex);
        }
    }
}

void MainApplication::toggleRec(int trackIndex)
{
    if (looper_.getTrackState(trackIndex).state != looper::State::Recording) {
        looper_.startRecording(trackIndex);
    } else {
        looper_.stopRecording(trackIndex);
    }
}

void MainApplication::togglePlay(int trackIndex)
{
    if (looper_.getTrackState(trackIndex).state == looper::State::Paused) {
        looper_.resume(trackIndex);
    } else {
        looper_.pause(trackIndex);
    }
}