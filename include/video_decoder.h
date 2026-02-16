#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <string>
#include <vector>
#include <memory>
#include <mutex>

namespace vp {

class VideoFrame {
public:
    VideoFrame();
    ~VideoFrame();
    
    bool isValid() const { return frame_ != nullptr; }
    AVFrame* get() { return frame_; }
    const AVFrame* get() const { return frame_; }
    
    double pts() const { return pts_; }
    void setPts(double pts) { pts_ = pts; }
    
    void setData(int width, int height, AVPixelFormat pix_fmt);
    
private:
    AVFrame* frame_;
    double pts_;
};

class VideoDecoder {
public:
    VideoDecoder();
    ~VideoDecoder();
    
    bool open(AVFormatContext* fmt_ctx, int stream_idx);
    void close();
    
    bool decodeFrame(VideoFrame& frame);
    void flush();
    
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    AVPixelFormat getPixelFormat() const { return pix_fmt_; }
    AVRational getTimeBase() const { return time_base_; }
    
    bool convertFrame(AVFrame* src_frame, AVFrame* dst_frame, 
                     AVPixelFormat dst_pix_fmt);

private:
    AVCodecContext* codec_ctx_;
    SwsContext* sws_ctx_;
    int stream_idx_;
    
    int width_;
    int height_;
    AVPixelFormat pix_fmt_;
    AVRational time_base_;
    
    std::mutex mutex_;
};

} // namespace vp
