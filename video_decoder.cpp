#include "video_decoder.h"
#include <iostream>

VDec::VDec():ctx(nullptr),frm(nullptr),run(false),sws(nullptr),pkts(0),frms(0),maxf(30){}

VDec::~VDec(){
    st();
    if(frm) av_frame_free(&frm);
    if(ctx) avcodec_free_context(&ctx);
    if(sws){
        sws_freeContext(sws);
        sws=nullptr;
    }
}

bool VDec::i(AVCoPara* p, int mf){
    const AVCodec* codec=avcodec_find_decoder(p->codec_id);
    if(!codec) return false;
    ctx=avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(ctx,p);
    avcodec_open2(ctx,codec,nullptr);
    frm=av_frame_alloc();
    maxf=mf;
    return true;
}

bool VDec::s(VPQ* q,const std::string& o){
    run=true;
    t=std::thread(&VDec::dt,this,q,o);
    return true;
}

void VDec::st(){
    run=false;
    if(t.joinable()) t.join();
}

int VDec::gpp() const {
    return pkts;
}

int VDec::gpf() const {
    return frms;
}

void VDec::dt(VPQ* q,const std::string& o){
    std::ofstream out(o,std::ios::binary);
    if(!out.is_open()) return;
    AVPacket* pkt=nullptr;
    pkts=0;
    frms=0;
    int ec=0;
    int err=0;
    bool qf=false;
    
    // 解码循环
    while(run||(!qf&&ec<50)){
        if(frms>=maxf)break;
        pkt=q->pp();
        if(!pkt){
            // 是否标记完成
            if(q->isF()){
                qf=true;
                if(q->sz() == 0) break;
            }
            
            ec++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        // 空包计数
        ec=0;
        pkts++;
        
        int s=avcodec_send_packet(ctx,pkt);
        av_packet_free(&pkt);
        
        if(s<0){
            err++;
            continue;
        }
    
        while(s>=0){
            int r=avcodec_receive_frame(ctx,frm);
            
            if(r==AVERROR(EAGAIN)||r==AVERROR_EOF) break;
            else if(r<0){
                err++;
                break;
            }

            sf(frm,out);
            frms++;
            if(frms>=maxf)break;
            
            av_frame_unref(frm);
        }
        if(frms>=maxf)break;
    }
    
    if(frms<maxf){
        avcodec_send_packet(ctx,nullptr);
        while(frms<maxf){
            int r=avcodec_receive_frame(ctx,frm);
            if(r<0)break;
            sf(frm,out);
            frms++;
            av_frame_unref(frm);
        }
    }
    
    out.close();
}

void VDec::sf(AVFrame* f,std::ofstream& o){
    int size=av_image_get_buffer_size(AV_PIX_FMT_YUV420P,f->width,f->height,1);
    uint8_t* buf=(uint8_t*)av_malloc(size);
    av_image_copy_to_buffer(buf,size,(const uint8_t* const*)f->data,f->linesize,AV_PIX_FMT_YUV420P,f->width,f->height,1);
    o.write(reinterpret_cast<const char*>(buf),size);
    av_free(buf);
} 
