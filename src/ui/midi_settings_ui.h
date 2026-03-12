#pragma once

#include "imgui.h"

#include "ui_window_base.h"
#include "midi/midi.h"

namespace ui {
    struct MidiSettingsUi : public WindowBase
    {
        explicit MidiSettingsUi(midi::MidiEngine *midiEngine) : midiEngine_(midiEngine) {}

        [[nodiscard]] const char* getTitle() const override { return "Midi Settings"; }

        void draw() override
        {
            if (!opened || !midiEngine_) return;

            ImGui::PushID(this);
            ImGui::Begin(getTitle(), &opened);

            using namespace midi;
            static constexpr auto kNoDeviceString = "No device";
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

            ImGui::End();
            ImGui::PopID();
        }

    private:
        midi::MidiEngine *midiEngine_ = nullptr;
    };
}