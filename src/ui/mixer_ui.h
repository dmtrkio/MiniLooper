#pragma once

#include "ui_window_base.h"
#include "looper/looper.h"
#include "parameter_ui.h"
#include "volume_meter.h"

namespace ui {
    class MixerUi final : public WindowBase
    {
    public:
        explicit MixerUi(looper::Looper &looper) : looper_(&looper)
        {
            eqOpened_.assign(looper_->getNumLooperTracks(), {});
        }

        [[nodiscard]] const char* getTitle() const override { return "Mixer"; }

    protected:
        void drawContent() override
        {
            auto &mixer = looper_->getMixerParams();

            for (auto i{0}; i < looper_->getNumLooperTracks(); ++i) {
                constexpr float sliderWidth = 100.0f;

                ImGui::PushID(i);

                ImGui::BeginGroup();

                ImGui::PushItemWidth(sliderWidth);

                const auto &track = looper_->getTrackState(i);
                const auto [left, right] = track.level;
                volumeMeter(left, right);

                auto &params = mixer.channels[i];

                ImGui::Dummy(ImVec2(0, 2.0f));
                parameterUi(params.gainDb);

                ImGui::Dummy(ImVec2(0, 2.0f));
                parameterUi(params.pan);

                if (ImGui::Button("Show EQ")) {
                    eqOpened_[i].value = !eqOpened_[i].value;
                }

                ImGui::PopItemWidth();
                ImGui::EndGroup();

                if (i < looper_->getNumLooperTracks() - 1) {
                    ImGui::SameLine();

                    ImGui::Dummy(ImVec2(10.0f, 0));
                    ImGui::SameLine();
                }

                parameterTreeUiWindowed(params.eqParamTree, &(eqOpened_[i].value), std::format("Track {}: ", i));

                ImGui::PopID();
            }
        }

    private:
        looper::Looper *looper_;

        struct Bool
        {
            bool value{};
        };

        std::vector<Bool> eqOpened_;
    };
}