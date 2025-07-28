#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H

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

typedef AVCodecParameters AVCoPara;

class VDec {
public:
    VDec();
    ~VDec();
    bool i(AVCoPara* p, int mf = 100);
    bool s(VPQ* q, const std::string& o);
    void st();
    int gpp() const;
    int gpf() const;

private:
    void dt(VPQ* q, const std::string& o); 
    void sf(AVFrame* f, std::ofstream& o);
    
    AVCodecContext* ctx;
    AVFrame* frm;
    std::thread t;
    std::atomic<bool> run;
    SwsContext* sws;
    
    std::atomic<int> pkts;
    std::atomic<int> frms;
    int maxf;
};

#endif