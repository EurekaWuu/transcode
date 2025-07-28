#ifndef AUDIO_FILTER_H
#define AUDIO_FILTER_H

#include <soundtouch/SoundTouch.h>
#include <cstdint>

class AudioFilter {
private:
    soundtouch::SoundTouch* soundtouch;
    float speed;
    int channels;
    int sampleRate;

public:
    AudioFilter();
    ~AudioFilter();
    
    // 初始化音频过滤器
    bool init(int channels, int sampleRate);
    
    // 设置播放速度
    void setSpeed(float newSpeed);
    
    // 处理音频数据 (浮点格式)
    int process(const float* inBuffer, float* outBuffer, int numSamples);
    
    // 处理音频数据 (字节格式)
    int process(const uint8_t* inBuffer, uint8_t* outBuffer, int inSize, int maxOutSize);
    
    // 清空缓冲区
    void flush(uint8_t* outBuffer, int& outSize, int maxOutSize);
    
    // 关闭并释放资源
    void close();
};

#endif // AUDIO_FILTER_H 