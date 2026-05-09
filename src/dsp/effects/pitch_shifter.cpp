#include "dsp/effects/pitch_shifter.h"

namespace dsp::effects {
    void PitchShifter::prepare(float sampleRate)
    {
        for (auto& buf : buffers_) {
            buf.resize(kBufferSize);
        }

        static constexpr float kSmoothingMs = 1.0f;
        const auto smoothFrames = kSmoothingMs * sampleRate * 0.001f;
        semitones_.setSmoothingFrames(smoothFrames);
        mix_.setSmoothingFrames(smoothFrames);
        semitones_.init(static_cast<float>(paramTree_["Semitones"].asParameterUnsafe().get<int>()));
        mix_.init(paramTree_["Mix"].asParameterUnsafe().get<float>());

        shifter_.configure(
            2,
            static_cast<int>(sampleRate * 0.025f),
            static_cast<int>(sampleRate * 0.0125f),
            true
        );

        shifter_.reset();

        const auto latency = static_cast<float>(shifter_.inputLatency() + shifter_.outputLatency());
        for(auto& d : delay_) {
            d.prepare(latency);
            d.setDelay(latency);
        }

        setOn();
    }

    void PitchShifter::process(float *const *data, unsigned int nFrames)
    {
        semitones_.setTarget(static_cast<float>(paramTree_["Semitones"].asParameterUnsafe().get<int>()));
        mix_.setTarget(paramTree_["Mix"].asParameterUnsafe().get<float>());

        setOn();
        if (!on_) return;

        assert(nFrames <= kBufferSize);

        shifter_.setTransposeSemitones(semitones_());
        shifter_.process(data, nFrames, buffers_, nFrames);

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
        const auto on = paramTree_["On"].asParameterUnsafe().get<bool>();
        if (!on && on_) {
            shifter_.reset();
            for (auto& d : delay_) {
                d.clear();
            }
        }
        on_ = on;
    }
}