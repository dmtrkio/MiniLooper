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

        [[nodiscard]] std::vector<AudioDevice> getAvailableDevices() override
        {
            scanDevices();
            return devices_;
        }

        bool startStream(int inputDeviceIndex, int outputDeviceIndex, StreamParams &params) override
        {
            if (isStreamRunning()) {
                std::cerr << "Stream is already running" << std::endl;
                return false;
            }

            if (const int numDevices = Pa_GetDeviceCount(); numDevices <= 0) {
                std::cerr << "No Devices available" << std::endl;
                return false;
            }

            PaDeviceIndex inputDevice = inputDeviceIndex;
            PaDeviceIndex outputDevice = outputDeviceIndex;

            if (!validateStreamParameters(inputDevice, outputDevice, params)) {
                inputDevice = Pa_GetDefaultInputDevice();
                outputDevice = Pa_GetDefaultOutputDevice();
                std::cerr << "Failed to use given devices. Default devices picked" << std::endl;
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
                std::cerr << "Format not supported by devices used" << std::endl;
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
                std::cerr << "PortAudio stream is already not running" << std::endl;
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
        static bool validateStreamParameters(int inputDeviceIndex, int outputDeviceIndex, StreamParams &params)
        {
            const PaDeviceInfo* inputDeviceInfo = Pa_GetDeviceInfo(inputDeviceIndex);
            const PaDeviceInfo* outputDeviceInfo = Pa_GetDeviceInfo(outputDeviceIndex);

            if (inputDeviceInfo && outputDeviceInfo) {
                PaStreamParameters iParams;
                iParams.device = inputDeviceIndex;
                iParams.channelCount = static_cast<int>(params.numInputChannels);
                iParams.sampleFormat = paFloat32;
                iParams.suggestedLatency = inputDeviceInfo->defaultLowInputLatency;
                iParams.hostApiSpecificStreamInfo = nullptr;

                PaStreamParameters oParams;
                oParams.device = outputDeviceIndex;
                oParams.channelCount = static_cast<int>(params.numOutputChannels);
                oParams.sampleFormat = paFloat32;
                oParams.suggestedLatency = outputDeviceInfo->defaultLowOutputLatency;
                oParams.hostApiSpecificStreamInfo = nullptr;

                const auto ip = iParams.channelCount > 0 ? &iParams : nullptr;
                const auto op = oParams.channelCount > 0 ? &oParams : nullptr;

                if (const auto err = Pa_IsFormatSupported(ip, op, params.sampleRate); err == paFormatIsSupported) {
                    return true;
                } else {
                    std::cout << Pa_GetErrorText(err) << std::endl;
                }
            }

            return false;
        }

        void scanDevices()
        {
            for (int i = 0; i < Pa_GetHostApiCount(); ++i) {
                const auto *hostApiInfo = Pa_GetHostApiInfo(i);
                if (!hostApiInfo) continue;

                //std::cout << "Host Api: " << hostApiInfo->name << std::endl;

                for (int j = 0; j < hostApiInfo->deviceCount; ++j) {
                    const auto deviceIndex = Pa_HostApiDeviceIndexToDeviceIndex(i, j);
                    const auto deviceInfo = Pa_GetDeviceInfo(deviceIndex);

                    /*std::cout << "Device name:" << deviceInfo->name << std::endl;
                    std::cout << "Max input channels: " << deviceInfo->maxInputChannels << std::endl;
                    std::cout << "Max output channels: " << deviceInfo->maxOutputChannels << std::endl;
                    std::cout << "Default Low Input Latency: " << deviceInfo->defaultLowInputLatency << std::endl;
                    std::cout << "Default Low Output Latency: " << deviceInfo->defaultLowOutputLatency << std::endl;
                    std::cout << "Default High Input Latency: " << deviceInfo->defaultHighInputLatency << std::endl;
                    std::cout << "Default High Output Latency: " << deviceInfo->defaultHighOutputLatency << std::endl;
                    std::cout << "Default Sample rate: " << deviceInfo->defaultSampleRate << std::endl;*/

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

                    /*std::cout << "Supported Sample Rates: ";
                    for (const auto sr : supportedSampleRates) {
                        std::cout << sr << " ";
                    }

                    std::cout << std::endl << std::endl;*/

                    AudioDevice device;
                    device.deviceIndex = deviceIndex;
                    device.deviceName = deviceInfo->name;
                    device.hostApiName = hostApiInfo->name;
                    device.maxInputChannels = deviceInfo->maxInputChannels;
                    device.maxOutputChannels = deviceInfo->maxOutputChannels;
                    device.supportedSampleRates = std::move(supportedSampleRates);
                    devices_.emplace_back(device);
                }
            }
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

        std::vector<AudioDevice> devices_;
    };

} // audio