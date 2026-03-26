#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "core/frame.h"

namespace vp::render {

enum class VideoRendererType {
    Auto,
    SoftwareSDL,
    D3D11,
    OpenGL
};

struct VideoRendererConfig {
    int width{0};
    int height{0};
    std::string title{"Video Player"};
};

struct RendererDiagnostics {
    uint64_t display_copy_frames{0};
    uint64_t display_copy_bytes{0};
    uint64_t display_copy_time_us_total{0};
    uint64_t display_copy_time_us_max{0};
    bool opengl_native_interop_active{false};
    bool opengl_native_interop_startup_disabled{false};
    std::string opengl_native_interop_disable_rule{"none"};
    uint64_t opengl_native_interop_frames{0};
    uint64_t opengl_native_interop_disable_events{0};
    uint64_t opengl_present_wait_timeouts{0};
    std::string opengl_present_mode_requested{"auto"};
    std::string opengl_present_mode_active{"unknown"};
    bool opengl_hdr_bridge_requested{false};
    bool opengl_hdr_bridge_active{false};
    std::string opengl_hdr_bridge_mode{"auto"};
    std::string opengl_hdr_bridge_decision{"not-evaluated"};
    bool opengl_output_lut_configured{false};
    bool opengl_output_lut_active{false};
    int opengl_output_lut_size{0};
    std::string opengl_output_lut_path;
    std::string opengl_output_lut_error;
    std::string opengl_output_lut_source{"none"};
    uint64_t opengl_output_lut_reload_count{0};
    int opengl_output_display_index{-1};
    std::string opengl_output_display_name{"unknown"};
    std::string opengl_output_display_device_name{"unknown"};
    bool opengl_output_icc_profile_available{false};
    std::string opengl_output_icc_profile_source{"none"};
    std::string opengl_output_icc_profile_path;
    std::string opengl_output_icc_profile_description{"unknown"};
    std::string opengl_output_binding_error;
    bool d3d11_hdr_present_requested{false};
    bool d3d11_hdr_present_active{false};
    bool d3d11_hdr_content_detected{false};
    std::string d3d11_hdr_present_mode{"auto"};
    std::string d3d11_hdr_present_decision{"not-evaluated"};
    std::string d3d11_hdr_swapchain_format{"unknown"};
    std::string d3d11_hdr_output_color_space{"unknown"};
    int d3d11_hdr_output_display_index{-1};
    std::string d3d11_hdr_output_display_name{"unknown"};
    std::string d3d11_hdr_output_device_name{"unknown"};
    bool d3d11_hdr_output_advanced_color_active{false};
    bool d3d11_hdr_output_hdr_active{false};
    bool d3d11_hdr_metadata_available{false};
    bool d3d11_hdr_metadata_applied{false};
    std::string d3d11_hdr_metadata_source{"none"};
    uint64_t d3d11_hdr_state_reload_count{0};
    uint64_t d3d11_present_count{0};
    uint64_t d3d11_present_failures{0};
    uint64_t d3d11_present_time_us_total{0};
    uint64_t d3d11_present_time_us_max{0};
    std::string d3d11_hdr_error;
};

class IVideoRenderer {
public:
    virtual ~IVideoRenderer() = default;

    virtual bool init(const VideoRendererConfig& config) = 0;
    virtual void close() = 0;

    virtual void renderFrame(const core::VideoFrame& frame) = 0;
    virtual void present() = 0;
    virtual void clear() = 0;

    virtual bool supportsNativeFrameFormat(AVPixelFormat format) const {
        (void)format;
        return false;
    }

    virtual bool supportsDirectFrameFormat(AVPixelFormat format) const {
        (void)format;
        return false;
    }

    virtual void* nativeDeviceHandle() const {
        return nullptr;
    }

    virtual RendererDiagnostics getDiagnostics() const {
        return {};
    }

    virtual void resetDiagnostics() {}

    virtual const char* rendererBackendName() const = 0;
};

using VideoRendererPtr = std::unique_ptr<IVideoRenderer>;

}  // namespace vp::render
