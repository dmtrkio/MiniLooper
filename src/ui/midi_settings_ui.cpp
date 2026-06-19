#include "midi_settings_ui.h"

#include <iostream>

#include "imgui.h"

namespace ml::ui {
    MidiSettingsUi::MidiSettingsUi(midi::MidiEngine *midiEngine)
        : midiEngine_(midiEngine)
    {}

    const char* MidiSettingsUi::getTitle() const { return "Midi Settings"; }

    void MidiSettingsUi::drawContent()
    {
        if (!midiEngine_) return;

        using namespace midi;
        static constexpr auto kNoDeviceString = "No device";
        auto &engine = *midiEngine_;

        const auto devices = engine.getMidiInputDevices();
        const auto index = engine.getCurrentMidiInputDevice();

        const auto it = std::ranges::find_if(devices, [index](const MidiDevice& device) {
            return device.deviceIndex == index;
        });

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
    }
}