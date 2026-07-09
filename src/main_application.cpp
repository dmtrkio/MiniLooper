#include "main_application.h"

#include <iostream>
#include <format>

#include <imgui.h>

#include "ui/popup_manager.h"
#include "audio/audio_engine.h"
#include "filepaths.h"
#include "json.h"

namespace ml {
    static std::unique_ptr<midi::MidiEngine> makeMidiEngine(looper::Looper &looper)
    {
        try {
            auto midiEngine = std::make_unique<midi::MidiEngine>([&looper](int, midi::MidiMessage msg) {
                (void)looper.sendMidiMessage(msg);
            });

            std::cout << "Midi engine started" << std::endl;
            return midiEngine;
        } catch (const std::exception& e) {
            ui::PopupManager::getInstance().errorPopup(std::format(
                "Error starting midi engine: {}\nProceeding without it.",
                e.what()
            ));
            return nullptr;
        }
    }

    MainApplication::MainApplication(const int argc, const char* const* argv)
        : audioEngine_(std::make_unique<audio::AudioEngine>())
        , sessionManager_(*audioEngine_)
        , looper_(*audioEngine_)
        , midiEngine_(makeMidiEngine(looper_))
        , ui_(sessionManager_, *audioEngine_, *midiEngine_, looper_)
    {
        (void)argc;
        (void)argv;

        if (const auto r = audioEngine_->rescanDevices(); !r) {
            ui::PopupManager::getInstance().errorPopup(r.error());
        }

        loadJsonSettings();

        if (const auto r = audioEngine_->start(); !r) {
            ui::PopupManager::getInstance().errorPopup("Failed to start audio engine: " + r.error());
        }

        std::cout << "Audio engine started\n";
    }

    MainApplication::~MainApplication()
    {
        if (audioEngine_->stop()) {
            std::cout << "Audio engine stopped successfully.\n";
        }

        saveJsonSettings();
    }

    void MainApplication::onFrame()
    {
        looper_.updateSnapshot();
        processInput();
        ui_.runFrame();
    }

    void MainApplication::loadJsonSettings()
    {
        if (const auto j = loadJsonFromFile(filepaths::settingsPath().string()); j.has_value()) {
            const auto& settings = j.value();

            if (const auto it = settings.find("midi"); it != settings.end() && it->is_object()) {
                if (midiEngine_)
                    midiEngine_->loadSettingsFromJson(*it);
            }

            if (const auto it = settings.find("audio"); it != settings.end() && it->is_object()) {
                audioEngine_->loadSettingsFromJson(*it);
            }

            if (const auto it = settings.find("sessionsPath"); it != settings.end() && it->is_string()) {
                if (const std::filesystem::path p = *it; std::filesystem::exists(p)) {
                    sessionManager_.setSessionsPath(p);
                } else {
                    sessionManager_.openSessionsPathDialog();
                }
            } else {
                sessionManager_.openSessionsPathDialog();
            }

            if (const auto it = settings.find("looper"); it != settings.end() && it->is_object()) {
                looper_.loadSettingsFromJson(*it);
            }

            if (const auto it = settings.find("ui"); it != settings.end() && it->is_object()) {
                ui_.deserializeJson(*it);
            }

            std::cout << "Settings loaded from " << filepaths::settingsPath() << std::endl;
        }
    }

    void MainApplication::saveJsonSettings() const
    {
        json j;
        j["audio"] = audioEngine_->getSettingsAsJson();

        if (midiEngine_) {
            j["midi"] = midiEngine_->getSettingsAsJson();
        }

        if (const auto r = sessionManager_.saveCurrentSession(looper_); !r) {
            std::cerr << r.error() << std::endl;
        }

        j["sessionsPath"] = sessionManager_.getSessionsPath();

        j["looper"] = looper_.getSettingsAsJson();

        j["ui"] = ui_.serializeToJson();

        if (const auto r = saveJsonToFile(filepaths::settingsPath().string(), j); !r) {
            std::cerr << r.error() << std::endl;
        } else {
            std::cout << "Settings saved to " << filepaths::settingsPath() << std::endl;
        }
    }

    void MainApplication::processInput()
    {
        for (auto trackIndex{0}; trackIndex < looper_.getNumLooperTracks(); ++trackIndex) {
            const auto key = ImGuiKey_1 + trackIndex;

            if (ImGui::Shortcut(key, ImGuiInputFlags_RouteGlobal)) {
                looper_.toggleRecording(trackIndex, false);
            }

            if (ImGui::Shortcut(key | ImGuiMod_Shift, ImGuiInputFlags_RouteGlobal)) {
                looper_.togglePlay(trackIndex, false);
            }

            if (ImGui::Shortcut(key | ImGuiMod_Ctrl, ImGuiInputFlags_RouteGlobal)) {
                looper_.clear(trackIndex);
            }

            {
                auto& fsParam = looper_.getParameterTree()["FootSwitchTrackIndex"].asParameterUnsafe();
                if (ImGui::Shortcut(key | ImGuiMod_Alt, ImGuiInputFlags_RouteGlobal)) {
                    const bool isFsTrack = (fsParam.get<int>() == trackIndex);
                    if (!isFsTrack) {
                        fsParam.set(trackIndex);
                    } else {
                        fsParam.set(-1);
                    }
                }
            }
        }

        if (ImGui::Shortcut(ImGuiKey_C | ImGuiMod_Ctrl, ImGuiInputFlags_RouteGlobal)) {
            looper_.clearAll();
        }

        if (ImGui::Shortcut(ImGuiKey_S | ImGuiMod_Ctrl, ImGuiInputFlags_RouteGlobal)) {
            if (const auto r = sessionManager_.saveCurrentSession(looper_); !r) {
                ui::PopupManager::getInstance().errorPopup(r.error());
            }
        }
    }
}