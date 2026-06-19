#include "looper_ui.h"

#include "imgui.h"

namespace ml::ui {
    LooperUi::LooperUi(looper::Looper &looper, SessionManager& sessionManager) : looper_(&looper), sessionManager_(&sessionManager) {}

    const char* LooperUi::getTitle() const { return "Looper"; }

    void LooperUi::drawContent()
    {
        const auto nTracks = looper_->getNumLooperTracks();

        for (auto i{0}; i < nTracks; ++i) {
            const auto trackIndex = i;
            const auto label = std::format("Track {}", trackIndex);
            ImGui::PushID(label.c_str());

            ImGui::BeginGroup();

            ImGui::TextUnformatted(label.c_str());
            drawTrack(trackIndex);

            ImGui::Separator();
            ImGui::EndGroup();

            ImGui::PopID();
        }

        if (ImGui::Button("Clear all")) {
            looper_->clearAll();
        }

        if (ImGui::Button("Save to disk")) {
            sessionManager_->saveCurrentSessionToDisk(*looper_);
        }
    }

    void LooperUi::drawTrack(const int trackIndex)
    {
        auto &track = looper_->getTrackState(trackIndex);

        ImGui::Text("State: %s", looper::stateToStr(track.state));

        const auto progress = (track.nFrames > 0) ? (static_cast<float>(track.position) / static_cast<float>(track.nFrames)) : 0.0f;
        ImGui::ProgressBar(progress);

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
    }
}