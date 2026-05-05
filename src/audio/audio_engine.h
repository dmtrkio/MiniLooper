#pragma once

#include <atomic>
#include <vector>
#include <mutex>
#include <memory>

#include "audio_device.h"
#include "json.h"

namespace audio {
    static constexpr unsigned int kMaxFramesInBuffer = 8096;

    class AudioBackend;

    class AudioCallback
    {
    public:
        virtual ~AudioCallback() = default;

        AudioCallback(const AudioCallback&) = delete;
        AudioCallback& operator=(const AudioCallback&) = delete;

        AudioCallback(AudioCallback&&) noexcept = default;
        AudioCallback& operator=(AudioCallback&&) noexcept = default;

        virtual void onProcess(const float *const *in, float *const *out, unsigned int nFrames) = 0;
        virtual void onStart() = 0;
        virtual void onStop() = 0;

    protected:
        AudioCallback() = default;
    };

    class AudioEngine
    {
    public:
        static AudioEngine& getInstance();

        AudioEngine(const AudioEngine&) = delete;
        AudioEngine& operator=(const AudioEngine&) = delete;

        AudioEngine(AudioEngine&&) noexcept = delete;
        AudioEngine& operator=(AudioEngine&&) noexcept = delete;

        [[nodiscard]] json getSettingsAsJson() const;
        void loadSettingsFromJson(const json& j);

        // -- Only these are safe to be called from an audio callback --
        unsigned int getNumInputChannels() const noexcept;
        unsigned int getNumOutputChannels() const noexcept;
        unsigned int getSampleRate() const noexcept;
        unsigned int getBufferSize() const noexcept;
        // -------------------------------------------------------------

        // rescanning will stop ongoing audio stream
        void rescanDevices();
        [[nodiscard]] const std::vector<AudioDevice>& getInputDevices() const;
        [[nodiscard]] const std::vector<AudioDevice>& getOutputDevices() const;
        [[nodiscard]] DeviceIndex getCurrentInputDevice() const;
        [[nodiscard]] DeviceIndex getCurrentOutputDevice() const;

        // these won't take effect until starting a new stream with fresh settings
        void setSampleRate(unsigned int sampleRate);
        void setBufferSize(unsigned int bufferSize);
        void setInputDevice(DeviceIndex inputDeviceIndex);
        void setOutputDevice(DeviceIndex outputDeviceIndex);
        const AudioDevice* getInputAudioDeviceByIndex(DeviceIndex inputDeviceIndex) const;
        const AudioDevice* getOutputAudioDeviceByIndex(DeviceIndex outputDeviceIndex) const;
        void pickDevices();

        void setAudioCallback(std::shared_ptr<AudioCallback> cb);

        bool start();
        bool stop();
        bool restart();
        bool isRunning() const;

    private:
        AudioEngine();
        ~AudioEngine();

        bool callback(const float *in, float *out, unsigned int nFrames);

        std::unique_ptr<AudioBackend> backend_;

        std::atomic<std::shared_ptr<AudioCallback>> userCallback_;

        mutable std::mutex streamMutex_;

        std::atomic<unsigned int> sampleRate_{48000};
        std::atomic<unsigned int> bufferSize_{128};

        DeviceIndex inputDeviceIndex_{kNoDevice};
        DeviceIndex outputDeviceIndex_{kNoDevice};

        unsigned int inputChannels_{2};
        unsigned int outputChannels_{2};

        std::vector<AudioDevice> inputDevices_;
        std::vector<AudioDevice> outputDevices_;

        struct PlanarAudioData
        {
            void setNumChannels(unsigned int numChannels);
            void deinterleave(const float *data, unsigned int nFrames);
            void interleave(float *data, unsigned int nFrames);

            std::vector<float*> planar;
            std::vector<std::vector<float>> buffers;
        };

        PlanarAudioData inputData_;
        PlanarAudioData outputData_;
    };
}