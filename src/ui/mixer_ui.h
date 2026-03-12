#pragma once

#include "looper/looper.h"
#include "parameter_ui.h"
#include "volume_meter.h"

namespace ui {
    struct MixerUi
    {
        bool opened = false;

        explicit MixerUi(looper::Looper &looper) : looper_(&looper)
        {
            eqOpened_.assign(looper_->getNumLooperTracks(), {});
        }

        void draw()
        {
            if (!opened || !looper_) return;

            auto &mixer = looper_->getMixerParams();

            ImGui::PushID(&mixer);
            ImGui::Begin("Mixer", &opened);

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

                parameterTreeUi(*params.eqParamTree, &(eqOpened_[i].value), std::format("Track {}: ", i));

                ImGui::PopID();
            }

            ImGui::End();
            ImGui::PopID();
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