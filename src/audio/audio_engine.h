#pragma once

#include <atomic>
#include <functional>
#include <vector>
#include <mutex>
#include <memory>

namespace rt::audio {
    class RtAudio;
}

namespace audio {

    class AudioBackend
    {
    public:
        using Callback = std::function<bool(const float *const *in, float *const *out, unsigned int nFrames)>;

        struct StreamParams
        {
            unsigned int sampleRate{44100};
            unsigned int bufferSize{512};
            unsigned int numInputChannels{2};
            unsigned int numOutputChannels{2};
        };

        explicit AudioBackend(Callback audioCallback) : audioCallback_(std::move(audioCallback)) {}
        virtual ~AudioBackend() = default;

        virtual bool startStream(StreamParams &params);
        virtual bool stopStream();
        [[nodiscard]] virtual bool isStreamRunning() const;

    protected:
        Callback audioCallback_;
    };

    class AudioCallback
    {
    public:
        virtual ~AudioCallback() = default;
        virtual void onProcess(const float **in, float **out, unsigned int nFrames) = 0;
        virtual void onStart() = 0;
        virtual void onStop() = 0;
    };

    class AudioEngine
    {
    public:
        static AudioEngine& getInstance();

        AudioEngine(const AudioEngine&) = delete;
        AudioEngine& operator=(const AudioEngine&) = delete;

        void printApis() const;
        void printDevices() const;

        // -- Only these are safe to be called from an audio callback --
        unsigned int getNumInputChannels() const noexcept;
        unsigned int getNumOutputChannels() const noexcept;
        unsigned int getSampleRate() const noexcept;
        unsigned int getBufferSize() const noexcept;
        // -------------------------------------------------------------

        void setSampleRate(unsigned int sampleRate);
        void setBufferSize(unsigned int bufferSize);
        void setAudioCallback(std::shared_ptr<AudioCallback> cb);

        bool startStream();
        void stopStream();
        bool isStreamRunning() const;

    private:
        AudioEngine();
        ~AudioEngine();

        bool isStreamOpen() const;
        bool openStream();
        void closeStream();
        void restartStream();

        static int staticCallback(void* outputBuffer,
                                  void* inputBuffer,
                                  unsigned int nFrames,
                                  double streamTime,
                                  unsigned int status,
                                  void* userData);

        std::unique_ptr<rt::audio::RtAudio> rtAudio_;
        std::atomic<std::shared_ptr<AudioCallback>> userCallback_;

        std::mutex streamMutex_;
        std::atomic<bool> streamRunning_{false};

        unsigned int inputDevice_{0};
        unsigned int outputDevice_{0};
        unsigned int inputChannels_{2};
        unsigned int outputChannels_{2};
        std::atomic<unsigned int> sampleRate_{44100};
        std::atomic<unsigned int> bufferSize_{512};
        std::vector<const float*> inputBuffers_;
        std::vector<float*> outputBuffers_;
    };

}