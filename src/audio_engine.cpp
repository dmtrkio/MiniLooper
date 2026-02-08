#include "audio_engine.h"

#include <iostream>

AudioEngine& AudioEngine::getInstance()
{
    static AudioEngine instance;
    return instance;
}

AudioEngine::AudioEngine()
{
    rtAudio_ = std::make_unique<RtAudio>();
    inputDevice_ = rtAudio_->getDefaultInputDevice();
    outputDevice_ = rtAudio_->getDefaultOutputDevice();
}

AudioEngine::~AudioEngine()
{
    stopStream();
}

unsigned int AudioEngine::getNumInputChannels() const noexcept { return inputChannels_; }
unsigned int AudioEngine::getNumOutputChannels() const noexcept { return outputChannels_; }
unsigned int AudioEngine::getSampleRate() const noexcept { return sampleRate_; }
unsigned int AudioEngine::getBufferSize() const noexcept { return bufferSize_; }

void AudioEngine::setSampleRate(unsigned int sampleRate)
{
    sampleRate_.store(sampleRate, std::memory_order_release);
    if (isStreamRunning())
        restartStream();
}

void AudioEngine::setBufferSize(unsigned int bufferSize)
{
    bufferSize_.store(bufferSize, std::memory_order_release);
    if (isStreamRunning())
        restartStream();
}

void AudioEngine::setAudioCallback(std::shared_ptr<AudioCallback> cb)
{
    if ((!cb) || (cb == userCallback_.load(std::memory_order_acquire)))
        return;

    if (isStreamRunning())
        cb->onStart();

    userCallback_.store(cb, std::memory_order_release);
}

bool AudioEngine::startStream()
{
    return openStream();
}

void AudioEngine::stopStream()
{
    closeStream();
}

bool AudioEngine::isStreamRunning() const
{
    return streamRunning_.load();
}

bool AudioEngine::isStreamOpen() const
{
    return rtAudio_->isStreamOpen();
}

bool AudioEngine::openStream()
{
    std::lock_guard<std::mutex> lock(streamMutex_);

    if (isStreamOpen()) {
        std::cerr << "Stream already open\n";
        return false;
    }

    RtAudio::StreamParameters iParams, oParams;

    iParams.deviceId = inputDevice_;
    iParams.nChannels = inputChannels_;
    iParams.firstChannel = 0;

    oParams.deviceId = outputDevice_;
    oParams.nChannels = outputChannels_;
    oParams.firstChannel = 0;

    RtAudio::StreamOptions options;
    options.flags = RTAUDIO_SCHEDULE_REALTIME | RTAUDIO_NONINTERLEAVED;

    inputBuffers_.resize(inputChannels_);
    outputBuffers_.resize(outputChannels_);

    unsigned int bufSize = bufferSize_.load(std::memory_order_acquire);

    if (rtAudio_->openStream(&oParams,
                             &iParams,
                             RTAUDIO_FLOAT32,
                             sampleRate_.load(std::memory_order_acquire),
                             &bufSize,
                             &AudioEngine::staticCallback,
                             this,
                             &options)) {
        std::cerr << "RtAudio open stream error: " << rtAudio_->getErrorText() << std::endl;
        return false;
    }

    if (auto cb = userCallback_.load(std::memory_order_acquire))
        cb->onStart();

    if (rtAudio_->startStream()) {
        std::cerr << "RtAudio start stream error: " << rtAudio_->getErrorText() << std::endl;
        return false;
    }

    streamRunning_.store(true);
    return true;
}

void AudioEngine::closeStream()
{
    std::lock_guard<std::mutex> lock(streamMutex_);

    if (rtAudio_->isStreamOpen()) {
        if (rtAudio_->isStreamRunning()) {
            if (rtAudio_->stopStream()) {
                std::cerr << "RtAudio stop stream error: " << rtAudio_->getErrorText() << std::endl;
                return;
            }
            if (auto cb = userCallback_.load(std::memory_order_acquire))
                cb->onStop();
        }
        rtAudio_->closeStream();
    }

    streamRunning_.store(false, std::memory_order_release);
}

void AudioEngine::restartStream()
{
    closeStream();
    (void)openStream();
}

int AudioEngine::staticCallback(void* outputBuffer,
                                void* inputBuffer,
                                unsigned int nFrames,
                                double streamTime,
                                RtAudioStreamStatus status,
                                void* userData)
{

    (void)streamTime;
    (void)status;

    if (auto* engine = static_cast<AudioEngine*>(userData)) {
        if (auto cb = engine->userCallback_.load(std::memory_order_acquire)) {
            const auto iChannels = engine->getNumInputChannels();
            const auto oChannels = engine->getNumOutputChannels();

            const float* in = static_cast<const float*>(inputBuffer);
            float* out = static_cast<float*>(outputBuffer);

            for (auto i{0u}; i < iChannels; ++i)
                engine->inputBuffers_[i] = in + i * nFrames;

            for (auto i{0u}; i < oChannels; ++i)
                engine->outputBuffers_[i] = out + i * nFrames;

            cb->onProcess(engine->inputBuffers_.data(),
                          engine->outputBuffers_.data(),
                          nFrames);
        }
    }

    return 0;
}

