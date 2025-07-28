#ifndef DEMUXER_H
#define DEMUXER_H

extern"C"{
#include<libavformat/avformat.h>
}

#include<string>
#include<thread>
#include<atomic>
#include"queue.h"

typedef AVCodecParameters AVCoPara;

class Dmx{
public:
  Dmx();
  ~Dmx();
  bool init(const std::string&f);
  bool start(VPQ*v,APQ*a);
  void stop();
  AVCoPara*getVCP();
  AVCoPara*getACP();
  int getVIdx();
  int getAIdx();
  double getDur();
  int getVPC();
  int getAPC();

private:
  void thr(VPQ*v,APQ*a);
  AVFormatContext*fmt;
  int vidx;
  int aidx;
  std::thread th;
  std::atomic<bool>run;
  int max;
  std::atomic<int>vpc;
  std::atomic<int>apc;
};

#endif