#include "audio_engine.h"

#include <iostream>

#include <RtAudio.h>

using namespace audio;

AudioEngine& AudioEngine::getInstance()
{
    static AudioEngine instance;
    return instance;
}

AudioEngine::AudioEngine()
{
#ifdef WIN32
    // Currently only WASAPI works on Windows, ASIO fails to initialize.
    rtAudio_ = std::make_unique<RtAudio>(RtAudio::Api::WINDOWS_WASAPI);
#else
    rtAudio_ = std::make_unique<RtAudio>();
#endif

    inputDevice_ = rtAudio_->getDefaultInputDevice();
    outputDevice_ = rtAudio_->getDefaultOutputDevice();

    rtAudio_->showWarnings(true);

    inputChannels_ = std::min(rtAudio_->getDeviceInfo(inputDevice_).inputChannels, 2u);
    outputChannels_ = std::min(rtAudio_->getDeviceInfo(outputDevice_).outputChannels, 2u);

    printApis();
    printDevices();
}

AudioEngine::~AudioEngine()
{
    stopStream();
}

void AudioEngine::printApis() const
{
    std::vector<RtAudio::Api> compiledApis;
    RtAudio::getCompiledApi(compiledApis);
    std::cout << std::endl;
    for (const auto& capi : compiledApis) {
        std::cout << "RtAudio compiled api = " << RtAudio::getApiDisplayName(capi) << std::endl;
    }

    const auto currentApi = rtAudio_->getCurrentApi();
    std::cout << "Current api = " << RtAudio::getApiDisplayName(currentApi) << std::endl;
    std::cout << std::endl;
}

void AudioEngine::printDevices() const
{
    const auto ids = rtAudio_->getDeviceIds();
    if (ids.empty()) {
        std::cout << "No devices found." << std::endl;
        return;
    }

    for (const auto id : ids) {
        const auto info = rtAudio_->getDeviceInfo(id);
        std::cout << "device id = " << id << std::endl;
        std::cout << "device name = " << info.name << std::endl;
        std::cout << "maximum input channels = " << info.inputChannels << std::endl;
        std::cout << "maximum output channels = " << info.outputChannels << std::endl;
        std::cout << "supports duplex channels = " << (info.duplexChannels ? "true" : "false") << std::endl;

        if (info.nativeFormats == 0) {
            std::cout << "No natively supported data formats\n";
        } else {
            std::cout << "Natively supported data formats:\n";
            if (info.nativeFormats & RTAUDIO_SINT8)
                std::cout << "  8-bit int\n";
            if ( info.nativeFormats & RTAUDIO_SINT16 )
                std::cout << "  16-bit int\n";
            if ( info.nativeFormats & RTAUDIO_SINT24 )
                std::cout << "  24-bit int\n";
            if ( info.nativeFormats & RTAUDIO_SINT32 )
                std::cout << "  32-bit int\n";
            if ( info.nativeFormats & RTAUDIO_FLOAT32 )
                std::cout << "  32-bit float\n";
            if ( info.nativeFormats & RTAUDIO_FLOAT64 )
                std::cout << "  64-bit float\n";
        }
        std::cout << std::endl;
    }

    std::cout << "picked input device id = " << inputDevice_ << std::endl;
    std::cout << "picked output device id = " << outputDevice_ << std::endl;
    std::cout << std::endl;
}

unsigned int AudioEngine::getNumInputChannels() const noexcept { return inputChannels_; }
unsigned int AudioEngine::getNumOutputChannels() const noexcept { return outputChannels_; }
unsigned int AudioEngine::getSampleRate() const noexcept { return sampleRate_.load(std::memory_order_relaxed); }
unsigned int AudioEngine::getBufferSize() const noexcept { return bufferSize_.load(std::memory_order_relaxed); }

void AudioEngine::setSampleRate(unsigned int sampleRate)
{
    sampleRate_.store(sampleRate, std::memory_order_relaxed);
    if (isStreamRunning())
        restartStream();
}

void AudioEngine::setBufferSize(unsigned int bufferSize)
{
    bufferSize_.store(bufferSize, std::memory_order_relaxed);
    if (isStreamRunning())
        restartStream();
}

void AudioEngine::setAudioCallback(std::shared_ptr<AudioCallback> cb)
{
    if ((!cb) || (cb == userCallback_.load(std::memory_order_relaxed)))
        return;

    if (isStreamRunning())
        cb->onStart();

    userCallback_.store(cb, std::memory_order_relaxed);
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
    options.flags = RTAUDIO_SCHEDULE_REALTIME | RTAUDIO_NONINTERLEAVED | RTAUDIO_HOG_DEVICE;

    inputBuffers_.resize(inputChannels_);
    outputBuffers_.resize(outputChannels_);

    unsigned int bufSize = bufferSize_.load(std::memory_order_relaxed);

    if (rtAudio_->openStream(&oParams,
                             &iParams,
                             RTAUDIO_FLOAT32,
                             sampleRate_.load(std::memory_order_relaxed),
                             &bufSize,
                             &AudioEngine::staticCallback,
                             this,
                             &options)) {
        std::cerr << "RtAudio open stream error: " << rtAudio_->getErrorText() << std::endl;
        return false;
    }

    sampleRate_.store(rtAudio_->getStreamSampleRate());
    bufferSize_.store(bufSize, std::memory_order_relaxed);

    if (auto cb = userCallback_.load(std::memory_order_relaxed))
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
            if (auto cb = userCallback_.load(std::memory_order_relaxed))
                cb->onStop();
        }
        rtAudio_->closeStream();
    }

    streamRunning_.store(false, std::memory_order_relaxed);
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
                                unsigned int status,
                                void* userData)
{

    (void)streamTime;
    (void)status;

    if (auto* engine = static_cast<AudioEngine*>(userData)) {
        if (auto cb = engine->userCallback_.load(std::memory_order_relaxed)) {
            const auto iChannels = engine->getNumInputChannels();
            const auto oChannels = engine->getNumOutputChannels();

            const auto* in = static_cast<const float*>(inputBuffer);
            auto* out = static_cast<float*>(outputBuffer);

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

