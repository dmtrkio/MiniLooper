#include "main_application.h"

#include <algorithm>
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
    settings();
}

void MainApplication::settings()
{
    ImGui::Begin("Audio settings");

    audioEngineSettings();

    ImGui::End();
}

void MainApplication::audioEngineSettings()
{
    ImGui::BeginGroup();

    using namespace audio;

    auto &audioEngine = AudioEngine::getInstance();
    const auto &inputDevices = audioEngine.getInputDevices();
    const auto &outputDevices = audioEngine.getOutputDevices();

    const auto displayDeviceSettings = [&](const bool input) {
        ImGui::PushID(input ? "input device" : "output device");

        const auto index = input ? audioEngine.getCurrentInputDevice() : audioEngine.getCurrentOutputDevice();
        const std::vector<AudioDevice> &devices = input ? inputDevices : outputDevices;

        const auto pred = [index](const AudioDevice& device) { return device.deviceIndex == index; };
        const auto it = std::ranges::find_if(devices, pred);
        constexpr auto noDeviceStr = "No device";

        std::string previewLabel = noDeviceStr;
        if (it != devices.end() && it->deviceIndex != kNoDevice) {
            previewLabel = std::format("({}) ", it->hostApiName) + it->deviceName;
        }

        if (ImGui::BeginCombo("Available devices", previewLabel.c_str())) {
            for (const auto &device : devices) {
                ImGui::PushID(device.deviceIndex);

                const bool isSelected = (device.deviceIndex == index);

                const auto label = std::format("({}) ", device.hostApiName) + device.deviceName;
                if (ImGui::Selectable(label.c_str(), isSelected)) {
                    if (input) {
                        audioEngine.setInputDevice(device.deviceIndex);
                    } else {
                        audioEngine.setOutputDevice(device.deviceIndex);
                    }
                }

                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }

                ImGui::PopID();
            }

            ImGui::EndCombo();
        }

        if (it != devices.end() && it->deviceIndex != kNoDevice) {
            const auto &device = *it;

            ImGui::Text("Device Index: %i", device.deviceIndex);
            ImGui::Text("Device Name: %s", device.deviceName.c_str());
            ImGui::Text("Host API: %s", device.hostApiName.c_str());
            ImGui::Text("Max Channels: %d", input ? device.maxInputChannels : device.maxOutputChannels);
        } else {
            ImGui::TextUnformatted(noDeviceStr);
        }

        ImGui::PopID();
    };

    if (ImGui::CollapsingHeader("Input Device")) {
        displayDeviceSettings(true);
    }

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Output Device")) {
        displayDeviceSettings(false);
    }

    ImGui::Separator();

    if (ImGui::Button("Restart Audio Stream")) {
        if (audioEngine.restart()) {
            std::cout << "Audio Engine restarted" << std::endl;
        } else {
            std::cerr << "Failed to restart" << std::endl;
        }
    }

    if (ImGui::Button("Rescan audio devices")) {
        audioEngine.rescanDevices();
    }

    ImGui::EndGroup();
}

void MainApplication::looperUi()
{
    const auto nTracks = looper_.getNumLooperTracks();

    ImGui::Begin("Tracks");

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

        ImGui::SameLine();
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