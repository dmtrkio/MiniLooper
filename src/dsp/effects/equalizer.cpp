#include "equalizer.h"

namespace ml::dsp::effects {
    Equalizer::Equalizer() : EffectBase("Equalizer")
    {
        std::vector<ParamTree> params = {
            ParamTree{"HighPass", {
                Param::makeFloat("Frequency", kDefaultFrequencies[0], kBandRanges[0]),
                Param::makeFloat("Q", static_cast<float>(Filter::FilterDefaults::kDefaultQ), {0.5f, 8.0f}),
            }},

            ParamTree{"LowShelf", {
                Param::makeFloat("Frequency", kDefaultFrequencies[1], kBandRanges[1]),
                Param::makeFloat("GainDb", 0.0f, {-12.0f, 12.0f}),
            }},

            ParamTree{"Peak1", {
                Param::makeFloat("Frequency", kDefaultFrequencies[2], kBandRanges[2]),
                Param::makeFloat("GainDb", 0.0f, {-12.0f, 12.0f}),
            }},

            ParamTree{"Peak2", {
                Param::makeFloat("Frequency", kDefaultFrequencies[3], kBandRanges[3]),
                Param::makeFloat("GainDb", 0.0f, {-12.0f, 12.0f}),
            }},

            ParamTree{"HighShelf", {
                Param::makeFloat("Frequency", kDefaultFrequencies[4], kBandRanges[4]),
                Param::makeFloat("GainDb", 0.0f, {-12.0f, 12.0f}),
            }},

            ParamTree{"LowPass", {
                Param::makeFloat("Frequency", kDefaultFrequencies[5], kBandRanges[5]),
                Param::makeFloat("Q", static_cast<float>(Filter::FilterDefaults::kDefaultQ), {0.5f, 8.0f}),
            }}
        };

        attachParameters(params);

        for (std::size_t i = 0; i < kBands; ++i) {
            auto& subTree = params[i];
            bandViews_[i].freq.referTo(subTree["Frequency"].asParameterUnsafe());

            const auto type = kFilterTypes[i];
            if (type == filter::FilterType::LowPass || type == filter::FilterType::HighPass) {
                bandViews_[i].q.referTo(subTree["Q"].asParameterUnsafe());
            } else {
                bandViews_[i].gainDb.referTo(subTree["GainDb"].asParameterUnsafe());
            }
        }
    }

    void Equalizer::prepareInner(const float sampleRate)
    {
        sampleRate_ = sampleRate;
        for (auto i{0u}; i < bands_.size(); ++i) {
            bands_[i].setParameters(kFilterTypes[i], kDefaultFrequencies[i], sampleRate);
        }
    }

    void Equalizer::processInner(float *const *data, const unsigned int nFrames) noexcept
    {
        for (std::size_t i = 0; i < kBands; ++i) {
            const auto freqHz = bandViews_[i].freq.get();
            const auto type = kFilterTypes[i];

            if (type == filter::FilterType::LowPass || type == filter::FilterType::HighPass) {
                bands_[i].setParameters(
                    type, sampleRate_, freqHz,
                    bandViews_[i].q.get(), Filter::FilterDefaults::kDefaultBw, Filter::FilterDefaults::kDefaultSlope, Filter::FilterDefaults::kDefaultGain
                );
            } else {
                bands_[i].setParameters(
                    type, sampleRate_, freqHz,
                    Filter::FilterDefaults::kDefaultQ, Filter::FilterDefaults::kDefaultBw, Filter::FilterDefaults::kDefaultSlope, dBtoLinear(bandViews_[i].gainDb.get())
                );
            }
        }

        for (auto &band : bands_) {
            band.processBlock(data, nFrames);
        }
    }
}