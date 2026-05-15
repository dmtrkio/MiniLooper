#include "main_application.h"

#include <iostream>
#include <optional>

#include <imgui.h>

#include "audio/audio_engine.h"
#include "ui/volume_meter.h"
#include "ui/audio_settings_ui.h"
#include "ui/looper_ui.h"
#include "ui/midi_settings_ui.h"
#include "ui/mixer_ui.h"
#include "ui/source_mixer_ui.h"
#include "ui/session_manager_ui.h"
#include "filepaths.h"
#include "fonts/Inter-Regular.h"
#include "json.h"

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
    : midiEngine_(makeMidiEngine(looper_))
{
    (void)argc;
    (void)argv;

    auto& audioEngine = audio::AudioEngine::getInstance();
    audioEngine.rescanDevices();

    if (const auto j = loadJsonFromFile(filepaths::settingsPath().string()); j.has_value()) {
        const auto& settings = j.value();

        if (const auto it = settings.find("midi"); it != settings.end() && it->is_object()) {
            if (midiEngine_)
                midiEngine_->loadSettingsFromJson(*it);
        }

        if (const auto it = settings.find("audio"); it != settings.end() && it->is_object()) {
            audio::AudioEngine::getInstance().loadSettingsFromJson(*it);
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

        std::cout << "Settings loaded from " << filepaths::settingsPath() << std::endl;
    }

    if (!audioEngine.start() || !audioEngine.isRunning()) {
        throw std::runtime_error("Failed to start audio engine.");
    }

    std::cout << "Audio engine started\n";

    auto &style = ImGui::GetStyle();
    constexpr float rounding = 4.0f;
    style.FrameRounding = rounding;
    style.WindowRounding = rounding;
    style.ChildRounding = rounding;
    style.PopupRounding = rounding;

    ImGui::GetIO().IniFilename = filepaths::imguiIniPath();

    ImFontConfig fontConfig;
    fontConfig.FontDataOwnedByAtlas = false;
    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
        res_Inter_18pt_Regular_ttf,
        static_cast<int>(res_Inter_18pt_Regular_ttf_len),
        15.0f,
        &fontConfig
    );

    windowRegistry_.emplace_back(std::make_unique<ui::SessionManagerUi>(sessionManager_, looper_));
    windowRegistry_.emplace_back(std::make_unique<ui::AudioSettingsUi>());
    windowRegistry_.emplace_back(std::make_unique<ui::MidiSettingsUi>(midiEngine_.get()));
    windowRegistry_.emplace_back(std::make_unique<ui::LooperUi>(looper_, sessionManager_));
    windowRegistry_.emplace_back(std::make_unique<ui::MixerUi>(looper_));
    windowRegistry_.emplace_back(std::make_unique<ui::SourceMixerUi>(looper_));
    windowRegistry_.emplace_back(std::make_unique<ui::VolumeMeterWindow>(looper_))->opened = true;
}

MainApplication::~MainApplication()
{
    if (audio::AudioEngine::getInstance().stop())
        std::cout << "Audio engine stopped successfully.\n";

    json j;
    j["audio"] = audio::AudioEngine::getInstance().getSettingsAsJson();

    if (midiEngine_) {
        j["midi"] = midiEngine_->getSettingsAsJson();
    }

    sessionManager_.saveCurrentSessionToDisk(looper_);
    j["sessionsPath"] = sessionManager_.getSessionsPath();

    j["looper"] = looper_.getSettingsAsJson();

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
            looper_.toggleRecording(trackIndex);
        }

        if (ImGui::Shortcut(key | ImGuiMod_Shift, ImGuiInputFlags_RouteGlobal)) {
            looper_.togglePlay(trackIndex);
        }
    }

    if (ImGui::Shortcut(ImGuiKey_C, ImGuiInputFlags_RouteGlobal)) {
        looper_.clearAll();
    }
}

void MainApplication::drawTopBarMenu() const
{
    ImGui::BeginMainMenuBar();

    for (auto& window : windowRegistry_) {
        ImGui::PushID(&window);
        ImGui::MenuItem(window->getTitle(), nullptr, &window->opened);
        ImGui::PopID();
    }

    ImGui::EndMainMenuBar();
}