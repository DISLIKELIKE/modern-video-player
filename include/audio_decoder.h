#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
}

#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace vp {

class AudioFrame {
public:
    AudioFrame();
    ~AudioFrame();
    
    bool isValid() const { return frame_ != nullptr; }
    AVFrame* get() { return frame_; }
    const AVFrame* get() const { return frame_; }
    
    double pts() const { return pts_; }
    void setPts(double pts) { pts_ = pts; }
    
    const uint8_t* getData() const;
    int getSize() const;

private:
    AVFrame* frame_;
    double pts_;
    uint8_t* converted_data_;
    int converted_size_;
};

class AudioDecoder {
public:
    AudioDecoder();
    ~AudioDecoder();
    
    bool open(AVFormatContext* fmt_ctx, int stream_idx);
    void close();
    
    bool decodeFrame(AudioFrame& frame);
    void flush();
    
    int getSampleRate() const { return sample_rate_; }
    int getChannels() const { return channels_; }
    AVSampleFormat getSampleFormat() const { return sample_fmt_; }
    AVRational getTimeBase() const { return time_base_; }
    
    bool convertToS16(AVFrame* src_frame, uint8_t** dst_data, int* dst_linesize);

private:
    AVCodecContext* codec_ctx_;
    SwrContext* swr_ctx_;
    int stream_idx_;
    
    int sample_rate_;
    int channels_;
    AVSampleFormat sample_fmt_;
    AVRational time_base_;
    
    std::mutex mutex_;
};

} // namespace vp
