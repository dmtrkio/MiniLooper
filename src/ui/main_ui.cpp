#include "main_ui.h"

#include "imgui.h"

#include "ui/parameter_ui.h"
#include "ui/audio_settings_ui.h"
#include "ui/looper_ui.h"
#include "ui/midi_settings_ui.h"
#include "ui/mixer_ui.h"
#include "ui/source_mixer_ui.h"
#include "ui/session_manager_ui.h"
#include "ui/controls_ui.h"
#include "filepaths.h"
#include "fonts/Inter-Regular.h"

namespace ml::ui {
    MainUi::MainUi(
        SessionManager& sessionManager,
        audio::AudioEngine& audioEngine,
        midi::MidiEngine& midiEngine,
        looper::Looper& looper
    )
        : looper_(looper)
    {
        windowRegistry_.emplace_back(std::make_unique<ui::SessionManagerUi>(sessionManager, looper));
        windowRegistry_.emplace_back(std::make_unique<ui::AudioSettingsUi>(audioEngine));
        windowRegistry_.emplace_back(std::make_unique<ui::MidiSettingsUi>(midiEngine));
        windowRegistry_.emplace_back(std::make_unique<ui::LooperUi>(looper, sessionManager));
        windowRegistry_.emplace_back(std::make_unique<ui::MixerUi>(looper));
        windowRegistry_.emplace_back(std::make_unique<ui::SourceMixerUi>(audioEngine, looper));
        windowRegistry_.emplace_back(std::make_unique<ui::ControlsHelpWindow>(looper.getNumLooperTracks()));

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

    json MainUi::serializeToJson() const
    {
        json j;

        j["theme"] = ui::themeToString(currentTheme_);

        j["windows"] = json::object();
        for (const auto& window : windowRegistry_) {
            j["windows"][window->getTitle()] = {
                {"opened", window->opened}
            };
        }

        return j;
    }

    void MainUi::deserializeJson(const json& j)
    {
        if (const auto it = j.find("theme"); it != j.end() && it->is_string()) {
            const auto themeName = it->get<std::string>();
            currentTheme_ = ui::themeFromString(std::string_view(themeName))
                                .value_or(ui::ImGuiTheme::WarmNeutral);
            ui::applyImGuiTheme(currentTheme_);
        }

        if (const auto it = j.find("windows"); it != j.end() && it->is_object()) {
            const auto windowJson = *it;
            for (auto& window : windowRegistry_) {
                if (const auto w = windowJson.find(window->getTitle()); w != windowJson.end() && w->is_object()) {
                    if (const auto openedIt = w->find("opened"); openedIt != w->end() && openedIt->is_boolean()) {
                        window->opened = *openedIt;
                    }
                }
            }
        }
    }

    void MainUi::runFrame()
    {
        drawTopBarMenu();

        for (const auto& window : windowRegistry_) {
            window->draw();
        }
    }

    void MainUi::drawTopBarMenu()
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
                currentTheme_ = nextTheme;
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