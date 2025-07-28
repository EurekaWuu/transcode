#ifndef QUEUE_H
#define QUEUE_H

extern "C" {
#include <libavformat/avformat.h>
}

#define MAX_QUEUE_SIZE 2000

class VPQ {
public:
    VPQ();
    ~VPQ();
    void psh(AVPacket* p);
    AVPacket* pp();
    int sz();
    void setF();
    bool isF();
    
private:
    AVPacket** q;
    int hd;
    int tl;
    int cap;
    int cnt;
    volatile bool lck;
    volatile bool f;
};

class APQ {
public:
    APQ();
    ~APQ();
    void psh(AVPacket* p);
    AVPacket* pp();
    int sz();
    void setF();
    bool isF();
    
private:
    AVPacket** q;
    int hd;
    int tl;
    int cap;
    int cnt;
    volatile bool lck;
    volatile bool f;
};

#endif