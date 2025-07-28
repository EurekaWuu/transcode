#ifndef BUFFER_H
#define BUFFER_H

#include <mutex>
#include <condition_variable>
#include <cstdint>

class BUF {
private:
    uint8_t* b;     // 缓冲区数据
    int cap;        // 容量
    int sz;         // 大小
    int r;          // 读位置
    int w;          // 写位置
    std::mutex m;   // 互斥锁
    std::condition_variable ne; // 非空
    std::condition_variable nf; // 非满
    bool e;         // 退出

public:
    BUF(int capacity);
    ~BUF();
    
    bool psh(const uint8_t* d, int dsz);  
    bool pop(uint8_t* d, int& dsz, int msz); 
    bool emp(); 
    bool ful(); 
    int gsz(); 
    void setE(); 
};

#endif