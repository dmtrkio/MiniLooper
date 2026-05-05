#include "audio_engine.h"

#include <iostream>
#include <memory>
#include <cassert>

#include "portaudio_backend.h"

namespace audio {
    using DefaultAudioBackend = audio::PortAudioBackend;

    AudioEngine& AudioEngine::getInstance()
    {
        static AudioEngine instance;
        return instance;
    }

    AudioEngine::AudioEngine()
    {
        auto audioCallback = [this](const float *in, float *out, unsigned int nFrames) -> bool {
            return this->callback(in, out, nFrames);
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

    json AudioEngine::getSettingsAsJson() const
    {
        auto audioDeviceToJson = [](const audio::AudioDevice &device) -> json {
            return {
                { "deviceIndex", device.deviceIndex },
                { "deviceName", device.deviceName },
                { "hostApi", device.hostApiName }
            };
        };

        json j;

        if (const auto *inputDevice = getInputAudioDeviceByIndex(getCurrentInputDevice())) {
            j["inputDevice"] = audioDeviceToJson(*inputDevice);
        }

        if (const auto *outputDevice = getOutputAudioDeviceByIndex(getCurrentOutputDevice())) {
            j["outputDevice"] = audioDeviceToJson(*outputDevice);
        }

        j["sampleRate"] = getSampleRate();
        j["bufferSize"] = getBufferSize();

        return j;
    }

    void AudioEngine::loadSettingsFromJson(const json& j)
    {
        if (j.contains("inputDevice") && j["inputDevice"].is_object()) {
            const auto& inputObj = j["inputDevice"];
            if (inputObj.contains("deviceIndex") && inputObj["deviceIndex"].is_number()) {
                const auto index = inputObj["deviceIndex"].get<audio::DeviceIndex>();
                setInputDevice(index);
            }
        }

        if (j.contains("outputDevice") && j["outputDevice"].is_object()) {
            const auto& outputObj = j["outputDevice"];
            if (outputObj.contains("deviceIndex") && outputObj["deviceIndex"].is_number()) {
                const auto index = outputObj["deviceIndex"].get<audio::DeviceIndex>();
                setOutputDevice(index);
            }
        }

        if (j.contains("sampleRate") && j["sampleRate"].is_number()) {
            const auto rate = j["sampleRate"].get<unsigned int>();
            setSampleRate(rate);
        }

        if (j.contains("bufferSize") && j["bufferSize"].is_number()) {
            const auto size = j["bufferSize"].get<unsigned int>();
            setBufferSize(size);
        }
    }

    unsigned int AudioEngine::getNumInputChannels() const noexcept { return inputChannels_; }
    unsigned int AudioEngine::getNumOutputChannels() const noexcept { return outputChannels_; }
    unsigned int AudioEngine::getSampleRate() const noexcept { return sampleRate_.load(std::memory_order_relaxed); }
    unsigned int AudioEngine::getBufferSize() const noexcept { return bufferSize_.load(std::memory_order_relaxed); }

    void AudioEngine::setSampleRate(unsigned int sampleRate)
    {
        sampleRate_.store(sampleRate, std::memory_order_relaxed);
    }

    void AudioEngine::setBufferSize(unsigned int bufferSize)
    {
        bufferSize_.store(bufferSize, std::memory_order_relaxed);
    }

    void AudioEngine::setAudioCallback(std::shared_ptr<AudioCallback> cb)
    {
        if ((!cb) || (cb == userCallback_.load(std::memory_order_relaxed)))
            return;

        if (isRunning())
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

        sampleRate_.store(params.sampleRate, std::memory_order_relaxed);
        bufferSize_.store(params.bufferSize, std::memory_order_relaxed);

        inputChannels_ = params.numInputChannels;
        outputChannels_ = params.numOutputChannels;

        inputData_.setNumChannels(inputChannels_);
        outputData_.setNumChannels(outputChannels_);

        if (const auto cb = userCallback_.load(std::memory_order_relaxed))
            cb->onStart();

        if (!backend_->startStream(inputDeviceIndex_, outputDeviceIndex_, params)) {
            std::cerr << "Error starting stream\n";
            return false;
        }

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

        return true;
    }

    bool AudioEngine::restart()
    {
        return stop() && start();
    }

    bool AudioEngine::isRunning() const
    {
        std::lock_guard<std::mutex> lock(streamMutex_);
        return backend_->isStreamRunning();
    }

    void AudioEngine::rescanDevices()
    {
        stop();

        inputDevices_.clear();
        outputDevices_.clear();

        const auto devices = backend_->getAvailableDevices();
        for (const auto& device : devices) {
            if (device.maxInputChannels > 0) {
                inputDevices_.push_back(device);
            }
            if (device.maxOutputChannels > 0) {
                outputDevices_.push_back(device);
            }
        }
    }

    const std::vector<AudioDevice>& AudioEngine::getInputDevices() const
    {
        return inputDevices_;
    }

    const std::vector<AudioDevice>& AudioEngine::getOutputDevices() const
    {
        return outputDevices_;
    }

    DeviceIndex AudioEngine::getCurrentInputDevice() const
    {
        return inputDeviceIndex_;
    }

    DeviceIndex AudioEngine::getCurrentOutputDevice() const
    {
        return outputDeviceIndex_;
    }

    void AudioEngine::setInputDevice(int inputDeviceIndex)
    {
        inputDeviceIndex_ = inputDeviceIndex;
    }

    void AudioEngine::setOutputDevice(int outputDeviceIndex)
    {
        outputDeviceIndex_ = outputDeviceIndex;
    }

    const AudioDevice* AudioEngine::getInputAudioDeviceByIndex(const DeviceIndex inputDeviceIndex) const
    {
        const auto it = std::ranges::find_if(inputDevices_, [inputDeviceIndex](const auto& device) {
            return device.deviceIndex == inputDeviceIndex;
        });

        if (it == inputDevices_.end()) return nullptr;
        return &(*it);
    }

    const AudioDevice* AudioEngine::getOutputAudioDeviceByIndex(const DeviceIndex outputDeviceIndex) const
    {
        const auto it = std::ranges::find_if(outputDevices_, [outputDeviceIndex](const auto& device) {
            return device.deviceIndex == outputDeviceIndex;
        });

        if (it == outputDevices_.end()) return nullptr;
        return &(*it);
    }

    void AudioEngine::pickDevices()
    {
        const auto devices = backend_->getAvailableDevices();
        if (devices.empty()) {
            std::cerr << "No devices available\n";
            return;
        }

        std::cout << "Available devices" << std::endl;
        for (const auto& device : devices) {
            device.printInfo();
        }

        std::cout << "Enter Input Device Index" << std::endl;
        int inputDeviceIndex = -1;
        std::cin >> inputDeviceIndex;

        std::cout << "Enter Output Device Index" << std::endl;
        int outputDeviceIndex = -1;
        std::cin >> outputDeviceIndex;

        inputDeviceIndex_ = inputDeviceIndex;
        outputDeviceIndex_ = outputDeviceIndex;
    }

    bool AudioEngine::callback(const float *in, float *out, unsigned int nFrames)
    {
        if (const auto cb = userCallback_.load(std::memory_order_relaxed)) {
            inputData_.deinterleave(in, nFrames);
            cb->onProcess(inputData_.planar.data(), outputData_.planar.data(), nFrames);
            outputData_.interleave(out, nFrames);
        }

        return true;
    }

    void AudioEngine::PlanarAudioData::setNumChannels(unsigned int numChannels)
    {
        planar.clear();
        buffers.clear();

        planar.resize(numChannels);
        buffers.resize(numChannels);

        for (auto i{0u}; i < numChannels; ++i) {
            buffers[i].resize(kMaxFramesInBuffer);
            planar[i] = buffers[i].data();
        }
    }

    void AudioEngine::PlanarAudioData::deinterleave(const float *data, unsigned int nFrames)
    {
        assert(nFrames <= kMaxFramesInBuffer);

        const auto nChannels = buffers.size();
        for (auto c{0u}; c < nChannels; ++c) {
            auto& channel = buffers[c];
            for (auto i{0u}; i < nFrames; ++i) {
                channel[i] = data[i * nChannels + c];
            }
        }
    }

    void AudioEngine::PlanarAudioData::interleave(float *data, unsigned int nFrames)
    {
        assert(nFrames <= kMaxFramesInBuffer);

        const auto nChannels = buffers.size();
        for (auto c{0u}; c < nChannels; ++c) {
            auto& channel = buffers[c];
            for (auto i{0u}; i < nFrames; ++i) {
                data[i * nChannels + c] = channel[i];
                channel[i] = 0.0f;
            }
        }
    }
}