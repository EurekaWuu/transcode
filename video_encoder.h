#ifndef VIDEO_ENCODER_H
#define VIDEO_ENCODER_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

#include <string>
#include <thread>
#include <atomic>
#include <fstream>
#include "queue.h"

class VEnc {
public:
    VEnc();
    ~VEnc();
    bool i(int w, int h, int fps = 25);
    bool s(const std::string& in, VPQ* q);
    void st();
    int gpf() const;
    int gpp() const;

private:
    void et(const std::string& in, VPQ* q);
    AVCodecContext* ctx;
    AVFrame* frm;
    AVPacket* pkt;
    std::thread t;
    std::atomic<bool> run;
    std::atomic<int> frms;
    std::atomic<int> pkts;
    int width;
    int height;
    std::string inf;
    VPQ* q;
};

#endif 