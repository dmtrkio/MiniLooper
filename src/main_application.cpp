#include "main_application.h"

#include <algorithm>
#include <iostream>
#include <format>

#include "imgui.h"

#include "audio/audio_engine.h"
#include "ui/parameter_ui.h"

void volumeMeter(const float leftDb, const float rightDb)
{
    constexpr float kMinDb = -60.0f;
    constexpr float kMaxDb = looper::kHeadRoomDb;

    constexpr int segments = 30;
    constexpr float width = 18.0f;
    constexpr float height = 140.0f;
    constexpr float spacing = 8.0f;
    constexpr float gap = 2.0f;

    auto normalize = [&](float db) {
        db = std::clamp(db, kMinDb, kMaxDb);
        return (db - kMinDb) / (kMaxDb - kMinDb);
    };

    const float l = normalize(leftDb);
    const float r = normalize(rightDb);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();

    constexpr float meterWidth = width * 2 + spacing;
    ImGui::InvisibleButton("##meter", ImVec2(meterWidth + 30.0f, height));

    const float top = pos.y;
    const float bottom = pos.y + height;

    auto segmentColor = [&](const int i) {
        const auto t = static_cast<float>(i) / static_cast<float>(segments);
        if (t > 0.9f)   return IM_COL32(255, 60, 60, 255);
        if (t > 0.7f)   return IM_COL32(255, 210, 60, 255);
        return IM_COL32(60, 220, 90, 255);
    };

    auto drawMeter = [&](const float value, const float xOffset) {
        constexpr float segHeight = (height - gap * (segments - 1)) / segments;

        for (int i = 0; i < segments; i++) {
            const float threshold = static_cast<float>(i + 1) / static_cast<float>(segments);
            const bool active = (value >= threshold);

            const float y0 = pos.y + height - static_cast<float>(i + 1) * segHeight - static_cast<float>(i) * gap;
            const float y1 = y0 + segHeight;

            ImVec2 p0(pos.x + xOffset, y0);
            ImVec2 p1(pos.x + xOffset + width, y1);

            const ImU32 col = active ? segmentColor(i) : IM_COL32(40, 40, 40, 255);
            draw->AddRectFilled(p0, p1, col, 2.0f);
        }
    };

    drawMeter(l, 0);
    drawMeter(r, width + spacing);

    auto dbToY = [&](const float db) {
        const float t = normalize(db);
        return bottom - (bottom - top) * t;
    };

    constexpr float ticks[] = { -60, -48, -36, -24, -12, -6, 0, 6 };

    for (const float db : ticks) {
        constexpr float tickWidth = 6.0f;
        const float y = dbToY(db);

        const ImVec2 t0(pos.x + meterWidth + 4, y);
        const ImVec2 t1(pos.x + meterWidth + 4 + tickWidth, y);

        //draw->AddLine(t0, t1, IM_COL32(200, 200, 200, 255), 1.0f);

        char label[8];
        snprintf(label, sizeof(label), "%.0f", db);

        draw->AddText(
            ImVec2(t1.x + 4, y - ImGui::GetFontSize() * 0.5f),
            IM_COL32(200, 200, 200, 255),
            label
        );
    }
}

void mixerUi(looper::Looper &looper)
{
    ImGui::Begin("Mixer");
    ImGui::PushID("Mixer");

    auto &mixer = looper.getMixerParams();

    constexpr float sliderWidth = 100.0f;

    for (auto i{0}; i < looper.getNumLooperTracks(); ++i) {
        ImGui::PushID(i);

        ImGui::BeginGroup();

        ImGui::PushItemWidth(sliderWidth);

        const auto &track = looper.getTrackState(i);
        const auto [left, right] = track.level;
        volumeMeter(left, right);

        auto &params = mixer.channels[i];

        ImGui::Dummy(ImVec2(0, 2.0f));
        ui::parameterUi(params.gainDb);

        ImGui::Dummy(ImVec2(0, 2.0f));
        ui::parameterUi(params.pan);

        ImGui::PopItemWidth();
        ImGui::EndGroup();

        if (i < looper.getNumLooperTracks() - 1) {
            ImGui::SameLine();

            ImGui::Dummy(ImVec2(10.0f, 0));
            ImGui::SameLine();
        }

        if (i == 0)
            ui::parameterTreeUi(*params.eqParamTree, std::format("Track {}: ", i));

        ImGui::PopID();
    }

    ImGui::PopID();
    ImGui::End();
}

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

void MainApplication::onFrame()
{
    looper_.updateSnapshot();
    processInput();

    ImGui::BeginMainMenuBar();

    ImGui::MenuItem("Audio settings", nullptr, &showAudioSettings_);
    ImGui::MenuItem("MIDI settings", nullptr, &showMidiSettings_);
    ImGui::MenuItem("Tracks", nullptr, &showTracks_);
    ImGui::MenuItem("Meter", nullptr, &showVolumeMeter_);
    ImGui::MenuItem("Mixer", nullptr, &showMixer_);

    ImGui::EndMainMenuBar();

    if (showTracks_) looperUi();
    if (showAudioSettings_) audioEngineSettings();
    if (showMidiSettings_) midiEngineSettings();
    if (showVolumeMeter_) {
        ImGui::Begin("Volume Meter");
        const auto [leftDb, rightDb] = looper_.getLooperState().level;
        volumeMeter(leftDb, rightDb);
        ImGui::End();
    }
    if (showMixer_) mixerUi(looper_);
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