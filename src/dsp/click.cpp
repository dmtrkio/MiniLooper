#include "click.h"

#include "dsp.h"

namespace ml::dsp {
    static constexpr float kPitchAttackMs  = 0.0f;
    static constexpr float kPitchReleaseMs = 3.0f;
    static constexpr float kAmpAttackMs    = 0.1f;
    static constexpr float kAmpReleaseMs   = 25.0f;

    static constexpr Range<float> kPitchRange = {
        .min = 440.0f,
        .max = 600.0f
    };

    void ClickGenerator::prepare(float sampleRate)
    {
        sampleRate_ = sampleRate;

        pitchEnv_.prepare(sampleRate_, kPitchAttackMs, kPitchReleaseMs);
        ampEnv_.prepare(sampleRate_, kAmpAttackMs, kAmpReleaseMs);
    }

    float ClickGenerator::process(bool click)
    {
        if (click) {
            osc_.reset();
        }

        const auto toggle = static_cast<float>(click);
        const auto pitch = kPitchRange.linmap(pitchEnv_.process(toggle));
        osc_.setFrequency(pitch, sampleRate_);
        const auto oscOutput = osc_.triangle();
        osc_.tick();
        const auto amp = ampEnv_.process(toggle);
        return oscOutput * amp;
    }
}