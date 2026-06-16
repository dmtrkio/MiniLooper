#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

namespace dsp
{
    class SimplePitchShifter
    {
    public:
        void prepare(float sampleRate)
        {
            constexpr float delayMs = 100.0f;

            size = static_cast<int>(sampleRate * delayMs * 0.001f);

            buffer.assign(size, 0.0f);

            writePos = 0;

            readA = 0.0f;
            readB = size * 0.5f;
        }

        void clearState()
        {
            std::ranges::fill(buffer, 0.0f);

            writePos = 0;

            readA = 0.0f;
            readB = size * 0.5f;
        }

        void setSemitones(float semitones)
        {
            ratio = std::pow(2.0f, semitones / 12.0f);
        }

        void setPitchRatio(float r)
        {
            ratio = r;
        }

        float process(float x)
        {
            buffer[writePos] = x;

            float a = readLinear(readA);
            float b = readLinear(readB);

            float gA, gB;
            computeGains(readA, gA, readB, gB);

            float y = a * gA + b * gB;

            writePos = (writePos + 1) % size;

            readA += ratio;
            readB += ratio;

            wrap(readA);
            wrap(readB);

            return y;
        }

    private:
        float readLinear(float pos) const
        {
            const int i0 = static_cast<int>(pos);
            const int i1 = (i0 + 1) % size;

            float t = pos - i0;

            return std::lerp(buffer[i0], buffer[i1], t);
        }

        void computeGains(float aPos, float& gA, float bPos, float& gB) noexcept
        {
            float dA = distanceToWrite(aPos);
            float dB = distanceToWrite(bPos);

            gA = window(dA);
            gB = window(dB);
        }

        float window(float d) const
        {
            float x = d / size;

            // Hann window
            return 0.5f - 0.5f * std::cos(2.0f * float(M_PI) * x);
        }

        float distanceToWrite(float r) const
        {
            float d = writePos - r;
            if (d < 0) d += size;
            return d;
        }

        void wrap(float& x) const
        {
            const auto s = static_cast<float>(size);

            while (x >= s) {
                x -= s;
            }

            while (x < 0.0f) {
                x += s;
            }
        }

    private:
        std::vector<float> buffer;

        int size = 0;

        int writePos = 0;

        float readA = 0.0f;
        float readB = 0.0f;

        float ratio = 1.0f;
    };
}
