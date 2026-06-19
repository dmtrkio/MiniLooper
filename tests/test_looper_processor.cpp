#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <numeric>

#include "looper/looper_processor.h"

using Catch::Matchers::WithinAbs;
using namespace ml::looper;

namespace test {
    constexpr float kSampleRate = 44100.0f;
    constexpr unsigned int kBufferSize = 64;

    struct Buffers
    {
        static constexpr auto count = kBufferSize;

        float l[count]{};
        float r[count]{};

        float* const data[2]{l, r};

        void clear()
        {
            std::ranges::fill(l, 0.0f);
            std::ranges::fill(r, 0.0f);
        }

        void fill(float v)
        {
            std::ranges::fill(l, v);
            std::ranges::fill(r, v);
        }

        [[nodiscard]] float energy() const
        {
            return std::transform_reduce(
                std::begin(l),
                std::end(l),
                std::begin(r),
                0.0f,
                std::plus<>(),
                [](float a, float b) {
                    return (a * a) + (b * b);
                }
            );
        }
    };

    void processSilence(LooperProcessor& p, unsigned int blocks = 1)
    {
        Buffers b;

        for (unsigned int i = 0; i < blocks; ++i)
            p.process(b.data, b.count);
    }
}

TEST_CASE("snapForwardSquareGrid")
{
    REQUIRE(snapForwardSquareGrid(4, 4) == 4);
    REQUIRE(snapForwardSquareGrid(2, 4) == 4);
    REQUIRE(snapForwardSquareGrid(5, 4) == 8);
    REQUIRE(snapForwardSquareGrid(5, 6) == 6);
}

TEST_CASE("estimateQuarterNoteUnit")
{
    constexpr float sr = 44100.0f;

    SECTION("Zero and negative input returns 0") {
        REQUIRE(estimateQuarterNoteUnit(0, sr) == 0);
        REQUIRE(estimateQuarterNoteUnit(-100, sr) == 0);
    }

    SECTION("Quarter note at 240 BPM — loop itself becomes the unit") {
        FrameInt loop = static_cast<FrameInt>(sr * 0.25f);
        REQUIRE(estimateQuarterNoteUnit(loop, sr) == loop);
    }

    SECTION("Quarter note at 120 BPM — loop itself becomes the unit") {
        FrameInt loop = static_cast<FrameInt>(sr * 0.5f);
        REQUIRE(estimateQuarterNoteUnit(loop, sr) == loop);
    }

    SECTION("Half bar at 120 BPM") {
        FrameInt loop = static_cast<FrameInt>(sr * 1.0f);
        REQUIRE(estimateQuarterNoteUnit(loop, sr) == static_cast<FrameInt>(sr * 0.5f));
    }

    SECTION("One bar at 120 BPM") {
        FrameInt loop = static_cast<FrameInt>(sr * 2.0f);
        REQUIRE(estimateQuarterNoteUnit(loop, sr) == static_cast<FrameInt>(sr * 0.5f));
    }

    SECTION("Two bars at 120 BPM") {
        FrameInt loop = static_cast<FrameInt>(sr * 4.0f);
        REQUIRE(estimateQuarterNoteUnit(loop, sr) == static_cast<FrameInt>(sr * 0.5f));
    }

    SECTION("One bar at 60 BPM is interpreted as two bars at 120 BPM") {
        FrameInt loop = static_cast<FrameInt>(sr * 4.0f);
        REQUIRE(estimateQuarterNoteUnit(loop, sr) == static_cast<FrameInt>(sr * 0.5f));
    }

    SECTION("Eight bars at 120 BPM") {
        FrameInt loop = static_cast<FrameInt>(sr * 16.0f);
        REQUIRE(estimateQuarterNoteUnit(loop, sr) == static_cast<FrameInt>(sr * 0.5f));
    }

    SECTION("Sixty-four bars at 120 BPM") {
        FrameInt loop = static_cast<FrameInt>(sr * 128.0f);
        REQUIRE(estimateQuarterNoteUnit(loop, sr) == static_cast<FrameInt>(sr * 0.5f));
    }

    SECTION("Very short loop falls back to itself") {
        FrameInt loop = static_cast<FrameInt>(sr * 0.1f);
        REQUIRE(estimateQuarterNoteUnit(loop, sr) == loop);
    }

    SECTION("Very long loop falls back to itself") {
        FrameInt loop = static_cast<FrameInt>(sr * 512.0f);
        REQUIRE(estimateQuarterNoteUnit(loop, sr) == loop);
    }
}

TEST_CASE("LooperProcessor State Machine", "[looper_processor][state]")
{
    LooperProcessor p;
    p.prepare(test::kSampleRate);

    const auto sr = test::kSampleRate;
    const auto maxFrames = p.getMaxFramesInLoop();

    SECTION("Correct max frame count") {
        REQUIRE(maxFrames == static_cast<unsigned int>(sr * kMaxLoopSecs));
    }

    SECTION("Initial recording") {
        constexpr int track = 0;
        test::Buffers b;

        p.process(b.data, b.count);
        REQUIRE(p.getState(track) == State::Cleared);

        p.startRecording(track);
        REQUIRE(p.getState(track) == State::Recording);

        p.process(b.data, b.count);
        REQUIRE(p.getState(track) == State::Recording);

        p.stopRecording(track);

        REQUIRE(p.getState(track) == State::Playback);
        REQUIRE(p.getCurrentPosition(track) == 0);
        REQUIRE(p.getCurrentNumFrames(track) == b.count);
    }

    SECTION("Cannot record two tracks simultaneously") {
        constexpr int t0 = 0;
        constexpr int t1 = 1;

        p.startRecording(t0);

        REQUIRE(p.getState(t0) == State::Recording);
        REQUIRE(p.getState(t1) == State::Cleared);

        p.startRecording(t1);

        REQUIRE(p.getState(t0) == State::Recording);
        REQUIRE(p.getState(t1) == State::Cleared);
    }

    SECTION("Synced recording of second track") {
        constexpr int t0 = 0;
        constexpr int t1 = 1;

        p.startRecording(t0);
        test::processSilence(p, 3);
        p.stopRecording(t0);

        test::processSilence(p, 2);
        p.startRecording(t1);
        REQUIRE(p.getState(t1) == State::Cleared);

        test::processSilence(p, 1);
        REQUIRE(p.getState(t1) == State::Cleared);

        test::processSilence(p, 1);
        REQUIRE(p.getState(t1) == State::Recording);

        p.stopRecording(t1);
        REQUIRE(p.getState(t1) == State::Recording);

        test::processSilence(p, 2);
        REQUIRE(p.getState(t1) == State::Recording);

        test::processSilence(p, 1);
        REQUIRE(p.getState(t1) == State::Playback);
        REQUIRE(p.getCurrentNumFrames(t1) == p.getCurrentNumFrames(t0));
    }

    SECTION("Reaching max loop length automatically toggles playback") {
        constexpr int track = 0;

        p.startRecording(track);
        test::processSilence(p, p.getMaxFramesInLoop() / test::kBufferSize + 1);
        REQUIRE(p.getState(track) == State::Playback);
        REQUIRE(p.getCurrentNumFrames(track) == p.getMaxFramesInLoop());
    }

    SECTION("Reaching max loop length for a non-first loop automatically overdubs") {
        constexpr int t0 = 0;
        constexpr int t1 = 1;

        p.startRecording(t0);
        test::processSilence(p);
        p.stopRecording(t0);

        p.startRecording(t1);
        test::processSilence(p, p.getMaxFramesInLoop() / test::kBufferSize + 1);
        REQUIRE(p.getState(t1) == State::Recording);
        const auto maxPossibleLoopLength = p.getMaxFramesInLoop() - p.getMaxFramesInLoop() % p.getCurrentNumFrames(t0);
        REQUIRE(p.getCurrentNumFrames(t1) == maxPossibleLoopLength);
    }

    SECTION("Pause and resume") {
        constexpr int track = 0;

        test::Buffers b;

        p.startRecording(track);
        p.process(b.data, b.count);

        p.stopRecording(track);

        REQUIRE(p.getState(track) == State::Playback);

        p.pause(track, false);
        p.process(b.data, b.count);
        REQUIRE(p.getState(track) == State::Paused);

        p.resume(track, false);
        p.process(b.data, b.count);
        REQUIRE(p.getState(track) == State::Playback);
    }

    SECTION("Clear resets track") {
        constexpr int track = 0;

        test::Buffers b;

        p.startRecording(track);
        p.process(b.data, b.count);
        p.stopRecording(track);

        REQUIRE(p.getState(track) == State::Playback);
        REQUIRE_FALSE(p.isEmpty(track));

        p.clear(track);

        REQUIRE(p.getState(track) == State::Cleared);
        REQUIRE(p.isEmpty(track));
        REQUIRE(p.getCurrentNumFrames(track) == 0);
    }
    
    SECTION("Clear All tracks") {
        constexpr int t0 = 0;
        constexpr int t1 = 0;

        p.startRecording(t0);
        test::processSilence(p, 1);
        p.stopRecording(t0);
        REQUIRE(!p.isEmpty(t0));

        p.startRecording(t1);
        test::processSilence(p, 1);
        p.stopRecording(t1);
        REQUIRE(!p.isEmpty(t1));

        p.clearAll();
        REQUIRE(p.isEmpty(t0));
        REQUIRE(p.isEmpty(t1));
    }
}

TEST_CASE("LooperProcessor DSP Behavior", "[looper_processor][dsp]")
{
    LooperProcessor p;
    p.prepare(test::kSampleRate);

    SECTION("Recorded audio plays back with non-zero energy") {
        constexpr int track = 0;

        test::Buffers in;
        in.fill(0.5f);

        p.startRecording(track);

        p.process(in.data, in.count);

        p.stopRecording(track);

        test::Buffers out;
        out.clear();

        p.process(out.data, out.count);

        REQUIRE(out.energy() > 0.0f);
    }

    SECTION("Cleared track outputs silence") {
        constexpr int track = 0;

        test::Buffers in;
        in.fill(1.0f);

        p.startRecording(track);
        p.process(in.data, in.count);
        p.stopRecording(track);

        p.clear(track);

        test::Buffers out;
        out.clear();

        p.process(out.data, out.count);

        REQUIRE_THAT(out.energy(), WithinAbs(0.0f, 1e-5f));
    }

    SECTION("Overdub increases loop energy") {
        constexpr int track = 0;

        test::Buffers in;
        in.fill(0.25f);

        p.startRecording(track);
        p.process(in.data, in.count);
        p.stopRecording(track);

        test::Buffers out1;
        p.process(out1.data, out1.count);

        const auto firstEnergy = out1.energy();

        p.startRecording(track, false);
        p.process(in.data, in.count);
        p.stopRecording(track, false);

        test::Buffers out2;
        p.process(out2.data, out2.count);

        REQUIRE(out2.energy() > firstEnergy);
    }

    SECTION("Playback remains stable over multiple loop cycles") {
        constexpr int track = 0;

        test::Buffers in;
        in.fill(0.5f);

        p.startRecording(track);
        p.process(in.data, in.count);
        p.stopRecording(track);

        float accumulatedEnergy = 0.0f;

        for (int i = 0; i < 8; ++i) {
            test::Buffers out;
            p.process(out.data, out.count);

            accumulatedEnergy += out.energy();
        }

        REQUIRE(accumulatedEnergy > 0.0f);
    }
}