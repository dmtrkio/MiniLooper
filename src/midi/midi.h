#pragma once

#include <atomic>

#include "portmidi.h"
#include "porttime.h"

namespace midi {

    class MidiEngine
    {
    public:
        MidiEngine()
        {
            if (Pt_Start(1, &timerCallback, this) != ptNoError) {
                throw std::runtime_error("PortTime error");
            }

            if (const auto err = Pm_Initialize(); err != pmNoError) {
                throw std::runtime_error(Pm_GetErrorText(err));
            }

            std::cout << std::endl;
            std::cout << "Available MIDI devices" << std::endl;
            for (auto i{0}; i < Pm_CountDevices(); ++i) {
                const auto deviceId = i;
                const auto* deviceInfo = Pm_GetDeviceInfo(deviceId);
                if (!deviceInfo) continue;
                if (!deviceInfo->input) continue;

                std::cout << "Device Id: " << deviceId << std::endl;
                std::cout << "Device api: " << deviceInfo->interf << std::endl;
                std::cout << "Device name: " << deviceInfo->name << std::endl;
                std::cout << "Virtual device: " << (deviceInfo->is_virtual ? "true" : "false") << std::endl;
                std::cout << std::endl;
            }

            std::cout << "Pick midi input device: ";
            std::cin >> deviceId_;
            std::cout << std::endl;

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

            std::cout << Pt_Started() << std::endl;
            std::cout << active_.load() << std::endl;
        }

        ~MidiEngine()
        {
            Pt_Stop();
            if (inputStream_) Pm_Close(inputStream_);
            Pm_Terminate();
        }

    private:
        static void timerCallback(PtTimestamp timestamp, void *userData) {
            const auto* midi = static_cast<MidiEngine*>(userData);
            if (!midi->active_.load(std::memory_order_acquire)) {
                return;
            }

            std::cout << "timeout" << std::endl;

            PmEvent event;
            while (const auto result = Pm_Read(midi->inputStream_, &event, 1)) {
                if (result == pmBufferOverflow) continue;

                const auto status = Pm_MessageStatus(event.message);
                const auto data1 = Pm_MessageData1(event.message);
                const auto data2 = Pm_MessageData2(event.message);

                std::cout << timestamp << ' ' << status << ' ' << data1 << ' ' << data2 << std::endl;
            }
        }

        std::atomic<bool> active_{false};
        PmDeviceID deviceId_{pmNoDevice};
        PmStream *inputStream_{nullptr};
    };

}