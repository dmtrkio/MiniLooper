#pragma once

#include <vector>
#include <array>

#include "dsp/filter/biquad_filter.h"
#include "dsp/audio_parameter.h"
#include "dsp/dsp.h"

namespace dsp::effects {
    class Equalizer
    {
    public:
        void prepare(const float sampleRate)
        {
            static constexpr std::array<filter::FilterType, 6> kFilterTypes = {
                filter::FilterType::LowPass,
                filter::FilterType::LowShelf,
                filter::FilterType::Peaking,
                filter::FilterType::Peaking,
                filter::FilterType::HighShelf,
                filter::FilterType::HighPass,
            };

            static constexpr std::array<Filter::DataType, 6> kDefaultFrequencies = {
                25, 80, 200, 500, 1000, 18000
            };

            for (int i = 0; i < bands_.size(); ++i) {
                bands_[i].setParameters(kFilterTypes[i], kDefaultFrequencies[i], sampleRate);
            }
        }

        void operator()(float *const *data, const unsigned int nFrames) noexcept
        {
            for (auto &band : bands_) {
                band(data, nFrames);
            }
        }

    private:
        using Filter = filter::BiquadFilter<double, 2>;

        std::array<Filter, 6> bands_{};
    };
}