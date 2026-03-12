#pragma once

#include "imgui.h"

#include "ui_window_base.h"
#include "looper/looper.h"

namespace ui {
    struct LooperUi : public WindowBase
    {
        explicit LooperUi(looper::Looper &looper) : looper_(&looper) {}

        [[nodiscard]] const char* getTitle() const override { return "Looper"; }

        void draw() override
        {
            processInput();

            if (!opened) return;

            ImGui::PushID(this);
            ImGui::Begin(getTitle(), &opened);

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

            if (ImGui::Button("Clear All")) {
                looper_->clearAll();
            }

            ImGui::End();
            ImGui::PopID();
        }

    private:
        void drawTrack(const int trackIndex)
        {
            auto &track = looper_->getTrackState(trackIndex);

            ImGui::Text("State: %s", looper::stateToStr(track.state));

            const auto progress = (track.nFrames > 0) ? (static_cast<float>(track.position) / static_cast<float>(track.nFrames)) : 0.0f;
            ImGui::ProgressBar(progress);

            if (ImGui::Button((track.state == looper::State::Recording) ? "Stop" : "Record")) {
                toggleRec(trackIndex);
            }

            ImGui::SameLine();

            if (ImGui::Button((track.state == looper::State::Paused) ? "Resume" : "Pause")) {
                togglePlay(trackIndex);
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
                    toggleRec(trackIndex);
                }

                if (ImGui::Shortcut(key | ImGuiMod_Shift, ImGuiInputFlags_RouteGlobal)) {
                    togglePlay(trackIndex);
                }
            }

            if (ImGui::Shortcut(ImGuiKey_C, ImGuiInputFlags_RouteGlobal)) {
                looper_->clearAll();
            }
        }

        void toggleRec(const int trackIndex)
        {
            if (looper_->getTrackState(trackIndex).state != looper::State::Recording) {
                looper_->startRecording(trackIndex);
            } else {
                looper_->stopRecording(trackIndex);
            }
        }

        void togglePlay(const int trackIndex)
        {
            if (looper_->getTrackState(trackIndex).state == looper::State::Paused) {
                looper_->resume(trackIndex);
            } else {
                looper_->pause(trackIndex);
            }
        }

        looper::Looper *looper_;
    };
}