#include "guitar_amp.h"
#include "dsp/dsp.h"
#include "dsp/effects/guitar_amp.h"

namespace ml::dsp::effects {
    GuitarAmp::GuitarAmp() : EffectBase("GuitarAmp")
    {
        std::vector<ParamTree> params = {
            ParamTree{Param::makeFloat("Drive", 0.0f, dsp::Range{0.0f, 1.0f})},
            ParamTree{Param::makeFloat("Tone", 0.5f, dsp::Range{0.0f, 1.0f})},
            ParamTree{Param::makeFloat("Level", 0.5f, dsp::Range{0.0f, 1.0f})},
            ParamTree{Param::makeFloat("DryWet", 1.0f, dsp::Range{0.0f, 1.0f})},
        };

        attachParameters(params);

        driveParam_.referTo(params[0].asParameterUnsafe());
        toneParam_.referTo(params[1].asParameterUnsafe());
        levelParam_.referTo(params[2].asParameterUnsafe());
        dryWetParam_.referTo(params[3].asParameterUnsafe());
    }

    void GuitarAmp::prepareInner(float sampleRate)
    {
        static constexpr float kSmoothingMs = 1.0f;
        const auto smoothFrames = kSmoothingMs * sampleRate * 0.001f;

        drive.setSmoothingFrames(smoothFrames);
        tone.setSmoothingFrames(smoothFrames);
        level.setSmoothingFrames(smoothFrames);
        dryWet.setSmoothingFrames(smoothFrames);

        drive.init(driveParam_.get());
        tone.init(toneParam_.get());
        level.init(levelParam_.get());
        dryWet.init(dryWetParam_.get());

        preamp.prepare(sampleRate);
        toneStack.prepare(sampleRate);
        cabinet.prepare(sampleRate);

        bias_ = 0.18f;
        biasTanh_ = std::tanh(bias_);
    }

    void GuitarAmp::processInner(float *const *data, const unsigned int nFrames) noexcept
    {
        drive = driveParam_.get();
        tone = toneParam_.get();
        level = levelParam_.get();
        dryWet = dryWetParam_.get();

        const auto powerAmpDrive = [bias = bias_, biasTanh = biasTanh_](float sample, float drive) -> float {
            const float gain = (1.0f + drive * 10.0f);
            return biasedTanh(sample * gain, bias, biasTanh);
        };

        staticFor<2>([&](auto channel) {
            for (std::size_t frame{}; frame < nFrames; ++frame) {
                const auto inputSample = data[channel][frame];
                preamp.setDrive(drive.get<channel>());
                toneStack.setTone(tone.get<channel>());
                const auto preampOutput = preamp.processSample<channel>(inputSample, bias_, biasTanh_);
                const auto toneStackOutput = toneStack.processSample<channel>(preampOutput);
                const auto powerAmpOutput = powerAmpDrive(toneStackOutput, drive.get<channel>());
                const auto cabinetOutput = cabinet.processSample<channel>(powerAmpOutput);
                const auto processedSample = cabinetOutput * level.get<channel>();
                data[channel][frame] = std::lerp(inputSample, processedSample, dryWet.get<channel>());
            }
        });
    }

    GuitarAmp::Preamp::Preamp()
        : driveMin(dBtoLinear(6.0f))
        , driveMax(dBtoLinear(30.0f))
        , drive(0.0f)
    {
        setDrive(0.0f);
    }

    void GuitarAmp::Preamp::prepare(float sampleRate)
    {
        hp.setParameters(filter::FilterType::HighPass, sampleRate, 80.0f);
        lp.setParameters(filter::FilterType::LowPass, sampleRate, 5000.0f);
        presenceBoost.setParameters(
            filter::FilterType::Peaking,
            sampleRate,
            2500.0f,
            1.0f,
            std::nullopt,
            std::nullopt,
            dBtoLinear(8.0f)
        );
        lowShelf.setParameters(
            filter::FilterType::LowShelf,
            sampleRate,
            150.0f,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            dBtoLinear(-6.0f)
        );
    }

    void GuitarAmp::Preamp::setDrive(float d) noexcept
    {
        drive = std::lerp(driveMin, driveMax, std::clamp(d, 0.0f, 1.0f));
    }

    void GuitarAmp::ToneStack::prepare(float sampleRate)
    {
        sr = sampleRate;
        setTone(0.5f);
    }

    void GuitarAmp::ToneStack::setTone(float tone) noexcept
    {
        float lowGain = 8.0f - tone * 12.0f;
            low.setParameters(
                filter::FilterType::LowShelf,
                sr,
                100.0f,
                std::nullopt,
                std::nullopt,
                std::nullopt,
                dBtoLinear(lowGain)
            );
            
            float midGain = (tone - 0.5f) * 6.0f;
            float midFreq = 400.0f + tone * 600.0f;
            float midQ = 1.5f - tone * 0.6f;
            mid.setParameters(
                filter::FilterType::Peaking,
                sr,
                midFreq,
                midQ,
                std::nullopt,
                std::nullopt,
                dBtoLinear(midGain)
            );
            
            float highGain = -6.0f + tone * 16.0f;
            float highFreq = 1500.0f + tone * 2000.0f;
            high.setParameters(
                filter::FilterType::Peaking,
                sr,
                highFreq,
                0.8f,
                std::nullopt,
                std::nullopt,
                dBtoLinear(highGain)
            );
            
            float presenceGain = (tone > 0.5f)
                ? ((tone - 0.5f) * 16.0f)
                : (tone * 4.0f - 2.0f);
            presence.setParameters(
                filter::FilterType::HighShelf,
                sr,
                4500.0f,
                std::nullopt,
                std::nullopt,
                std::nullopt,
                dBtoLinear(presenceGain)
            );
    }

    void GuitarAmp::Cabinet::prepare(float sampleRate)
    {
        highPass.setParameters(filter::FilterType::HighPass, sampleRate, 80.0f);
        resonance.setParameters(
            filter::FilterType::Peaking,
            sampleRate,
            400.0f,
            2.0f,
            std::nullopt,
            std::nullopt,
            dBtoLinear(4.0f)
        );
        notch.setParameters(filter::FilterType::Notch, sampleRate, 3000.0f, 3.0f);
        lowPass.setParameters(filter::FilterType::LowPass, sampleRate, 5000.0f);
    }
}