#include "audio_decoder.h"
#include <iostream>

ADec::ADec()
    : ctx(nullptr)
    , frm(nullptr)
    , swr(nullptr)
    , run(false)
    , sr(44100)
    , ch(2)
    , cl(AV_CH_LAYOUT_STEREO)
    , fmt(AV_SAMPLE_FMT_S16)
    , pkts(0)
    , frms(0)
    , maxf(30)
{
}

ADec::~ADec() {
    stop();
    if (frm) av_frame_free(&frm);
    if (ctx) avcodec_free_context(&ctx);
    if (swr) swr_free(&swr);
}

bool ADec::i(AVCoPara* p, int mf) {
    const AVCodec* codec = avcodec_find_decoder(p->codec_id);
    if (!codec) return false; 
    ctx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(ctx, p);
    avcodec_open2(ctx, codec, nullptr);
    frm = av_frame_alloc();
    swr = swr_alloc();
    av_opt_set_int(swr, "in_channel_layout", ctx->channel_layout, 0);
    av_opt_set_int(swr, "in_sample_rate", ctx->sample_rate, 0);
    av_opt_set_sample_fmt(swr, "in_sample_fmt", ctx->sample_fmt, 0);
    av_opt_set_int(swr, "out_channel_layout", cl, 0);
    av_opt_set_int(swr, "out_sample_rate", sr, 0);
    av_opt_set_sample_fmt(swr, "out_sample_fmt", fmt, 0);
    swr_init(swr);
    maxf = mf;
    return true;
}

bool ADec::s(APQ* q, const std::string& o) {
    run = true;
    t = std::thread(&ADec::dt, this, q, o);
    return true;
}

void ADec::stop() {
    run = false;
    if (t.joinable()) t.join();
}

int ADec::gpp() const {
    return pkts;
}

int ADec::gpf() const {
    return frms;
}

void ADec::dt(APQ* q, const std::string& o) {
    if(q->isF() && q->sz() == 0) return;
    std::ofstream out(o, std::ios::binary);
    if (!out.is_open()) return;
    AVPacket* pkt = nullptr;
    char h[44] = {
        'R','I','F','F',0,0,0,0,
        'W','A','V','E','f','m','t',' ',
        16,0,0,0,1,0,(char)ch,0,
        (char)(sr&0xFF),(char)((sr>>8)&0xFF),(char)((sr>>16)&0xFF),(char)((sr>>24)&0xFF),
        (char)((sr*ch*2)&0xFF),(char)(((sr*ch*2)>>8)&0xFF),(char)(((sr*ch*2)>>16)&0xFF),(char)(((sr*ch*2)>>24)&0xFF),
        (char)(ch*2),0,16,0,
        'd','a','t','a',0,0,0,0
    };
    
    out.write(h, 44);
    long ds = 0;
    pkts = 0;
    frms = 0;
    int sc = 0;
    int ec = 0;
    int err = 0;
    bool qf = false;
    
    while (run || (!qf && ec < 50)) {
        if (frms >= maxf) break;
        pkt = q->pp();
        if (!pkt) {
            if (q->isF()) {
                qf = true;
                if (q->sz() == 0) break;
            }
            ec++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        
        ec = 0;
        pkts++;
        int s = avcodec_send_packet(ctx, pkt);
        av_packet_free(&pkt);
        if (s < 0) {
            err++;
            continue;
        }
        
        while (s >= 0) {
            int r = avcodec_receive_frame(ctx, frm);
            if (r == AVERROR(EAGAIN) || r == AVERROR_EOF) break;
            else if (r < 0) {
                err++;
                break;
            }
            sf(frm, out);
            frms++;
            sc += frm->nb_samples;
            ds += frm->nb_samples * ch * 2;
            if (frms >= maxf) break;
            av_frame_unref(frm);
        }
        if (frms >= maxf) break;
    }
    
    if (frms < maxf) {
        avcodec_send_packet(ctx, nullptr);
        while (frms < maxf) {
            int r = avcodec_receive_frame(ctx, frm);
            if (r < 0) break;
            sf(frm, out);
            frms++;
            sc += frm->nb_samples;
            ds += frm->nb_samples * ch * 2;
            av_frame_unref(frm);
        }
    }
    
    out.seekp(4);
    int fs = ds + 36;
    out.write(reinterpret_cast<const char*>(&fs), 4);
    out.seekp(40);
    out.write(reinterpret_cast<const char*>(&ds), 4);
    out.close();
}

void ADec::sf(AVFrame* f, std::ofstream& o) {
    int os = swr_get_out_samples(swr, f->nb_samples);
    uint8_t** ob = (uint8_t**)calloc(ch, sizeof(*ob));
    av_samples_alloc(ob, nullptr, ch, os, fmt, 0);
    int s = swr_convert(swr, ob, os, (const uint8_t**)f->data, f->nb_samples);
    if (s > 0) {
        int sz = av_samples_get_buffer_size(nullptr, ch, s, fmt, 1);
        o.write(reinterpret_cast<const char*>(ob[0]), sz);
    }
    if (ob) {
        av_freep(&ob[0]);
        free(ob);
    }
} 