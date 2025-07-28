#include<iostream>
#include<string>
#include<thread>
#include<chrono>
#include<fstream>
#include<vector>
extern"C"{
#include<libavcodec/avcodec.h>
#include<libavformat/avformat.h>
#include<libavutil/imgutils.h>
#include<libavfilter/avfilter.h>
#include<libavfilter/buffersink.h>
#include<libavfilter/buffersrc.h>
}
#include"demuxer.h"
#include"queue.h"
#include"video_decoder.h"
#include"audio_decoder.h"
#include"video_encoder.h"
#include"gl.h"
#include"muxer.h"
#include"buffer.h"
#include"audio_encoder.h"

// 从YUV文件读取一帧数据
bool rYuvF(std::ifstream& yf, AVFrame* f, int w, int h) {
  int ys = w * h;
  int uvs = ys / 4;
  
  if(!yf.read(reinterpret_cast<char*>(f->data[0]), ys)) 
    return false;
  
  if(!yf.read(reinterpret_cast<char*>(f->data[1]), uvs)) 
    return false;
  
  if(!yf.read(reinterpret_cast<char*>(f->data[2]), uvs)) 
    return false;
  
  return true;
}

// 旋转
void pRotV(const std::string& inf, const std::string& outf, int w, int h, float ang) {
  std::ifstream yf(inf, std::ios::binary);
  if(!yf.is_open()) return;
  
  std::ofstream of(outf, std::ios::binary);
  if(!of.is_open()) {
    yf.close();
    return;
  }
  
  GL gl;
  if(!gl.i(w, h)) {
    yf.close();
    of.close();
    return;
  }
  
  AVFrame* inFrm = av_frame_alloc();
  AVFrame* outFrm = av_frame_alloc();
  
  inFrm->format = AV_PIX_FMT_YUV420P;
  inFrm->width = w;
  inFrm->height = h;
  av_frame_get_buffer(inFrm, 0);
  
  outFrm->format = AV_PIX_FMT_YUV420P;
  outFrm->width = w;
  outFrm->height = h;
  av_frame_get_buffer(outFrm, 0);
  
  int fc = 0;
  while(rYuvF(yf, inFrm, w, h)) {
    gl.p(inFrm, outFrm, ang);
    
    int ys = w * h;
    int uvs = ys / 4;
    
    of.write(reinterpret_cast<char*>(outFrm->data[0]), ys);
    of.write(reinterpret_cast<char*>(outFrm->data[1]), uvs);
    of.write(reinterpret_cast<char*>(outFrm->data[2]), uvs);
    
    fc++;
  }
  
  av_frame_free(&inFrm);
  av_frame_free(&outFrm);
  yf.close();
  of.close();
}

// 从PCM文件读取数据
bool rPcmD(std::ifstream& pf, uint8_t* buf, int& sz, int max_sz) {
  pf.read(reinterpret_cast<char*>(buf), max_sz);
  sz = pf.gcount();
  return sz > 0;
}

// atempo滤镜
bool processPcmWithFilter(const std::string& input, const std::string& output, float speed, int sample_rate, int channels, AVSampleFormat format) {


  std::ifstream pcm_file(input, std::ios::binary);
  if (!pcm_file.is_open()) {

    return false;
  }

  std::ofstream out_file(output, std::ios::binary);
  if (!out_file.is_open()) {

    pcm_file.close();
    return false;
  }

  // 初始化滤镜
  AVFilterGraph* filter_graph = avfilter_graph_alloc();
  if (!filter_graph) {
    pcm_file.close();
    out_file.close();
    return false;
  }

  char filter_desc[256];
  if (speed <= 2.0 && speed >= 0.5) {
    // 单个atempo
    snprintf(filter_desc, sizeof(filter_desc), 
             "abuffer=sample_rate=%d:sample_fmt=%s:channel_layout=%lld,atempo=%.3f,abuffersink",
             sample_rate, av_get_sample_fmt_name(format), 
             (long long)av_get_default_channel_layout(channels), speed);
  } else if (speed > 2.0) {
    // 多个atempo
    if (speed <= 4.0) {
      // 2.0 < speed <= 4.0, 使用两个atempo
      float second_tempo = speed / 2.0f;
      snprintf(filter_desc, sizeof(filter_desc), 
               "abuffer=sample_rate=%d:sample_fmt=%s:channel_layout=%lld,atempo=2.0,atempo=%.3f,abuffersink",
               sample_rate, av_get_sample_fmt_name(format), 
               (long long)av_get_default_channel_layout(channels), second_tempo);
    } else {
      // speed > 4.0, 使用三个atempo
      snprintf(filter_desc, sizeof(filter_desc), 
               "abuffer=sample_rate=%d:sample_fmt=%s:channel_layout=%lld,atempo=2.0,atempo=2.0,atempo=%.3f,abuffersink",
               sample_rate, av_get_sample_fmt_name(format), 
               (long long)av_get_default_channel_layout(channels), speed / 4.0f);
    }
  } else {
    // 速度 < 0.5，使用setpts调整
    float tempo_factor = (speed < 0.5f) ? 0.5f : speed;
    snprintf(filter_desc, sizeof(filter_desc), 
             "abuffer=sample_rate=%d:sample_fmt=%s:channel_layout=%lld,atempo=%.3f,abuffersink",
             sample_rate, av_get_sample_fmt_name(format), 
             (long long)av_get_default_channel_layout(channels), tempo_factor);
  }

  // 解析滤镜字符串
  AVFilterInOut* inputs = NULL;
  AVFilterInOut* outputs = NULL;
  int ret = avfilter_graph_parse2(filter_graph, filter_desc, &inputs, &outputs);
  if (ret < 0) {
    avfilter_graph_free(&filter_graph);
    pcm_file.close();
    out_file.close();
    return false;
  }

  // 配置滤镜图
  ret = avfilter_graph_config(filter_graph, NULL);
  if (ret < 0) {
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    avfilter_graph_free(&filter_graph);
    pcm_file.close();
    out_file.close();
    return false;
  }

  // 获取源和输出滤镜上下文
  AVFilterContext* buffer_ctx = avfilter_graph_get_filter(filter_graph, "Parsed_abuffer_0");
  AVFilterContext* buffersink_ctx = avfilter_graph_get_filter(filter_graph, "Parsed_abuffersink_0");
  
  if (!buffer_ctx || !buffersink_ctx) {
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    avfilter_graph_free(&filter_graph);
    pcm_file.close();
    out_file.close();
    return false;
  }

  // 创建帧
  AVFrame* in_frame = av_frame_alloc();
  AVFrame* out_frame = av_frame_alloc();
  if (!in_frame || !out_frame) {
    if (in_frame) av_frame_free(&in_frame);
    if (out_frame) av_frame_free(&out_frame);
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);
    avfilter_graph_free(&filter_graph);
    pcm_file.close();
    out_file.close();
    return false;
  }

  // 处理PCM数据
  const int nb_samples = 1024;
  const int bytes_per_sample = av_get_bytes_per_sample(format);
  const int buffer_size = nb_samples * channels * bytes_per_sample;
  uint8_t* buffer = new uint8_t[buffer_size];
  int64_t pts = 0;
  int nb_failed = 0;
  
  while (true) {
    int read_size = 0;
    if (!rPcmD(pcm_file, buffer, read_size, buffer_size) || read_size <= 0) {
      break;
    }
    
    // 将数据填充到帧
    in_frame->sample_rate = sample_rate;
    in_frame->format = format;
    in_frame->channel_layout = av_get_default_channel_layout(channels);
    in_frame->channels = channels;
    in_frame->nb_samples = read_size / (bytes_per_sample * channels);
    in_frame->pts = pts;
    pts += in_frame->nb_samples;
    
    ret = av_frame_get_buffer(in_frame, 0);
    if (ret < 0) {
      std::cerr << "无法为输入帧分配内存" << std::endl;
      break;
    }

    // 复制PCM数据到帧
    memcpy(in_frame->data[0], buffer, read_size);
    
    // 将帧发送到滤镜
    ret = av_buffersrc_add_frame_flags(buffer_ctx, in_frame, AV_BUFFERSRC_FLAG_KEEP_REF);
    if (ret < 0) {
      nb_failed++;
      if (nb_failed > 10) {
        break;
      }
      continue;
    }

    // 获取过滤后的帧
    while (true) {
      ret = av_buffersink_get_frame(buffersink_ctx, out_frame);
      if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        break;
      }
      if (ret < 0) {
        break;
      }

      int out_size = out_frame->nb_samples * out_frame->channels * bytes_per_sample;
      out_file.write(reinterpret_cast<const char*>(out_frame->data[0]), out_size);
      
      av_frame_unref(out_frame);
    }
    
    av_frame_unref(in_frame);
  }

  // 冲刷滤镜
  ret = av_buffersrc_add_frame(buffer_ctx, NULL);
  if (ret < 0) {
    std::cerr << "冲刷失败" << std::endl;
  }
  while (true) {
    ret = av_buffersink_get_frame(buffersink_ctx, out_frame);
    if (ret < 0) {
      break;
    }
    

    int out_size = out_frame->nb_samples * out_frame->channels * bytes_per_sample;
    out_file.write(reinterpret_cast<const char*>(out_frame->data[0]), out_size);
    
    av_frame_unref(out_frame);
  }


  delete[] buffer;
  av_frame_free(&in_frame);
  av_frame_free(&out_frame);
  avfilter_inout_free(&inputs);
  avfilter_inout_free(&outputs);
  avfilter_graph_free(&filter_graph);
  pcm_file.close();
  out_file.close();

  return true;
}

void encodeVideoWithSpeed(const std::string& yuv_file, float speed, int width, int height, 
                        const std::string& out_file, int fps) {
  std::cout << "开始编码" << speed << "倍速视频..." << std::endl;
  
  // 调整帧率
  int adjusted_fps = static_cast<int>(fps * speed);
  
  VPQ enc_q;
  VEnc enc;
  if(enc.i(width, height, adjusted_fps)) { // 修改编码器初始化，传入调整后的帧率
    enc.s(yuv_file, &enc_q);
    
    Muxer mux;
    if(mux.i(width, height, adjusted_fps, out_file)) { // 使用调整后的帧率
      mux.s(&enc_q);
      
      bool enc_done = false;
      int evp = 0, evf = 0;
      
      while(!enc_done) {
        evp = enc.gpp();
        evf = enc.gpf();
        
        if(enc_q.isF()) enc_done = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
      
      enc.st();
      mux.stop(); 
      
      std::cout << speed << "倍速视频处理完成: " << evf << " 帧输入, " << evp << " 包编码, " << mux.gpp() << " 包封装" << std::endl;
    }
  }
}

int main(int a,char*b[]){
  if(a<2){
    return 1;
  }
  std::string in=b[1];
  VPQ vq;
  APQ aq;
  Dmx dm;
  dm.init(in);
  
  std::string base_name = in.substr(0, in.find_last_of("."));
  std::string vout = base_name + "_dec.yuv";
  std::string aout = base_name + "_dec.pcm";
  
  VDec vd;
  ADec ad;
  AVCoPara*vp=dm.getVCP();
  AVCoPara*ap=dm.getACP();
  bool hv=false;
  if(vp)hv=vd.i(vp,1000000);
  bool ha=false;
  if(ap)ha=ad.i(ap,1000000);
  dm.start(&vq,&aq);
  if(hv)vd.s(&vq,vout);
  if(ha)ad.s(&aq,aout);
  bool done=false;
  int tvp=0,tvf=0,tap=0,taf=0;
  while(!done){
    if(hv){
      tvp=vd.gpp();
      tvf=vd.gpf();
    }
    if(ha){
      tap=ad.gpp();
      taf=ad.gpf();
    }
    if(vq.isF()&&aq.isF())done=true;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  dm.stop();
  if(hv){
    vd.st();
    tvp=vd.gpp();
    tvf=vd.gpf();
  }
  if(ha){
    ad.stop();
    tap=ad.gpp();
    taf=ad.gpf();
  }
  std::cout<<"解封装：视频 "<<dm.getVPC()<<" 包，音频 "<<dm.getAPC()<<" 包"<<std::endl;
  std::cout<<"视频："<<tvp<<" 包，"<<tvf<<" 帧"<<std::endl;
  std::cout<<"音频："<<tap<<" 包，"<<taf<<" 帧"<<std::endl;
  
  // 清理剩余的包
  int rvp=0,rap=0;
  AVPacket*pkt;
  while((pkt=vq.pp())!=nullptr){
    rvp++;
    av_packet_free(&pkt);
  }
  while((pkt=aq.pp())!=nullptr){
    rap++;
    av_packet_free(&pkt);
  }
  
  if(hv && vp) {
    std::string rot_yuv = base_name + "_rot.yuv";
    std::cout << "开始旋转..." << std::endl;
    pRotV(vout, rot_yuv, vp->width, vp->height, 45.0f);

    std::vector<float> speeds = {0.5f, 1.0f, 2.0f, 3.0f};
    
    for(auto speed : speeds) {

      if(ha && ap) {
        std::string speed_pcm = base_name + "_" + std::to_string(static_cast<int>(speed * 10)) + "x.pcm";

        processPcmWithFilter(aout, speed_pcm, speed, ap->sample_rate, ap->channels, static_cast<AVSampleFormat>(ap->format));
        

        std::cout << "开始编码" << speed << "倍速音频..." << std::endl;
        BUF a_buf(1024*1024);
        AE a_enc;
        if(a_enc.init(ap->sample_rate, ap->channels, static_cast<AVSampleFormat>(ap->format))) {
          std::ifstream pcm_file(speed_pcm, std::ios::binary);
          if(pcm_file.is_open()) {
            std::thread enc_thread([&]() {
              uint8_t* buf = new uint8_t[16384];
              int read_size = 0;
              while(rPcmD(pcm_file, buf, read_size, 16384)) {
                AVPacket* pkt = nullptr;
                if(a_enc.encPcm(buf, read_size, &pkt)) {
                  AVPacket* new_pkt = av_packet_alloc();
                  av_packet_ref(new_pkt, pkt);
                  a_buf.psh(reinterpret_cast<uint8_t*>(&new_pkt), sizeof(AVPacket*));
                }
              }
              a_buf.setE();
              delete[] buf;
              pcm_file.close();
            });
            enc_thread.join();
            std::string ac3_out = base_name + "_" + std::to_string(static_cast<int>(speed * 10)) + "x.ac3";
            std::ofstream ac3_file(ac3_out, std::ios::binary);
            if(ac3_file.is_open()) {
              AVPacket* pkt = nullptr;
              int pkt_size = 0;
              while(a_buf.pop(reinterpret_cast<uint8_t*>(&pkt), pkt_size, sizeof(AVPacket*))) {
                ac3_file.write(reinterpret_cast<char*>(pkt->data), pkt->size);
                av_packet_free(&pkt);
              }
              ac3_file.close();

            }
          }
        }
      }

      std::string avi_out = base_name + "_" + std::to_string(static_cast<int>(speed * 10)) + "x.avi";
      encodeVideoWithSpeed(rot_yuv, speed, vp->width, vp->height, avi_out, 25);
    }
  }
  
  return 0;
} 