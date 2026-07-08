#include "looper_ui.h"

#include <iostream>

#include "dsp/dsp.h"
#include "imgui.h"

#include "looper/looper_processor.h"
#include "looper/looper_thumbnail_cache.h"
#include "audio_thumbnail.h"

namespace ml::ui {
    LooperUi::LooperUi(looper::Looper &looper, SessionManager& sessionManager)
        : looper_(&looper)
        , thumbnailCache_(looper)
        , sessionManager_(&sessionManager)
    {}

    const char* LooperUi::getTitle() const { return "Looper"; }

    void LooperUi::drawContent()
    {
        const auto nTracks = looper_->getNumLooperTracks();

        for (int trackIndex{0}; trackIndex < nTracks; ++trackIndex) {
            drawTrack(trackIndex);
            ImGui::Separator();
        }

        if (ImGui::Button("Clear all")) {
            looper_->clearAll();
        }

        ImGui::SameLine();

        if (ImGui::Button("Save to disk")) {
            if (const auto r = sessionManager_->saveCurrentSession(*looper_); !r) {
                std::cerr << r.error() << std::endl;
            }
        }
    }

    void LooperUi::drawTrack(const int trackIndex)
    {
        const auto label = std::format("Track {}", trackIndex + 1);
        ImGui::PushID(label.c_str());
        ImGui::BeginGroup();
        ImGui::TextUnformatted(label.c_str());

        auto &track = looper_->getTrackState(trackIndex);

        ImGui::Text("State: %s", looper::stateToStr(track.state));

        thumbnailCache_.update(trackIndex);

        const auto progress = (track.nFrames > 0) ? (static_cast<float>(track.position) / static_cast<float>(track.nFrames)) : 0.0f;
        const bool drawPlayhead = (track.state == looper::State::Recording && track.nFrames > 0) || (track.state == looper::State::Playback);
        const ImU32 waveformColor = [&] {
            if (track.state == looper::State::Recording) return IM_COL32(230, 20, 20, 255);
            else return ImGui::GetColorU32(ImGuiCol_PlotHistogram);
        }();

        const auto *thumb = thumbnailCache_.get(trackIndex);
        assert(thumb);

        ui::audioThumbnail(
            std::format("##thumbnail{}", trackIndex).c_str(),
            std::span<const dsp::MinMax>{thumb->buckets},
            progress,
            drawPlayhead,
            ImVec2(0, 0),
            waveformColor
        );

        auto& fsParam = looper_->getParameterTree()["FootSwitchTrackIndex"].asParameterUnsafe();
        bool isFsTrack = (fsParam.get<int>() == trackIndex);
        if (ImGui::Checkbox("Footswitch", &isFsTrack)) {
            if (isFsTrack) {
                fsParam.set(trackIndex);
            }
        }

        ImGui::SameLine();

        if (ImGui::Button((track.state == looper::State::Recording) ? "Stop" : "Record")) {
            looper_->toggleRecording(trackIndex);
        }

        ImGui::SameLine();

        if (ImGui::Button((track.state == looper::State::Paused) ? "Resume" : "Pause")) {
            looper_->togglePlay(trackIndex);
        }

        if (track.state != looper::State::Cleared) {
            ImGui::SameLine();
            if (ImGui::Button("Clear")) {
                looper_->clear(trackIndex);
            }
        }

        ImGui::EndGroup();
        ImGui::PopID();
    }
}