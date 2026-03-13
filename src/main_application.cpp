#include "main_application.h"

#include <iostream>

#include "imgui.h"

#include "audio/audio_engine.h"
#include "ui/volume_meter.h"
#include "ui/audio_settings_ui.h"
#include "ui/looper_ui.h"
#include "ui/midi_settings_ui.h"
#include "ui/mixer_ui.h"

static std::unique_ptr<midi::MidiEngine> makeMidiEngine(looper::Looper &looper)
{
    try {
        auto midiEngine = std::make_unique<midi::MidiEngine>([&looper](int, midi::MidiMessage msg) {
            looper.sendMidiMessage(msg);
        });

        std::cout << "Midi engine started" << std::endl;
        return midiEngine;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << "Failed to start midi engine. Proceeding without it.\n";
        return nullptr;
    }
}

struct VolumeMeterWindow : public ui::WindowBase
{
    explicit VolumeMeterWindow(looper::Looper &looper) : looper_(&looper) {}
    [[nodiscard]] const char* getTitle() const override { return "Volume Meter"; }
    void draw() override
    {
        if (!opened) return;
        ImGui::PushID(this);
        ImGui::Begin(getTitle(), &opened);
        const auto [leftDb, rightDb] = looper_->getLooperState().level;
        ui::volumeMeter(leftDb, rightDb);
        ImGui::End();
        ImGui::PopID();
    }
private:
    looper::Looper *looper_;
};

MainApplication::MainApplication(const int argc, const char* const* argv)
    : midiEngine_(makeMidiEngine(looper_))
{
    (void)argc;
    (void)argv;

    auto& audioEngine = audio::AudioEngine::getInstance();
    audioEngine.setSampleRate(48000);
    audioEngine.setBufferSize(64);
    audioEngine.rescanDevices();

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

    windowRegistry_.emplace_back(std::make_unique<ui::AudioSettingsUi>());
    windowRegistry_.emplace_back(std::make_unique<ui::MidiSettingsUi>(midiEngine_.get()));
    windowRegistry_.emplace_back(std::make_unique<ui::LooperUi>(looper_));
    windowRegistry_.emplace_back(std::make_unique<ui::MixerUi>(looper_));
    windowRegistry_.emplace_back(std::make_unique<VolumeMeterWindow>(looper_))->opened = true;
}

MainApplication::~MainApplication()
{
    if (audio::AudioEngine::getInstance().stop())
        std::cout << "Audio engine stopped successfully.\n";
}

void MainApplication::onFrame()
{
    looper_.updateSnapshot();

    drawTopBarMenu();

    for (const auto& window : windowRegistry_) {
        window->draw();
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

    ImGui::EndMainMenuBar();
}
