#pragma once

#include <iostream>
#include <algorithm>
#include <utility>

#include <portaudio.h>

#ifdef WIN32
    #include <pa_win_wasapi.h>
#endif

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

            scanDevices();
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

            PaDeviceIndex inputDevice = paNoDevice;
            PaDeviceIndex outputDevice = paNoDevice;

            if (!pickDevices(inputDevice, outputDevice, params)) {
                inputDevice = Pa_GetDefaultInputDevice();
                outputDevice = Pa_GetDefaultOutputDevice();
                std::cerr << "Default devices picked\n";
            }

            constexpr PaSampleFormat sampleFormat = paFloat32;

            PaStreamParameters inputParameters;
            inputParameters.device = inputDevice;
            const auto* inputDeviceInfo = Pa_GetDeviceInfo(inputParameters.device);
            inputParameters.channelCount = std::min(static_cast<int>(params.numInputChannels), inputDeviceInfo->maxInputChannels);
            inputParameters.sampleFormat = sampleFormat;
            inputParameters.suggestedLatency = inputDeviceInfo->defaultLowInputLatency;
            inputParameters.hostApiSpecificStreamInfo = nullptr;
            std::cout << "Input device name: " << inputDeviceInfo->name << std::endl;
            const auto inputHostApiInfo = Pa_GetHostApiInfo(inputDeviceInfo->hostApi);
            std::cout << "Input Host Api: " << inputHostApiInfo->name << std::endl;

            PaStreamParameters outputParameters;
            outputParameters.device = outputDevice;
            const auto* outputDeviceInfo = Pa_GetDeviceInfo(outputParameters.device);
            outputParameters.channelCount = std::min(static_cast<int>(params.numOutputChannels), outputDeviceInfo->maxOutputChannels);
            outputParameters.sampleFormat = sampleFormat;
            outputParameters.suggestedLatency = outputDeviceInfo->defaultLowOutputLatency;
            outputParameters.hostApiSpecificStreamInfo = nullptr;
            std::cout << "Output device name: " << outputDeviceInfo->name << std::endl;
            const auto outputHostApiInfo = Pa_GetHostApiInfo(outputDeviceInfo->hostApi);
            std::cout << "Output Host Api: " << outputHostApiInfo->name << std::endl;

            if (Pa_IsFormatSupported(&inputParameters, &outputParameters, params.sampleRate) != paFormatIsSupported) {
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
                std::cerr << "PortAudio stream is already not running\n";
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
        static void scanDevices()
        {
            for (int i = 0; i < Pa_GetHostApiCount(); ++i) {
                const auto *hostApiInfo = Pa_GetHostApiInfo(i);
                if (!hostApiInfo) continue;

                std::cout << "Host Api: " << hostApiInfo->name << std::endl;
                std::cout << std::endl;

                for (int j = 0; j < hostApiInfo->deviceCount; ++j) {
                    const auto deviceIndex = Pa_HostApiDeviceIndexToDeviceIndex(i, j);
                    const auto deviceInfo = Pa_GetDeviceInfo(deviceIndex);

                    std::cout << "Device name:" << deviceInfo->name << std::endl;
                    std::cout << "Max input channels: " << deviceInfo->maxInputChannels << std::endl;
                    std::cout << "Max output channels: " << deviceInfo->maxOutputChannels << std::endl;
                    std::cout << "Default Low Input Latency: " << deviceInfo->defaultLowInputLatency << std::endl;
                    std::cout << "Default Low Output Latency: " << deviceInfo->defaultLowOutputLatency << std::endl;
                    std::cout << "Default High Input Latency: " << deviceInfo->defaultHighInputLatency << std::endl;
                    std::cout << "Default High Output Latency: " << deviceInfo->defaultHighOutputLatency << std::endl;
                    std::cout << "Default Sample rate: " << deviceInfo->defaultSampleRate << std::endl;

#ifdef WIN32
                    if (deviceInfo->hostApi == paWASAPI && PaWasapi_IsLoopback(i) != 0)
                        continue;
#endif

                    PaStreamParameters iParams;
                    iParams.device = deviceIndex;
                    iParams.channelCount = deviceInfo->maxInputChannels;
                    iParams.sampleFormat = paFloat32;
                    iParams.suggestedLatency = deviceInfo->defaultLowInputLatency;
                    iParams.hostApiSpecificStreamInfo = nullptr;

                    PaStreamParameters oParams;
                    oParams.device = deviceIndex;
                    oParams.channelCount = deviceInfo->maxOutputChannels;
                    oParams.sampleFormat = paFloat32;
                    oParams.suggestedLatency = deviceInfo->defaultLowOutputLatency;
                    oParams.hostApiSpecificStreamInfo = nullptr;

                    const auto ip = iParams.channelCount > 0 ? &iParams : nullptr;
                    const auto op = oParams.channelCount > 0 ? &oParams : nullptr;

                    std::vector<unsigned int> supportedSampleRates;

                    for (const auto sr : {22050, 32000, 44100, 48000, 88200, 96000, 192000}) {
                        if (const auto err = Pa_IsFormatSupported(ip, op, sr); err == paFormatIsSupported) {
                            supportedSampleRates.push_back(static_cast<unsigned int>(sr));
                        /*} else {
                            std::cout << Pa_GetErrorText(err) << std::endl;*/
                        }
                    }

                    std::cout << "Supported Sample Rates: ";
                    for (const auto sr : supportedSampleRates) {
                        std::cout << sr << " ";
                    }

                    std::cout << std::endl << std::endl;
                }
            }

            std::cout << std::endl;
        }

        static bool pickDevices(PaDeviceIndex &inputDevice, PaDeviceIndex &outputDevice, StreamParams &params)
        {
            const auto numDevices = Pa_GetDeviceCount();
            if (numDevices <= 0) {
                std::cerr << "No Devices" << std::endl;
                return false;
            }

            bool inputDeviceFound = false;
            bool outputDeviceFound = false;

            constexpr PaHostApiTypeId targetApiType = paWASAPI;
            const auto targetApiIndex = Pa_HostApiTypeIdToHostApiIndex(targetApiType);

            for (int i = 0; i < numDevices; ++i) {
                const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);

#ifdef WIN32
                if (deviceInfo->hostApi == paWASAPI && PaWasapi_IsLoopback(i) != 0)
                    continue;
#endif

                if (deviceInfo->hostApi == targetApiIndex) {
                    if (deviceInfo->maxInputChannels > 0) {
                        inputDevice = i;
                        inputDeviceFound = true;
                    }

                    if (deviceInfo->maxOutputChannels > 1) {
                        outputDevice = i;
                        outputDeviceFound = true;
                    }
                }
            }

            return inputDeviceFound && outputDeviceFound;
        }

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