#pragma once

#include <string>
#include <vector>

#include "subtitle/subtitle_parser.h"

namespace vp::render {

class IRenderOverlaySink {
public:
    virtual ~IRenderOverlaySink() = default;

    virtual void setOverlayState(double position, double duration, float volume, bool paused) = 0;
    virtual void setSubtitleClock(double subtitle_time_seconds) {
        (void)subtitle_time_seconds;
    }
    virtual void setSubtitleText(const std::string& text) = 0;
    virtual void setSubtitleItems(const std::vector<subtitle::SubtitleItem>& items) {
        setSubtitleText(subtitle::flattenSubtitleText(items));
    }
    virtual void setSubtitleTrackState(int current_ordinal, int track_count) {
        (void)current_ordinal;
        (void)track_count;
    }
    virtual void setSubtitleTrackCatalog(const std::vector<std::string>& track_labels, int current_ordinal) {
        (void)track_labels;
        (void)current_ordinal;
    }
};

}  // namespace vp::render
