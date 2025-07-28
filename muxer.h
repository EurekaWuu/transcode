#ifndef MUXER_H
#define MUXER_H

#include <string>
#include <thread>
#include <atomic>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
#include "queue.h"

class Muxer {
private:
    AVFormatContext* ctx;
    AVStream* stream;
    std::thread t;
    std::atomic<bool> r;
    std::atomic<int> pp;
    std::string outf;
    VPQ* q;
    int width;
    int height;
    int fps;

public:
    Muxer();
    ~Muxer();
    bool i(int w, int h, int fps, const std::string& out);
    bool s(VPQ* q);
    void stop();
    int gpp() const;
};

#endif