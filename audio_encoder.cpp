#include "audio_encoder.h"
#include <cstring>

AE::AE() {
    ctx = nullptr;
    frm = nullptr;
    pkt = nullptr;
    fifo = nullptr;
    fsz = 0;
    sr = 0;
    ch = 0;
    fmt = AV_SAMPLE_FMT_NONE;
}

AE::~AE() {
    cls();
}

bool AE::init(int s_r, int c_h, AVSampleFormat s_fmt) {
    sr = s_r;
    ch = c_h;
    fmt = s_fmt;
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_AC3);
    if (!codec) return false;
    ctx = avcodec_alloc_context3(codec);
    if (!ctx) return false;
    ctx->sample_rate = sr;
    ctx->channels = ch;
    ctx->channel_layout = av_get_default_channel_layout(ch);
    ctx->sample_fmt = fmt;
    ctx->bit_rate = 192000;
    
    if (avcodec_open2(ctx, codec, nullptr) < 0) {
        cls();
        return false;
    }
    
    fsz = ctx->frame_size;
    
    frm = av_frame_alloc();
    if (!frm) {
        cls();
        return false;
    }
    
    frm->nb_samples = fsz;
    frm->format = fmt;
    frm->channel_layout = ctx->channel_layout;
    frm->channels = ch;
    
    if (av_frame_get_buffer(frm, 0) < 0) {
        cls();
        return false;
    }
    
    fifo = av_audio_fifo_alloc(fmt, ch, fsz * 2);
    if (!fifo) {
        cls();
        return false;
    }
    
    pkt = av_packet_alloc();
    if (!pkt) {
        cls();
        return false;
    }
    return true;
}

bool AE::enc(AVFrame* f, AVPacket** op) {
    if (avcodec_send_frame(ctx, f) < 0) return false;
    if (avcodec_receive_packet(ctx, pkt) < 0) return false;
    *op = pkt;
    return true;
}

bool AE::encPcm(const uint8_t* d, int sz, AVPacket** op) {
    int smp = sz / (av_get_bytes_per_sample(fmt) * ch);
    if (av_audio_fifo_write(fifo, (void**)&d, smp) < smp) return false;
    if (av_audio_fifo_size(fifo) >= fsz) {
        av_frame_make_writable(frm);
        if (av_audio_fifo_read(fifo, (void**)frm->data, fsz) < fsz) return false;
        return enc(frm, op);
    }
    return false;
}

void AE::cls() {
    if (ctx) avcodec_free_context(&ctx);
    if (frm) av_frame_free(&frm);
    if (pkt) av_packet_free(&pkt);
    if (fifo) av_audio_fifo_free(fifo);
} 