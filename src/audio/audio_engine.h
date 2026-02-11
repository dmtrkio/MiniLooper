#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <memory>

namespace audio {

    class AudioBackend;

    class AudioCallback
    {
    public:
        virtual ~AudioCallback() = default;

        AudioCallback(const AudioCallback&) = delete;
        AudioCallback& operator=(const AudioCallback&) = delete;

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

        // -- Only these are safe to be called from an audio callback --
        unsigned int getNumInputChannels() const noexcept;
        unsigned int getNumOutputChannels() const noexcept;
        unsigned int getSampleRate() const noexcept;
        unsigned int getBufferSize() const noexcept;
        // -------------------------------------------------------------

        void setSampleRate(unsigned int sampleRate);
        void setBufferSize(unsigned int bufferSize);
        void setAudioCallback(std::shared_ptr<AudioCallback> cb);

        bool start();
        bool stop();
        bool restart();
        bool isStreamRunning() const;

    private:
        AudioEngine();
        ~AudioEngine();

        bool callback(const float *const *in, float *const *out, unsigned int nFrames);

        std::unique_ptr<AudioBackend> backend_;

        std::atomic<std::shared_ptr<AudioCallback>> userCallback_;

        mutable std::mutex streamMutex_;

        std::atomic<unsigned int> sampleRate_{48000};
        std::atomic<unsigned int> bufferSize_{256};

        unsigned int inputDevice_{0};
        unsigned int outputDevice_{0};

        unsigned int inputChannels_{2};
        unsigned int outputChannels_{2};
    };

}
