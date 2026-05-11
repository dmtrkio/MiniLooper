#include "dsp/effects/pitch_shifter.h"

#include <bungee/Stream.h>

#include "dsp/dsp.h"

namespace dsp::effects {
    struct PitchShifter::PitcherState
    {
        Bungee::Stretcher<Bungee::Basic> stretcher;
        Bungee::Stream<Bungee::Basic> stream;

        PitcherState(float sampleRate, int bufferSize)
            : stretcher({static_cast<int>(sampleRate), static_cast<int>(sampleRate)}, 2, -1)
            , stream(stretcher, bufferSize, 2)
        {
            //stretcher.enableInstrumentation(true);
        }

        void process(float *const *input, float *const *output, int nFrames, double pitch)
        {
            const auto result = stream.process(
                input,
                output,
                nFrames,
                static_cast<double>(nFrames),
                pitch
            );
            assert(result == nFrames);
        }

        double latency() const
        {
            return stream.latency();
        }
    };

    PitchShifter::PitchShifter() = default;
    PitchShifter::~PitchShifter() = default;

    PitchShifter::PitchShifter(PitchShifter&&) noexcept = default;
    PitchShifter& PitchShifter::operator=(PitchShifter&&) noexcept = default;

    void PitchShifter::prepare(float sampleRate)
    {
        for (auto& buf : buffers_) {
            buf.resize(kBufferSize);
        }
        
        onParam_.referTo(paramTree_["On"].asParameterUnsafe());
        semitonesParam_.referTo(paramTree_["Semitones"].asParameterUnsafe());
        mixParam_.referTo(paramTree_["Mix"].asParameterUnsafe());

        static constexpr float kSmoothingMs = 1.0f;
        const auto smoothFrames = kSmoothingMs * sampleRate * 0.001f;

        semitones_.setSmoothingFrames(smoothFrames);
        mix_.setSmoothingFrames(smoothFrames);

        semitones_.init(static_cast<float>(semitonesParam_.get()));
        mix_.init(mixParam_.get());

        pitcher_ = std::make_unique<PitcherState>(sampleRate, kBufferSize);

        const float maxLatency = sampleRate;
        for (auto& d : delay_) {
            d.prepare(maxLatency);
        }

        setOn();
    }

    void PitchShifter::process(float *const *data, unsigned int nFrames)
    {
        semitones_.setTarget(static_cast<float>(semitonesParam_.get()));
        mix_.setTarget(mixParam_.get());

        setOn();
        if (!on_) return;

        assert(nFrames <= kBufferSize);

        float *const streamData[2] = {buffers_[0].data(), buffers_[1].data()};
        const auto pitch = dsp::semitonesToPitchMultiplier(semitones_());
        pitcher_->process(data, streamData, nFrames, static_cast<double>(pitch));

        const auto latency = static_cast<float>(pitcher_->latency());
        for(auto& d : delay_) {
            d.setDelay(latency);
        }

        staticFor<2>([&](auto channel) {
            for (std::size_t i{0u}; i < nFrames; ++i) {
                const auto delayed = delay_[channel].process(data[channel][i]);
                data[channel][i] = std::lerp(delayed, buffers_[channel][i], mix_.get<channel>());
            }
        });
    }

    parameter::ParameterTree PitchShifter::getParameterTree() const
    {
        return paramTree_;
    }

    void PitchShifter::setOn()
    {
        const auto on = onParam_.get();
        if (!on && on_) {
            for (auto& d : delay_) {
                d.clear();
            }
        }
        on_ = on;
    }
}