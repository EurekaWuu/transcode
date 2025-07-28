#include "audio_filter.h"
#include <iostream>
#include <cstring>

AudioFilter::AudioFilter() {
    soundtouch = nullptr;
    speed = 1.0f;
}

AudioFilter::~AudioFilter() {
    close();
}

bool AudioFilter::init(int channels, int sampleRate) {
    try {
        soundtouch = new soundtouch::SoundTouch();
        soundtouch->setSampleRate(sampleRate);
        soundtouch->setChannels(channels);
        soundtouch->setTempo(speed);
        soundtouch->setPitch(1.0);
        this->channels = channels;
        this->sampleRate = sampleRate;
        return true;
    } catch (std::exception& e) {
        std::cerr << "SoundTouch初始化错误: " << e.what() << std::endl;
        return false;
    }
}

void AudioFilter::setSpeed(float newSpeed) {
    if (newSpeed != speed && soundtouch) {
        speed = newSpeed;
        soundtouch->setTempo(speed);
    }
}

int AudioFilter::process(const float* inBuffer, float* outBuffer, int numSamples) {
    if (!soundtouch) return 0;

    soundtouch->putSamples(inBuffer, numSamples);

    int receivedSamples = soundtouch->receiveSamples(outBuffer, numSamples);
    
    return receivedSamples;
}

int AudioFilter::process(const uint8_t* inBuffer, uint8_t* outBuffer, int inSize, int maxOutSize) {
    if (!soundtouch) return 0;

    int numSamples = inSize / (4 * channels);
    const float* inputFloat = reinterpret_cast<const float*>(inBuffer);
    float* outputFloat = reinterpret_cast<float*>(outBuffer);

    soundtouch->putSamples(inputFloat, numSamples);

    int maxOutputSamples = maxOutSize / (4 * channels);

    int receivedSamples = soundtouch->receiveSamples(outputFloat, maxOutputSamples);

    return receivedSamples * 4 * channels;
}

void AudioFilter::flush(uint8_t* outBuffer, int& outSize, int maxOutSize) {
    if (!soundtouch) {
        outSize = 0;
        return;
    }
    
    soundtouch->flush();
    
    float* outputFloat = reinterpret_cast<float*>(outBuffer);
    int maxOutputSamples = maxOutSize / (4 * channels);
    
    int receivedSamples = soundtouch->receiveSamples(outputFloat, maxOutputSamples);
    outSize = receivedSamples * 4 * channels;
}

void AudioFilter::close() {
    if (soundtouch) {
        delete soundtouch;
        soundtouch = nullptr;
    }
} 