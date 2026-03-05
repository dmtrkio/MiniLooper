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

void volumeMeter(float left, float right)
{
    static constexpr float kMinDb = -100.0f;
    static constexpr float kMaxDb = 12.0f;

    left = std::clamp(left, kMinDb, kMaxDb);
    right = std::clamp(right, kMinDb, kMaxDb);

    constexpr float range = kMaxDb - kMinDb;
    const auto percentageL = (left - kMinDb) / range;
    const auto percentageR = (right - kMinDb) / range;

    ImGui::Value("L", left);
    ImGui::Value("R", right);
}

void MainApplication::onFrame()
{
    looper_.updateSnapshot();
    processInput();

    ImGui::BeginMainMenuBar();

    ImGui::MenuItem("Audio settings", nullptr, &showAudioSettings_);
    ImGui::MenuItem("MIDI settings", nullptr, &showMidiSettings_);
    ImGui::MenuItem("Tracks", nullptr, &showTracks_);

    volumeMeter(looper_.getLooperState().levelL, looper_.getLooperState().levelR);

    ImGui::EndMainMenuBar();

    if (showTracks_) looperUi();
    if (showAudioSettings_) audioEngineSettings();
    if (showMidiSettings_) midiEngineSettings();
}

void MainApplication::processInput()
{
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

}

void MainApplication::audioEngineSettings()
{
    ImGui::Begin("Audio settings");
    ImGui::PushID("Audio settings");
    ImGui::BeginGroup();

    using namespace audio;

    auto &audioEngine = AudioEngine::getInstance();
    const auto &inputDevices = audioEngine.getInputDevices();
    const auto &outputDevices = audioEngine.getOutputDevices();

    std::vector<unsigned int> sampleRates;

    const auto displayDeviceSettings = [&](const bool input) {
        ImGui::PushID(input ? "input device" : "output device");

        const auto index = input ? audioEngine.getCurrentInputDevice() : audioEngine.getCurrentOutputDevice();
        const std::vector<AudioDevice> &devices = input ? inputDevices : outputDevices;

        const auto pred = [index](const AudioDevice& device) { return device.deviceIndex == index; };
        const auto it = std::ranges::find_if(devices, pred);

        std::string previewLabel = kNoDeviceString;
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

            if (input) {
                sampleRates = device.supportedSampleRates;
            } else {
                std::erase_if(sampleRates, [&device](const auto& sr) {
                    return !std::ranges::contains(device.supportedSampleRates, sr);
                });
            }

            ImGui::Text("Device Index: %i", device.deviceIndex);
            ImGui::Text("Device Name: %s", device.deviceName.c_str());
            ImGui::Text("Host API: %s", device.hostApiName.c_str());
            ImGui::Text("Max Channels: %d", input ? device.maxInputChannels : device.maxOutputChannels);
        } else {
            ImGui::TextUnformatted(kNoDeviceString);
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

    const auto currentSr = audioEngine.getSampleRate();
    if (ImGui::BeginCombo("Sample rate", std::to_string(currentSr).c_str())) {
        for (const auto sr : sampleRates) {
            if (ImGui::Selectable(std::to_string(sr).c_str(), (sr == currentSr))) {
                audioEngine.setSampleRate(sr);
            }
        }
        ImGui::EndCombo();
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
    ImGui::PopID();
    ImGui::End();
}

void MainApplication::midiEngineSettings()
{
    if (!midiEngine_) return;

    ImGui::Begin("MIDI settings");
    ImGui::PushID("MIDI settings");
    ImGui::BeginGroup();

    using namespace midi;
    auto &engine = *midiEngine_;

    const auto devices = engine.getMidiInputDevices();
    const auto index = engine.getCurrentMidiInputDevice();

    const auto pred = [index](const MidiDevice& device) { return device.deviceIndex == index; };
    const auto it = std::ranges::find_if(devices, pred);

    std::string previewLabel = kNoDeviceString;
    if (it != devices.end() && it->deviceIndex != kNoDevice) {
        previewLabel = std::format("({}) ", it->apiName) + it->deviceName;
    }

    if (ImGui::BeginCombo("Available devices", previewLabel.c_str())) {
        for (const auto &device : devices) {
            ImGui::PushID(device.deviceIndex);

            const bool isSelected = (device.deviceIndex == index);

            const auto label = std::format("({}) ", device.apiName) + device.deviceName;
            if (ImGui::Selectable(label.c_str(), isSelected)) {
                if (engine.setMidiInputDevice(device.deviceIndex)) {
                    std::cout << "Midi input device set\n";
                } else {
                    std::cerr << "Failed to set midi input device\n";
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
        ImGui::Text("Host API: %s", device.apiName.c_str());
    } else {
        ImGui::TextUnformatted(kNoDeviceString);
    }

    ImGui::Separator();

    if (ImGui::Button("Rescan midi input devices")) {
        engine.rescanDevices();
    }

    ImGui::EndGroup();
    ImGui::PopID();
    ImGui::End();
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