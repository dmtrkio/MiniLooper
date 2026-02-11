#pragma once

#include <iostream>
#include <algorithm>
#include <vector>

#include <portaudio.h>

#include "audio_backend.h"

namespace audio {

    class PortAudioBackend final : public AudioBackend
    {
    public:
        explicit PortAudioBackend(Callback audioCallback) : AudioBackend(std::move(audioCallback))
        {
            if (const auto err = Pa_Initialize(); err != paNoError) {
                Pa_Terminate();
                throw std::runtime_error(Pa_GetErrorText(err));
            }
        }

        ~PortAudioBackend() override
        {
            Pa_Terminate();
        }

        bool startStream(StreamParams &params) override
        {
            if (isStreamRunning()) {
                std::cerr << "Stream is already running\n";
                return false;
            }

            const int numDevices = Pa_GetDeviceCount();
            if (numDevices <= 0) {
                std::cerr << "No Devices" << std::endl;
                return false;
            }

            for (int i = 0; i < numDevices; ++i) {
                const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);

                std::cout << "Device:" << deviceInfo->name << std::endl;
                std::cout << "	Max IN channels: " << deviceInfo->maxInputChannels << std::endl;
                std::cout << "	Max OUT channels: " << deviceInfo->maxOutputChannels << std::endl;
                std::cout << "	Default Low Input Latency: " << deviceInfo->defaultLowInputLatency << std::endl;
                std::cout << "	Default Low Output Latency: " << deviceInfo->defaultLowOutputLatency << std::endl;
                std::cout << "	Default High Input Latency: " << deviceInfo->defaultHighInputLatency << std::endl;
                std::cout << "	Default High Output Latency: " << deviceInfo->defaultHighOutputLatency << std::endl;
                std::cout << "	Default Sample rate: " << deviceInfo->defaultSampleRate << std::endl;
                std::cout << "	Host Api: " << deviceInfo->hostApi << std::endl;
            }

            constexpr PaSampleFormat sampleFormat = paFloat32;

            PaStreamParameters inputParameters;
            inputParameters.device = Pa_GetDefaultInputDevice();
            const auto* inputDeviceInfo = Pa_GetDeviceInfo(inputParameters.device);
            inputParameters.channelCount = std::min(static_cast<int>(params.numInputChannels), inputDeviceInfo->maxInputChannels);
            inputParameters.sampleFormat = sampleFormat;
            inputParameters.suggestedLatency = inputDeviceInfo->defaultLowInputLatency;
            inputParameters.hostApiSpecificStreamInfo = nullptr;
            std::cout << "Input device: " << inputDeviceInfo->name << std::endl;

            PaStreamParameters outputParameters;
            outputParameters.device = Pa_GetDefaultOutputDevice();
            const auto* outputDeviceInfo = Pa_GetDeviceInfo(outputParameters.device);
            outputParameters.channelCount = std::min(static_cast<int>(params.numOutputChannels), outputDeviceInfo->maxOutputChannels);
            outputParameters.sampleFormat = sampleFormat;
            outputParameters.suggestedLatency = outputDeviceInfo->defaultLowOutputLatency;
            outputParameters.hostApiSpecificStreamInfo = nullptr;
            std::cout << "Output device: " << outputDeviceInfo->name << std::endl;

            if (Pa_IsFormatSupported(&inputParameters, &outputParameters, params.sampleRate)) {
                std::cerr << "Format not supported by PortAudio\n";
                return false;
            }

            {
                const auto err = Pa_OpenStream(&stream_,
                                                      &inputParameters,
                                                      &outputParameters,
                                                      params.sampleRate,
                                                      params.bufferSize,
                                                      paNoFlag,
                                                      &paCallback,
                                                      this);

                if (err != paNoError) {
                    std::cerr << "PortAudio error opening stream: " << Pa_GetErrorText(err) << std::endl;
                    return false;
                }
            }

            if (const auto err = Pa_StartStream(stream_); err != paNoError) {
                std::cerr << "PortAudio error starting stream: " << Pa_GetErrorText(err) << std::endl;
                return false;
            }

            return true;
        }

        bool stopStream() override
        {
            if (!isStreamRunning()) {
                std::cerr << "PortAudio stream is not running\n";
                return false;
            }

            if (const auto err = Pa_StopStream(stream_); err != paNoError) {
                std::cerr << "PortAudio error stopping stream: " << Pa_GetErrorText(err) << std::endl;
                return false;
            }

            if (const auto err = Pa_CloseStream(stream_); err != paNoError) {
                std::cerr << "PortAudio error closing stream: " << Pa_GetErrorText(err) << std::endl;
                return false;
            }

            return true;
        }

        [[nodiscard]] bool isStreamRunning() const override
        {
            if (!stream_) return false;
            const auto err = Pa_IsStreamActive(stream_);
            if (err == 1) return true;
            if (err < 0) {
                std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
            }
            return false;
        }

    private:
        static int paCallback(const void *input,
                              void *output,
                              unsigned long frameCount,
                              const PaStreamCallbackTimeInfo* timeInfo,
                              PaStreamCallbackFlags statusFlags,
                              void *userData)
        {
            (void) timeInfo;
            (void) statusFlags;

            const auto *backend = static_cast<const PortAudioBackend*>(userData);

            const auto in = static_cast<const float*>(input);
            auto out = static_cast<float*>(output);

            if (!backend->audioCallback_(in, out, frameCount))
                return paAbort;

            return paContinue;
        }

        PaStream* stream_{nullptr};
    };

} // audio