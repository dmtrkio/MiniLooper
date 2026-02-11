#include "audio_engine.h"

#include <iostream>
#include <memory>


#include "portaudio_backend.h"
using DefaultAudioBackend = audio::PortAudioBackend;

using namespace audio;

AudioEngine& AudioEngine::getInstance()
{
    static AudioEngine instance;
    return instance;
}

AudioEngine::AudioEngine()
{
    AudioBackend::Callback audioCallback = [&](const float *const *in, float *const *out, unsigned int nFrames) -> bool {
        /*for (unsigned int i = 0; i < nFrames; ++i) {
            std::cout << in[0][i] << '\n';
        }*/

        if (const auto cb = userCallback_.load(std::memory_order_relaxed)) {
            cb->onProcess(in, out, nFrames);
        }
        return true;
    };

    try {
        backend_ = std::make_unique<DefaultAudioBackend>(std::move(audioCallback));
    } catch (std::exception &e) {
        std::cerr << "Error creating audio backend: " << e.what() << std::endl;
    }
}

AudioEngine::~AudioEngine()
{
    backend_ = nullptr;
}

unsigned int AudioEngine::getNumInputChannels() const noexcept { return inputChannels_; }
unsigned int AudioEngine::getNumOutputChannels() const noexcept { return outputChannels_; }
unsigned int AudioEngine::getSampleRate() const noexcept { return sampleRate_.load(std::memory_order_relaxed); }
unsigned int AudioEngine::getBufferSize() const noexcept { return bufferSize_.load(std::memory_order_relaxed); }

void AudioEngine::setSampleRate(unsigned int sampleRate)
{
    sampleRate_.store(sampleRate, std::memory_order_relaxed);
    if (isStreamRunning())
        restart();
}

void AudioEngine::setBufferSize(unsigned int bufferSize)
{
    bufferSize_.store(bufferSize, std::memory_order_relaxed);
    if (isStreamRunning())
        restart();
}

void AudioEngine::setAudioCallback(std::shared_ptr<AudioCallback> cb)
{
    if ((!cb) || (cb == userCallback_.load(std::memory_order_relaxed)))
        return;

    if (isStreamRunning())
        cb->onStart();

    userCallback_.store(cb, std::memory_order_relaxed);
}

bool AudioEngine::start()
{
    std::lock_guard<std::mutex> lock(streamMutex_);

    AudioBackend::StreamParams params;
    params.sampleRate = sampleRate_.load(std::memory_order_relaxed);
    params.bufferSize = bufferSize_.load(std::memory_order_relaxed);
    params.numInputChannels = inputChannels_;
    params.numOutputChannels = outputChannels_;

    if (!backend_->startStream(params)) {
        std::cerr << "Error starting stream\n";
        return false;
    }

    sampleRate_.store(params.sampleRate, std::memory_order_relaxed);
    bufferSize_.store(params.bufferSize, std::memory_order_relaxed);
    inputChannels_ = params.numInputChannels;
    outputChannels_ = params.numOutputChannels;

    streamRunning_.store(true, std::memory_order_relaxed);
    return true;
}

bool AudioEngine::stop()
{
    std::lock_guard<std::mutex> lock(streamMutex_);

    if (!backend_->stopStream()) {
        std::cerr << "Error closing stream\n";
        return false;
    }

    if (const auto cb = userCallback_.load(std::memory_order_relaxed))
        cb->onStop();

    streamRunning_.store(false, std::memory_order_relaxed);
    return true;
}

bool AudioEngine::restart()
{
    return stop() && start();
}

bool AudioEngine::isStreamRunning() const
{
    std::lock_guard<std::mutex> lock(streamMutex_);
    return backend_->isStreamRunning();
}