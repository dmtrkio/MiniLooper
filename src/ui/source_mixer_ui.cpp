#include "source_mixer_ui.h"

#include <format>
#include <cassert>

#include <imgui.h>

#include "audio/audio_engine.h"
#include "parameter_ui.h"
#include "ui/parameter_ui.h"
#include "volume_meter.h"

namespace ml::ui {
    SourceMixerUi::SourceMixerUi(const audio::AudioEngine& audioEngine, looper::Looper& looper)
        : audioEngine_(&audioEngine)
        , looper_(&looper)
    {
        const auto& levels = looper_->getLooperState().sourceChannelLevels;
        const std::size_t count = levels.size();

        fxWindowOpened_.assign(count, {});
        inputSelectorWindowOpened_.assign(count, {});
    }

    const char* SourceMixerUi::getTitle() const { return "Source Mixer"; }

    void SourceMixerUi::drawContent()
    {
        const auto paramTree = looper_->getParameterTree()["SourceMixer"];
        //// temporary view
        //parameterTreeUi(paramTree);

        const auto& levels = looper_->getLooperState().sourceChannelLevels;
        const std::size_t count = levels.size();

        const auto nInputs = static_cast<int>(audioEngine_->getNumInputChannels());

        for (std::size_t i{}; i < count; ++i) {
            const auto id = std::format("SourceChannel{}", i + 1);
            const auto params = paramTree[id];
            assert(params.isValid());

            ImGui::PushID(id.c_str());
            ImGui::BeginGroup();
            ImGui::PushItemWidth(100.0f);

            const auto [left, right] = levels[i];
            volumeMeter(left, right);

            const auto label = std::format("Input Channel {}", i + 1);
            ImGui::TextUnformatted(label.c_str());

            const auto mix = params["Mix"];
            assert(mix.isValid());
            mix["Enabled"].asParameterUnsafe().set(true);

            auto gain = mix["Gain"];
            auto pan = mix["Pan"];
            gain["Enabled"].asParameterUnsafe().set(true);
            pan["Enabled"].asParameterUnsafe().set(true);

            parameterUi(gain["GainDb"].asParameterUnsafe());
            parameterUi(pan["Pan"].asParameterUnsafe());

            if (ImGui::Button("FX")) {
                fxWindowOpened_[i].value = !fxWindowOpened_[i].value;
            }

            if (ImGui::Button("Inputs")) {
                inputSelectorWindowOpened_[i].value = !inputSelectorWindowOpened_[i].value;
            }

            ImGui::PopItemWidth();
            ImGui::EndGroup();
            ImGui::PopID();

            ImGui::SameLine();

            const auto fx = params["FX"];
            assert(fx.isValid());
            parameterTreeUiWindowed(params["FX"], fxWindowOpened_[i].value, label + ": ");

            // Input Selector window
            if (inputSelectorWindowOpened_[i].value) {
                const auto inputsLabel = std::format("{} ports", label);
                ImGui::PushID(inputsLabel.c_str());
                ImGui::Begin(inputsLabel.c_str(), &inputSelectorWindowOpened_[i].value);

                const auto inputPicker = [&](const dsp::parameter::ParameterTree& inputParam, int sourceIndex) {
                    auto& sourceParam = inputParam["Source"].asParameterUnsafe();
                    const auto paramRange = sourceParam.getRange<int>().value();
                    const auto minPort = paramRange.min;
                    const auto maxPort = std::min(nInputs - 1, paramRange.max);

                    auto portToString = [minPort](int portIndex) -> std::string {
                        if (portIndex == minPort) return "No input";
                        else return std::to_string(portIndex);
                    };

                    const int currentPort = sourceParam.get<int>();
                    const auto comboLabel = std::to_string(sourceIndex);
                    const auto preview = portToString(currentPort);

                    if (ImGui::BeginCombo(comboLabel.c_str(), preview.c_str())) {
                        for (int port = minPort; port <= maxPort; ++port) {
                            const bool isSelected = (currentPort == port);

                            if (ImGui::Selectable(portToString(port).c_str(), isSelected)) {
                                sourceParam.set(port);
                            }

                            if (isSelected) {
                                ImGui::SetItemDefaultFocus();
                            }
                        }

                        ImGui::EndCombo();
                    }

                    parameterUi(inputParam["GainDb"].asParameterUnsafe(), "Volume");
                };

                inputPicker(params["Input1"], 1);
                inputPicker(params["Input2"], 2);

                parameterUi(params["Stereo"].asParameterUnsafe());

                ImGui::End();
                ImGui::PopID();
            }
        }
    }
}