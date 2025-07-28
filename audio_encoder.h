#ifndef AUDIO_ENCODER_H
#define AUDIO_ENCODER_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/audio_fifo.h>
}

class AE {
private:
    AVCodecContext* ctx; // 编码器
    AVFrame* frm;        // 音频帧
    AVPacket* pkt;       // 音频包
    AVAudioFifo* fifo;   // 音频队列
    int fsz;            // 帧大小
    int sr;             // 采样率
    int ch;             // 通道数
    AVSampleFormat fmt;  // 采样格式

public:
    AE();
    ~AE();
    bool init(int sr, int ch, AVSampleFormat fmt);
    bool enc(AVFrame* f, AVPacket** op);
    bool encPcm(const uint8_t* d, int sz, AVPacket** op);
    void cls();
};

#endif 