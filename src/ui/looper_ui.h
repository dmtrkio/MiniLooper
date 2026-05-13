#pragma once

#include "imgui.h"

#include "ui_window_base.h"
#include "looper/looper.h"
#include "session_manager.h"

namespace ui {
    class LooperUi final : public WindowBase
    {
    public:
        explicit LooperUi(looper::Looper &looper, SessionManager& sessionManager) : looper_(&looper), sessionManager_(&sessionManager) {}

        [[nodiscard]] const char* getTitle() const override { return "Looper"; }

    protected:
        void onFrame() override
        {
            // to make input processing work even when the window is closed
            processInput();
        }

        void drawContent() override
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

    private:
        void drawTrack(const int trackIndex)
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

        void processInput()
        {
            for (auto trackIndex{0}; trackIndex < looper_->getNumLooperTracks(); ++trackIndex) {
                const auto key = ImGuiKey_1 + trackIndex;

                if (ImGui::Shortcut(key, ImGuiInputFlags_RouteGlobal)) {
                    looper_->toggleRecording(trackIndex);
                }

                if (ImGui::Shortcut(key | ImGuiMod_Shift, ImGuiInputFlags_RouteGlobal)) {
                    looper_->togglePlay(trackIndex);
                }
            }

            if (ImGui::Shortcut(ImGuiKey_C, ImGuiInputFlags_RouteGlobal)) {
                looper_->clearAll();
            }
        }

        looper::Looper *looper_;
        SessionManager *sessionManager_;
    };
}