#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "input/playback_input_source.h"
#include "render/render_overlay_sink.h"
#include "render/video_renderer.h"

namespace vp::render {

struct D3D11FormatSupportSnapshot {
    bool check_succeeded{false};
    long check_hr{0};
    uint32_t raw_support{0};
    bool texture2d{false};
    bool shader_sample{false};
    bool shader_load{false};
    bool decoder_output{false};
};

struct D3D11DecoderProfileSupport {
    bool enumeration_succeeded{false};
    uint32_t enumerated_profile_count{0};
    bool h264_vld_nofgt{false};
    bool h264_vld_fgt{false};
    bool hevc_main{false};
    bool hevc_main10{false};
    bool vp9_profile0{false};
    bool vp9_profile2_10bit{false};
    bool av1_profile0{false};
    bool av1_profile1{false};
    bool av1_profile2{false};
    bool av1_profile2_12bit{false};
    bool av1_profile2_12bit_420{false};
};

struct D3D11HdrOutputSnapshot {
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

struct D3D11DiagnosticsSnapshot {
    bool probe_succeeded{false};
    long create_device_hr{0};
    std::string adapter_name{"unknown"};
    uint32_t vendor_id{0};
    uint32_t device_id{0};
    uint32_t subsystem_id{0};
    uint32_t revision{0};
    std::string driver_version{"unknown"};
    bool software_adapter{false};
    uint64_t dedicated_video_mib{0};
    uint64_t dedicated_system_mib{0};
    uint64_t shared_system_mib{0};
    std::string feature_level{"unknown"};
    bool debug_layer_enabled{false};
    bool multithread_protected{false};
    bool has_device3{false};
    bool has_video_device{false};
    bool has_video_context{false};
    D3D11FormatSupportSnapshot nv12_support{};
    D3D11FormatSupportSnapshot p010_support{};
    D3D11FormatSupportSnapshot p016_support{};
    D3D11DecoderProfileSupport decoder_profiles{};
    bool native_direct_allowed{false};
    bool native_direct_startup_disabled{false};
    std::string native_direct_disable_rule{"none"};
    std::string native_direct_disable_reason{};
    D3D11HdrOutputSnapshot hdr_output{};
};

class D3D11VideoRenderer final : public IVideoRenderer,
                                 public input::IPlaybackInputSource,
                                 public IRenderOverlaySink {
public:
    D3D11VideoRenderer();
    ~D3D11VideoRenderer() override;

    static D3D11DiagnosticsSnapshot probeSystemDiagnostics();

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
    void setHotkeyManager(const input::HotkeyManager& hotkey_manager) override;
    RendererDiagnostics getDiagnostics() const override;
    void resetDiagnostics() override;
    bool supportsNativeFrameFormat(AVPixelFormat format) const override;
    void* nativeDeviceHandle() const override;
    const char* rendererBackendName() const override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace vp::render
