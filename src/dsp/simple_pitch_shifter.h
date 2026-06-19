#pragma once

#include <vector>

namespace dsp {
    class SimplePitchShifter
    {
    public:
        void prepare(float sampleRate);
        float process(float x);
        void clearState();
        void setSemitones(float semitones);
        void setPitchRatio(float r);

    private:
        float readLinear(float pos) const;
        void computeGains(float aPos, float& gA, float bPos, float& gB) noexcept;
        float window(float d) const;
        float distanceToWrite(float r) const;
        void wrap(float& x) const;

        std::vector<float> buffer;
        int size = 0;
        int writePos = 0;
        float readA = 0.0f;
        float readB = 0.0f;
        float ratio = 1.0f;
    };
}
