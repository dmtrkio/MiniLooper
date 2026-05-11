#pragma once

#include <array>

#include "../filter/biquad_filter.h"
#include "../parameter/parameter_tree.h"
#include "../parameter/parameter_view.h"
#include "effect_base.h"

namespace dsp::effects {
    class Equalizer final : public EffectBase
    {
    public:
        static constexpr std::size_t kBands = 6;

        Equalizer() : paramTree_("Equalizer")
        {
            using namespace parameter;
            using namespace dsp::filter::FilterDefaults;

            auto highPass = ParameterTree("HighPass", {
                Parameter::makeFloat("Frequency", kDefaultFrequencies[0], kBandRanges[0]),
                Parameter::makeFloat("Q", kDefaultQ, {0.5f, 8.0f}),
            });

            auto lowShelf = ParameterTree("LowShelf", {
                Parameter::makeFloat("Frequency", kDefaultFrequencies[1], kBandRanges[1]),
                Parameter::makeFloat("GainDb", 0.0f, {-12.0f, 12.0f}),
            });

            auto peaking1 = ParameterTree("Peak1", {
                Parameter::makeFloat("Frequency", kDefaultFrequencies[2], kBandRanges[2]),
                Parameter::makeFloat("GainDb", 0.0f, {-12.0f, 12.0f}),
            });

            auto peaking2 = ParameterTree("Peak2", {
                Parameter::makeFloat("Frequency", kDefaultFrequencies[3], kBandRanges[3]),
                Parameter::makeFloat("GainDb", 0.0f, {-12.0f, 12.0f}),
            });

            auto highShelf = ParameterTree("HighShelf", {
                Parameter::makeFloat("Frequency", kDefaultFrequencies[4], kBandRanges[4]),
                Parameter::makeFloat("GainDb", 0.0f, {-12.0f, 12.0f}),
            });

            auto lowPass = ParameterTree("LowPass", {
                Parameter::makeFloat("Frequency", kDefaultFrequencies[5], kBandRanges[5]),
                Parameter::makeFloat("Q", kDefaultQ, {0.5f, 8.0f}),
            });

            paramTree_.addSubTree(std::move(highPass));
            paramTree_.addSubTree(std::move(lowShelf));
            paramTree_.addSubTree(std::move(peaking1));
            paramTree_.addSubTree(std::move(peaking2));
            paramTree_.addSubTree(std::move(highShelf));
            paramTree_.addSubTree(std::move(lowPass));

            bindParameterViews();
        }

        void prepare(const float sampleRate) override
        {
            sampleRate_ = sampleRate;
            for (auto i{0u}; i < bands_.size(); ++i) {
                bands_[i].setParameters(kFilterTypes[i], kDefaultFrequencies[i], sampleRate);
            }
        }

        void process(float *const *data, const unsigned int nFrames) noexcept override
        {
            applyParams();

            for (auto &band : bands_) {
                band.processBlock(data, nFrames);
            }
        }

        parameter::ParameterTree getParameterTree() const noexcept override { return paramTree_; }

    private:
        struct BandViews {
            parameter::FloatParameterView freq;
            parameter::FloatParameterView q;
            parameter::FloatParameterView gainDb;
        };

        void bindParameterViews()
        {
            for (std::size_t i = 0; i < kBands; ++i) {
                auto subTree = paramTree_[kBandNames[i]];
                bandViews_[i].freq.referTo(subTree["Frequency"].asParameterUnsafe());

                const auto type = kFilterTypes[i];
                if (type == filter::FilterType::LowPass || type == filter::FilterType::HighPass) {
                    bandViews_[i].q.referTo(subTree["Q"].asParameterUnsafe());
                } else {
                    bandViews_[i].gainDb.referTo(subTree["GainDb"].asParameterUnsafe());
                }
            }
        }

        void applyParams()
        {
            using namespace dsp::filter::FilterDefaults;

            for (std::size_t i = 0; i < kBands; ++i) {
                const auto freqHz = bandViews_[i].freq.get();
                const auto type = kFilterTypes[i];

                if (type == filter::FilterType::LowPass || type == filter::FilterType::HighPass) {
                    bands_[i].setParameters(
                        type, sampleRate_, freqHz,
                        bandViews_[i].q.get(), kDefaultBw, kDefaultSlope, kDefaultGain
                    );
                } else {
                    bands_[i].setParameters(
                        type, sampleRate_, freqHz,
                        kDefaultQ, kDefaultBw, kDefaultSlope, dBtoLinear(bandViews_[i].gainDb.get())
                    );
                }
            }
        }

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

        dsp::parameter::ParameterTree paramTree_;
    };
}