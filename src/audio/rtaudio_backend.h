#pragma once

#include <iostream>
#include <algorithm>
#include <vector>

#include <RtAudio.h>

#include "audio_backend.h"

namespace audio {

    class RtAudioBackend final : public AudioBackend
    {
    public:
        explicit RtAudioBackend(Callback audioCallback) : AudioBackend(std::move(audioCallback))
        {
#ifdef WIN32
            // Currently only WASAPI works on Windows, ASIO fails to initialize.
            rtAudio_ = std::make_unique<RtAudio>(RtAudio::Api::WINDOWS_WASAPI);
#else
            rtAudio_ = std::make_unique<RtAudio>();
#endif

            if (rtAudio_ == nullptr) {
                throw std::runtime_error("Failed to create RtAudio");
            }

            rtAudio_->showWarnings(true);

            printApis();
            printDevices();
        }

        ~RtAudioBackend() override
        {
            rtAudio_ = nullptr;
        }

        bool startStream(StreamParams &params) override
        {
            if (isStreamRunning()) {
                std::cerr << "RtAudio stream is already running\n";
                return false;
            }

            RtAudio::StreamParameters inputParameters;
            inputParameters.deviceId = rtAudio_->getDefaultInputDevice();
            const auto inputDeviceInfo = rtAudio_->getDeviceInfo(inputParameters.deviceId);
            inputParameters.nChannels = std::min(params.numInputChannels, inputDeviceInfo.inputChannels);

            RtAudio::StreamParameters outputParameters;
            outputParameters.deviceId = rtAudio_->getDefaultOutputDevice();
            const auto outputDeviceInfo = rtAudio_->getDeviceInfo(outputParameters.deviceId);
            outputParameters.nChannels = std::min(params.numOutputChannels, outputDeviceInfo.outputChannels);

            RtAudio::StreamOptions options;
            options.flags = RTAUDIO_SCHEDULE_REALTIME | RTAUDIO_NONINTERLEAVED;

            const auto err = rtAudio_->openStream(&inputParameters, &outputParameters,
                RTAUDIO_FLOAT32, params.sampleRate, &params.bufferSize, &rtCallback, this, &options);

            if (err != RTAUDIO_NO_ERROR) {
                std::cerr << "RtAudio open stream error: " << rtAudio_->getErrorText() << std::endl;
                return false;
            }

            params.sampleRate = rtAudio_->getStreamSampleRate();
            params.numInputChannels = inputParameters.nChannels;
            params.numOutputChannels = outputParameters.nChannels;

            std::cout << "Input device id = " << inputParameters.deviceId << std::endl;
            std::cout << "Output device id = " << outputParameters.deviceId << std::endl;

            inputBuffers_.resize(params.numInputChannels);
            outputBuffers_.resize(params.numOutputChannels);

            return true;
        }

        bool stopStream() override
        {
            if (!rtAudio_->isStreamOpen()) {
                std::cerr << "RtAudio stream is not open\n";
                return false;
            }

            if (!rtAudio_->isStreamRunning()) {
                std::cerr << "RtAudio stream is not running\n";
                return false;
            }

            if (rtAudio_->stopStream() != RTAUDIO_NO_ERROR) {
                std::cerr << "RtAudio stop stream error: " << rtAudio_->getErrorText() << std::endl;
                return false;
            }

            rtAudio_->closeStream();

            return true;
        }

        [[nodiscard]] bool isStreamRunning() const override
        {
            return rtAudio_->isStreamRunning();
        }

    private:
        void printApis() const
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

        void printDevices() const
        {
            const auto ids = rtAudio_->getDeviceIds();
            if (ids.empty()) {
                std::cerr << "No devices found." << std::endl;
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
        }

        static int rtCallback(void* outputBuffer,
                              void* inputBuffer,
                              unsigned int nFrames,
                              double streamTime,
                              unsigned int status,
                              void* userData)
        {
            (void)streamTime;
            (void)status;

            auto *backend = static_cast<RtAudioBackend*>(userData);

            const auto iChannels = backend->inputBuffers_.size();
            const auto oChannels = backend->outputBuffers_.size();

            const auto* in = static_cast<const float*>(inputBuffer);
            auto* out = static_cast<float*>(outputBuffer);

            for (auto i{0u}; i < iChannels; ++i)
                backend->inputBuffers_[i] = in + i * nFrames;

            for (auto i{0u}; i < oChannels; ++i)
                backend->outputBuffers_[i] = out + i * nFrames;

            return backend->audioCallback_(backend->inputBuffers_.data(), backend->outputBuffers_.data(), nFrames);
        }

        std::vector<const float*> inputBuffers_;
        std::vector<float*> outputBuffers_;
        std::unique_ptr<RtAudio> rtAudio_;
    };

}
