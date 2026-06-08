#pragma once

#include <algorithm>
#include <vector>

#include "imgui.h"

#include "ui_window_base.h"
#include "audio/audio_engine.h"

namespace ui {
    class AudioSettingsUi final : public WindowBase
    {
    public:
        AudioSettingsUi(audio::AudioEngine& audioEngine)
            : WindowBase()
            , audioEngine_(audioEngine)
        {}
        [[nodiscard]] const char* getTitle() const override { return "Audio Settings"; }

    protected:
        void drawContent() override
        {
            //ImGui::BeginGroup();

            using namespace audio;
            static constexpr auto kNoDeviceString = "No device";

            const auto &inputDevices = audioEngine_.getInputDevices();
            const auto &outputDevices = audioEngine_.getOutputDevices();

            std::vector<unsigned int> sampleRates;

            const auto displayDeviceSettings = [&](const bool input) {
                ImGui::PushID(input ? "input device" : "output device");

                const auto index = input ? audioEngine_.getCurrentInputDevice() : audioEngine_.getCurrentOutputDevice();
                const auto &devices = input ? inputDevices : outputDevices;

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
                                audioEngine_.setInputDevice(device.deviceIndex);
                            } else {
                                audioEngine_.setOutputDevice(device.deviceIndex);
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

            const auto currentSr = audioEngine_.getSampleRate();
            if (ImGui::BeginCombo("Sample rate", std::to_string(currentSr).c_str())) {
                for (const auto sr : sampleRates) {
                    if (ImGui::Selectable(std::to_string(sr).c_str(), (sr == currentSr))) {
                        audioEngine_.setSampleRate(sr);
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::Separator();

            if (ImGui::Button("Restart Audio Stream")) {
                if (audioEngine_.restart()) {
                    std::cout << "Audio Engine restarted" << std::endl;
                } else {
                    std::cerr << "Failed to restart" << std::endl;
                }
            }

            if (ImGui::Button("Rescan audio devices")) {
                audioEngine_.rescanDevices();
            }

            //ImGui::EndGroup();
        }

    private:
        audio::AudioEngine& audioEngine_;
    };
}