#include "mixer_ui.h"

#include "parameter_ui.h"
#include "volume_meter.h"
#include "audio_thumbnail.h"

namespace ui {
    MixerUi::MixerUi(looper::Looper &looper)
        : looper_(&looper)
        , thumbnailCache_(*looper_)
        , paramTree_(looper_->getParameterTree()) 
    {
        assert(paramTree_.isValid());
        eqOpened_.assign(looper_->getNumLooperTracks(), {});
    }

    const char* MixerUi::getTitle() const { return "Mixer"; }

    void MixerUi::drawContent()
    {
        for (auto i{0}; i < looper_->getNumLooperTracks(); ++i) {
            drawTrack(i);
        }
    }

    void MixerUi::drawTrack(const int trackIndex)
    {
        ImGui::PushID(trackIndex);

        ImGui::BeginGroup();

        constexpr float sliderWidth = 100.0f;
        ImGui::PushItemWidth(sliderWidth);

        const auto &track = looper_->getTrackState(trackIndex);
        const auto [left, right] = track.level;
        volumeMeter(left, right);

        const auto id = std::format("Track {}", trackIndex + 1);
        auto params = paramTree_["LooperMixer"][id];

        ImGui::Dummy(ImVec2(0, 2.0f));
        parameterUi(params["GainDb"].asParameterUnsafe());

        ImGui::Dummy(ImVec2(0, 2.0f));
        parameterUi(params["Pan"].asParameterUnsafe());

        if (ImGui::Button("Show EQ")) {
            eqOpened_[trackIndex].value = !eqOpened_[trackIndex].value;
        }

        ImGui::Text("State: %s", looper::stateToStr(track.state));

        thumbnailCache_.update(trackIndex);

        const auto *thumb = thumbnailCache_.get(trackIndex);
        const auto progress = (track.nFrames > 0) ? (static_cast<float>(track.position) / static_cast<float>(track.nFrames)) : 0.0f;
        const bool drawPlayhead = (track.state == looper::State::Recording && track.nFrames > 0) || (track.state == looper::State::Playback);
        const ImU32 waveformColor = [&] {
            if (track.state == looper::State::Recording) {
                return IM_COL32(230, 20, 20, 255);
            }
            return ImGui::GetColorU32(ImGuiCol_PlotHistogram);
        }();

        ui::audioThumbnail(
            std::format("##thumbnail{}", trackIndex).c_str(),
            thumb ? thumb->buckets : nullptr,
            looper::ThumbnailCache::kBuckets,
            progress,
            drawPlayhead,
            ImVec2(0, 0),
            waveformColor
        );

        if (ImGui::Button((track.state == looper::State::Recording) ? "Stop" : "Record")) {
            looper_->toggleRecording(trackIndex, false);
        }

        if (ImGui::Button((track.state == looper::State::Paused) ? "Resume" : "Pause")) {
            looper_->togglePlay(trackIndex, false);
        }

        if (track.state != looper::State::Cleared) {
            if (ImGui::Button("Clear")) {
                looper_->clear(trackIndex);
            }
        }

        auto& fsParam = paramTree_["FootSwitchTrackIndex"].asParameterUnsafe();
        bool isFsTrack = (fsParam.get<int>() == trackIndex);
        if (ImGui::Checkbox("Footswitch", &isFsTrack)) {
            if (isFsTrack) {
                fsParam.set(trackIndex);
            }
        }

        ImGui::PopItemWidth();
        ImGui::EndGroup();

        if (trackIndex < looper_->getNumLooperTracks() - 1) {
            ImGui::SameLine();
            ImGui::Dummy(ImVec2(10.0f, 0));
            ImGui::SameLine();
        }

        parameterTreeUiWindowed(params["Equalizer"], &(eqOpened_[trackIndex].value), id);

        ImGui::PopID();
    }
}