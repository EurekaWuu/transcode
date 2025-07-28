#include"demuxer.h"
#include<iostream>

Dmx::Dmx():fmt(nullptr),vidx(-1),aidx(-1),run(false),max(0),vpc(0),apc(0){}

Dmx::~Dmx(){
    stop();
    if(fmt)avformat_close_input(&fmt);
}

bool Dmx::init(const std::string& f){
    fmt=avformat_alloc_context();
    avformat_open_input(&fmt,f.c_str(),nullptr,nullptr);
    avformat_find_stream_info(fmt,nullptr);
    vidx=-1;
    aidx=-1;
    for(unsigned int i=0;i<fmt->nb_streams;i++){
        AVStream* s=fmt->streams[i];
        switch(s->codecpar->codec_type){
            case AVMEDIA_TYPE_VIDEO:
                if(vidx<0) vidx=i;
                break;
            case AVMEDIA_TYPE_AUDIO:
                if(aidx<0) aidx=i;
                break;
            default:
                break;
    }
    }
    
    return true;
}

bool Dmx::start(VPQ* v,APQ* a){
    run=true;
    th=std::thread(&Dmx::thr,this,v,a);
    return true;
}

void Dmx::stop(){
    run=false;
    if(th.joinable()) th.join();
}

AVCoPara* Dmx::getVCP(){
    if(vidx>=0) return fmt->streams[vidx]->codecpar;
    return nullptr;
}

AVCoPara* Dmx::getACP(){
    if(aidx>=0) return fmt->streams[aidx]->codecpar;
    return nullptr;
}

int Dmx::getVIdx(){
    return vidx;
}

int Dmx::getAIdx(){
    return aidx;
}

double Dmx::getDur(){
    if(fmt&&fmt->duration!=AV_NOPTS_VALUE) return fmt->duration/(double)AV_TIME_BASE;
    return 0.0;
}

int Dmx::getVPC(){
    return vpc;
}

int Dmx::getAPC(){
    return apc;
}

void Dmx::thr(VPQ* v,APQ* a){
    AVPacket* p=av_packet_alloc();
    vpc=0;
    apc=0;
    int tot=0;
    
    while(run){
        int r=av_read_frame(this->fmt,p);
        if(r<0)break;
        tot++;
        if(p->stream_index==vidx&&v){
            AVPacket* vp=av_packet_alloc();
            av_packet_ref(vp,p);
            v->psh(vp);
            vpc++;
        }else if(p->stream_index==aidx&&a){
            AVPacket* ap=av_packet_alloc();
            av_packet_ref(ap,p);
            a->psh(ap);
            apc++;
        }
        av_packet_unref(p);
    }
    
    if(v){
        int wc=0;
        const int mw=100;
        while(v->sz()>0&&wc<mw){
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            wc++;
        }
        v->setF();
    }
    
    if(a){
        int wc=0;
        const int mw=100;
        while(a->sz()>0&&wc<mw){
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            wc++;
        }
        a->setF();
    }
    
    av_packet_free(&p);
} 