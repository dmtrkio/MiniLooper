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

            auto lowPass = ParameterTree("LowPass", {
                Parameter::makeFloat("Frequency", kDefaultFrequencies[0], {20.0f, 1000.0f}),
                Parameter::makeFloat("Q", kDefaultQ, {0.5f, 8.0f}),
            });

            auto lowShelf = ParameterTree("LowShelf", {
                Parameter::makeFloat("Frequency", kDefaultFrequencies[1], {20.0f, 1000.0f}),
                Parameter::makeFloat("GainDb", 0.0f, {-12.0f, 12.0f}),
            });

            auto peaking1 = ParameterTree("Peak1", {
                Parameter::makeFloat("Frequency", kDefaultFrequencies[2], {80.0f, 600.0f}),
                Parameter::makeFloat("GainDb", 0.0f, {-12.0f, 12.0f}),
            });

            auto peaking2 = ParameterTree("Peak2", {
                Parameter::makeFloat("Frequency", kDefaultFrequencies[3], {600.0f, 2000.0f}),
                Parameter::makeFloat("GainDb", 0.0f, {-12.0f, 12.0f}),
            });

            auto highShelf = ParameterTree("HighShelf", {
                Parameter::makeFloat("Frequency", kDefaultFrequencies[4], {200.0f, 20000.0f}),
                Parameter::makeFloat("GainDb", 0.0f, {-12.0f, 12.0f}),
            });

            auto highPass = ParameterTree("HighPass", {
                Parameter::makeFloat("Frequency", kDefaultFrequencies[5], {200.0f, 20000.0f}),
                Parameter::makeFloat("Q", kDefaultQ, {0.5f, 8.0f}),
            });

            params_.addSubTree(std::move(lowPass));
            params_.addSubTree(std::move(lowShelf));
            params_.addSubTree(std::move(peaking1));
            params_.addSubTree(std::move(peaking2));
            params_.addSubTree(std::move(highShelf));
            params_.addSubTree(std::move(highPass));
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
            for (auto &band : bands_) {
                band(data, nFrames);
            }
        }

        parameter::ParameterTree& getParameterTree() noexcept { return params_; }

    private:
        void applyParams()
        {
            static constexpr std::array<const char*, kBands> kBandNames = {
                "LowPass", "LowShelf", "Peak1", "Peak2", "HighShelf", "HighPass"
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
            filter::FilterType::LowPass,
            filter::FilterType::LowShelf,
            filter::FilterType::Peaking,
            filter::FilterType::Peaking,
            filter::FilterType::HighShelf,
            filter::FilterType::HighPass,
        };

        static constexpr std::array<Filter::DataType, kBands> kDefaultFrequencies = {
            25, 80, 200, 500, 1000, 18000
        };

        float sampleRate_{44100.0f};
        std::array<Filter, kBands> bands_{};

        parameter::ParameterTree params_;
    };
}