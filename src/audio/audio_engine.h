#pragma once

#include <vector>
#include <memory>
#include <string>
#include <expected>

#include "audio_device.h"
#include "json.h"
#include "rt_sanitizer.h"

namespace ml::audio {
    inline constexpr int kMaxFramesInBuffer = 8096;

    class AudioBackend;

    class AudioCallback
    {
    public:
        virtual ~AudioCallback() = default;

        AudioCallback(const AudioCallback&) = delete;
        AudioCallback& operator=(const AudioCallback&) = delete;

        AudioCallback(AudioCallback&&) noexcept = default;
        AudioCallback& operator=(AudioCallback&&) noexcept = default;

        virtual void onProcess(const float *const *in, float *const *out, int nFrames) RT_SAN = 0;
        virtual void onStart(float sampleRate, int nInputChannels, int nOutputChannels) = 0;
        virtual void onStop() = 0;

    protected:
        AudioCallback() = default;
    };

    // None of the methods in this class are thread-safe
    class AudioEngine
    {
    public:
        AudioEngine();
        ~AudioEngine();

        AudioEngine(const AudioEngine&) = delete;
        AudioEngine& operator=(const AudioEngine&) = delete;

        AudioEngine(AudioEngine&&) noexcept = delete;
        AudioEngine& operator=(AudioEngine&&) noexcept = delete;

        [[nodiscard]] json getSettingsAsJson() const;
        void loadSettingsFromJson(const json& j);

        int getNumInputChannels() const noexcept;
        int getNumOutputChannels() const noexcept;
        int getSampleRate() const noexcept;
        int getBufferSize() const noexcept;

        // Always rescan after construction.
        // Rescanning will stop ongoing audio stream.
        [[nodiscard]] std::expected<void, std::string> rescanDevices();

        [[nodiscard]] const std::vector<AudioDevice>& getInputDevices() const;
        [[nodiscard]] const std::vector<AudioDevice>& getOutputDevices() const;

        [[nodiscard]] DeviceIndex getCurrentInputDevice() const;
        [[nodiscard]] DeviceIndex getCurrentOutputDevice() const;

        // these won't take effect until starting a new stream with fresh settings
        void setSampleRate(int sampleRate);
        void setBufferSize(int bufferSize);
        void setInputDevice(DeviceIndex inputDeviceIndex);
        void setOutputDevice(DeviceIndex outputDeviceIndex);
        [[nodiscard]] const AudioDevice* getInputAudioDeviceByIndex(DeviceIndex inputDeviceIndex) const;
        [[nodiscard]] const AudioDevice* getOutputAudioDeviceByIndex(DeviceIndex outputDeviceIndex) const;
        void pickDevices();

        // only call when audio is not running
        void setAudioCallback(std::shared_ptr<AudioCallback> cb);

        [[nodiscard]] std::expected<void, std::string> start();
        [[nodiscard]] std::expected<void, std::string> stop();
        [[nodiscard]] std::expected<void, std::string> restart();

        bool isRunning() const;

    private:
        bool callback(const float *in, float *out, int nFrames) RT_SAN;

        std::unique_ptr<AudioBackend> backend_;

        std::shared_ptr<AudioCallback> userCallback_;

        int sampleRate_{48000};
        int bufferSize_{128};

        DeviceIndex inputDeviceIndex_{kNoDevice};
        DeviceIndex outputDeviceIndex_{kNoDevice};

        int inputChannels_{2};
        int outputChannels_{2};

        std::vector<AudioDevice> inputDevices_;
        std::vector<AudioDevice> outputDevices_;

        struct PlanarAudioData
        {
            void setNumChannels(int numChannels);
            void deinterleave(const float *data, int nFrames);
            void interleave(float *data, int nFrames);

            std::vector<float*> planar;
            std::vector<std::vector<float>> buffers;
        };

        PlanarAudioData inputData_;
        PlanarAudioData outputData_;
    };
}