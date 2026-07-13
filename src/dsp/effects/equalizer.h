#pragma once

#include <array>

#include "dsp/filter/biquad_filter.h"
#include "dsp/parameter/parameter_view.h"
#include "effect_base.h"

namespace ml::dsp::effects {
    class Equalizer final : public EffectBase
    {
    public:
        static constexpr std::size_t kBands = 6;

        Equalizer();

    protected:
        void prepareInner(const float sampleRate) override;
        void processInner(float *const *data, const unsigned int nFrames) noexcept override;

    private:
        struct BandViews {
            parameter::FloatParameterView freq;
            parameter::FloatParameterView q;
            parameter::FloatParameterView gainDb;
        };

        using Filter = filter::BiquadFilter<float, 2>;

        static constexpr std::array<filter::FilterType, kBands> kFilterTypes = {
            filter::FilterType::HighPass,
            filter::FilterType::LowShelf,
            filter::FilterType::Peaking,
            filter::FilterType::Peaking,
            filter::FilterType::HighShelf,
            filter::FilterType::LowPass,
        };

        static constexpr std::array<std::string_view, kBands> kBandNames = {
            "HighPass", "LowShelf", "Peak1", "Peak2", "HighShelf", "LowPass"
        };

        static constexpr std::array<Range<float>, kBands> kBandRanges = {
            Range{20.0f, 1000.0f},
            Range{20.0f, 1000.0f},
            Range{50.0f , 800.0f},
            Range{50.0f, 800.0f},
            Range{800.0f, 20000.0f},
            Range{800.0f, 20000.0f},
        };

        static constexpr std::array<Filter::DataType, kBands> kDefaultFrequencies = {
            25, 80, 200, 500, 1500, 18000
        };

        float sampleRate_{44100.0f};
        std::array<Filter, kBands> bands_{};
        std::array<BandViews, kBands> bandViews_;
    };
}