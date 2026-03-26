#pragma once

#include <string>
#include <vector>

namespace vp::render {

struct OutputLut3DData {
    int size{0};
    std::vector<unsigned char> rgb8;
};

struct IccProfileSummary {
    bool valid{false};
    bool matrix_shaper_rgb{false};
    std::string device_class{"unknown"};
    std::string color_space{"unknown"};
    std::string connection_space{"unknown"};
    std::string description{"unknown"};
};

bool loadCubeLut3D(const std::string& path, OutputLut3DData& out, std::string& error);

bool generateIccProfileLut(const std::string& path,
                           int lut_size,
                           OutputLut3DData& out,
                           IccProfileSummary& summary,
                           std::string& error);

}  // namespace vp::render
