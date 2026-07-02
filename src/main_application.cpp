#include "main_application.h"

#include <iostream>
#include <optional>

#include <imgui.h>

#include "audio/audio_engine.h"
#include "ui/parameter_ui.h"
#include "ui/audio_settings_ui.h"
#include "ui/looper_ui.h"
#include "ui/midi_settings_ui.h"
#include "ui/mixer_ui.h"
#include "ui/source_mixer_ui.h"
#include "ui/session_manager_ui.h"
#include "ui/theme.h"
#include "ui/controls_ui.h"
#include "filepaths.h"
#include "fonts/Inter-Regular.h"
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
            std::cerr << e.what() << std::endl;
            std::cerr << "Failed to start midi engine. Proceeding without it.\n";
            return nullptr;
        }
    }

    MainApplication::MainApplication(const int argc, const char* const* argv)
        : audioEngine_(std::make_unique<audio::AudioEngine>())
        , sessionManager_(*audioEngine_)
        , looper_(*audioEngine_)
        , midiEngine_(makeMidiEngine(looper_))
    {
        (void)argc;
        (void)argv;

        if (const auto r = audioEngine_->rescanDevices(); !r) {
            std::cerr << r.error() << std::endl;
        }

        windowRegistry_.emplace_back(std::make_unique<ui::SessionManagerUi>(sessionManager_, looper_));
        windowRegistry_.emplace_back(std::make_unique<ui::AudioSettingsUi>(*audioEngine_));
        windowRegistry_.emplace_back(std::make_unique<ui::MidiSettingsUi>(midiEngine_.get()));
        windowRegistry_.emplace_back(std::make_unique<ui::LooperUi>(looper_, sessionManager_));
        windowRegistry_.emplace_back(std::make_unique<ui::MixerUi>(looper_));
        windowRegistry_.emplace_back(std::make_unique<ui::SourceMixerUi>(*audioEngine_, looper_));
        windowRegistry_.emplace_back(std::make_unique<ui::ControlsHelpWindow>(looper_.getNumLooperTracks()));

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

            if (const auto it = settings.find("theme"); it != settings.end() && it->is_string()) {
                const auto themeName = it->get<std::string>();
                currentTheme_ = ui::themeFromString(std::string_view(themeName))
                                    .value_or(ui::ImGuiTheme::WarmNeutral);
            }

            if (const auto it = settings.find("windows"); it != settings.end() && it->is_object()) {
                const auto windowJson = *it;
                for (auto& window : windowRegistry_) {
                    if (const auto w = windowJson.find(window->getTitle()); w != windowJson.end() && w->is_object()) {
                        if (const auto openedIt = w->find("opened"); openedIt != w->end() && openedIt->is_boolean()) {
                            window->opened = *openedIt;
                        }
                    }
                }
            }

            std::cout << "Settings loaded from " << filepaths::settingsPath() << std::endl;
        }

        if (const auto r = audioEngine_->start(); !r) {
            throw std::runtime_error("Failed to start audio engine: " + r.error());
        }

        std::cout << "Audio engine started\n";

        ImGui::GetIO().IniFilename = filepaths::imguiIniPath();

        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
            res_Inter_18pt_Regular_ttf,
            static_cast<int>(res_Inter_18pt_Regular_ttf_len),
            15.0f,
            &fontConfig
        );

        ui::applyImGuiTheme(currentTheme_);
    }

    MainApplication::~MainApplication()
    {
        if (audioEngine_->stop())
            std::cout << "Audio engine stopped successfully.\n";

        json j;
        j["audio"] = audioEngine_->getSettingsAsJson();

        if (midiEngine_) {
            j["midi"] = midiEngine_->getSettingsAsJson();
        }

        sessionManager_.saveCurrentSessionToDisk(looper_);
        j["sessionsPath"] = sessionManager_.getSessionsPath();

        j["looper"] = looper_.getSettingsAsJson();

        j["theme"] = ui::themeToString(currentTheme_);

        j["windows"] = json::object();
        for (const auto& window : windowRegistry_) {
            j["windows"][window->getTitle()] = {
                {"opened", window->opened}
            };
        }

        if (saveJsonToFile(filepaths::settingsPath().string(), j)) {
            std::cout << "Settings saved to " << filepaths::settingsPath() << std::endl;
        }
    }

    void MainApplication::onFrame()
    {
        looper_.updateSnapshot();
        processInput();

        drawTopBarMenu();

        for (const auto& window : windowRegistry_) {
            window->draw();
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
            sessionManager_.saveCurrentSessionToDisk(looper_);
        }
    }

    void MainApplication::drawTopBarMenu()
    {
        ImGui::BeginMainMenuBar();

        for (auto& window : windowRegistry_) {
            ImGui::PushID(&window);
            ImGui::MenuItem(window->getTitle(), nullptr, &window->opened);
            ImGui::PopID();
        }

        {
            const auto themeMenuLabel = std::format("Theme: {}", ui::themeToString(currentTheme_));
            ImGui::PushID(themeMenuLabel.c_str());
            if (ImGui::MenuItem(themeMenuLabel.c_str())) {
                const auto nextTheme = static_cast<ui::ImGuiTheme>((static_cast<int>(currentTheme_) + 1) % static_cast<int>(ui::ImGuiTheme::Count));
                ui::applyImGuiTheme(nextTheme);
                const_cast<MainApplication*>(this)->currentTheme_ = nextTheme;
            }
            ImGui::PopID();
        }

        {
            auto clickParams = looper_.getParameterTree()["Click"];
            ui::parameterUi(clickParams["Enabled"].asParameterUnsafe(), "Click");
        }

        ImGui::EndMainMenuBar();
    }
}