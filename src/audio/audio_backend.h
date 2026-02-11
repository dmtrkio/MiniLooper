#pragma once

#include <functional>

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

        virtual bool startStream(StreamParams &params) = 0;
        virtual bool stopStream() = 0;
        [[nodiscard]] virtual bool isStreamRunning() const = 0;

    protected:
        Callback audioCallback_;
    };

}
