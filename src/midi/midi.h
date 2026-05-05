#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <vector>

#include "portmidi.h"
#include "porttime.h"

#include "midi_message.h"
#include "json.h"

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
        explicit MidiEngine(MidiInputCallback inputCallback);
        ~MidiEngine();

        [[nodiscard]] json getSettingsAsJson() const;
        void loadSettingsFromJson(const json& j);

        void start();
        void stop();
        void rescanDevices();

        std::vector<MidiDevice> getMidiInputDevices() const;
        const MidiDevice* getMidiInputDeviceByIndex(DeviceIndex deviceIndex) const;
        DeviceIndex getCurrentMidiInputDevice() const noexcept;
        bool setMidiInputDevice(DeviceIndex deviceIndex);

    private:
        static void timerCallback(PtTimestamp timestamp, void* userData);

        struct PortTimeWrapper
        {
            PortTimeWrapper(PtCallback* cb, MidiEngine* engine);
            ~PortTimeWrapper();

            PortTimeWrapper(const PortTimeWrapper&) = delete;
            PortTimeWrapper& operator=(const PortTimeWrapper&) = delete;
            PortTimeWrapper(PortTimeWrapper&&) = delete;
            PortTimeWrapper& operator=(PortTimeWrapper&&) = delete;

            void start(PtCallback* cb, MidiEngine* engine);
            void stop();
        };

        MidiInputCallback inputCallback_;
        std::atomic<bool> active_{false};
        PmDeviceID deviceId_{pmNoDevice};
        PmStream* inputStream_{nullptr};

        PortTimeWrapper timer_;

        std::vector<MidiDevice> devices_;
    };
}