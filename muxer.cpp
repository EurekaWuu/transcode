#include "muxer.h"
#include <iostream>

Muxer::Muxer() : ctx(nullptr), stream(nullptr), r(false), pp(0), width(0), height(0), fps(0) {
}

Muxer::~Muxer() {
    stop();
    if (ctx) {
        if (ctx->pb) {
            av_write_trailer(ctx);
            avio_close(ctx->pb);
        }
        avformat_free_context(ctx);
    }
}

bool Muxer::i(int w, int h, int framerate, const std::string& out) {
    width = w;
    height = h;
    fps = framerate;
    outf = out;
    
    avformat_alloc_output_context2(&ctx, nullptr, "avi", out.c_str());
    if (!ctx) return false;
    stream = avformat_new_stream(ctx, nullptr);
    if (!stream) return false;
    
    AVCodecParameters* params = stream->codecpar;
    params->codec_type = AVMEDIA_TYPE_VIDEO;
    params->codec_id = AV_CODEC_ID_MPEG4;
    params->width = width;
    params->height = height;
    params->format = AV_PIX_FMT_YUV420P;
    params->bit_rate = 1000000;
    
    // 使用指定的帧率
    stream->time_base = (AVRational){1, fps};
    
    if (avio_open(&ctx->pb, outf.c_str(), AVIO_FLAG_WRITE) < 0) 
        return false;
    if (avformat_write_header(ctx, nullptr) < 0) 
        return false;
    return true;
}

bool Muxer::s(VPQ* queue) {
    q = queue;
    r = true;
    t = std::thread([this]() {
        AVPacket* pkt = nullptr;
        int ec = 0;
        bool qf = false;
        
        while (r || (!qf && ec < 50)) {
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
            pkt->stream_index = stream->index;
            if (av_interleaved_write_frame(ctx, pkt) >= 0) {
                pp++;
            }
            av_packet_free(&pkt);
        }
        
        // 处理队列中剩余的包
        while ((pkt = q->pp()) != nullptr) {
            pkt->stream_index = stream->index;
            if (av_interleaved_write_frame(ctx, pkt) >= 0) {
                pp++;
            }
            av_packet_free(&pkt);
        }
    });
    return true;
}

void Muxer::stop() {
    r = false;
    if (t.joinable()) t.join();
}

int Muxer::gpp() const {
    return pp;
}