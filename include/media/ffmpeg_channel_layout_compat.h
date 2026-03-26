#pragma once

#include <algorithm>
#include <cstdint>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/frame.h>
#include <libavutil/version.h>
}

namespace vp::media::ffmpeg_compat {

#if LIBAVUTIL_VERSION_MAJOR >= 57
constexpr bool kHasChannelLayoutStruct = true;
#else
constexpr bool kHasChannelLayoutStruct = false;
#endif

inline int channelCountFromLayoutMask(uint64_t layout_mask) {
    int count = 0;
    while (layout_mask != 0) {
        count += static_cast<int>(layout_mask & 1ULL);
        layout_mask >>= 1ULL;
    }
    return count;
}

#if LIBAVUTIL_VERSION_MAJOR >= 57

inline uint64_t defaultChannelLayout(int channels) {
    if (channels <= 0) {
        return 0;
    }
    AVChannelLayout layout{};
    av_channel_layout_default(&layout, channels);
    if (layout.order == AV_CHANNEL_ORDER_NATIVE) {
        return layout.u.mask;
    }
    return 0;
}

inline uint64_t codecParametersChannelLayout(const AVCodecParameters* codecpar) {
    if (!codecpar) {
        return 0;
    }
    if (codecpar->ch_layout.order == AV_CHANNEL_ORDER_NATIVE) {
        return codecpar->ch_layout.u.mask;
    }
    return 0;
}

inline uint64_t codecContextChannelLayout(const AVCodecContext* codec_ctx) {
    if (!codec_ctx) {
        return 0;
    }
    if (codec_ctx->ch_layout.order == AV_CHANNEL_ORDER_NATIVE) {
        return codec_ctx->ch_layout.u.mask;
    }
    return 0;
}

inline uint64_t frameChannelLayout(const AVFrame* frame) {
    if (!frame) {
        return 0;
    }
    if (frame->ch_layout.order == AV_CHANNEL_ORDER_NATIVE) {
        return frame->ch_layout.u.mask;
    }
    return 0;
}

inline int codecParametersChannels(const AVCodecParameters* codecpar, int fallback = 0) {
    if (!codecpar) {
        return std::max(0, fallback);
    }
    if (codecpar->ch_layout.nb_channels > 0) {
        return codecpar->ch_layout.nb_channels;
    }
    return std::max(0, fallback);
}

inline int codecContextChannels(const AVCodecContext* codec_ctx, int fallback = 0) {
    if (!codec_ctx) {
        return std::max(0, fallback);
    }
    if (codec_ctx->ch_layout.nb_channels > 0) {
        return codec_ctx->ch_layout.nb_channels;
    }
    return std::max(0, fallback);
}

inline int frameChannels(const AVFrame* frame, int fallback = 0) {
    if (!frame) {
        return std::max(0, fallback);
    }
    if (frame->ch_layout.nb_channels > 0) {
        return frame->ch_layout.nb_channels;
    }
    return std::max(0, fallback);
}

#else

inline uint64_t defaultChannelLayout(int channels) {
    if (channels <= 0) {
        return 0;
    }
    return static_cast<uint64_t>(av_get_default_channel_layout(channels));
}

inline uint64_t codecParametersChannelLayout(const AVCodecParameters* codecpar) {
    if (!codecpar) {
        return 0;
    }
    return static_cast<uint64_t>(codecpar->channel_layout);
}

inline uint64_t codecContextChannelLayout(const AVCodecContext* codec_ctx) {
    if (!codec_ctx) {
        return 0;
    }
    return static_cast<uint64_t>(codec_ctx->channel_layout);
}

inline uint64_t frameChannelLayout(const AVFrame* frame) {
    if (!frame) {
        return 0;
    }
    return static_cast<uint64_t>(frame->channel_layout);
}

inline int codecParametersChannels(const AVCodecParameters* codecpar, int fallback = 0) {
    if (!codecpar) {
        return std::max(0, fallback);
    }
    if (codecpar->channels > 0) {
        return codecpar->channels;
    }
    const int layout_count = channelCountFromLayoutMask(codecpar->channel_layout);
    if (layout_count > 0) {
        return layout_count;
    }
    return std::max(0, fallback);
}

inline int codecContextChannels(const AVCodecContext* codec_ctx, int fallback = 0) {
    if (!codec_ctx) {
        return std::max(0, fallback);
    }
    if (codec_ctx->channels > 0) {
        return codec_ctx->channels;
    }
    const int layout_count = channelCountFromLayoutMask(codec_ctx->channel_layout);
    if (layout_count > 0) {
        return layout_count;
    }
    return std::max(0, fallback);
}

inline int frameChannels(const AVFrame* frame, int fallback = 0) {
    if (!frame) {
        return std::max(0, fallback);
    }
    if (frame->channels > 0) {
        return frame->channels;
    }
    const int layout_count = channelCountFromLayoutMask(frame->channel_layout);
    if (layout_count > 0) {
        return layout_count;
    }
    return std::max(0, fallback);
}

#endif

inline int64_t frameDuration(const AVFrame* frame) {
    if (!frame) {
        return 0;
    }
#if LIBAVUTIL_VERSION_MAJOR >= 57
    return frame->duration;
#else
    return frame->pkt_duration;
#endif
}

} // namespace vp::media::ffmpeg_compat
