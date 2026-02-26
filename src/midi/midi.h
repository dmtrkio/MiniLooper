#pragma once

#include <atomic>
#include <functional>
#include <utility>

#include "portmidi.h"
#include "porttime.h"

#include "midi_message.h"
#include "spsc_mailbox.h"

namespace midi {

    using MidiQueue = SpscMailbox<midi::MidiMessage>;

    using MidiInputCallback = std::function<void(int, MidiMessage)>;

    class MidiEngine
    {
    public:
        explicit MidiEngine(MidiInputCallback  inputCallback)
            : timer_(&timerCallback, this),
              inputCallback_(std::move(inputCallback))
        {
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
        }

        ~MidiEngine()
        {
            if (inputStream_) Pm_Close(inputStream_);
            Pm_Terminate();
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
                if (!Pt_Started()) return;
                if (const auto err = Pt_Stop(); err != ptNoError) {
                    std::cerr << "PortTime error when stopping timer: " << err << std::endl;
                }
            }
        };

        PortTimeWrapper timer_;
        MidiInputCallback inputCallback_;
        std::atomic<bool> active_{false};
        PmDeviceID deviceId_{pmNoDevice};
        PmStream *inputStream_{nullptr};
    };

}