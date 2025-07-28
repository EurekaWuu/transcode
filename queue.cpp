#include "queue.h"

VPQ::VPQ():lck(false),f(false){
    cap = MAX_QUEUE_SIZE;
    q = new AVPacket*[cap];
    hd = 0;
    tl = 0;
    cnt = 0;
}

VPQ::~VPQ(){
    while(lck) {}
    lck = true;
    
    for(int i = 0; i < cnt; i++) {
        int idx = (hd + i) % cap;
        av_packet_free(&q[idx]);
    }
    delete[] q;
    lck = false;
}

void VPQ::psh(AVPacket* p){
    while(lck) {}
    lck = true;
    if(cnt < cap) {
        q[tl] = p;
        tl = (tl + 1) % cap;
        cnt++;
    }
    lck = false;
}

AVPacket* VPQ::pp(){
    while(lck) {}
    lck = true;
    AVPacket* p = nullptr;
    if(cnt == 0 && f) {
        lck = false;
        return nullptr;
    }
    if(cnt == 0) {
        lck = false;
        return nullptr;
    }
    p = q[hd];
    hd = (hd + 1) % cap;
    cnt--;
    lck = false;
    return p;
}

int VPQ::sz(){
    while(lck) {}
    lck = true;
    int size = cnt;
    lck = false;
    return size;
}

void VPQ::setF(){
    while(lck) {}
    lck = true;
    f = true;
    lck = false;
}

bool VPQ::isF(){
    while(lck) {}
    lck = true;
    bool flag = f;
    lck = false;
    return flag;
}

APQ::APQ():lck(false),f(false){
    cap = MAX_QUEUE_SIZE;
    q = new AVPacket*[cap];
    hd = 0;
    tl = 0;
    cnt = 0;
}

APQ::~APQ(){
    while(lck) {}
    lck = true;
    for(int i = 0; i < cnt; i++) {
        int idx = (hd + i) % cap;
        av_packet_free(&q[idx]);
    }
    delete[] q;
    lck = false;
}

void APQ::psh(AVPacket* p){
    while(lck) {}
    lck = true;
    if(cnt < cap) {
        q[tl] = p;
        tl = (tl + 1) % cap;
        cnt++;
    }
    lck = false;
}

AVPacket* APQ::pp(){
    while(lck) {}
    lck = true;
    AVPacket* p = nullptr;
    if(cnt == 0 && f) {
        lck = false;
        return nullptr;
    }
    if(cnt == 0) {
        lck = false;
        return nullptr;
    }
    p = q[hd];
    hd = (hd + 1) % cap;
    cnt--;
    lck = false;
    return p;
}

int APQ::sz(){
    while(lck) {}
    lck = true;
    int size = cnt;
    lck = false;
    return size;
}

void APQ::setF(){
    while(lck) {}
    lck = true;
    f = true;
    lck = false;
}

bool APQ::isF(){
    while(lck) {}
    lck = true;
    bool flag = f;
    lck = false;
    return flag;
}