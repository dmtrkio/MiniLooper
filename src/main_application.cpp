#include "main_application.h"

#include <iostream>
#include <fstream>
#include <optional>

#include "imgui.h"
#include <nlohmann/json.hpp>

#include "audio/audio_engine.h"
#include "ui/volume_meter.h"
#include "ui/audio_settings_ui.h"
#include "ui/looper_ui.h"
#include "ui/midi_settings_ui.h"
#include "ui/mixer_ui.h"
#include "filepaths.h"

using json = nlohmann::ordered_json;

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

static json audioSettingsToJson()
{
    const auto &audioEngine = audio::AudioEngine::getInstance();

    auto audioDeviceToJson = [](const audio::AudioDevice &device) -> json {
        return {
            { "deviceIndex", device.deviceIndex },
            { "deviceName", device.deviceName },
            { "hostApi", device.hostApiName }
        };
    };

    json j;

    if (const auto *inputDevice = audioEngine.getInputAudioDeviceByIndex(audioEngine.getCurrentInputDevice())) {
        j["inputDevice"] = audioDeviceToJson(*inputDevice);
    }

    if (const auto *outputDevice = audioEngine.getOutputAudioDeviceByIndex(audioEngine.getCurrentOutputDevice())) {
        j["outputDevice"] = audioDeviceToJson(*outputDevice);
    }

    j["sampleRate"] = audioEngine.getSampleRate();
    j["bufferSize"] = audioEngine.getBufferSize();

    return j;
}

static void loadAudioSettingsFromJson(const json& j)
{
    auto &audioEngine = audio::AudioEngine::getInstance();

    if (j.contains("inputDevice") && j["inputDevice"].is_object()) {
        const auto& inputObj = j["inputDevice"];
        if (inputObj.contains("deviceIndex") && inputObj["deviceIndex"].is_number()) {
            const auto index = inputObj["deviceIndex"].get<audio::DeviceIndex>();
            audioEngine.setInputDevice(index);
        }
    }

    if (j.contains("outputDevice") && j["outputDevice"].is_object()) {
        const auto& outputObj = j["outputDevice"];
        if (outputObj.contains("deviceIndex") && outputObj["deviceIndex"].is_number()) {
            const auto index = outputObj["deviceIndex"].get<audio::DeviceIndex>();
            audioEngine.setOutputDevice(index);
        }
    }

    if (j.contains("sampleRate") && j["sampleRate"].is_number()) {
        const auto rate = j["sampleRate"].get<unsigned int>();
        audioEngine.setSampleRate(rate);
    }

    if (j.contains("bufferSize") && j["bufferSize"].is_number()) {
        const auto size = j["bufferSize"].get<unsigned int>();
        audioEngine.setBufferSize(size);
    }
}

static json midiSettingsToJson(const midi::MidiEngine& midiEngine)
{
    json j;

    if (const auto* inputDevice = midiEngine.getMidiInputDeviceByIndex(midiEngine.getCurrentMidiInputDevice())) {
        j["inputDevice"]["deviceIndex"] = inputDevice->deviceIndex;
        j["inputDevice"]["deviceName"] = inputDevice->deviceName;
        j["inputDevice"]["apiName"] = inputDevice->apiName;
    }

    return j;
}

static void loadMidiSettingsFromJson(const json& j, midi::MidiEngine& midiEngine)
{
    if (j.contains("inputDevice")) {
        const auto& inputDevice = j["inputDevice"];
        if (inputDevice.contains("deviceIndex") && inputDevice["deviceIndex"].is_number()) {
            const auto deviceIndex = inputDevice["deviceIndex"].get<midi::DeviceIndex>();
            midiEngine.setMidiInputDevice(deviceIndex);
        }
    }
}

bool saveJsonToFile(const std::string& filename, const json& j)
{
    try {
        std::ofstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Cound not open " << filename << std::endl;
            return false;
        }

        file << j.dump(4);
        return true;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return false;
    }
}

std::optional<json> loadJsonFromFile(const std::string& filename)
{
    try {
        std::ifstream file(filename);
        if (!file.is_open())
            return std::nullopt;
        return json::parse(file);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return std::nullopt;
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

        if (settings.contains("midi") && midiEngine_) {
            loadMidiSettingsFromJson(settings["midi"], *midiEngine_);
        }

        if (settings.contains("audio")) {
            loadAudioSettingsFromJson(settings["audio"]);
        }

        if (settings.contains("sessions path") && settings["sessionsPath"].is_string()) {
            const std::filesystem::path sessionsPath = settings["sessionsPath"];
            if (std::filesystem::exists(sessionsPath)) {
                sessionManager_.setSessionsPath(sessionsPath);
                std::cout << "sessions path = " << sessionsPath << std::endl;
            }
        }

        std::cout << "Settings loaded from " << filepaths::settingsPath() << std::endl;
    } else {
        audioEngine.setSampleRate(48000);
        audioEngine.setBufferSize(64);
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

    windowRegistry_.emplace_back(std::make_unique<ui::AudioSettingsUi>());
    windowRegistry_.emplace_back(std::make_unique<ui::MidiSettingsUi>(midiEngine_.get()));
    windowRegistry_.emplace_back(std::make_unique<ui::LooperUi>(looper_, sessionManager_));
    windowRegistry_.emplace_back(std::make_unique<ui::MixerUi>(looper_));
    windowRegistry_.emplace_back(std::make_unique<ui::VolumeMeterWindow>(looper_))->opened = true;
}

MainApplication::~MainApplication()
{
    if (audio::AudioEngine::getInstance().stop())
        std::cout << "Audio engine stopped successfully.\n";

    json j;
    j["audio"] = audioSettingsToJson();

    if (midiEngine_) {
        j["midi"] = midiSettingsToJson(*midiEngine_);
    }

    if (saveJsonToFile(filepaths::settingsPath().string(), j)) {
        std::cout << "Settings saved to " << filepaths::settingsPath() << std::endl;
    }
}

void MainApplication::onFrame()
{
    looper_.updateSnapshot();

    drawTopBarMenu();

    for (const auto& window : windowRegistry_) {
        window->draw();
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