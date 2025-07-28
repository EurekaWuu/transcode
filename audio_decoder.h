#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

#include <string>
#include <thread>
#include <atomic>
#include <fstream>
#include "queue.h"

typedef AVCodecParameters AVCoPara;

class ADec {
public:
    ADec();
    ~ADec();
    bool i(AVCoPara* p, int mf = 100);
    bool s(APQ* q, const std::string& o);
    void stop();
    int gpp() const;
    int gpf() const;

private:
    void dt(APQ* q, const std::string& o);
    void sf(AVFrame* f, std::ofstream& o);
    AVCodecContext* ctx;
    AVFrame* frm;
    SwrContext* swr;
    std::thread t;
    std::atomic<bool> run;
    
    int sr;
    int ch;
    uint64_t cl;
    AVSampleFormat fmt;
    
    std::atomic<int> pkts;
    std::atomic<int> frms;
    int maxf;
};

#endif