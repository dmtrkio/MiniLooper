#pragma once

#include <array>
#include <cassert>

#include "../filter/biquad_filter.h"
#include "../parameter/parameter_tree.h"

namespace dsp::effects {
    class Equalizer
    {
    public:
        static constexpr std::size_t kBands = 6;

        Equalizer() : params_("Equalizer")
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

            params_.addSubTree(std::move(lowShelf));
            params_.addSubTree(std::move(peaking1));
            params_.addSubTree(std::move(peaking2));
            params_.addSubTree(std::move(highShelf));
            params_.addSubTree(std::move(highPass));
            params_.addSubTree(std::move(lowPass));
        }

        void prepare(const float sampleRate)
        {
            sampleRate_ = sampleRate;
            for (int i = 0; i < bands_.size(); ++i) {
                bands_[i].setParameters(kFilterTypes[i], kDefaultFrequencies[i], sampleRate);
            }
        }

        void operator()(float *const *data, const unsigned int nFrames) noexcept
        {
            applyParams();

            bands_[0](data, nFrames);
            //bands_[1](data, nFrames);
            bands_[2](data, nFrames);
            bands_[3](data, nFrames);
            //bands_[4](data, nFrames);
            bands_[5](data, nFrames);

            /* for (auto &band : bands_) {
                band(data, nFrames);
            } */
        }

        parameter::ParameterTree& getParameterTree() noexcept { return params_; }

    private:
        void applyParams()
        {
            static constexpr std::array<const char*, kBands> kBandNames = {
                "HighPass", "LowShelf", "Peak1", "Peak2", "HighShelf", "LowPass"
            };

            for (std::size_t i = 0; i < kBands; ++i) {
                auto* subTree = params_[kBandNames[i]];
                assert(subTree);

                const auto freqHz = subTree->getParameter("Frequency")->get().get<float>();

                const auto type = kFilterTypes[i];

                std::optional<float> qOpt = std::nullopt;
                std::optional<float> gainOpt = std::nullopt;

                if (type == filter::FilterType::LowPass || type == filter::FilterType::HighPass) {
                    auto qParam = subTree->getParameter("Q");
                    if (qParam) qOpt = qParam->get().get<float>();
                } else {
                    auto gainParam = subTree->getParameter("GainDb");
                    if (gainParam) {
                        const float gainDb = gainParam->get().get<float>();
                        gainOpt = dBtoLinear(gainDb);
                    }
                }

                using namespace dsp::filter::FilterDefaults;
                bands_[i].setParameters(
                    type,
                    sampleRate_,
                    freqHz,
                    qOpt.value_or(kDefaultQ),
                    kDefaultBw,
                    kDefaultSlope,
                    gainOpt.value_or(kDefaultGain)
                );
            }
        }

        using Filter = filter::BiquadFilter<double, 2>;

        static constexpr std::array<filter::FilterType, kBands> kFilterTypes = {
            filter::FilterType::HighPass,
            filter::FilterType::LowShelf,
            filter::FilterType::Peaking,
            filter::FilterType::Peaking,
            filter::FilterType::HighShelf,
            filter::FilterType::LowPass,
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

        parameter::ParameterTree params_;
    };
}