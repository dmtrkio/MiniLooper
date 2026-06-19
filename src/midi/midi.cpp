#include "midi.h"

#include <iostream>
#include <utility>

namespace ml::midi {
    MidiEngine::MidiEngine(MidiInputCallback inputCallback)
        : inputCallback_(std::move(inputCallback))
        , timer_(&timerCallback, this)
    {
        if (const auto err = Pm_Initialize(); err != pmNoError) {
            throw std::runtime_error(Pm_GetErrorText(err));
        }

        rescanDevices();

        deviceId_ = Pm_GetDefaultInputDeviceID();

        start();
    }

    MidiEngine::~MidiEngine()
    {
        active_.store(false, std::memory_order_release);

        timer_.stop();

        if (inputStream_) {
            if (const auto err = Pm_Close(inputStream_); err != pmNoError) {
                std::cerr << "Error closing input stream: " << err << std::endl;
            }

            inputStream_ = nullptr;
        }

        Pm_Terminate();
    }

    json MidiEngine::getSettingsAsJson() const
    {
        json j;

        if (const auto* inputDevice = getMidiInputDeviceByIndex(getCurrentMidiInputDevice())) {
            j["inputDevice"]["deviceIndex"] = inputDevice->deviceIndex;
            j["inputDevice"]["deviceName"] = inputDevice->deviceName;
            j["inputDevice"]["apiName"] = inputDevice->apiName;
        }

        return j;
    }

    void MidiEngine::loadSettingsFromJson(const json& j)
    {
        if (j.contains("inputDevice")) {
            const auto& inputDevice = j["inputDevice"];
            if (inputDevice.contains("deviceIndex") && inputDevice["deviceIndex"].is_number()) {
                const auto deviceIndex = inputDevice["deviceIndex"].get<midi::DeviceIndex>();
                setMidiInputDevice(deviceIndex);
            }
        }
    }

    void MidiEngine::start()
    {
        if (active_.load(std::memory_order_acquire)) {
            std::cerr << "MidiEngine already started" << std::endl;
            return;
        }

        {
            const auto err = Pm_OpenInput(
                &inputStream_,
                deviceId_,
                nullptr,
                0,
                nullptr,
                nullptr
            );

            if (err != pmNoError) {
                throw std::runtime_error(Pm_GetErrorText(err));
            }
        }

        if (const auto err = Pm_SetFilter(inputStream_, PM_FILT_ACTIVE | PM_FILT_SYSEX); err != pmNoError) {
            throw std::runtime_error(Pm_GetErrorText(err));
        }

        active_.store(true, std::memory_order_release);
    }

    void MidiEngine::stop()
    {
        active_.store(false, std::memory_order_release);

        if (inputStream_) {
            if (const auto err = Pm_Close(inputStream_); err != pmNoError) {
                throw std::runtime_error(Pm_GetErrorText(err));
            }

            inputStream_ = nullptr;
        }
    }

    void MidiEngine::rescanDevices()
    {
        devices_.clear();

        std::cout << std::endl;
        std::cout << "Available MIDI devices" << std::endl;
        for (auto i{0}; i < Pm_CountDevices(); ++i) {
            const auto deviceId = i;
            const auto* deviceInfo = Pm_GetDeviceInfo(deviceId);
            if (!deviceInfo) continue;
            if (!deviceInfo->input) continue;

            const MidiDevice device = {
                .deviceIndex = deviceId,
                .deviceName = deviceInfo->name,
                .apiName = deviceInfo->interf
            };

            devices_.emplace_back(device);

            std::cout << "Device Id: " << deviceId << std::endl;
            std::cout << "Device api: " << deviceInfo->interf << std::endl;
            std::cout << "Device name: " << deviceInfo->name << std::endl;
            std::cout << "Virtual device: " << (deviceInfo->is_virtual ? "true" : "false") << std::endl;
            std::cout << std::endl;
        }
    }

    std::vector<MidiDevice> MidiEngine::getMidiInputDevices() const
    {
        return devices_;
    }

    const MidiDevice* MidiEngine::getMidiInputDeviceByIndex(DeviceIndex deviceIndex) const
    {
        const auto it = std::ranges::find_if(devices_, [deviceIndex](const auto& device) {
            return device.deviceIndex == deviceIndex;
        });

        if (it == devices_.end()) return nullptr;
        return &(*it);
    }

    DeviceIndex MidiEngine::getCurrentMidiInputDevice() const noexcept
    {
        return deviceId_;
    }

    bool MidiEngine::setMidiInputDevice(const DeviceIndex deviceIndex)
    {
        deviceId_ = deviceIndex;

        try {
            stop();
            start();
        } catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
            return false;
        }

        return true;
    }

    void MidiEngine::timerCallback(PtTimestamp timestamp, void* userData)
    {
        const auto* midi = static_cast<MidiEngine*>(userData);

        if (!midi->active_.load(std::memory_order_acquire)) {
            return;
        }

        PmEvent event;
        while (true) {
            if (Pm_Read(midi->inputStream_, &event, 1) <= 0) break;
            midi->inputCallback_(timestamp, MidiMessage{event.message});
        }
    }

    MidiEngine::PortTimeWrapper::PortTimeWrapper(PtCallback* cb, MidiEngine* engine)
    {
        if (Pt_Start(1, cb, engine) != ptNoError) {
            throw std::runtime_error("PortTime error");
        }
    }

    MidiEngine::PortTimeWrapper::~PortTimeWrapper()
    {
        stop();
    }

    void MidiEngine::PortTimeWrapper::start(PtCallback* cb, MidiEngine* engine)
    {
        if (!Pt_Started()) return;

        if (Pt_Start(1, cb, engine) != ptNoError) {
            throw std::runtime_error("PortTime error");
        }
    }

    void MidiEngine::PortTimeWrapper::stop()
    {
        if (!Pt_Started()) return;

        if (Pt_Stop() != ptNoError) {
            std::cerr << "PortTime error" << std::endl;
        }
    }
}