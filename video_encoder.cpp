#include "video_encoder.h"
#include <iostream>

VEnc::VEnc():ctx(nullptr),frm(nullptr),pkt(nullptr),run(false),frms(0),pkts(0),width(0),height(0){}

VEnc::~VEnc(){
    st();
    if(frm) av_frame_free(&frm);
    if(pkt) av_packet_free(&pkt);
    if(ctx) avcodec_free_context(&ctx);
}

bool VEnc::i(int w, int h, int fps){
    width = w;
    height = h;
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
    if(!codec) return false;
    ctx = avcodec_alloc_context3(codec);
    if(!ctx) return false;
    ctx->bit_rate = 400000;
    ctx->width = width;
    ctx->height = height;
    ctx->time_base = (AVRational){1, fps};
    ctx->framerate = (AVRational){fps, 1};
    ctx->gop_size = 10;
    ctx->max_b_frames = 1;
    ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    if(avcodec_open2(ctx, codec, nullptr) < 0) return false;
    frm = av_frame_alloc();
    if(!frm) return false;
    frm->format = ctx->pix_fmt;
    frm->width = ctx->width;
    frm->height = ctx->height;
    if(av_frame_get_buffer(frm, 0) < 0) return false;
    
    pkt = av_packet_alloc();
    if(!pkt) return false;
    return true;
}

bool VEnc::s(const std::string& in, VPQ* q){
    if(!ctx || !frm) return false;
    
    run = true;
    t = std::thread(&VEnc::et, this, in, q);
    return true;
}

void VEnc::st(){
    run = false;
    if(t.joinable()) t.join();
}

int VEnc::gpf() const {
    return frms;
}

int VEnc::gpp() const {
    return pkts;
}

void VEnc::et(const std::string& in, VPQ* q){
    std::ifstream yuv_file(in, std::ios::binary);
    
    int frame_size = width * height * 3 / 2; 
    uint8_t* frame_data = new uint8_t[frame_size];
    int frame_count = 0;
    while(run && yuv_file.read(reinterpret_cast<char*>(frame_data), frame_size)){
        if(av_frame_make_writable(frm) < 0) break;
        // Y分量
        for(int y = 0; y < height; y++){
            memcpy(frm->data[0] + y * frm->linesize[0], 
                   frame_data + y * width, width);
        }
        // U分量
        for(int y = 0; y < height/2; y++){
            memcpy(frm->data[1] + y * frm->linesize[1], 
                   frame_data + width * height + y * width/2, width/2);
        }
        // V分量
        for(int y = 0; y < height/2; y++){
            memcpy(frm->data[2] + y * frm->linesize[2], 
                   frame_data + width * height * 5/4 + y * width/2, width/2);
        }
        
        frm->pts = frame_count++;
        frms++;
        int ret = avcodec_send_frame(ctx, frm);
        if(ret < 0) break;
        
        while(ret >= 0){
            AVPacket* pkt = av_packet_alloc();
            ret = avcodec_receive_packet(ctx, pkt);
            
            if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
                av_packet_free(&pkt);
                break;
            }
            else if(ret < 0){
                av_packet_free(&pkt);
                break;
            }
            q->psh(pkt);
            pkts++;
        }
    }
    // 冲刷编码器
    avcodec_send_frame(ctx, nullptr);
    while(true){
        AVPacket* pkt = av_packet_alloc();
        int ret = avcodec_receive_packet(ctx, pkt);
        if(ret < 0){
            av_packet_free(&pkt);
            break;
        }
        
        q->psh(pkt);
        pkts++;
    }
    q->setF();
    delete[] frame_data;
    yuv_file.close();
} 