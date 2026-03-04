#pragma once

#include <atomic>
#include <functional>
#include <utility>
#include <iostream>

#include "portmidi.h"
#include "porttime.h"

#include "midi_message.h"

namespace midi {
    using MidiInputCallback = std::function<void(int, MidiMessage)>;
    using DeviceIndex = int;
    constexpr DeviceIndex kNoDevice = pmNoDevice;

    struct MidiDevice
    {
        DeviceIndex deviceIndex;
        std::string deviceName;
        std::string apiName;
    };

    class MidiEngine
    {
    public:
        explicit MidiEngine(MidiInputCallback  inputCallback)
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

        ~MidiEngine()
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

        void start()
        {
            if (active_.load(std::memory_order_acquire)) {
                std::cerr << "MidiEngine already started" << std::endl;
                return;
            }

            {
                const auto err = Pm_OpenInput(&inputStream_,
                                                     deviceId_,
                                                     nullptr,
                                                     0,
                                                     nullptr,
                                                     nullptr);

                if (err != pmNoError) {
                    throw std::runtime_error(Pm_GetErrorText(err));
                }
            }

            if (const auto err = Pm_SetFilter(inputStream_, PM_FILT_ACTIVE | PM_FILT_SYSEX); err != pmNoError) {
                throw std::runtime_error(Pm_GetErrorText(err));
            }

            active_.store(true, std::memory_order_release);
        }

        void stop()
        {
            active_.store(false, std::memory_order_release);

            if (inputStream_) {
                if (const auto err = Pm_Close(inputStream_); err != pmNoError) {
                    throw std::runtime_error(Pm_GetErrorText(err));
                }

                inputStream_ = nullptr;
            }
        }

        void rescanDevices()
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

        std::vector<MidiDevice> getMidiInputDevices() const
        {
            return devices_;
        }

        DeviceIndex getCurrentMidiInputDevice() const noexcept
        {
            return deviceId_;
        }

        bool setMidiInputDevice(const DeviceIndex deviceIndex)
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

    private:
        static void timerCallback(PtTimestamp timestamp, void *userData) {
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

        struct PortTimeWrapper
        {
            PortTimeWrapper(PtCallback* cb, MidiEngine* engine)
            {
                if (Pt_Start(1, cb, engine) != ptNoError) {
                    throw std::runtime_error("PortTime error");
                }
            }

            ~PortTimeWrapper()
            {
                stop();
            }

            PortTimeWrapper(const PortTimeWrapper&) = delete;
            PortTimeWrapper& operator=(const PortTimeWrapper&) = delete;

            PortTimeWrapper(PortTimeWrapper&&) = delete;
            PortTimeWrapper& operator=(PortTimeWrapper&&) = delete;

            void start(PtCallback* cb, MidiEngine* engine)
            {
                if (!Pt_Started()) return;

                if (Pt_Start(1, cb, engine) != ptNoError) {
                    throw std::runtime_error("PortTime error");
                }
            }

            void stop()
            {
                if (!Pt_Started()) return;

                if (Pt_Stop() != ptNoError) {
                    std::cerr << "PortTime error" << std::endl;
                }
            }
        };

        MidiInputCallback inputCallback_;
        std::atomic<bool> active_{false};
        PmDeviceID deviceId_{pmNoDevice};
        PmStream *inputStream_{nullptr};

        PortTimeWrapper timer_;

        std::vector<MidiDevice> devices_;
    };
}
