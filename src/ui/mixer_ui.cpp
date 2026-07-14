#include "mixer_ui.h"

#include "imgui.h"
#include "imgui_internal.h"

#include "parameter_ui.h"
#include "volume_meter.h"

namespace ml::ui {
    MixerUi::MixerUi(looper::Looper &looper)
        : looper_(&looper)
    {
        fxWindowOpened_.assign(looper_->getNumLooperTracks(), {});
    }

    const char* MixerUi::getTitle() const { return "Mixer"; }

    void MixerUi::drawContent()
    {
        ImGui::PushItemWidth(100.0f);

        for (auto i{0}; i < looper_->getNumLooperTracks(); ++i) {
            drawTrack(i);
            ImGui::SameLine();
            ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine();
        }

        ImGui::BeginGroup();

        const auto [leftDb, rightDb] = looper_->getLooperState().level;
        volumeMeter(leftDb, rightDb);
        ImGui::TextUnformatted("Output");

        parameterUi(
            looper_->getParameterTree()["MasterGain"]["GainDb"].asParameterUnsafe(),
            "Master"
        );

        ImGui::EndGroup();

        ImGui::PopItemWidth();
    }

    void MixerUi::drawTrack(const int trackIndex)
    {
        ImGui::PushID(trackIndex);

        ImGui::BeginGroup();

        const auto &track = looper_->getTrackState(trackIndex);
        const auto [left, right] = track.level;
        volumeMeter(left, right);

        const auto id = std::format("Track {}", trackIndex + 1);
        ImGui::TextUnformatted(id.c_str());

        auto looperParamTree = looper_->getParameterTree();
        assert(looperParamTree.isValid());

        auto params = looperParamTree["LooperMixer"][id];
        assert(params.isValid());

        ImGui::Dummy(ImVec2(0, 2.0f));
        parameterUi(params["GainDb"].asParameterUnsafe());

        ImGui::Dummy(ImVec2(0, 2.0f));
        parameterUi(params["Pan"].asParameterUnsafe());

        if (ImGui::Button("FX")) {
            fxWindowOpened_[trackIndex].value = !fxWindowOpened_[trackIndex].value;
        }

        ImGui::EndGroup();

        if (trackIndex < looper_->getNumLooperTracks() - 1) {
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(10.0f, 0));
            ImGui::SameLine();
        }

        parameterTreeUiWindowed(params["FX"], fxWindowOpened_[trackIndex].value, id + ": ");

        ImGui::PopID();
    }
}