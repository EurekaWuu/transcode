#include "buffer.h"
#include <cstring>

BUF::BUF(int capacity) {
    cap = capacity;
    b = new uint8_t[cap];
    sz = 0;
    r = 0;
    w = 0;
    e = false;
}

BUF::~BUF() {
    delete[] b;
}

bool BUF::psh(const uint8_t* d, int dsz) {
    if (dsz > cap) return false;
    std::unique_lock<std::mutex> lk(m);
    // 等待直到有足够空间或退出
    while (cap - sz < dsz && !e) {
        nf.wait(lk);
    }
    if (e) return false;
    // 写入数据
    if (w + dsz <= cap) {
        // 连续空间
        memcpy(b + w, d, dsz);
        w = (w + dsz) % cap;
    } else {
        // 分两段写入
        int fp = cap - w;
        memcpy(b + w, d, fp);
        memcpy(b, d + fp, dsz - fp);
        w = dsz - fp;
    }
    sz += dsz;
    ne.notify_one();
    return true;
}

bool BUF::pop(uint8_t* d, int& dsz, int msz) {
    std::unique_lock<std::mutex> lk(m);
    while (sz == 0 && !e) {
        ne.wait(lk);
    }
    
    if (sz == 0 && e) return false;
    dsz = (sz < msz) ? sz : msz;
    if (r + dsz <= cap) {
        memcpy(d, b + r, dsz);
        r = (r + dsz) % cap;
    } else {
        int fp = cap - r;
        memcpy(d, b + r, fp);
        memcpy(d + fp, b, dsz - fp);
        r = dsz - fp;
    }
    
    sz -= dsz;
    nf.notify_one();
    return true;
}

bool BUF::emp() {
    std::unique_lock<std::mutex> lk(m);
    return sz == 0;
}

bool BUF::ful() {
    std::unique_lock<std::mutex> lk(m);
    return sz == cap;
}

int BUF::gsz() {
    std::unique_lock<std::mutex> lk(m);
    return sz;
}

void BUF::setE() {
    std::unique_lock<std::mutex> lk(m);
    e = true;
    ne.notify_all();
    nf.notify_all();
} 