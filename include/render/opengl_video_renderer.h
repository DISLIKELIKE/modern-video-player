#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "input/playback_input_source.h"
#include "render/d3d11_video_renderer.h"
#include "render/render_overlay_sink.h"
#include "render/video_renderer.h"

namespace vp::render {

struct OpenGLHdrOutputDiagnosticsSnapshot {
    bool probe_succeeded{false};
    std::string probe_error{};
    bool adapter_matched{false};
    uint32_t adapter_index{0};
    bool output_found{false};
    uint32_t output_index{0};
    std::string output_name{"unknown"};
    std::string output_device_name{"unknown"};
    bool output_attached_to_desktop{false};
    bool has_output6{false};
    std::string color_space{"unknown"};
    bool advanced_color_active{false};
    bool hdr_active{false};
    uint32_t bits_per_color{0};
    double min_luminance_nits{0.0};
    double max_luminance_nits{0.0};
    double max_full_frame_luminance_nits{0.0};
};

struct OpenGLOutputDisplayDiagnosticsSnapshot {
    bool binding_succeeded{false};
    int sdl_display_index{-1};
    std::string sdl_display_name{"unknown"};
    std::string output_device_name{"unknown"};
    bool icc_profile_available{false};
    std::string icc_profile_source{"none"};
    std::string icc_profile_path{"none"};
    std::string icc_profile_description{"unknown"};
    std::string binding_error{};
};

struct OpenGLDiagnosticsSnapshot {
    bool supported_platform{false};
    bool probe_succeeded{false};
    bool gl_context_created{false};
    std::string create_context_error{};
    std::string gl_vendor{"unknown"};
    std::string gl_renderer{"unknown"};
    std::string gl_version{"unknown"};
    bool has_wgl_dx_interop{false};
    std::string native_interop_env_override{"auto"};
    D3D11DiagnosticsSnapshot d3d11{};
    bool native_direct_allowed{false};
    bool native_direct_startup_disabled{false};
    std::string native_direct_disable_rule{"none"};
    std::string native_direct_disable_reason{};
    bool hard_blocker_matched{false};
    std::string hard_blocker_rule{"none"};
    std::string hard_blocker_reason{};
    bool quirk_rule_matched{false};
    std::string quirk_rule_name{"none"};
    std::string quirk_rule_reason{};
    bool env_force_overrode_quirk{false};
    OpenGLHdrOutputDiagnosticsSnapshot hdr_output{};
    OpenGLOutputDisplayDiagnosticsSnapshot output_display{};
};

class OpenGLVideoRenderer final : public IVideoRenderer,
                                  public input::IPlaybackInputSource,
                                  public IRenderOverlaySink {
public:
    OpenGLVideoRenderer();
    ~OpenGLVideoRenderer() override;

    static OpenGLDiagnosticsSnapshot probeSystemDiagnostics();

    bool init(const VideoRendererConfig& config) override;
    void close() override;
    void renderFrame(const core::VideoFrame& frame) override;
    void present() override;
    void clear() override;
    void handleEvents() override;
    bool shouldQuit() const override;
    bool consumeTogglePauseRequest() override;
    bool consumeSeekRequest(double& normalized_position) override;
    bool consumeSeekDeltaRequest(double& delta_seconds) override;
    bool consumeVolumeChangeRequest(float& volume) override;
    bool consumeSpeedChangeRequest(double& speed_delta) override;
    bool consumeResetSpeedRequest() override;
    bool consumeToggleSubtitleRequest() override;
    bool consumeSetABRepeatStartRequest() override;
    bool consumeSetABRepeatEndRequest() override;
    bool consumeClearABRepeatRequest() override;
    bool consumeScreenshotRequest() override;
    bool consumeStepFrameBackwardRequest() override;
    bool consumeStepFrameForwardRequest() override;
    bool consumePreviousSubtitleTrackRequest() override;
    bool consumeNextSubtitleTrackRequest() override;
    bool consumeSubtitleDelayChangeRequest(double& delta_seconds) override;
    bool consumeAudioDelayChangeRequest(double& delta_seconds) override;
    bool consumeNextChapterRequest() override;
    bool consumePreviousChapterRequest() override;
    bool consumeNextItemRequest() override;
    bool consumePreviousItemRequest() override;
    bool consumeOpenFileRequest(std::string& path) override;
    void setOverlayState(double position, double duration, float volume, bool paused) override;
    void setSubtitleClock(double subtitle_time_seconds) override;
    void setSubtitleText(const std::string& text) override;
    void setSubtitleItems(const std::vector<subtitle::SubtitleItem>& items) override;
    void setSubtitleTrackState(int current_ordinal, int track_count) override;
    void setSubtitleTrackCatalog(const std::vector<std::string>& track_labels, int current_ordinal) override;
    void setHotkeyManager(const input::HotkeyManager& hotkey_manager) override;
    RendererDiagnostics getDiagnostics() const override;
    void resetDiagnostics() override;
    bool supportsNativeFrameFormat(AVPixelFormat format) const override;
    bool supportsDirectFrameFormat(AVPixelFormat format) const override;
    void* nativeDeviceHandle() const override;
    const char* rendererBackendName() const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace vp::render
