#include "render/d3d11_video_renderer.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cwchar>
#include <cstring>
#include <iomanip>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#elif __has_include(<SDL.h>)
#include <SDL.h>
#include <SDL_syswm.h>
#else
#error "SDL2 headers not found"
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3d11_3.h>
#include <d3d11_4.h>
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <d3dcompiler.h>
#include <dwrite.h>
#include <dwrite_1.h>
#include <dxgi1_2.h>
#include <dxgi1_4.h>
#include <dxgi1_5.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

extern "C" {
#include <libavutil/hwcontext_d3d11va.h>
#include <libavutil/mastering_display_metadata.h>
#include <libavutil/pixdesc.h>
}

#include "logger.h"
#include "subtitle/subtitle_font_registry.h"

namespace vp::render {

using Microsoft::WRL::ComPtr;

namespace {

constexpr int kMinWindowWidth = 320;
constexpr int kMinWindowHeight = 180;
constexpr int kControlPanelInset = 8;
constexpr int kControlPanelHeight = 44;
constexpr int kControlPadding = 10;
constexpr int kControlGap = 14;
constexpr int kBarHeight = 8;
constexpr int kVolumeBarWidth = 96;
constexpr int kMinProgressBarWidth = 80;
constexpr float kVolumeStep = 0.05f;
constexpr double kSeekStepSeconds = 5.0;
constexpr double kSeekStepSecondsCtrl = 30.0;
constexpr double kSpeedStep = 0.1;
constexpr float kMinSubtitleFontSize = 20.0f;
constexpr float kMaxSubtitleFontSize = 72.0f;
constexpr float kSubtitleFontScale = 0.055f;
constexpr float kSubtitleHorizontalPadding = 18.0f;
constexpr float kSubtitleVerticalPadding = 10.0f;
constexpr float kSubtitleShadowOffset = 2.0f;
constexpr size_t kSubtitleBitmapCacheLimit = 32;

uint64_t hashSubtitleBitmap(const subtitle::SubtitleBitmap& bitmap) {
    constexpr uint64_t kFnvOffset = 1469598103934665603ull;
    constexpr uint64_t kFnvPrime = 1099511628211ull;

    auto hash_bytes = [kFnvPrime](uint64_t seed, const uint8_t* data, size_t size) {
        uint64_t hash = seed;
        for (size_t i = 0; i < size; ++i) {
            hash ^= static_cast<uint64_t>(data[i]);
            hash *= kFnvPrime;
        }
        return hash;
    };

    uint64_t hash = kFnvOffset;
    const std::array<int, 2> header = {bitmap.width, bitmap.height};
    hash = hash_bytes(hash,
                      reinterpret_cast<const uint8_t*>(header.data()),
                      header.size() * sizeof(header[0]));
    return hash_bytes(hash, bitmap.rgba.data(), bitmap.rgba.size());
}

std::vector<uint8_t> premultiplySubtitleBitmapBgra(const subtitle::SubtitleBitmap& bitmap) {
    const size_t pixel_count = static_cast<size_t>(bitmap.width) * static_cast<size_t>(bitmap.height);
    std::vector<uint8_t> premul_bgra(pixel_count * 4u, 0u);
    for (size_t i = 0; i < pixel_count; ++i) {
        const uint8_t r = bitmap.rgba[i * 4u + 0u];
        const uint8_t g = bitmap.rgba[i * 4u + 1u];
        const uint8_t b = bitmap.rgba[i * 4u + 2u];
        const uint8_t a = bitmap.rgba[i * 4u + 3u];
        const uint16_t alpha = static_cast<uint16_t>(a);
        premul_bgra[i * 4u + 0u] = static_cast<uint8_t>((static_cast<uint16_t>(b) * alpha + 127u) / 255u);
        premul_bgra[i * 4u + 1u] = static_cast<uint8_t>((static_cast<uint16_t>(g) * alpha + 127u) / 255u);
        premul_bgra[i * 4u + 2u] = static_cast<uint8_t>((static_cast<uint16_t>(r) * alpha + 127u) / 255u);
        premul_bgra[i * 4u + 3u] = a;
    }
    return premul_bgra;
}

const char* dxgiFormatName(DXGI_FORMAT format) {
    switch (format) {
    case DXGI_FORMAT_NV12:
        return "NV12";
    case DXGI_FORMAT_P010:
        return "P010";
    case DXGI_FORMAT_P016:
        return "P016";
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return "B8G8R8A8_UNORM";
    case DXGI_FORMAT_R10G10B10A2_UNORM:
        return "R10G10B10A2_UNORM";
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        return "R16G16B16A16_FLOAT";
    default:
        return "UNKNOWN";
    }
}

const char* boolName(bool value) { return value ? "true" : "false"; }

const char* featureLevelName(D3D_FEATURE_LEVEL level) {
    switch (level) {
    case D3D_FEATURE_LEVEL_11_1:
        return "11_1";
    case D3D_FEATURE_LEVEL_11_0:
        return "11_0";
    case D3D_FEATURE_LEVEL_10_1:
        return "10_1";
    case D3D_FEATURE_LEVEL_10_0:
        return "10_0";
    default:
        return "unknown";
    }
}

const char* swapEffectName(DXGI_SWAP_EFFECT effect) {
    switch (effect) {
    case DXGI_SWAP_EFFECT_DISCARD:
        return "DISCARD";
    case DXGI_SWAP_EFFECT_SEQUENTIAL:
        return "SEQUENTIAL";
    case DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL:
        return "FLIP_SEQUENTIAL";
    case DXGI_SWAP_EFFECT_FLIP_DISCARD:
        return "FLIP_DISCARD";
    default:
        return "UNKNOWN";
    }
}

const char* alphaModeName(DXGI_ALPHA_MODE mode) {
    switch (mode) {
    case DXGI_ALPHA_MODE_UNSPECIFIED:
        return "UNSPECIFIED";
    case DXGI_ALPHA_MODE_PREMULTIPLIED:
        return "PREMULTIPLIED";
    case DXGI_ALPHA_MODE_STRAIGHT:
        return "STRAIGHT";
    case DXGI_ALPHA_MODE_IGNORE:
        return "IGNORE";
    default:
        return "UNKNOWN";
    }
}

std::string utf8FromWide(const wchar_t* text) {
    if (!text || text[0] == L'\0') {
        return std::string();
    }

    const int required = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
    if (required <= 1) {
        return std::string();
    }

    std::string result(static_cast<size_t>(required - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), required, nullptr, nullptr);
    return result;
}

bool tryGetWindowHandle(SDL_Window* window, HWND& hwnd) {
    hwnd = nullptr;
    if (!window) {
        return false;
    }
    SDL_SysWMinfo wm_info{};
    SDL_VERSION(&wm_info.version);
    if (!SDL_GetWindowWMInfo(window, &wm_info)) {
        return false;
    }
    hwnd = wm_info.info.win.window;
    return hwnd != nullptr;
}

uint64_t bytesToMiB(SIZE_T bytes) {
    return static_cast<uint64_t>(bytes / (1024ull * 1024ull));
}

std::string adapterDriverVersionString(IDXGIAdapter* adapter) {
    if (!adapter) {
        return "unknown";
    }

    LARGE_INTEGER version{};
    if (FAILED(adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &version))) {
        return "unknown";
    }

    std::ostringstream oss;
    oss << HIWORD(version.HighPart) << '.'
        << LOWORD(version.HighPart) << '.'
        << HIWORD(version.LowPart) << '.'
        << LOWORD(version.LowPart);
    return oss.str();
}

bool guidEquals(const GUID& lhs, const GUID& rhs) {
    return std::memcmp(&lhs, &rhs, sizeof(GUID)) == 0;
}

struct D3D11DeviceProbeContext {
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<ID3D11Device3> device3;
    D3D_FEATURE_LEVEL feature_level{D3D_FEATURE_LEVEL_11_0};
    bool debug_layer_enabled{false};
    bool multithread_protected{false};
    HRESULT create_device_hr{E_FAIL};
};

struct NativeDirectBlacklistRule {
    const char* rule_name;
    std::optional<UINT> vendor_id;
    std::optional<UINT> device_id;
    const char* adapter_name_substring;
    const char* driver_version_prefix;
    const char* reason;
};

constexpr NativeDirectBlacklistRule kNativeDirectBlacklistRules[] = {
    {"microsoft-basic-render-driver",
     0x1414u,
     std::nullopt,
     "Microsoft Basic Render Driver",
     nullptr,
     "software/basic adapters are excluded from D3D11 native direct video surfaces"},
};

const GUID kDecoderProfileH264VldNoFgt =
    {0x1b81be68, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5}};
const GUID kDecoderProfileH264VldFgt =
    {0x1b81be69, 0xa0c7, 0x11d3, {0xb9, 0x84, 0x00, 0xc0, 0x4f, 0x2e, 0x73, 0xc5}};
const GUID kDecoderProfileHevcMain =
    {0x5b11d51b, 0x2f4c, 0x4452, {0xbc, 0xc3, 0x09, 0xf2, 0xa1, 0x16, 0x0c, 0xc0}};
const GUID kDecoderProfileHevcMain10 =
    {0x107af0e0, 0xef1a, 0x4d19, {0xab, 0xa8, 0x67, 0xa1, 0x63, 0x07, 0x3d, 0x13}};
const GUID kDecoderProfileVp9Profile0 =
    {0x463707f8, 0xa1d0, 0x4585, {0x87, 0x6d, 0x83, 0xaa, 0x6d, 0x60, 0xb8, 0x9e}};
const GUID kDecoderProfileVp9Profile2_10Bit =
    {0xa4c749ef, 0x6ecf, 0x48aa, {0x84, 0x48, 0x50, 0xa7, 0xa1, 0x16, 0x5f, 0xf7}};
const GUID kDecoderProfileAv1Profile0 =
    {0xb8be4ccb, 0xcf53, 0x46ba, {0x8d, 0x59, 0xd6, 0xb8, 0xa6, 0xda, 0x5d, 0x2a}};
const GUID kDecoderProfileAv1Profile1 =
    {0x6936ff0f, 0x45b1, 0x4163, {0x9c, 0xc1, 0x64, 0x6e, 0xf6, 0x94, 0x61, 0x08}};
const GUID kDecoderProfileAv1Profile2 =
    {0x0c5f2aa1, 0xe541, 0x4089, {0xbb, 0x7b, 0x98, 0x11, 0x0a, 0x19, 0xd7, 0xc8}};
const GUID kDecoderProfileAv1Profile2_12Bit =
    {0x17127009, 0xa00f, 0x4ce1, {0x99, 0x4e, 0xbf, 0x40, 0x81, 0xf6, 0xf3, 0xf0}};
const GUID kDecoderProfileAv1Profile2_12Bit420 =
    {0x2d80bed6, 0x9cac, 0x4835, {0x9e, 0x91, 0x32, 0x7b, 0xbc, 0x4f, 0x9e, 0xe8}};

D3D11FormatSupportSnapshot queryFormatSupport(ID3D11Device* device, DXGI_FORMAT format) {
    D3D11FormatSupportSnapshot snapshot;
    if (!device) {
        snapshot.check_hr = static_cast<long>(E_POINTER);
        return snapshot;
    }

    UINT support = 0;
    const HRESULT hr = device->CheckFormatSupport(format, &support);
    snapshot.check_hr = static_cast<long>(hr);
    if (FAILED(hr)) {
        return snapshot;
    }

    snapshot.check_succeeded = true;
    snapshot.raw_support = support;
    snapshot.texture2d = (support & D3D11_FORMAT_SUPPORT_TEXTURE2D) != 0;
    snapshot.shader_sample = (support & D3D11_FORMAT_SUPPORT_SHADER_SAMPLE) != 0;
    snapshot.shader_load = (support & D3D11_FORMAT_SUPPORT_SHADER_LOAD) != 0;
    snapshot.decoder_output = (support & D3D11_FORMAT_SUPPORT_DECODER_OUTPUT) != 0;
    return snapshot;
}

std::string formatSupportSummary(DXGI_FORMAT format, const D3D11FormatSupportSnapshot& snapshot) {
    std::ostringstream oss;
    oss << dxgiFormatName(format);
    if (!snapshot.check_succeeded) {
        oss << "{check_failed_hr=" << snapshot.check_hr << '}';
        return oss.str();
    }

    oss << "{raw=0x" << std::hex << std::uppercase << snapshot.raw_support << std::dec
        << " texture2d=" << boolName(snapshot.texture2d)
        << " shader_sample=" << boolName(snapshot.shader_sample)
        << " shader_load=" << boolName(snapshot.shader_load)
        << " decoder_output=" << boolName(snapshot.decoder_output)
        << '}';
    return oss.str();
}

D3D11DecoderProfileSupport probeDecoderProfiles(ID3D11VideoDevice* video_device) {
    D3D11DecoderProfileSupport snapshot;
    if (!video_device) {
        return snapshot;
    }

    snapshot.enumeration_succeeded = true;
    snapshot.enumerated_profile_count = video_device->GetVideoDecoderProfileCount();
    for (UINT i = 0; i < snapshot.enumerated_profile_count; ++i) {
        GUID profile{};
        if (FAILED(video_device->GetVideoDecoderProfile(i, &profile))) {
            continue;
        }

        if (guidEquals(profile, kDecoderProfileH264VldNoFgt)) {
            snapshot.h264_vld_nofgt = true;
            continue;
        }
        if (guidEquals(profile, kDecoderProfileH264VldFgt)) {
            snapshot.h264_vld_fgt = true;
            continue;
        }
        if (guidEquals(profile, kDecoderProfileHevcMain)) {
            snapshot.hevc_main = true;
            continue;
        }
        if (guidEquals(profile, kDecoderProfileHevcMain10)) {
            snapshot.hevc_main10 = true;
            continue;
        }
        if (guidEquals(profile, kDecoderProfileVp9Profile0)) {
            snapshot.vp9_profile0 = true;
            continue;
        }
        if (guidEquals(profile, kDecoderProfileVp9Profile2_10Bit)) {
            snapshot.vp9_profile2_10bit = true;
            continue;
        }
        if (guidEquals(profile, kDecoderProfileAv1Profile0)) {
            snapshot.av1_profile0 = true;
            continue;
        }
        if (guidEquals(profile, kDecoderProfileAv1Profile1)) {
            snapshot.av1_profile1 = true;
            continue;
        }
        if (guidEquals(profile, kDecoderProfileAv1Profile2)) {
            snapshot.av1_profile2 = true;
            continue;
        }
        if (guidEquals(profile, kDecoderProfileAv1Profile2_12Bit)) {
            snapshot.av1_profile2_12bit = true;
            continue;
        }
        if (guidEquals(profile, kDecoderProfileAv1Profile2_12Bit420)) {
            snapshot.av1_profile2_12bit_420 = true;
            continue;
        }
    }

    return snapshot;
}

bool stringContainsCaseInsensitive(const std::string& text, const char* needle) {
    if (!needle || needle[0] == '\0') {
        return true;
    }

    std::string lhs(text);
    std::string rhs(needle);
    std::transform(lhs.begin(), lhs.end(), lhs.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    std::transform(rhs.begin(), rhs.end(), rhs.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return lhs.find(rhs) != std::string::npos;
}

bool stringStartsWith(const std::string& text, const char* prefix) {
    if (!prefix || prefix[0] == '\0') {
        return true;
    }
    return text.rfind(prefix, 0) == 0;
}

void applyNativeDirectStartupPolicy(D3D11DiagnosticsSnapshot& snapshot) {
    snapshot.native_direct_allowed = true;
    snapshot.native_direct_startup_disabled = false;
    snapshot.native_direct_disable_rule = "none";
    snapshot.native_direct_disable_reason.clear();

    if (!snapshot.probe_succeeded) {
        snapshot.native_direct_allowed = false;
        snapshot.native_direct_startup_disabled = true;
        snapshot.native_direct_disable_rule = "device_create_failed";
        snapshot.native_direct_disable_reason = "D3D11 device creation failed";
        return;
    }

    if (snapshot.software_adapter) {
        snapshot.native_direct_allowed = false;
        snapshot.native_direct_startup_disabled = true;
        snapshot.native_direct_disable_rule = "software_adapter";
        snapshot.native_direct_disable_reason =
            "software adapter is not a stable target for native direct D3D11 video surfaces";
        return;
    }

    if (!snapshot.has_device3) {
        snapshot.native_direct_allowed = false;
        snapshot.native_direct_startup_disabled = true;
        snapshot.native_direct_disable_rule = "missing_device3";
        snapshot.native_direct_disable_reason =
            "ID3D11Device3 is required for plane-sliced CreateShaderResourceView1";
        return;
    }

    if (!snapshot.has_video_device || !snapshot.has_video_context) {
        snapshot.native_direct_allowed = false;
        snapshot.native_direct_startup_disabled = true;
        snapshot.native_direct_disable_rule = "missing_video_interfaces";
        snapshot.native_direct_disable_reason =
            "D3D11 video decode interfaces are incomplete on this adapter/driver";
        return;
    }

    const bool nv12_ready = snapshot.nv12_support.check_succeeded &&
                            snapshot.nv12_support.texture2d &&
                            snapshot.nv12_support.shader_sample &&
                            snapshot.nv12_support.decoder_output;
    if (!nv12_ready) {
        snapshot.native_direct_allowed = false;
        snapshot.native_direct_startup_disabled = true;
        snapshot.native_direct_disable_rule = "nv12_capability_gap";
        snapshot.native_direct_disable_reason =
            "NV12 lacks texture2d/shader_sample/decoder_output support for native direct";
        return;
    }

    for (const NativeDirectBlacklistRule& rule : kNativeDirectBlacklistRules) {
        if (rule.vendor_id && snapshot.vendor_id != *rule.vendor_id) {
            continue;
        }
        if (rule.device_id && snapshot.device_id != *rule.device_id) {
            continue;
        }
        if (!stringContainsCaseInsensitive(snapshot.adapter_name, rule.adapter_name_substring)) {
            continue;
        }
        if (!stringStartsWith(snapshot.driver_version, rule.driver_version_prefix)) {
            continue;
        }
        snapshot.native_direct_allowed = false;
        snapshot.native_direct_startup_disabled = true;
        snapshot.native_direct_disable_rule = rule.rule_name;
        snapshot.native_direct_disable_reason = rule.reason;
        return;
    }
}

const char* dxgiColorSpaceName(DXGI_COLOR_SPACE_TYPE color_space) {
    switch (color_space) {
    case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:
        return "rgb_full_g22_p709";
    case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:
        return "rgb_full_g10_p709";
    case DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P709:
        return "rgb_studio_g22_p709";
    case DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020:
        return "rgb_studio_g22_p2020";
    case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
        return "rgb_full_pq_p2020";
    case DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020:
        return "rgb_studio_pq_p2020";
    case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020:
        return "rgb_full_g22_p2020";
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020:
        return "ycbcr_studio_pq_p2020";
    case DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020:
        return "ycbcr_studio_hlg_p2020";
    case DXGI_COLOR_SPACE_YCBCR_FULL_GHLG_TOPLEFT_P2020:
        return "ycbcr_full_hlg_p2020";
    default:
        return "unknown";
    }
}

bool dxgiColorSpaceHdrActive(DXGI_COLOR_SPACE_TYPE color_space) {
    return color_space == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 ||
           color_space == DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020 ||
           color_space == DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020 ||
           color_space == DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020 ||
           color_space == DXGI_COLOR_SPACE_YCBCR_FULL_GHLG_TOPLEFT_P2020;
}

bool dxgiColorSpaceAdvancedColorActive(DXGI_COLOR_SPACE_TYPE color_space) {
    return color_space == DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709 ||
           color_space == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 ||
           color_space == DXGI_COLOR_SPACE_RGB_STUDIO_G2084_NONE_P2020 ||
           color_space == DXGI_COLOR_SPACE_RGB_STUDIO_G22_NONE_P2020 ||
           color_space == DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020 ||
           color_space == DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020 ||
           color_space == DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020 ||
           color_space == DXGI_COLOR_SPACE_YCBCR_FULL_GHLG_TOPLEFT_P2020;
}

bool matchesD3D11DiagnosticsAdapter(const DXGI_ADAPTER_DESC1& desc, const D3D11DiagnosticsSnapshot& d3d11) {
    if (desc.VendorId != d3d11.vendor_id || desc.DeviceId != d3d11.device_id) {
        return false;
    }
    if (d3d11.subsystem_id != 0 && desc.SubSysId != d3d11.subsystem_id) {
        return false;
    }
    if (d3d11.revision != 0 && desc.Revision != d3d11.revision) {
        return false;
    }
    return true;
}

D3D11HdrOutputSnapshot probeD3D11HdrOutput(const D3D11DiagnosticsSnapshot& d3d11,
                                           const wchar_t* preferred_device_name = nullptr) {
    D3D11HdrOutputSnapshot snapshot;
    if (!d3d11.probe_succeeded) {
        snapshot.probe_error = "d3d11 diagnostics did not succeed";
        return snapshot;
    }

    ComPtr<IDXGIFactory1> factory;
    const HRESULT factory_hr = CreateDXGIFactory1(IID_PPV_ARGS(factory.GetAddressOf()));
    if (FAILED(factory_hr) || !factory) {
        snapshot.probe_error = "CreateDXGIFactory1 failed";
        return snapshot;
    }

    for (UINT adapter_index = 0;; ++adapter_index) {
        ComPtr<IDXGIAdapter1> adapter;
        const HRESULT adapter_hr = factory->EnumAdapters1(adapter_index, adapter.GetAddressOf());
        if (adapter_hr == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        if (FAILED(adapter_hr) || !adapter) {
            continue;
        }

        DXGI_ADAPTER_DESC1 adapter_desc{};
        if (FAILED(adapter->GetDesc1(&adapter_desc)) ||
            !matchesD3D11DiagnosticsAdapter(adapter_desc, d3d11)) {
            continue;
        }

        snapshot.adapter_matched = true;
        snapshot.adapter_index = adapter_index;

        ComPtr<IDXGIOutput> first_output;
        UINT first_output_index = 0;
        ComPtr<IDXGIOutput> matched_output;
        UINT matched_output_index = 0;
        for (UINT output_index = 0;; ++output_index) {
            ComPtr<IDXGIOutput> output;
            const HRESULT output_hr = adapter->EnumOutputs(output_index, output.GetAddressOf());
            if (output_hr == DXGI_ERROR_NOT_FOUND) {
                break;
            }
            if (FAILED(output_hr) || !output) {
                continue;
            }

            DXGI_OUTPUT_DESC output_desc{};
            if (FAILED(output->GetDesc(&output_desc))) {
                continue;
            }
            if (!first_output) {
                first_output = output;
                first_output_index = output_index;
            }
            if (preferred_device_name &&
                preferred_device_name[0] != L'\0' &&
                std::wcscmp(output_desc.DeviceName, preferred_device_name) == 0) {
                matched_output = output;
                matched_output_index = output_index;
                break;
            }
            if (!preferred_device_name && output_desc.AttachedToDesktop) {
                matched_output = output;
                matched_output_index = output_index;
                break;
            }
        }

        if (!matched_output) {
            matched_output = first_output;
            matched_output_index = first_output_index;
        }
        if (!matched_output) {
            snapshot.probe_succeeded = true;
            snapshot.probe_error = "matched adapter has no DXGI outputs";
            return snapshot;
        }

        snapshot.output_found = true;
        snapshot.output_index = matched_output_index;

        DXGI_OUTPUT_DESC output_desc{};
        if (SUCCEEDED(matched_output->GetDesc(&output_desc))) {
            snapshot.output_name = utf8FromWide(output_desc.DeviceName);
            snapshot.output_device_name = utf8FromWide(output_desc.DeviceName);
            snapshot.output_attached_to_desktop = output_desc.AttachedToDesktop;
        }

        ComPtr<IDXGIOutput6> output6;
        if (SUCCEEDED(matched_output.As(&output6)) && output6) {
            snapshot.has_output6 = true;
            DXGI_OUTPUT_DESC1 output_desc1{};
            if (SUCCEEDED(output6->GetDesc1(&output_desc1))) {
                snapshot.output_name = utf8FromWide(output_desc1.DeviceName);
                snapshot.output_device_name = utf8FromWide(output_desc1.DeviceName);
                snapshot.output_attached_to_desktop = output_desc1.AttachedToDesktop;
                snapshot.color_space = dxgiColorSpaceName(output_desc1.ColorSpace);
                snapshot.advanced_color_active = dxgiColorSpaceAdvancedColorActive(output_desc1.ColorSpace);
                snapshot.hdr_active = dxgiColorSpaceHdrActive(output_desc1.ColorSpace);
                snapshot.bits_per_color = output_desc1.BitsPerColor;
                snapshot.min_luminance_nits = static_cast<double>(output_desc1.MinLuminance);
                snapshot.max_luminance_nits = static_cast<double>(output_desc1.MaxLuminance);
                snapshot.max_full_frame_luminance_nits =
                    static_cast<double>(output_desc1.MaxFullFrameLuminance);
            }
        } else {
            snapshot.probe_error = "IDXGIOutput6 unavailable; HDR output capability details are limited";
        }

        snapshot.probe_succeeded = true;
        return snapshot;
    }

    snapshot.probe_error = "no DXGI adapter matched the D3D11 diagnostics adapter";
    return snapshot;
}

D3D11DiagnosticsSnapshot buildD3D11DiagnosticsSnapshot(const D3D11DeviceProbeContext& probe) {
    D3D11DiagnosticsSnapshot snapshot;
    snapshot.create_device_hr = static_cast<long>(probe.create_device_hr);
    snapshot.probe_succeeded = SUCCEEDED(probe.create_device_hr) && probe.device && probe.context;
    snapshot.feature_level = featureLevelName(probe.feature_level);
    snapshot.debug_layer_enabled = probe.debug_layer_enabled;
    snapshot.multithread_protected = probe.multithread_protected;
    snapshot.has_device3 = probe.device3 != nullptr;

    if (!snapshot.probe_succeeded) {
        applyNativeDirectStartupPolicy(snapshot);
        return snapshot;
    }

    ComPtr<IDXGIDevice> dxgi_device;
    ComPtr<IDXGIAdapter> adapter;
    ComPtr<IDXGIAdapter1> adapter1;
    if (SUCCEEDED(probe.device->QueryInterface(IID_PPV_ARGS(dxgi_device.GetAddressOf()))) &&
        dxgi_device &&
        SUCCEEDED(dxgi_device->GetAdapter(adapter.GetAddressOf())) &&
        adapter) {
        snapshot.driver_version = adapterDriverVersionString(adapter.Get());
        if (SUCCEEDED(adapter.As(&adapter1)) && adapter1) {
            DXGI_ADAPTER_DESC1 desc1{};
            if (SUCCEEDED(adapter1->GetDesc1(&desc1))) {
                snapshot.adapter_name = utf8FromWide(desc1.Description);
                snapshot.vendor_id = desc1.VendorId;
                snapshot.device_id = desc1.DeviceId;
                snapshot.subsystem_id = desc1.SubSysId;
                snapshot.revision = desc1.Revision;
                snapshot.dedicated_video_mib = bytesToMiB(desc1.DedicatedVideoMemory);
                snapshot.dedicated_system_mib = bytesToMiB(desc1.DedicatedSystemMemory);
                snapshot.shared_system_mib = bytesToMiB(desc1.SharedSystemMemory);
                snapshot.software_adapter = (desc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
            }
        } else {
            DXGI_ADAPTER_DESC desc{};
            if (SUCCEEDED(adapter->GetDesc(&desc))) {
                snapshot.adapter_name = utf8FromWide(desc.Description);
                snapshot.vendor_id = desc.VendorId;
                snapshot.device_id = desc.DeviceId;
                snapshot.subsystem_id = desc.SubSysId;
                snapshot.revision = desc.Revision;
                snapshot.dedicated_video_mib = bytesToMiB(desc.DedicatedVideoMemory);
                snapshot.dedicated_system_mib = bytesToMiB(desc.DedicatedSystemMemory);
                snapshot.shared_system_mib = bytesToMiB(desc.SharedSystemMemory);
            }
        }
    }

    ComPtr<ID3D11VideoDevice> video_device;
    ComPtr<ID3D11VideoContext> video_context;
    snapshot.has_video_device =
        SUCCEEDED(probe.device->QueryInterface(IID_PPV_ARGS(video_device.GetAddressOf()))) && video_device;
    snapshot.has_video_context =
        SUCCEEDED(probe.context->QueryInterface(IID_PPV_ARGS(video_context.GetAddressOf()))) && video_context;
    snapshot.nv12_support = queryFormatSupport(probe.device.Get(), DXGI_FORMAT_NV12);
    snapshot.p010_support = queryFormatSupport(probe.device.Get(), DXGI_FORMAT_P010);
    snapshot.p016_support = queryFormatSupport(probe.device.Get(), DXGI_FORMAT_P016);
    snapshot.decoder_profiles = probeDecoderProfiles(video_device.Get());
    applyNativeDirectStartupPolicy(snapshot);
    snapshot.hdr_output = probeD3D11HdrOutput(snapshot);
    return snapshot;
}

std::string decoderProfileSummary(const D3D11DecoderProfileSupport& snapshot) {
    std::ostringstream oss;
    oss << "[diag:d3d11-init] decoder_profiles"
        << " enumeration_succeeded=" << boolName(snapshot.enumeration_succeeded)
        << " enumerated_profile_count=" << snapshot.enumerated_profile_count
        << " h264_vld_nofgt=" << boolName(snapshot.h264_vld_nofgt)
        << " h264_vld_fgt=" << boolName(snapshot.h264_vld_fgt)
        << " hevc_main=" << boolName(snapshot.hevc_main)
        << " hevc_main10=" << boolName(snapshot.hevc_main10)
        << " vp9_profile0=" << boolName(snapshot.vp9_profile0)
        << " vp9_profile2_10bit=" << boolName(snapshot.vp9_profile2_10bit)
        << " av1_profile0=" << boolName(snapshot.av1_profile0)
        << " av1_profile1=" << boolName(snapshot.av1_profile1)
        << " av1_profile2=" << boolName(snapshot.av1_profile2)
        << " av1_profile2_12bit=" << boolName(snapshot.av1_profile2_12bit)
        << " av1_profile2_12bit_420=" << boolName(snapshot.av1_profile2_12bit_420);
    return oss.str();
}

void logD3D11StartupDiagnostics(const D3D11DiagnosticsSnapshot& snapshot) {
    std::ostringstream adapter_log;
    adapter_log << "[diag:d3d11-init] adapter=\"" << snapshot.adapter_name
                << "\" vendor_id=0x" << std::hex << std::uppercase << snapshot.vendor_id
                << " device_id=0x" << snapshot.device_id
                << " subsystem_id=0x" << snapshot.subsystem_id
                << std::dec << " revision=" << snapshot.revision
                << " driver_version=" << snapshot.driver_version
                << " software_adapter=" << boolName(snapshot.software_adapter)
                << " dedicated_video_mib=" << snapshot.dedicated_video_mib
                << " dedicated_system_mib=" << snapshot.dedicated_system_mib
                << " shared_system_mib=" << snapshot.shared_system_mib;
    LOG_INFO(adapter_log.str());

    std::ostringstream device_log;
    device_log << "[diag:d3d11-init] device feature_level=" << snapshot.feature_level
               << " debug_layer=" << boolName(snapshot.debug_layer_enabled)
               << " multithread_protected=" << boolName(snapshot.multithread_protected)
               << " device3=" << boolName(snapshot.has_device3)
               << " video_device=" << boolName(snapshot.has_video_device)
               << " video_context=" << boolName(snapshot.has_video_context);
    LOG_INFO(device_log.str());

    LOG_INFO(std::string("[diag:d3d11-init] format_support ") +
             formatSupportSummary(DXGI_FORMAT_NV12, snapshot.nv12_support) + " " +
             formatSupportSummary(DXGI_FORMAT_P010, snapshot.p010_support) + " " +
             formatSupportSummary(DXGI_FORMAT_P016, snapshot.p016_support));
    LOG_INFO(decoderProfileSummary(snapshot.decoder_profiles));

    std::ostringstream native_direct_log;
    native_direct_log << "[diag:d3d11-init] native_direct startup_allowed="
                      << boolName(snapshot.native_direct_allowed)
                      << " startup_disabled=" << boolName(snapshot.native_direct_startup_disabled)
                      << " rule=" << snapshot.native_direct_disable_rule;
    if (!snapshot.native_direct_disable_reason.empty()) {
        native_direct_log << " reason=\"" << snapshot.native_direct_disable_reason << '"';
    }
    if (!snapshot.native_direct_allowed) {
        native_direct_log << " fallback=copyback-to-software";
        LOG_WARNING(native_direct_log.str());
    } else {
        LOG_INFO(native_direct_log.str());
    }

    std::ostringstream hdr_log;
    hdr_log << "[diag:d3d11-hdr] probe_succeeded=" << boolName(snapshot.hdr_output.probe_succeeded)
            << " adapter_matched=" << boolName(snapshot.hdr_output.adapter_matched)
            << " output_found=" << boolName(snapshot.hdr_output.output_found)
            << " output=\"" << snapshot.hdr_output.output_name << "\""
            << " color_space=" << snapshot.hdr_output.color_space
            << " advanced_color_active=" << boolName(snapshot.hdr_output.advanced_color_active)
            << " hdr_active=" << boolName(snapshot.hdr_output.hdr_active)
            << " bits_per_color=" << snapshot.hdr_output.bits_per_color
            << " min_luminance_nits=" << snapshot.hdr_output.min_luminance_nits
            << " max_luminance_nits=" << snapshot.hdr_output.max_luminance_nits
            << " max_full_frame_luminance_nits=" << snapshot.hdr_output.max_full_frame_luminance_nits;
    if (!snapshot.hdr_output.probe_error.empty()) {
        hdr_log << " note=\"" << snapshot.hdr_output.probe_error << '"';
    }
    LOG_INFO(hdr_log.str());
}

D3D11DeviceProbeContext createD3D11DeviceProbeContext() {
    D3D11DeviceProbeContext probe;
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(DEBUG_MODE)
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    const D3D_FEATURE_LEVEL requested_levels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};
    probe.feature_level = D3D_FEATURE_LEVEL_11_0;
    probe.create_device_hr = D3D11CreateDevice(nullptr,
                                               D3D_DRIVER_TYPE_HARDWARE,
                                               nullptr,
                                               flags,
                                               requested_levels,
                                               2u,
                                               D3D11_SDK_VERSION,
                                               probe.device.GetAddressOf(),
                                               &probe.feature_level,
                                               probe.context.GetAddressOf());
#if defined(DEBUG_MODE)
    if (FAILED(probe.create_device_hr)) {
        flags &= ~D3D11_CREATE_DEVICE_DEBUG;
        probe.create_device_hr = D3D11CreateDevice(nullptr,
                                                   D3D_DRIVER_TYPE_HARDWARE,
                                                   nullptr,
                                                   flags,
                                                   requested_levels,
                                                   2u,
                                                   D3D11_SDK_VERSION,
                                                   probe.device.ReleaseAndGetAddressOf(),
                                                   &probe.feature_level,
                                                   probe.context.ReleaseAndGetAddressOf());
    }
#endif
    probe.debug_layer_enabled = (flags & D3D11_CREATE_DEVICE_DEBUG) != 0;
    if (FAILED(probe.create_device_hr) || !probe.device || !probe.context) {
        return probe;
    }

    probe.device.As(&probe.device3);
    ComPtr<ID3D11Multithread> multithread;
    if (SUCCEEDED(probe.context.As(&multithread)) && multithread) {
        multithread->SetMultithreadProtected(TRUE);
        probe.multithread_protected = true;
    }
    return probe;
}

void logD3D11SwapChainDiagnostics(const DXGI_SWAP_CHAIN_DESC1& desc) {
    std::ostringstream oss;
    oss << "[diag:d3d11-init] swap_chain width=" << desc.Width
        << " height=" << desc.Height
        << " format=" << dxgiFormatName(desc.Format)
        << " buffer_count=" << desc.BufferCount
        << " sample_count=" << desc.SampleDesc.Count
        << " swap_effect=" << swapEffectName(desc.SwapEffect)
        << " alpha_mode=" << alphaModeName(desc.AlphaMode)
        << " usage=0x" << std::hex << std::uppercase << desc.BufferUsage;
    LOG_INFO(oss.str());
}

struct WindowSize {
    int width;
    int height;
};

struct VideoVertex {
    float x;
    float y;
    float u;
    float v;
};

struct ColorVertex {
    float x;
    float y;
    float r;
    float g;
    float b;
    float a;
};

struct Color {
    float r;
    float g;
    float b;
    float a;
};

struct ColorMatrixConstants {
    float coeff_r[4];
    float coeff_g[4];
    float coeff_b[4];
    float color_config0[4];
};

WindowSize computeInitialWindowSize(int video_width, int video_height) {
    const int safe_video_width = std::max(1, video_width);
    const int safe_video_height = std::max(1, video_height);
    WindowSize result{safe_video_width, safe_video_height};

    SDL_Rect usable_bounds{};
    if (SDL_GetDisplayUsableBounds(0, &usable_bounds) != 0 || usable_bounds.w <= 0 || usable_bounds.h <= 0) {
        return result;
    }

    constexpr double kMaxDisplayRatio = 0.9;
    const double max_width = static_cast<double>(usable_bounds.w) * kMaxDisplayRatio;
    const double max_height = static_cast<double>(usable_bounds.h) * kMaxDisplayRatio;
    const double width_scale = max_width / static_cast<double>(safe_video_width);
    const double height_scale = max_height / static_cast<double>(safe_video_height);
    const double scale = std::min(1.0, std::min(width_scale, height_scale));
    result.width = std::max(1, static_cast<int>(std::lround(static_cast<double>(safe_video_width) * scale)));
    result.height = std::max(1, static_cast<int>(std::lround(static_cast<double>(safe_video_height) * scale)));
    return result;
}

void updateWindowSizeFromSdl(SDL_Window* window, std::atomic<int>& width, std::atomic<int>& height) {
    if (!window) {
        return;
    }
    int window_width = width.load();
    int window_height = height.load();
    SDL_GetWindowSize(window, &window_width, &window_height);
    width.store(std::max(1, window_width));
    height.store(std::max(1, window_height));
}

SDL_Rect computeRenderRect(int window_width, int window_height, int frame_width, int frame_height) {
    SDL_Rect dst_rect{0, 0, std::max(1, window_width), std::max(1, window_height)};
    if (window_width <= 0 || window_height <= 0 || frame_width <= 0 || frame_height <= 0) {
        return dst_rect;
    }

    const double src_aspect = static_cast<double>(frame_width) / static_cast<double>(frame_height);
    const double dst_aspect = static_cast<double>(window_width) / static_cast<double>(window_height);
    if (dst_aspect > src_aspect) {
        dst_rect.h = window_height;
        dst_rect.w = std::max(1, static_cast<int>(std::lround(static_cast<double>(window_height) * src_aspect)));
        dst_rect.x = (window_width - dst_rect.w) / 2;
        dst_rect.y = 0;
    } else {
        dst_rect.w = window_width;
        dst_rect.h = std::max(1, static_cast<int>(std::lround(static_cast<double>(window_width) / src_aspect)));
        dst_rect.x = 0;
        dst_rect.y = (window_height - dst_rect.h) / 2;
    }
    return dst_rect;
}

double clampRatio(double value) {
    return std::max(0.0, std::min(1.0, value));
}

float clampVolume(float value) {
    return std::max(0.0f, std::min(1.0f, value));
}

bool pointInRect(int x, int y, const SDL_Rect& rect) {
    return x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
}

bool isLimitedRange(const AVFrame* frame) {
    return !frame || frame->color_range != AVCOL_RANGE_JPEG;
}

enum class ColorMatrixKind {
    Bt709 = 0,
    Bt601 = 1,
    Bt2020 = 2,
};

enum class ColorTransferMode {
    Sdr = 0,
    Pq = 1,
    Hlg = 2,
};

enum class ColorGamutMode {
    None = 0,
    Bt2020ToBt709 = 1,
};

enum class D3D11HdrPresentMode {
    Auto = 0,
    Off = 1,
    Force = 2,
};

ColorMatrixKind colorMatrixKind(const AVFrame* frame) {
    if (!frame) {
        return ColorMatrixKind::Bt709;
    }
    switch (frame->colorspace) {
    case AVCOL_SPC_BT470BG:
    case AVCOL_SPC_SMPTE170M:
    case AVCOL_SPC_FCC:
        return ColorMatrixKind::Bt601;
    case AVCOL_SPC_BT2020_CL:
    case AVCOL_SPC_BT2020_NCL:
        return ColorMatrixKind::Bt2020;
    default:
        return ColorMatrixKind::Bt709;
    }
}

bool useBt2020GamutMapping(const AVFrame* frame) {
    if (!frame) {
        return false;
    }
    return frame->colorspace == AVCOL_SPC_BT2020_CL ||
           frame->colorspace == AVCOL_SPC_BT2020_NCL ||
           frame->color_primaries == AVCOL_PRI_BT2020;
}

bool isPqTransfer(AVColorTransferCharacteristic transfer) {
    return transfer == AVCOL_TRC_SMPTE2084;
}

bool isHlgTransfer(AVColorTransferCharacteristic transfer) {
    return transfer == AVCOL_TRC_ARIB_STD_B67;
}

bool isHdrTransfer(AVColorTransferCharacteristic transfer) {
    return isPqTransfer(transfer) || isHlgTransfer(transfer);
}

const char* transferName(AVColorTransferCharacteristic transfer) {
    switch (transfer) {
    case AVCOL_TRC_SMPTE2084:
        return "smpte2084";
    case AVCOL_TRC_ARIB_STD_B67:
        return "arib-std-b67";
    case AVCOL_TRC_BT709:
        return "bt709";
    case AVCOL_TRC_BT2020_10:
        return "bt2020-10";
    case AVCOL_TRC_BT2020_12:
        return "bt2020-12";
    default:
        return "unspecified";
    }
}

ColorMatrixConstants buildColorMatrix(const AVFrame* frame,
                                      bool high_bit_depth,
                                      bool hdr_output_active) {
    const bool limited = isLimitedRange(frame);
    const float y_bias = limited ? (high_bit_depth ? (-64.0f / 1023.0f) : (-16.0f / 255.0f)) : 0.0f;
    const float uv_bias = limited ? (high_bit_depth ? (-512.0f / 1023.0f) : (-128.0f / 255.0f)) : -0.5f;
    const float y_scale = limited ? (high_bit_depth ? (1023.0f / 876.0f) : (255.0f / 219.0f)) : 1.0f;
    float rv = 1.792741f;
    float gu = -0.213249f;
    float gv = -0.532909f;
    float bu = 2.112402f;
    switch (colorMatrixKind(frame)) {
    case ColorMatrixKind::Bt601:
        rv = 1.596027f;
        gu = -0.391762f;
        gv = -0.812968f;
        bu = 2.017232f;
        break;
    case ColorMatrixKind::Bt2020:
        rv = 1.474600f;
        gu = -0.164553f;
        gv = -0.571353f;
        bu = 1.881400f;
        break;
    case ColorMatrixKind::Bt709:
    default:
        break;
    }

    ColorMatrixConstants constants{};
    constants.coeff_r[0] = y_scale;
    constants.coeff_r[2] = rv;
    constants.coeff_r[3] = y_scale * y_bias + rv * uv_bias;
    constants.coeff_g[0] = y_scale;
    constants.coeff_g[1] = gu;
    constants.coeff_g[2] = gv;
    constants.coeff_g[3] = y_scale * y_bias + (gu + gv) * uv_bias;
    constants.coeff_b[0] = y_scale;
    constants.coeff_b[1] = bu;
    constants.coeff_b[3] = y_scale * y_bias + bu * uv_bias;
    constants.color_config0[0] = isPqTransfer(frame ? frame->color_trc : AVCOL_TRC_UNSPECIFIED)
                                     ? static_cast<float>(ColorTransferMode::Pq)
                                     : (isHlgTransfer(frame ? frame->color_trc : AVCOL_TRC_UNSPECIFIED)
                                            ? static_cast<float>(ColorTransferMode::Hlg)
                                            : static_cast<float>(ColorTransferMode::Sdr));
    constants.color_config0[1] = useBt2020GamutMapping(frame)
                                     ? static_cast<float>(ColorGamutMode::Bt2020ToBt709)
                                     : static_cast<float>(ColorGamutMode::None);
    constants.color_config0[2] = hdr_output_active ? 1.0f : 0.0f;
    return constants;
}

std::optional<std::string> readEnvVar(const char* key) {
    if (!key || key[0] == '\0') {
        return std::nullopt;
    }
#if defined(_WIN32)
    char* value = nullptr;
    size_t length = 0;
    if (_dupenv_s(&value, &length, key) != 0 || !value) {
        return std::nullopt;
    }
    std::string result(value);
    std::free(value);
    return result;
#else
    if (const char* value = std::getenv(key)) {
        return std::string(value);
    }
    return std::nullopt;
#endif
}

std::string toLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

D3D11HdrPresentMode readD3D11HdrPresentMode() {
    const auto value = readEnvVar("MVP_D3D11_HDR_OUTPUT_MODE");
    if (!value) {
        return D3D11HdrPresentMode::Auto;
    }
    const std::string normalized = toLowerAscii(*value);
    if (normalized == "0" || normalized == "off" || normalized == "false" ||
        normalized == "disable" || normalized == "disabled" || normalized == "sdr") {
        return D3D11HdrPresentMode::Off;
    }
    if (normalized == "1" || normalized == "on" || normalized == "true" ||
        normalized == "force" || normalized == "forced" || normalized == "hdr") {
        return D3D11HdrPresentMode::Force;
    }
    return D3D11HdrPresentMode::Auto;
}

const char* d3d11HdrPresentModeName(D3D11HdrPresentMode value) {
    switch (value) {
    case D3D11HdrPresentMode::Off:
        return "off";
    case D3D11HdrPresentMode::Force:
        return "force";
    case D3D11HdrPresentMode::Auto:
    default:
        return "auto";
    }
}

uint16_t encodeChromaticityCoordinate(const AVRational& value) {
    const double normalized = av_q2d(value);
    const long encoded = std::lround(std::clamp(normalized, 0.0, 1.0) * 50000.0);
    return static_cast<uint16_t>(std::clamp<long>(encoded, 0, 50000));
}

uint32_t encodeHdr10MaxLuminance(const AVRational& value) {
    const double nits = av_q2d(value);
    const long long encoded = std::llround(std::max(0.0, nits));
    return static_cast<uint32_t>(std::min<long long>(encoded, std::numeric_limits<uint32_t>::max()));
}

uint32_t encodeHdr10MinLuminance(const AVRational& value) {
    const double nits = av_q2d(value);
    const long long encoded = std::llround(std::max(0.0, nits) * 10000.0);
    return static_cast<uint32_t>(std::min<long long>(encoded, std::numeric_limits<uint32_t>::max()));
}

bool extractHdr10Metadata(const AVFrame* frame,
                          DXGI_HDR_METADATA_HDR10& out_metadata,
                          std::string& source) {
    std::memset(&out_metadata, 0, sizeof(out_metadata));
    source = "none";
    if (!frame) {
        return false;
    }

    const AVFrameSideData* mastering_side_data =
        av_frame_get_side_data(frame, AV_FRAME_DATA_MASTERING_DISPLAY_METADATA);
    const AVFrameSideData* content_light_side_data =
        av_frame_get_side_data(frame, AV_FRAME_DATA_CONTENT_LIGHT_LEVEL);
    const auto* mastering = mastering_side_data
        ? reinterpret_cast<const AVMasteringDisplayMetadata*>(mastering_side_data->data)
        : nullptr;
    const auto* content_light = content_light_side_data
        ? reinterpret_cast<const AVContentLightMetadata*>(content_light_side_data->data)
        : nullptr;

    bool any_metadata = false;
    if (mastering && mastering->has_primaries) {
        out_metadata.RedPrimary[0] = encodeChromaticityCoordinate(mastering->display_primaries[0][0]);
        out_metadata.RedPrimary[1] = encodeChromaticityCoordinate(mastering->display_primaries[0][1]);
        out_metadata.GreenPrimary[0] = encodeChromaticityCoordinate(mastering->display_primaries[1][0]);
        out_metadata.GreenPrimary[1] = encodeChromaticityCoordinate(mastering->display_primaries[1][1]);
        out_metadata.BluePrimary[0] = encodeChromaticityCoordinate(mastering->display_primaries[2][0]);
        out_metadata.BluePrimary[1] = encodeChromaticityCoordinate(mastering->display_primaries[2][1]);
        out_metadata.WhitePoint[0] = encodeChromaticityCoordinate(mastering->white_point[0]);
        out_metadata.WhitePoint[1] = encodeChromaticityCoordinate(mastering->white_point[1]);
        any_metadata = true;
    }
    if (mastering && mastering->has_luminance) {
        out_metadata.MaxMasteringLuminance = encodeHdr10MaxLuminance(mastering->max_luminance);
        out_metadata.MinMasteringLuminance = encodeHdr10MinLuminance(mastering->min_luminance);
        any_metadata = true;
    }
    if (content_light) {
        out_metadata.MaxContentLightLevel = content_light->MaxCLL;
        out_metadata.MaxFrameAverageLightLevel = content_light->MaxFALL;
        any_metadata = true;
    }

    if (any_metadata) {
        source = "frame-side-data";
    }
    return any_metadata;
}

HRESULT compileShader(const char* source, const char* entry, const char* profile, ComPtr<ID3DBlob>& blob) {
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(DEBUG_MODE)
    flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    ComPtr<ID3DBlob> errors;
    const HRESULT hr = D3DCompile(source, std::strlen(source), nullptr, nullptr, nullptr, entry, profile, flags, 0,
                                  blob.GetAddressOf(), errors.GetAddressOf());
    if (FAILED(hr) && errors) {
        LOG_ERROR("D3D11 shader compile failed: " << static_cast<const char*>(errors->GetBufferPointer()));
    }
    return hr;
}

std::wstring decodeSubtitleText(const std::string& text) {
    if (text.empty()) {
        return {};
    }

    auto convert = [&](UINT code_page, DWORD flags) -> std::wstring {
        const int wide_length = MultiByteToWideChar(code_page,
                                                    flags,
                                                    text.data(),
                                                    static_cast<int>(text.size()),
                                                    nullptr,
                                                    0);
        if (wide_length <= 0) {
            return {};
        }
        std::wstring wide(static_cast<size_t>(wide_length), L'\0');
        const int converted = MultiByteToWideChar(code_page,
                                                  flags,
                                                  text.data(),
                                                  static_cast<int>(text.size()),
                                                  wide.data(),
                                                  wide_length);
        if (converted != wide_length) {
            return {};
        }
        return wide;
    };

    std::wstring wide = convert(CP_UTF8, MB_ERR_INVALID_CHARS);
    if (!wide.empty()) {
        return wide;
    }
    return convert(CP_ACP, 0);
}

size_t countUtf16CodeUnits(const std::string& text) {
    size_t count = 0;
    for (size_t i = 0; i < text.size();) {
        const unsigned char ch = static_cast<unsigned char>(text[i]);
        uint32_t codepoint = 0;
        size_t sequence_length = 1;

        if ((ch & 0x80u) == 0) {
            codepoint = ch;
        } else if ((ch & 0xE0u) == 0xC0u && i + 1 < text.size()) {
            codepoint = (static_cast<uint32_t>(ch & 0x1Fu) << 6u) |
                        static_cast<uint32_t>(static_cast<unsigned char>(text[i + 1]) & 0x3Fu);
            sequence_length = 2;
        } else if ((ch & 0xF0u) == 0xE0u && i + 2 < text.size()) {
            codepoint = (static_cast<uint32_t>(ch & 0x0Fu) << 12u) |
                        (static_cast<uint32_t>(static_cast<unsigned char>(text[i + 1]) & 0x3Fu) << 6u) |
                        static_cast<uint32_t>(static_cast<unsigned char>(text[i + 2]) & 0x3Fu);
            sequence_length = 3;
        } else if ((ch & 0xF8u) == 0xF0u && i + 3 < text.size()) {
            codepoint = (static_cast<uint32_t>(ch & 0x07u) << 18u) |
                        (static_cast<uint32_t>(static_cast<unsigned char>(text[i + 1]) & 0x3Fu) << 12u) |
                        (static_cast<uint32_t>(static_cast<unsigned char>(text[i + 2]) & 0x3Fu) << 6u) |
                        static_cast<uint32_t>(static_cast<unsigned char>(text[i + 3]) & 0x3Fu);
            sequence_length = 4;
        } else {
            codepoint = 0xFFFDu;
        }

        count += codepoint > 0xFFFFu ? 2u : 1u;
        i += sequence_length;
    }
    return count;
}

subtitle::SubtitleStyle makePlainSubtitleStyle() {
    subtitle::SubtitleStyle style;
    style.style_name = "plain";
    style.font_family = "Microsoft YaHei UI";
    style.font_size = 36.0;
    style.bold = true;
    style.primary_color = subtitle::SubtitleColor(255, 255, 255, 250);
    style.secondary_color = subtitle::SubtitleColor(255, 210, 64, 250);
    style.outline_color = subtitle::SubtitleColor(0, 0, 0, 255);
    style.background_color = subtitle::SubtitleColor(5, 5, 5, 148);
    style.alignment = 2;
    style.margin_l = 24;
    style.margin_r = 24;
    style.margin_v = 24;
    style.border_style = 3;
    style.outline = 0.0;
    style.shadow = 2.0;
    style.outline_x = 0.0;
    style.outline_y = 0.0;
    style.shadow_x = 2.0;
    style.shadow_y = 2.0;
    return style;
}

int clampSubtitleAlignment(int alignment) {
    return alignment >= 1 && alignment <= 9 ? alignment : 2;
}

int subtitleHorizontalGroup(int alignment) {
    return (clampSubtitleAlignment(alignment) - 1) % 3;
}

int subtitleVerticalGroup(int alignment) {
    return (clampSubtitleAlignment(alignment) - 1) / 3;
}

DWRITE_TEXT_ALIGNMENT toTextAlignment(int alignment) {
    switch (subtitleHorizontalGroup(alignment)) {
    case 0:
        return DWRITE_TEXT_ALIGNMENT_LEADING;
    case 2:
        return DWRITE_TEXT_ALIGNMENT_TRAILING;
    default:
        return DWRITE_TEXT_ALIGNMENT_CENTER;
    }
}

D2D1_COLOR_F toD2DColor(const subtitle::SubtitleColor& color) {
    return D2D1::ColorF(static_cast<float>(color.r) / 255.0f,
                        static_cast<float>(color.g) / 255.0f,
                        static_cast<float>(color.b) / 255.0f,
                        static_cast<float>(color.a) / 255.0f);
}

subtitle::SubtitleColor applySubtitleOpacity(const subtitle::SubtitleColor& color, float opacity) {
    subtitle::SubtitleColor adjusted = color;
    const int scaled_alpha = static_cast<int>(std::lround(static_cast<double>(color.a) *
                                                          std::clamp(opacity, 0.0f, 1.0f)));
    adjusted.a = static_cast<uint8_t>(std::clamp(scaled_alpha, 0, 255));
    return adjusted;
}

bool subtitleAnimationHasMotion(const subtitle::SubtitleStyleAnimation& animation) {
    if (!animation.has_move) {
        return false;
    }
    if (std::abs(animation.move_x2 - animation.move_x1) <= 0.001 &&
        std::abs(animation.move_y2 - animation.move_y1) <= 0.001) {
        return false;
    }
    return !animation.move_has_timing || animation.move_t2_seconds > animation.move_t1_seconds;
}

bool subtitleAnimationHasFade(const subtitle::SubtitleStyleAnimation& animation) {
    switch (animation.fade_mode) {
    case subtitle::SubtitleFadeMode::Simple:
        return animation.fade_in_seconds > 0.0005 || animation.fade_out_seconds > 0.0005;
    case subtitle::SubtitleFadeMode::Complex:
        return animation.fade_alpha1 != animation.fade_alpha2 ||
               animation.fade_alpha2 != animation.fade_alpha3 ||
               animation.fade_t2_seconds > animation.fade_t1_seconds ||
               animation.fade_t4_seconds > animation.fade_t3_seconds;
    case subtitle::SubtitleFadeMode::None:
    default:
        return false;
    }
}

bool subtitleAnimationHasStyleTransitions(const subtitle::SubtitleStyleAnimation& animation) {
    return std::any_of(animation.style_transitions.begin(),
                       animation.style_transitions.end(),
                       [](const subtitle::SubtitleStyleTransition& transition) {
                           return transition.property_mask != 0;
                       });
}

struct SubtitleAnimationState {
    bool has_position{false};
    double position_x{0.0};
    double position_y{0.0};
    float opacity{1.0f};
};

float evaluateComplexFadeOpacity(const subtitle::SubtitleStyleAnimation& animation, double item_elapsed_seconds) {
    const double a1 = static_cast<double>(animation.fade_alpha1);
    const double a2 = static_cast<double>(animation.fade_alpha2);
    const double a3 = static_cast<double>(animation.fade_alpha3);
    double value = a3;
    if (item_elapsed_seconds <= animation.fade_t1_seconds) {
        value = a1;
    } else if (item_elapsed_seconds < animation.fade_t2_seconds && animation.fade_t2_seconds > animation.fade_t1_seconds) {
        const double progress = (item_elapsed_seconds - animation.fade_t1_seconds) /
                                (animation.fade_t2_seconds - animation.fade_t1_seconds);
        value = a1 + (a2 - a1) * std::clamp(progress, 0.0, 1.0);
    } else if (item_elapsed_seconds <= animation.fade_t3_seconds) {
        value = a2;
    } else if (item_elapsed_seconds < animation.fade_t4_seconds && animation.fade_t4_seconds > animation.fade_t3_seconds) {
        const double progress = (item_elapsed_seconds - animation.fade_t3_seconds) /
                                (animation.fade_t4_seconds - animation.fade_t3_seconds);
        value = a2 + (a3 - a2) * std::clamp(progress, 0.0, 1.0);
    }
    return static_cast<float>(std::clamp(value / 255.0, 0.0, 1.0));
}

float evaluateStyleTransitionProgress(const subtitle::SubtitleStyleTransition& transition,
                                      double item_elapsed_seconds,
                                      double item_duration_seconds) {
    const double start_seconds = transition.has_timing ? transition.start_seconds : 0.0;
    const double end_seconds = transition.has_timing ? transition.end_seconds : item_duration_seconds;
    if (item_elapsed_seconds <= start_seconds) {
        return 0.0f;
    }
    if (end_seconds <= start_seconds + 0.0005) {
        return item_elapsed_seconds >= end_seconds ? 1.0f : 0.0f;
    }
    const double linear = std::clamp((item_elapsed_seconds - start_seconds) / (end_seconds - start_seconds), 0.0, 1.0);
    return static_cast<float>(std::pow(linear, std::max(0.01, transition.accel)));
}

uint8_t lerpByte(uint8_t from, uint8_t to, float progress) {
    const float blended = static_cast<float>(from) + (static_cast<float>(to) - static_cast<float>(from)) * progress;
    long value = std::lround(blended);
    value = std::max<long>(0, std::min<long>(255, value));
    return static_cast<uint8_t>(value);
}

subtitle::SubtitleColor lerpSubtitleColor(const subtitle::SubtitleColor& from,
                                          const subtitle::SubtitleColor& to,
                                          float progress) {
    return subtitle::SubtitleColor(
        lerpByte(from.r, to.r, progress),
        lerpByte(from.g, to.g, progress),
        lerpByte(from.b, to.b, progress),
        lerpByte(from.a, to.a, progress));
}

double lerpDouble(double from, double to, float progress) {
    return from + (to - from) * static_cast<double>(progress);
}

void applyStyleTransition(subtitle::SubtitleStyle& style,
                          const subtitle::SubtitleStyleTransition& transition,
                          float progress) {
    if (progress <= 0.0001f || transition.property_mask == 0) {
        return;
    }

    const subtitle::SubtitleStyle& target = transition.target_style;
    if ((transition.property_mask & subtitle::kSubtitleTransitionFontSize) != 0) {
        style.font_size = lerpDouble(style.font_size, target.font_size, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionPrimaryColor) != 0) {
        style.primary_color = lerpSubtitleColor(style.primary_color, target.primary_color, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionSecondaryColor) != 0) {
        style.secondary_color = lerpSubtitleColor(style.secondary_color, target.secondary_color, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionOutlineColor) != 0) {
        style.outline_color = lerpSubtitleColor(style.outline_color, target.outline_color, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionBackgroundColor) != 0) {
        style.background_color = lerpSubtitleColor(style.background_color, target.background_color, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionOutlineX) != 0) {
        style.outline_x = lerpDouble(style.outline_x, target.outline_x, progress);
        style.outline = (style.outline_x + style.outline_y) * 0.5;
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionOutlineY) != 0) {
        style.outline_y = lerpDouble(style.outline_y, target.outline_y, progress);
        style.outline = (style.outline_x + style.outline_y) * 0.5;
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionShadowX) != 0) {
        style.shadow_x = lerpDouble(style.shadow_x, target.shadow_x, progress);
        style.shadow = (std::abs(style.shadow_x) + std::abs(style.shadow_y)) * 0.5;
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionShadowY) != 0) {
        style.shadow_y = lerpDouble(style.shadow_y, target.shadow_y, progress);
        style.shadow = (std::abs(style.shadow_x) + std::abs(style.shadow_y)) * 0.5;
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionSpacing) != 0) {
        style.spacing = lerpDouble(style.spacing, target.spacing, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionScaleX) != 0) {
        style.scale_x_percent = lerpDouble(style.scale_x_percent, target.scale_x_percent, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionScaleY) != 0) {
        style.scale_y_percent = lerpDouble(style.scale_y_percent, target.scale_y_percent, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionRotationZ) != 0) {
        style.rotation_degrees = lerpDouble(style.rotation_degrees, target.rotation_degrees, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionRotationX) != 0) {
        style.rotation_x_degrees = lerpDouble(style.rotation_x_degrees, target.rotation_x_degrees, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionRotationY) != 0) {
        style.rotation_y_degrees = lerpDouble(style.rotation_y_degrees, target.rotation_y_degrees, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionShearX) != 0) {
        style.shear_x = lerpDouble(style.shear_x, target.shear_x, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionShearY) != 0) {
        style.shear_y = lerpDouble(style.shear_y, target.shear_y, progress);
    }
    if ((transition.property_mask & subtitle::kSubtitleTransitionRotationOrigin) != 0) {
        style.rotation_origin_x = lerpDouble(style.rotation_origin_x, target.rotation_origin_x, progress);
        style.rotation_origin_y = lerpDouble(style.rotation_origin_y, target.rotation_origin_y, progress);
        style.has_rotation_origin = style.has_rotation_origin || target.has_rotation_origin;
        if (progress >= 0.999f) {
            style.has_rotation_origin = target.has_rotation_origin;
        }
    }
}

subtitle::SubtitleStyle resolveAnimatedSubtitleStyle(const subtitle::SubtitleStyle& base_style,
                                                     const subtitle::SubtitleStyleAnimation& animation,
                                                     double item_elapsed_seconds,
                                                     double item_duration_seconds) {
    subtitle::SubtitleStyle style = base_style;
    for (const subtitle::SubtitleStyleTransition& transition : animation.style_transitions) {
        applyStyleTransition(style,
                             transition,
                             evaluateStyleTransitionProgress(transition, item_elapsed_seconds, item_duration_seconds));
    }
    return style;
}

SubtitleAnimationState resolveSubtitleAnimationState(const subtitle::SubtitleItem& item, double subtitle_clock_seconds) {
    SubtitleAnimationState state;
    const subtitle::SubtitleStyleAnimation& animation = item.animation;
    const double item_elapsed_seconds = subtitle_clock_seconds - item.start_seconds;
    const double item_duration_seconds = std::max(0.0, item.end_seconds - item.start_seconds);

    if (animation.has_move) {
        state.has_position = true;
        const double start_seconds = animation.move_has_timing ? animation.move_t1_seconds : 0.0;
        const double end_seconds = animation.move_has_timing ? animation.move_t2_seconds : item_duration_seconds;
        double progress = 1.0;
        if (item_elapsed_seconds <= start_seconds) {
            progress = 0.0;
        } else if (end_seconds > start_seconds + 0.0005) {
            progress = std::clamp((item_elapsed_seconds - start_seconds) / (end_seconds - start_seconds), 0.0, 1.0);
        }
        state.position_x = animation.move_x1 + (animation.move_x2 - animation.move_x1) * progress;
        state.position_y = animation.move_y1 + (animation.move_y2 - animation.move_y1) * progress;
    } else if (item.style.has_position) {
        state.has_position = true;
        state.position_x = item.style.position_x;
        state.position_y = item.style.position_y;
    }

    switch (animation.fade_mode) {
    case subtitle::SubtitleFadeMode::Simple: {
        const double fade_in = std::max(0.0, animation.fade_in_seconds);
        const double fade_out = std::max(0.0, animation.fade_out_seconds);
        float opacity = 1.0f;
        if (fade_in > 0.0005) {
            opacity = std::min(opacity,
                               static_cast<float>(std::clamp(item_elapsed_seconds / fade_in, 0.0, 1.0)));
        }
        if (fade_out > 0.0005) {
            opacity = std::min(opacity,
                               static_cast<float>(std::clamp((item_duration_seconds - item_elapsed_seconds) / fade_out,
                                                             0.0,
                                                             1.0)));
        }
        state.opacity = std::clamp(opacity, 0.0f, 1.0f);
        break;
    }
    case subtitle::SubtitleFadeMode::Complex:
        state.opacity = evaluateComplexFadeOpacity(animation, item_elapsed_seconds);
        break;
    case subtitle::SubtitleFadeMode::None:
    default:
        state.opacity = 1.0f;
        break;
    }

    return state;
}

float scaleSubtitleMetric(double value, int script_extent, float target_extent, float fallback) {
    if (script_extent > 0 && value > 0.0) {
        return static_cast<float>(value * static_cast<double>(target_extent) / static_cast<double>(script_extent));
    }
    if (value > 0.0) {
        return static_cast<float>(value);
    }
    return fallback;
}

float clampSubtitleCoordinate(float value, float lower, float upper) {
    if (lower > upper) {
        return (lower + upper) * 0.5f;
    }
    return std::clamp(value, lower, upper);
}

float subtitlePercentScale(double percent) {
    if (percent <= 0.0) {
        return 1.0f;
    }
    return std::max(0.01f, static_cast<float>(percent / 100.0));
}

DWRITE_WORD_WRAPPING toWordWrapping(int wrap_style) {
    return wrap_style == 2 ? DWRITE_WORD_WRAPPING_NO_WRAP : DWRITE_WORD_WRAPPING_WRAP;
}

float subtitleAnchorLocalX(float layout_width, int alignment) {
    switch (subtitleHorizontalGroup(alignment)) {
    case 0:
        return 0.0f;
    case 2:
        return layout_width;
    default:
        return layout_width * 0.5f;
    }
}

float subtitleAnchorLocalY(float layout_height, int alignment) {
    switch (subtitleVerticalGroup(alignment)) {
    case 2:
        return 0.0f;
    case 1:
        return layout_height * 0.5f;
    default:
        return layout_height;
    }
}

struct SubtitleLocalBounds {
    float left{0.0f};
    float top{0.0f};
    float right{0.0f};
    float bottom{0.0f};
    float width() const { return right - left; }
    float height() const { return bottom - top; }
    float centerY() const { return (top + bottom) * 0.5f; }
};

SubtitleLocalBounds computeTransformedSubtitleBounds(float rect_left,
                                                    float rect_top,
                                                    float rect_right,
                                                    float rect_bottom,
                                                    float anchor_local_x,
                                                    float anchor_local_y,
                                                    float scale_x,
                                                    float scale_y,
                                                    float rotation_degrees) {
    const float radians = static_cast<float>(rotation_degrees * 3.14159265358979323846 / 180.0);
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);

    const std::array<D2D1_POINT_2F, 4> corners = {{
        D2D1::Point2F(rect_left, rect_top),
        D2D1::Point2F(rect_right, rect_top),
        D2D1::Point2F(rect_left, rect_bottom),
        D2D1::Point2F(rect_right, rect_bottom),
    }};

    SubtitleLocalBounds bounds;
    bool initialized = false;
    for (const D2D1_POINT_2F& corner : corners) {
        const float local_x = (corner.x - anchor_local_x) * scale_x;
        const float local_y = (corner.y - anchor_local_y) * scale_y;
        const float rotated_x = local_x * cosine - local_y * sine;
        const float rotated_y = local_x * sine + local_y * cosine;
        const float transformed_x = anchor_local_x + rotated_x;
        const float transformed_y = anchor_local_y + rotated_y;
        if (!initialized) {
            bounds.left = bounds.right = transformed_x;
            bounds.top = bounds.bottom = transformed_y;
            initialized = true;
            continue;
        }
        bounds.left = std::min(bounds.left, transformed_x);
        bounds.right = std::max(bounds.right, transformed_x);
        bounds.top = std::min(bounds.top, transformed_y);
        bounds.bottom = std::max(bounds.bottom, transformed_y);
    }
    return bounds;
}

struct SubtitleAffineTransform {
    float m11{1.0f};
    float m12{0.0f};
    float m21{0.0f};
    float m22{1.0f};
    D2D1_POINT_2F origin{D2D1::Point2F(0.0f, 0.0f)};
};

float subtitleProjectedScale(float base_scale, double rotation_degrees) {
    float projection = static_cast<float>(std::cos(rotation_degrees * 3.14159265358979323846 / 180.0));
    if (std::abs(projection) < 0.01f) {
        projection = projection < 0.0f ? -0.01f : 0.01f;
    }
    return base_scale * projection;
}

SubtitleAffineTransform buildSubtitleAffineTransform(const subtitle::SubtitleStyle& style,
                                                     float base_scale_x,
                                                     float base_scale_y,
                                                     float script_scale_x,
                                                     float script_scale_y,
                                                     const SDL_Rect& video_rect,
                                                     const D2D1_POINT_2F& default_origin) {
    SubtitleAffineTransform transform;
    transform.origin = default_origin;
    if (style.has_rotation_origin) {
        transform.origin = D2D1::Point2F(
            static_cast<float>(video_rect.x) + static_cast<float>(style.rotation_origin_x) * script_scale_x,
            static_cast<float>(video_rect.y) + static_cast<float>(style.rotation_origin_y) * script_scale_y);
    }

    const float scale_x = subtitleProjectedScale(base_scale_x, style.rotation_y_degrees);
    const float scale_y = subtitleProjectedScale(base_scale_y, style.rotation_x_degrees);
    const float shear_x = static_cast<float>(style.shear_x);
    const float shear_y = static_cast<float>(style.shear_y);
    const float radians = static_cast<float>(style.rotation_degrees * 3.14159265358979323846 / 180.0);
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);

    transform.m11 = scale_x * (cosine - sine * shear_y);
    transform.m12 = scale_x * (sine + cosine * shear_y);
    transform.m21 = scale_y * (cosine * shear_x - sine);
    transform.m22 = scale_y * (sine * shear_x + cosine);
    return transform;
}

D2D1_POINT_2F transformSubtitleWorldPoint(float x,
                                          float y,
                                          const SubtitleAffineTransform& transform) {
    const float dx = x - transform.origin.x;
    const float dy = y - transform.origin.y;
    return D2D1::Point2F(transform.origin.x + transform.m11 * dx + transform.m21 * dy,
                         transform.origin.y + transform.m12 * dx + transform.m22 * dy);
}

SubtitleLocalBounds computeTransformedSubtitleBounds(float rect_left,
                                                    float rect_top,
                                                    float rect_right,
                                                    float rect_bottom,
                                                    const SubtitleAffineTransform& transform) {
    const std::array<D2D1_POINT_2F, 4> corners = {{
        D2D1::Point2F(rect_left, rect_top),
        D2D1::Point2F(rect_right, rect_top),
        D2D1::Point2F(rect_left, rect_bottom),
        D2D1::Point2F(rect_right, rect_bottom),
    }};

    SubtitleLocalBounds bounds;
    bool initialized = false;
    for (const D2D1_POINT_2F& corner : corners) {
        const D2D1_POINT_2F transformed = transformSubtitleWorldPoint(corner.x, corner.y, transform);
        if (!initialized) {
            bounds.left = bounds.right = transformed.x;
            bounds.top = bounds.bottom = transformed.y;
            initialized = true;
            continue;
        }
        bounds.left = std::min(bounds.left, transformed.x);
        bounds.right = std::max(bounds.right, transformed.x);
        bounds.top = std::min(bounds.top, transformed.y);
        bounds.bottom = std::max(bounds.bottom, transformed.y);
    }
    return bounds;
}

D2D1_MATRIX_3X2_F subtitleTransformMatrix(const SubtitleAffineTransform& transform) {
    return D2D1::Matrix3x2F(
        transform.m11,
        transform.m12,
        transform.m21,
        transform.m22,
        transform.origin.x - (transform.m11 * transform.origin.x + transform.m21 * transform.origin.y),
        transform.origin.y - (transform.m12 * transform.origin.x + transform.m22 * transform.origin.y));
}

float subtitleDrawingUnitScale(int drawing_scale) {
    if (drawing_scale <= 1) {
        return 1.0f;
    }
    const int shift = std::min(20, drawing_scale - 1);
    return 1.0f / static_cast<float>(1u << shift);
}

enum class AssVectorOpType {
    Move,
    Line,
    Bezier,
    Close,
};

struct AssVectorOp {
    AssVectorOpType type{AssVectorOpType::Move};
    std::array<D2D1_POINT_2F, 3> points{};
    size_t point_count{0};
};

struct ParsedAssVectorShape {
    std::vector<AssVectorOp> ops;
    SubtitleLocalBounds bounds{};
    bool valid{false};
};

bool isAssVectorCommandToken(const std::string& token) {
    return token.size() == 1 && std::isalpha(static_cast<unsigned char>(token[0])) != 0;
}

bool parseAssVectorFloat(const std::string& token, float& value) {
    try {
        value = std::stof(token);
        return true;
    } catch (...) {
        return false;
    }
}

std::vector<std::string> tokenizeAssVectorCommands(const std::string& commands) {
    std::vector<std::string> tokens;
    std::string current;
    const auto flush_current = [&]() {
        if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
        }
    };

    for (char ch : commands) {
        const unsigned char uch = static_cast<unsigned char>(ch);
        if (std::isalpha(uch) != 0) {
            flush_current();
            std::string token(1, static_cast<char>(std::tolower(uch)));
            tokens.push_back(token);
            continue;
        }
        if (std::isdigit(uch) != 0 || ch == '-' || ch == '+' || ch == '.') {
            current.push_back(ch);
            continue;
        }
        flush_current();
    }
    flush_current();
    return tokens;
}

bool parseAssVectorShape(const std::string& commands,
                         float unit_scale_x,
                         float unit_scale_y,
                         bool normalize_origin,
                         ParsedAssVectorShape& shape_out) {
    shape_out = {};
    const std::vector<std::string> tokens = tokenizeAssVectorCommands(commands);
    if (tokens.empty()) {
        return false;
    }

    std::vector<AssVectorOp> ops;
    SubtitleLocalBounds bounds;
    bool bounds_initialized = false;
    const auto update_bounds = [&](float x, float y) {
        if (!bounds_initialized) {
            bounds.left = bounds.right = x;
            bounds.top = bounds.bottom = y;
            bounds_initialized = true;
            return;
        }
        bounds.left = std::min(bounds.left, x);
        bounds.right = std::max(bounds.right, x);
        bounds.top = std::min(bounds.top, y);
        bounds.bottom = std::max(bounds.bottom, y);
    };

    char command = '\0';
    size_t index = 0;
    while (index < tokens.size()) {
        if (isAssVectorCommandToken(tokens[index])) {
            command = static_cast<char>(std::tolower(static_cast<unsigned char>(tokens[index][0])));
            ++index;
            if (command == 'c') {
                ops.push_back(AssVectorOp{AssVectorOpType::Close, {}, 0});
            }
            continue;
        }

        if (command == '\0') {
            ++index;
            continue;
        }

        auto parse_point = [&](D2D1_POINT_2F& point) -> bool {
            if (index + 1 >= tokens.size() || isAssVectorCommandToken(tokens[index]) || isAssVectorCommandToken(tokens[index + 1])) {
                return false;
            }
            float x = 0.0f;
            float y = 0.0f;
            if (!parseAssVectorFloat(tokens[index], x) || !parseAssVectorFloat(tokens[index + 1], y)) {
                return false;
            }
            index += 2;
            point = D2D1::Point2F(x * unit_scale_x, y * unit_scale_y);
            update_bounds(point.x, point.y);
            return true;
        };

        if (command == 'm' || command == 'n') {
            bool first_pair = true;
            while (index < tokens.size() && !isAssVectorCommandToken(tokens[index])) {
                D2D1_POINT_2F point{};
                if (!parse_point(point)) {
                    break;
                }
                AssVectorOp op{};
                op.type = first_pair ? AssVectorOpType::Move : AssVectorOpType::Line;
                op.points[0] = point;
                op.point_count = 1;
                ops.push_back(op);
                first_pair = false;
            }
            continue;
        }

        if (command == 'l' || command == 's' || command == 'p') {
            while (index < tokens.size() && !isAssVectorCommandToken(tokens[index])) {
                D2D1_POINT_2F point{};
                if (!parse_point(point)) {
                    break;
                }
                AssVectorOp op{};
                op.type = AssVectorOpType::Line;
                op.points[0] = point;
                op.point_count = 1;
                ops.push_back(op);
            }
            continue;
        }

        if (command == 'b') {
            while (index < tokens.size() && !isAssVectorCommandToken(tokens[index])) {
                D2D1_POINT_2F p1{};
                D2D1_POINT_2F p2{};
                D2D1_POINT_2F p3{};
                if (!parse_point(p1) || !parse_point(p2) || !parse_point(p3)) {
                    break;
                }
                AssVectorOp op{};
                op.type = AssVectorOpType::Bezier;
                op.points[0] = p1;
                op.points[1] = p2;
                op.points[2] = p3;
                op.point_count = 3;
                ops.push_back(op);
            }
            continue;
        }

        ++index;
    }

    if (!bounds_initialized || ops.empty()) {
        return false;
    }

    if (normalize_origin) {
        for (AssVectorOp& op : ops) {
            for (size_t i = 0; i < op.point_count; ++i) {
                op.points[i].x -= bounds.left;
                op.points[i].y -= bounds.top;
            }
        }
        bounds.right -= bounds.left;
        bounds.bottom -= bounds.top;
        bounds.left = 0.0f;
        bounds.top = 0.0f;
    }

    shape_out.ops = std::move(ops);
    shape_out.bounds = bounds;
    shape_out.valid = true;
    return true;
}

bool buildAssVectorGeometry(ID2D1Factory* factory,
                            const ParsedAssVectorShape& shape,
                            float offset_x,
                            float offset_y,
                            ID2D1PathGeometry** geometry_out) {
    if (!factory || !geometry_out || !shape.valid) {
        return false;
    }

    *geometry_out = nullptr;
    ComPtr<ID2D1PathGeometry> geometry;
    if (FAILED(factory->CreatePathGeometry(geometry.GetAddressOf())) || !geometry) {
        return false;
    }

    ComPtr<ID2D1GeometrySink> sink;
    if (FAILED(geometry->Open(sink.GetAddressOf())) || !sink) {
        return false;
    }

    bool figure_open = false;
    for (const AssVectorOp& op : shape.ops) {
        switch (op.type) {
        case AssVectorOpType::Move: {
            if (figure_open) {
                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
            }
            const D2D1_POINT_2F point = D2D1::Point2F(offset_x + op.points[0].x, offset_y + op.points[0].y);
            sink->BeginFigure(point, D2D1_FIGURE_BEGIN_FILLED);
            figure_open = true;
            break;
        }
        case AssVectorOpType::Line: {
            const D2D1_POINT_2F point = D2D1::Point2F(offset_x + op.points[0].x, offset_y + op.points[0].y);
            if (!figure_open) {
                sink->BeginFigure(point, D2D1_FIGURE_BEGIN_FILLED);
                figure_open = true;
            } else {
                sink->AddLine(point);
            }
            break;
        }
        case AssVectorOpType::Bezier: {
            if (!figure_open) {
                sink->BeginFigure(D2D1::Point2F(offset_x + op.points[2].x, offset_y + op.points[2].y),
                                  D2D1_FIGURE_BEGIN_FILLED);
                figure_open = true;
            }
            sink->AddBezier(D2D1::BezierSegment(
                D2D1::Point2F(offset_x + op.points[0].x, offset_y + op.points[0].y),
                D2D1::Point2F(offset_x + op.points[1].x, offset_y + op.points[1].y),
                D2D1::Point2F(offset_x + op.points[2].x, offset_y + op.points[2].y)));
            break;
        }
        case AssVectorOpType::Close:
            if (figure_open) {
                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                figure_open = false;
            }
            break;
        }
    }
    if (figure_open) {
        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
    }
    sink->Close();

    *geometry_out = geometry.Detach();
    return true;
}

bool buildAssClipGeometry(ID2D1Factory* factory,
                          const std::string& commands,
                          int clip_scale,
                          const SDL_Rect& video_rect,
                          float script_scale_x,
                          float script_scale_y,
                          ID2D1PathGeometry** geometry_out) {
    ParsedAssVectorShape shape;
    const float unit_scale = subtitleDrawingUnitScale(clip_scale);
    if (!parseAssVectorShape(commands, script_scale_x * unit_scale, script_scale_y * unit_scale, false, shape)) {
        return false;
    }
    return buildAssVectorGeometry(factory,
                                  shape,
                                  static_cast<float>(video_rect.x),
                                  static_cast<float>(video_rect.y),
                                  geometry_out);
}

bool buildInverseClipGeometry(ID2D1Factory* factory,
                              ID2D1Geometry* clip_geometry,
                              const SDL_Rect& video_rect,
                              ID2D1PathGeometry** geometry_out) {
    if (!factory || !clip_geometry || !geometry_out) {
        return false;
    }

    *geometry_out = nullptr;
    ComPtr<ID2D1RectangleGeometry> full_rect_geometry;
    if (FAILED(factory->CreateRectangleGeometry(
            D2D1::RectF(static_cast<float>(video_rect.x),
                        static_cast<float>(video_rect.y),
                        static_cast<float>(video_rect.x + video_rect.w),
                        static_cast<float>(video_rect.y + video_rect.h)),
            full_rect_geometry.GetAddressOf())) || !full_rect_geometry) {
        return false;
    }

    ComPtr<ID2D1PathGeometry> combined_geometry;
    if (FAILED(factory->CreatePathGeometry(combined_geometry.GetAddressOf())) || !combined_geometry) {
        return false;
    }

    ComPtr<ID2D1GeometrySink> sink;
    if (FAILED(combined_geometry->Open(sink.GetAddressOf())) || !sink) {
        return false;
    }

    const HRESULT hr = full_rect_geometry->CombineWithGeometry(
        clip_geometry,
        D2D1_COMBINE_MODE_EXCLUDE,
        nullptr,
        D2D1_DEFAULT_FLATTENING_TOLERANCE,
        sink.Get());
    if (FAILED(hr) || FAILED(sink->Close())) {
        return false;
    }

    *geometry_out = combined_geometry.Detach();
    return true;
}

bool subtitleItemHasAnimatedRuns(const subtitle::SubtitleItem& item) {
    if (subtitleAnimationHasMotion(item.animation) ||
        subtitleAnimationHasFade(item.animation) ||
        subtitleAnimationHasStyleTransitions(item.animation)) {
        return true;
    }
    return std::any_of(item.runs.begin(), item.runs.end(), [](const subtitle::SubtitleTextRun& run) {
        return run.karaoke_mode != subtitle::SubtitleKaraokeMode::None &&
               run.karaoke_end_centiseconds > run.karaoke_start_centiseconds;
    });
}

bool subtitleItemsHaveAnimatedRuns(const std::vector<subtitle::SubtitleItem>& items) {
    return std::any_of(items.begin(), items.end(), [](const subtitle::SubtitleItem& item) {
        return subtitleItemHasAnimatedRuns(item);
    });
}

struct KaraokeRunVisualState {
    subtitle::SubtitleColor base_fill_color{};
    bool has_highlight_overlay{false};
    subtitle::SubtitleColor overlay_fill_color{};
    float overlay_progress{0.0f};
};

KaraokeRunVisualState resolveKaraokeRunVisualState(const subtitle::SubtitleStyle& style,
                                                   const subtitle::SubtitleTextRun& run,
                                                   double item_elapsed_seconds) {
    KaraokeRunVisualState state;
    state.base_fill_color = style.primary_color;
    if (run.karaoke_mode == subtitle::SubtitleKaraokeMode::None ||
        run.karaoke_end_centiseconds <= run.karaoke_start_centiseconds) {
        return state;
    }

    const double karaoke_start_seconds = static_cast<double>(run.karaoke_start_centiseconds) / 100.0;
    const double karaoke_end_seconds = static_cast<double>(run.karaoke_end_centiseconds) / 100.0;
    if (item_elapsed_seconds < karaoke_start_seconds) {
        state.base_fill_color = style.secondary_color;
        return state;
    }
    if (item_elapsed_seconds >= karaoke_end_seconds) {
        state.base_fill_color = style.primary_color;
        return state;
    }

    if (run.karaoke_mode == subtitle::SubtitleKaraokeMode::Sweep) {
        state.base_fill_color = style.secondary_color;
        state.has_highlight_overlay = true;
        state.overlay_fill_color = style.primary_color;
        const double duration = std::max(0.001, karaoke_end_seconds - karaoke_start_seconds);
        state.overlay_progress = static_cast<float>(
            std::clamp((item_elapsed_seconds - karaoke_start_seconds) / duration, 0.0, 1.0));
        return state;
    }

    state.base_fill_color = style.primary_color;
    return state;
}

struct KaraokeHighlightClip {
    D2D1_RECT_F world_rect{};
};

std::vector<KaraokeHighlightClip> buildKaraokeHighlightClips(const std::vector<DWRITE_HIT_TEST_METRICS>& metrics,
                                                             float progress,
                                                             float draw_x,
                                                             float draw_y,
                                                             const SubtitleAffineTransform& transform);

bool collectTextRangeMetrics(IDWriteTextLayout* text_layout,
                             UINT32 start_index,
                             UINT32 length,
                             std::vector<DWRITE_HIT_TEST_METRICS>& metrics) {
    metrics.clear();
    if (!text_layout || length == 0) {
        return false;
    }

    UINT32 actual_count = 0;
    HRESULT hr = text_layout->HitTestTextRange(start_index, length, 0.0f, 0.0f, nullptr, 0, &actual_count);
    if (FAILED(hr) || actual_count == 0) {
        return false;
    }

    metrics.resize(actual_count);
    hr = text_layout->HitTestTextRange(start_index,
                                       length,
                                       0.0f,
                                       0.0f,
                                       metrics.data(),
                                       actual_count,
                                       &actual_count);
    if (FAILED(hr) || actual_count == 0) {
        metrics.clear();
        return false;
    }
    metrics.resize(actual_count);
    return true;
}

std::vector<KaraokeHighlightClip> buildKaraokeHighlightClips(const std::vector<DWRITE_HIT_TEST_METRICS>& metrics,
                                                             float progress,
                                                             float draw_x,
                                                             float draw_y,
                                                             float anchor_local_x,
                                                             float anchor_local_y,
                                                             float glyph_scale_x,
                                                             float glyph_scale_y,
                                                             float rotation_degrees) {
    const float radians = static_cast<float>(rotation_degrees * 3.14159265358979323846 / 180.0);
    const float cosine = std::cos(radians);
    const float sine = std::sin(radians);
    SubtitleAffineTransform transform;
    transform.origin = D2D1::Point2F(draw_x + anchor_local_x, draw_y + anchor_local_y);
    transform.m11 = glyph_scale_x * cosine;
    transform.m12 = glyph_scale_x * sine;
    transform.m21 = -glyph_scale_y * sine;
    transform.m22 = glyph_scale_y * cosine;
    return buildKaraokeHighlightClips(metrics, progress, draw_x, draw_y, transform);
}

std::vector<KaraokeHighlightClip> buildKaraokeHighlightClips(const std::vector<DWRITE_HIT_TEST_METRICS>& metrics,
                                                             float progress,
                                                             float draw_x,
                                                             float draw_y,
                                                             const SubtitleAffineTransform& transform) {
    std::vector<KaraokeHighlightClip> clips;
    if (metrics.empty() || progress <= 0.0f) {
        return clips;
    }

    float total_width = 0.0f;
    for (const DWRITE_HIT_TEST_METRICS& metric : metrics) {
        total_width += metric.width;
    }
    if (total_width <= 0.001f) {
        return clips;
    }

    float remaining_width = total_width * std::clamp(progress, 0.0f, 1.0f);
    for (const DWRITE_HIT_TEST_METRICS& metric : metrics) {
        if (remaining_width <= 0.001f) {
            break;
        }
        const float highlight_width = std::min(metric.width, remaining_width);
        remaining_width -= highlight_width;

        const SubtitleLocalBounds world_bounds = computeTransformedSubtitleBounds(
            draw_x + metric.left,
            draw_y + metric.top,
            draw_x + metric.left + highlight_width,
            draw_y + metric.top + metric.height,
            transform);

        KaraokeHighlightClip clip{};
        clip.world_rect = D2D1::RectF(world_bounds.left, world_bounds.top, world_bounds.right, world_bounds.bottom);
        clips.push_back(clip);
    }
    return clips;
}

std::vector<D2D1_RECT_F> buildSubtitleClipRegions(const SDL_Rect& video_rect,
                                                  const D2D1_RECT_F& clip_rect,
                                                  bool inverse_clip) {
    std::vector<D2D1_RECT_F> regions;
    const float video_left = static_cast<float>(video_rect.x);
    const float video_top = static_cast<float>(video_rect.y);
    const float video_right = static_cast<float>(video_rect.x + video_rect.w);
    const float video_bottom = static_cast<float>(video_rect.y + video_rect.h);

    const D2D1_RECT_F bounded_clip = D2D1::RectF(
        std::clamp(clip_rect.left, video_left, video_right),
        std::clamp(clip_rect.top, video_top, video_bottom),
        std::clamp(clip_rect.right, video_left, video_right),
        std::clamp(clip_rect.bottom, video_top, video_bottom));

    const auto append_region = [&](float left, float top, float right, float bottom) {
        if ((right - left) > 0.001f && (bottom - top) > 0.001f) {
            regions.push_back(D2D1::RectF(left, top, right, bottom));
        }
    };

    if (!inverse_clip) {
        append_region(bounded_clip.left, bounded_clip.top, bounded_clip.right, bounded_clip.bottom);
        return regions;
    }

    if (bounded_clip.right <= bounded_clip.left || bounded_clip.bottom <= bounded_clip.top) {
        append_region(video_left, video_top, video_right, video_bottom);
        return regions;
    }

    append_region(video_left, video_top, video_right, bounded_clip.top);
    append_region(video_left, bounded_clip.top, bounded_clip.left, bounded_clip.bottom);
    append_region(bounded_clip.right, bounded_clip.top, video_right, bounded_clip.bottom);
    append_region(video_left, bounded_clip.bottom, video_right, video_bottom);
    return regions;
}

}  // namespace

class D3D11VideoRenderer::Impl {
public:
    Impl() = default;
    ~Impl() { close(); }

    bool init(const VideoRendererConfig& config);
    void close();
    void renderFrame(const core::VideoFrame& frame);
    void present();
    void clear();
    void handleEvents();
    bool shouldQuit() const { return should_quit_.load(); }
    bool consumeTogglePauseRequest();
    bool consumeSeekRequest(double& normalized_position);
    bool consumeSeekDeltaRequest(double& delta_seconds);
    bool consumeVolumeChangeRequest(float& volume);
    bool consumeSpeedChangeRequest(double& speed_delta);
    bool consumeResetSpeedRequest();
    bool consumeToggleSubtitleRequest();
    bool consumeSetABRepeatStartRequest();
    bool consumeSetABRepeatEndRequest();
    bool consumeClearABRepeatRequest();
    bool consumeScreenshotRequest();
    bool consumeStepFrameBackwardRequest();
    bool consumeStepFrameForwardRequest();
    bool consumePreviousSubtitleTrackRequest();
    bool consumeNextSubtitleTrackRequest();
    bool consumeSubtitleDelayChangeRequest(double& delta_seconds);
    bool consumeAudioDelayChangeRequest(double& delta_seconds);
    bool consumeNextChapterRequest();
    bool consumePreviousChapterRequest();
    bool consumeNextItemRequest();
    bool consumePreviousItemRequest();
    bool consumeOpenFileRequest(std::string& path);
    void setOverlayState(double position, double duration, float volume, bool paused);
    void setSubtitleClock(double subtitle_time_seconds);
    void setSubtitleText(const std::string& text);
    void setSubtitleItems(const std::vector<subtitle::SubtitleItem>& items);
    void setHotkeyManager(const input::HotkeyManager& hotkey_manager);
    RendererDiagnostics getDiagnostics() const;
    void resetDiagnostics();
    bool supportsNativeFrameFormat(AVPixelFormat format) const;
    void* nativeDeviceHandle() const;

private:
    struct SubtitleBitmapCacheEntry {
        uint64_t key{0};
        int width{0};
        int height{0};
        std::vector<uint8_t> premul_bgra;
        ComPtr<ID2D1Bitmap1> d2d_bitmap;
        uint64_t last_use_token{0};
    };

    struct ControlLayout {
        SDL_Rect panel;
        SDL_Rect progress_track;
        SDL_Rect volume_track;
    };

    struct OutputDisplayBindingState {
        bool binding_succeeded{false};
        int sdl_display_index{-1};
        std::string sdl_display_name{"unknown"};
        std::string output_device_name{"unknown"};
        std::string binding_error{};
        D3D11HdrOutputSnapshot hdr_output{};
    };

    bool createDeviceResources();
    bool createSwapChainForWindow();
    bool createBackBuffer();
    bool createShaders();
    bool createBlendState();
    bool createTextResources();
    bool createTextTarget();
    bool ensureSubtitleTextFormat(float font_size);
    void clearSubtitleBitmapCache();
    ID2D1Bitmap1* getCachedSubtitleBitmap(const subtitle::SubtitleBitmap& bitmap);
    bool ensureSoftwareTextures(int width, int height);
    bool ensureOverlayBuffer(size_t vertex_count);
    bool resizeSwapChainLocked(int width, int height, DXGI_FORMAT format);
    void disableNativeDirectRendering(const char* reason,
                                      const D3D11_TEXTURE2D_DESC* texture_desc,
                                      intptr_t texture_index,
                                      HRESULT y_plane_hr = S_OK,
                                      HRESULT uv_plane_hr = S_OK);
    bool ensureNativeShaderResourcesLocked(ID3D11Texture2D* texture, intptr_t index, DXGI_FORMAT format);
    bool bindSoftwareFrameLocked(const AVFrame* frame, bool hdr_output_active);
    bool bindNativeFrameLocked(const AVFrame* frame, bool hdr_output_active);
    void appendRect(std::vector<ColorVertex>& vertices, int x, int y, int w, int h, Color color) const;
    ControlLayout computeControlLayout(int window_width, int window_height) const;
    void drawSubtitleLocked(const ControlLayout& layout);
    bool refreshOutputBindingLocked();
    bool updateHdrPresentationLocked(const AVFrame* frame);
    void updatePresentTiming(uint64_t value_us);
    void markHdrOutputDirty();
    bool renderLocked();
    void redrawIfPaused();
    void resetRequests();
    void resetHdrDiagnosticsLocked();
    void toggleFullscreen();

    SDL_Window* window_{nullptr};
    std::atomic<int> width_{0};
    std::atomic<int> height_{0};
    std::atomic<bool> should_quit_{false};
    std::atomic<bool> fullscreen_{false};
    std::atomic<bool> minimized_{false};
    std::thread::id event_thread_id_{};
    std::atomic<bool> event_thread_guard_reported_{false};
    bool initialized_{false};

    mutable std::mutex render_mutex_;
    mutable std::mutex request_mutex_;
    mutable std::mutex subtitle_mutex_;

    std::atomic<double> overlay_position_{0.0};
    std::atomic<double> overlay_duration_{0.0};
    std::atomic<double> subtitle_clock_seconds_{0.0};
    std::atomic<float> overlay_volume_{1.0f};
    std::atomic<bool> overlay_paused_{false};
    std::vector<subtitle::SubtitleItem> subtitle_items_;
    std::atomic<bool> subtitle_has_animated_content_{false};
    input::HotkeyManager hotkey_manager_{};

    bool toggle_pause_requested_{false};
    bool seek_requested_{false};
    double seek_ratio_{0.0};
    bool seek_delta_requested_{false};
    double seek_delta_seconds_{0.0};
    bool volume_change_requested_{false};
    float requested_volume_{1.0f};
    bool speed_change_requested_{false};
    double speed_delta_{0.0};
    bool speed_reset_requested_{false};
    bool subtitle_toggle_requested_{false};
    bool ab_repeat_start_requested_{false};
    bool ab_repeat_end_requested_{false};
    bool ab_repeat_clear_requested_{false};
    bool screenshot_requested_{false};
    bool step_frame_backward_requested_{false};
    bool step_frame_forward_requested_{false};
    bool subtitle_delay_change_requested_{false};
    double subtitle_delay_delta_seconds_{0.0};
    bool audio_delay_change_requested_{false};
    double audio_delay_delta_seconds_{0.0};
    bool next_chapter_requested_{false};
    bool previous_chapter_requested_{false};
    bool next_item_requested_{false};
    bool previous_item_requested_{false};
    bool open_file_requested_{false};
    std::string open_file_path_;
    float last_nonzero_volume_{1.0f};
    bool dragging_seek_{false};
    bool dragging_volume_{false};
    bool seek_preview_active_{false};
    double seek_preview_ratio_{0.0};

    AVFrame* current_frame_{nullptr};
    bool frame_available_{false};

    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    ComPtr<ID3D11Device3> device3_;
    ComPtr<IDXGISwapChain1> swap_chain_;
    ComPtr<IDXGISwapChain3> swap_chain3_;
    ComPtr<IDXGISwapChain4> swap_chain4_;
    ComPtr<ID3D11RenderTargetView> render_target_view_;
    ComPtr<ID3D11VertexShader> video_vertex_shader_;
    ComPtr<ID3D11PixelShader> yuv420_pixel_shader_;
    ComPtr<ID3D11PixelShader> nv12_pixel_shader_;
    ComPtr<ID3D11InputLayout> video_input_layout_;
    ComPtr<ID3D11Buffer> video_vertex_buffer_;
    ComPtr<ID3D11Buffer> color_matrix_buffer_;
    ComPtr<ID3D11SamplerState> sampler_state_;
    ComPtr<ID3D11VertexShader> color_vertex_shader_;
    ComPtr<ID3D11PixelShader> color_pixel_shader_;
    ComPtr<ID3D11InputLayout> color_input_layout_;
    ComPtr<ID3D11Buffer> overlay_vertex_buffer_;
    size_t overlay_vertex_capacity_{0};
    ComPtr<ID3D11BlendState> alpha_blend_state_;
    ComPtr<ID2D1Factory1> d2d_factory_;
    ComPtr<ID2D1Device> d2d_device_;
    ComPtr<ID2D1DeviceContext> d2d_context_;
    ComPtr<ID2D1Bitmap1> d2d_target_bitmap_;
    ComPtr<IDWriteFactory> dwrite_factory_;
    ComPtr<IDWriteFontCollection> subtitle_dwrite_font_collection_;
    ComPtr<IDWriteTextFormat> subtitle_text_format_;
    ComPtr<ID2D1SolidColorBrush> subtitle_fill_brush_;
    ComPtr<ID2D1SolidColorBrush> subtitle_shadow_brush_;
    ComPtr<ID2D1SolidColorBrush> subtitle_background_brush_;
    float subtitle_font_size_{0.0f};
    std::vector<SubtitleBitmapCacheEntry> subtitle_bitmap_cache_;
    uint64_t subtitle_bitmap_cache_use_token_{0};

    int software_width_{0};
    int software_height_{0};
    ComPtr<ID3D11Texture2D> tex_y_;
    ComPtr<ID3D11Texture2D> tex_u_;
    ComPtr<ID3D11Texture2D> tex_v_;
    ComPtr<ID3D11ShaderResourceView> srv_y_;
    ComPtr<ID3D11ShaderResourceView> srv_u_;
    ComPtr<ID3D11ShaderResourceView> srv_v_;

    ID3D11Texture2D* native_texture_ptr_{nullptr};
    intptr_t native_texture_index_{0};
    DXGI_FORMAT native_texture_format_{DXGI_FORMAT_UNKNOWN};
    ComPtr<ID3D11ShaderResourceView1> native_srv_y_;
    ComPtr<ID3D11ShaderResourceView1> native_srv_uv_;
    std::atomic<bool> native_direct_rendering_disabled_{false};
    D3D11DiagnosticsSnapshot d3d11_diagnostics_{};
    D3D11HdrPresentMode hdr_present_mode_{D3D11HdrPresentMode::Auto};
    DXGI_FORMAT current_swapchain_format_{DXGI_FORMAT_B8G8R8A8_UNORM};
    DXGI_COLOR_SPACE_TYPE current_output_color_space_{DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709};
    OutputDisplayBindingState output_binding_{};
    bool hdr_output_dirty_{true};
    bool hdr_present_requested_{false};
    bool hdr_present_active_{false};
    bool hdr_content_detected_{false};
    bool hdr_output_advanced_color_active_{false};
    bool hdr_output_hdr_active_{false};
    bool hdr_metadata_available_{false};
    bool hdr_metadata_applied_{false};
    std::string hdr_metadata_source_{"none"};
    std::string hdr_present_decision_{"not-evaluated"};
    std::string hdr_error_{};
    std::atomic<uint64_t> hdr_state_reload_count_{0};
    std::atomic<uint64_t> present_count_{0};
    std::atomic<uint64_t> present_failures_{0};
    std::atomic<uint64_t> present_time_us_total_{0};
    std::atomic<uint64_t> present_time_us_max_{0};
};
bool D3D11VideoRenderer::Impl::init(const VideoRendererConfig& config) {
    close();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        LOG_ERROR("SDL init failed: " << SDL_GetError());
        return false;
    }

    const WindowSize window_size = computeInitialWindowSize(config.width, config.height);
    width_.store(window_size.width);
    height_.store(window_size.height);
    window_ = SDL_CreateWindow(config.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               width_.load(), height_.load(), SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window_) {
        LOG_ERROR("SDL window creation failed: " << SDL_GetError());
        SDL_Quit();
        return false;
    }
    SDL_SetWindowMinimumSize(window_, kMinWindowWidth, kMinWindowHeight);

    current_frame_ = av_frame_alloc();
    if (!current_frame_) {
        LOG_ERROR("Failed to allocate render frame cache");
        close();
        return false;
    }

    resetRequests();
    hdr_present_mode_ = readD3D11HdrPresentMode();
    resetHdrDiagnosticsLocked();
    event_thread_id_ = std::this_thread::get_id();
    event_thread_guard_reported_.store(false);
    native_direct_rendering_disabled_.store(false);
    if (!createDeviceResources() || !createTextResources() || !createSwapChainForWindow() || !createBackBuffer() || !createShaders() ||
        !createBlendState()) {
        close();
        return false;
    }
    initialized_ = true;
    LOG_INFO("Native D3D11 renderer initialized: window=" << width_.load() << "x" << height_.load());
    return true;
}

void D3D11VideoRenderer::Impl::close() {
    std::lock_guard<std::mutex> render_lock(render_mutex_);
    initialized_ = false;
    event_thread_id_ = std::thread::id{};
    event_thread_guard_reported_.store(false);
    frame_available_ = false;
    subtitle_has_animated_content_.store(false);
    native_texture_ptr_ = nullptr;
    native_texture_index_ = 0;
    native_texture_format_ = DXGI_FORMAT_UNKNOWN;
    native_srv_y_.Reset();
    native_srv_uv_.Reset();
    native_direct_rendering_disabled_.store(false);
    d3d11_diagnostics_ = {};
    hdr_present_mode_ = D3D11HdrPresentMode::Auto;
    resetHdrDiagnosticsLocked();
    tex_y_.Reset();
    tex_u_.Reset();
    tex_v_.Reset();
    srv_y_.Reset();
    srv_u_.Reset();
    srv_v_.Reset();
    subtitle_font_size_ = 0.0f;
    if (d2d_context_) {
        d2d_context_->SetTarget(nullptr);
    }
    clearSubtitleBitmapCache();
    subtitle_background_brush_.Reset();
    subtitle_shadow_brush_.Reset();
    subtitle_fill_brush_.Reset();
    subtitle_text_format_.Reset();
    subtitle_dwrite_font_collection_.Reset();
    d2d_target_bitmap_.Reset();
    dwrite_factory_.Reset();
    d2d_context_.Reset();
    d2d_device_.Reset();
    d2d_factory_.Reset();
    alpha_blend_state_.Reset();
    overlay_vertex_buffer_.Reset();
    color_input_layout_.Reset();
    color_pixel_shader_.Reset();
    color_vertex_shader_.Reset();
    sampler_state_.Reset();
    color_matrix_buffer_.Reset();
    video_vertex_buffer_.Reset();
    video_input_layout_.Reset();
    yuv420_pixel_shader_.Reset();
    nv12_pixel_shader_.Reset();
    video_vertex_shader_.Reset();
    render_target_view_.Reset();
    swap_chain4_.Reset();
    swap_chain3_.Reset();
    swap_chain_.Reset();
    context_.Reset();
    device3_.Reset();
    device_.Reset();
    if (current_frame_) {
        av_frame_free(&current_frame_);
    }
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Quit();
}

void D3D11VideoRenderer::Impl::clearSubtitleBitmapCache() {
    subtitle_bitmap_cache_.clear();
    subtitle_bitmap_cache_use_token_ = 0;
}

void D3D11VideoRenderer::Impl::resetHdrDiagnosticsLocked() {
    current_swapchain_format_ = DXGI_FORMAT_B8G8R8A8_UNORM;
    current_output_color_space_ = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
    output_binding_ = {};
    hdr_output_dirty_ = true;
    hdr_present_requested_ = false;
    hdr_present_active_ = false;
    hdr_content_detected_ = false;
    hdr_output_advanced_color_active_ = false;
    hdr_output_hdr_active_ = false;
    hdr_metadata_available_ = false;
    hdr_metadata_applied_ = false;
    hdr_metadata_source_ = "none";
    hdr_present_decision_ = "not-evaluated";
    hdr_error_.clear();
    hdr_state_reload_count_.store(0);
    present_count_.store(0);
    present_failures_.store(0);
    present_time_us_total_.store(0);
    present_time_us_max_.store(0);
}

void D3D11VideoRenderer::Impl::updatePresentTiming(uint64_t value_us) {
    present_time_us_total_.fetch_add(value_us);
    uint64_t current = present_time_us_max_.load();
    while (value_us > current && !present_time_us_max_.compare_exchange_weak(current, value_us)) {
    }
}

void D3D11VideoRenderer::Impl::markHdrOutputDirty() {
    hdr_output_dirty_ = true;
}

bool D3D11VideoRenderer::Impl::refreshOutputBindingLocked() {
    output_binding_ = {};
    output_binding_.hdr_output = d3d11_diagnostics_.hdr_output;
    if (!window_) {
        output_binding_.binding_error = "window unavailable";
        hdr_output_dirty_ = false;
        return false;
    }

    const int display_index = SDL_GetWindowDisplayIndex(window_);
    output_binding_.sdl_display_index = display_index;
    if (display_index >= 0) {
        output_binding_.binding_succeeded = true;
        if (const char* display_name = SDL_GetDisplayName(display_index)) {
            output_binding_.sdl_display_name = display_name;
        }
    } else {
        output_binding_.binding_error = "SDL_GetWindowDisplayIndex failed";
    }

    HWND hwnd = nullptr;
    if (!tryGetWindowHandle(window_, hwnd)) {
        if (output_binding_.binding_error.empty()) {
            output_binding_.binding_error = "failed to resolve HWND";
        }
        hdr_output_dirty_ = false;
        return output_binding_.binding_succeeded;
    }

    const HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    if (!monitor) {
        if (output_binding_.binding_error.empty()) {
            output_binding_.binding_error = "MonitorFromWindow failed";
        }
        hdr_output_dirty_ = false;
        return output_binding_.binding_succeeded;
    }

    MONITORINFOEXW monitor_info{};
    monitor_info.cbSize = sizeof(monitor_info);
    if (!GetMonitorInfoW(monitor, &monitor_info)) {
        if (output_binding_.binding_error.empty()) {
            output_binding_.binding_error = "GetMonitorInfoW failed";
        }
        hdr_output_dirty_ = false;
        return output_binding_.binding_succeeded;
    }

    output_binding_.output_device_name = utf8FromWide(monitor_info.szDevice);
    output_binding_.hdr_output = probeD3D11HdrOutput(d3d11_diagnostics_, monitor_info.szDevice);
    if (!output_binding_.hdr_output.probe_error.empty() && output_binding_.binding_error.empty()) {
        output_binding_.binding_error = output_binding_.hdr_output.probe_error;
    }

    hdr_output_dirty_ = false;
    return output_binding_.binding_succeeded;
}

bool D3D11VideoRenderer::Impl::createDeviceResources() {
    D3D11DeviceProbeContext probe = createD3D11DeviceProbeContext();
    if (FAILED(probe.create_device_hr) || !probe.device || !probe.context) {
        LOG_ERROR("D3D11CreateDevice failed: hr=" << static_cast<long>(probe.create_device_hr));
        return false;
    }

    device_ = probe.device;
    context_ = probe.context;
    device3_ = probe.device3;
    d3d11_diagnostics_ = buildD3D11DiagnosticsSnapshot(probe);
    logD3D11StartupDiagnostics(d3d11_diagnostics_);
    native_direct_rendering_disabled_.store(d3d11_diagnostics_.native_direct_startup_disabled);
    output_binding_.hdr_output = d3d11_diagnostics_.hdr_output;

    D3D11_SAMPLER_DESC sampler_desc{};
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampler_desc.MaxLOD = D3D11_FLOAT32_MAX;
    if (FAILED(device_->CreateSamplerState(&sampler_desc, sampler_state_.GetAddressOf()))) {
        LOG_ERROR("CreateSamplerState failed");
        return false;
    }
    return true;
}
bool D3D11VideoRenderer::Impl::createSwapChainForWindow() {
    SDL_SysWMinfo wm_info{};
    SDL_VERSION(&wm_info.version);
    if (!SDL_GetWindowWMInfo(window_, &wm_info)) {
        LOG_ERROR("SDL_GetWindowWMInfo failed: " << SDL_GetError());
        return false;
    }
    HWND hwnd = wm_info.info.win.window;
    if (!hwnd) {
        LOG_ERROR("Failed to resolve HWND for SDL window");
        return false;
    }

    ComPtr<IDXGIDevice> dxgi_device;
    ComPtr<IDXGIAdapter> adapter;
    ComPtr<IDXGIFactory2> factory;
    if (FAILED(device_.As(&dxgi_device)) || FAILED(dxgi_device->GetAdapter(adapter.GetAddressOf())) ||
        FAILED(adapter->GetParent(IID_PPV_ARGS(factory.GetAddressOf())))) {
        LOG_ERROR("Failed to resolve DXGI factory for D3D11 renderer");
        return false;
    }

    DXGI_SWAP_CHAIN_DESC1 swap_desc{};
    swap_desc.Width = static_cast<UINT>(std::max(1, width_.load()));
    swap_desc.Height = static_cast<UINT>(std::max(1, height_.load()));
    swap_desc.Format = current_swapchain_format_;
    swap_desc.SampleDesc.Count = 1;
    swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_desc.BufferCount = 2;
    swap_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swap_desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    if (FAILED(factory->CreateSwapChainForHwnd(device_.Get(), hwnd, &swap_desc, nullptr, nullptr,
                                               swap_chain_.GetAddressOf()))) {
        LOG_ERROR("CreateSwapChainForHwnd failed");
        return false;
    }
    swap_chain3_.Reset();
    swap_chain4_.Reset();
    swap_chain_.As(&swap_chain3_);
    swap_chain_.As(&swap_chain4_);
    logD3D11SwapChainDiagnostics(swap_desc);
    const HRESULT window_assoc_hr = factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
    if (FAILED(window_assoc_hr)) {
        LOG_WARNING("MakeWindowAssociation failed: hr=" << static_cast<long>(window_assoc_hr));
    }
    return true;
}

bool D3D11VideoRenderer::Impl::createBackBuffer() {
    render_target_view_.Reset();
    ComPtr<ID3D11Texture2D> back_buffer;
    if (FAILED(swap_chain_->GetBuffer(0, IID_PPV_ARGS(back_buffer.GetAddressOf())))) {
        LOG_ERROR("Failed to acquire D3D11 swap chain backbuffer");
        return false;
    }
    D3D11_TEXTURE2D_DESC back_buffer_desc{};
    back_buffer->GetDesc(&back_buffer_desc);
    current_swapchain_format_ = back_buffer_desc.Format;
    if (FAILED(device_->CreateRenderTargetView(back_buffer.Get(), nullptr, render_target_view_.GetAddressOf()))) {
        LOG_ERROR("Failed to create D3D11 render target view");
        return false;
    }
    return createTextTarget();
}


bool D3D11VideoRenderer::Impl::resizeSwapChainLocked(int width, int height, DXGI_FORMAT format) {
    if (!swap_chain_) {
        return false;
    }
    context_->OMSetRenderTargets(0, nullptr, nullptr);
    if (d2d_context_) {
        d2d_context_->SetTarget(nullptr);
    }
    d2d_target_bitmap_.Reset();
    render_target_view_.Reset();
    const HRESULT hr = swap_chain_->ResizeBuffers(0, static_cast<UINT>(std::max(1, width)),
                                                  static_cast<UINT>(std::max(1, height)), format, 0);
    if (FAILED(hr)) {
        LOG_ERROR("ResizeBuffers failed");
        return false;
    }
    return createBackBuffer();
}


bool D3D11VideoRenderer::Impl::createShaders() {
    static const char* kVideoVs = R"(
struct VSIn { float2 pos : POSITION; float2 uv : TEXCOORD0; };
struct VSOut { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };
VSOut main(VSIn input) { VSOut output; output.pos = float4(input.pos, 0.0, 1.0); output.uv = input.uv; return output; }
)";
    static const char* kYuv420Ps = R"(
cbuffer ColorMatrix : register(b0) { float4 coeffR; float4 coeffG; float4 coeffB; float4 colorConfig0; };
Texture2D texY : register(t0); Texture2D texU : register(t1); Texture2D texV : register(t2); SamplerState samp : register(s0);
struct PSIn { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };

float3 decodeGamma22(float3 value) {
    return pow(max(value, float3(0.0, 0.0, 0.0)), float3(2.2, 2.2, 2.2));
}

float3 encodeGamma22(float3 value) {
    return pow(max(value, float3(0.0, 0.0, 0.0)), float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
}

float3 decodePq(float3 value) {
    const float m1 = 0.1593017578125;
    const float m2 = 78.84375;
    const float c1 = 0.8359375;
    const float c2 = 18.8515625;
    const float c3 = 18.6875;
    float3 vp = pow(max(value, float3(0.0, 0.0, 0.0)), float3(1.0 / m2, 1.0 / m2, 1.0 / m2));
    float3 num = max(vp - float3(c1, c1, c1), float3(0.0, 0.0, 0.0));
    float3 den = max(float3(c2, c2, c2) - float3(c3, c3, c3) * vp, float3(1e-6, 1e-6, 1e-6));
    return pow(num / den, float3(1.0 / m1, 1.0 / m1, 1.0 / m1));
}

float3 decodeHlg(float3 value) {
    float3 lower = (value * value) / 3.0;
    float3 upper = (exp((value - float3(0.55991073, 0.55991073, 0.55991073)) /
                        float3(0.17883277, 0.17883277, 0.17883277)) +
                    float3(0.28466892, 0.28466892, 0.28466892)) / 12.0;
    return float3(value.x <= 0.5 ? lower.x : upper.x,
                  value.y <= 0.5 ? lower.y : upper.y,
                  value.z <= 0.5 ? lower.z : upper.z);
}

float3 bt2020ToBt709(float3 value) {
    return float3(
        1.6605 * value.r + (-0.1246) * value.g + (-0.0182) * value.b,
        (-0.5876) * value.r + 1.1329 * value.g + (-0.1006) * value.b,
        (-0.0728) * value.r + (-0.0083) * value.g + 1.1187 * value.b
    );
}

float3 postProcessRgb(float3 encodedRgb) {
    const int transferMode = (int)(colorConfig0.x + 0.5);
    const int gamutMode = (int)(colorConfig0.y + 0.5);
    const bool hdrOutputActive = colorConfig0.z >= 0.5;
    float3 working = max(encodedRgb, float3(0.0, 0.0, 0.0));
    bool linearized = false;

    if (transferMode == 1) {
        working = decodePq(working) * float3(10000.0 / 80.0, 10000.0 / 80.0, 10000.0 / 80.0);
        linearized = true;
    } else if (transferMode == 2) {
        working = decodeHlg(working) * float3(1000.0 / 80.0, 1000.0 / 80.0, 1000.0 / 80.0);
        linearized = true;
    }

    if ((gamutMode == 1 || hdrOutputActive) && !linearized) {
        working = decodeGamma22(working);
        linearized = true;
    }
    if (gamutMode == 1) {
        working = max(bt2020ToBt709(working), float3(0.0, 0.0, 0.0));
    }
    if (hdrOutputActive) {
        return max(working, float3(0.0, 0.0, 0.0));
    }
    if (transferMode != 0) {
        working = working / (float3(1.0, 1.0, 1.0) + working);
    }
    return saturate(linearized ? encodeGamma22(working) : working);
}

float4 main(PSIn input) : SV_Target {
    float y = texY.Sample(samp, input.uv).r;
    float u = texU.Sample(samp, input.uv).r;
    float v = texV.Sample(samp, input.uv).r;
    float3 rgb = float3(dot(float4(y, u, v, 1.0), coeffR), dot(float4(y, u, v, 1.0), coeffG), dot(float4(y, u, v, 1.0), coeffB));
    return float4(postProcessRgb(rgb), 1.0);
}
)";
    static const char* kNv12Ps = R"(
cbuffer ColorMatrix : register(b0) { float4 coeffR; float4 coeffG; float4 coeffB; float4 colorConfig0; };
Texture2D texY : register(t0); Texture2D texUV : register(t1); SamplerState samp : register(s0);
struct PSIn { float4 pos : SV_POSITION; float2 uv : TEXCOORD0; };

float3 decodeGamma22(float3 value) {
    return pow(max(value, float3(0.0, 0.0, 0.0)), float3(2.2, 2.2, 2.2));
}

float3 encodeGamma22(float3 value) {
    return pow(max(value, float3(0.0, 0.0, 0.0)), float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
}

float3 decodePq(float3 value) {
    const float m1 = 0.1593017578125;
    const float m2 = 78.84375;
    const float c1 = 0.8359375;
    const float c2 = 18.8515625;
    const float c3 = 18.6875;
    float3 vp = pow(max(value, float3(0.0, 0.0, 0.0)), float3(1.0 / m2, 1.0 / m2, 1.0 / m2));
    float3 num = max(vp - float3(c1, c1, c1), float3(0.0, 0.0, 0.0));
    float3 den = max(float3(c2, c2, c2) - float3(c3, c3, c3) * vp, float3(1e-6, 1e-6, 1e-6));
    return pow(num / den, float3(1.0 / m1, 1.0 / m1, 1.0 / m1));
}

float3 decodeHlg(float3 value) {
    float3 lower = (value * value) / 3.0;
    float3 upper = (exp((value - float3(0.55991073, 0.55991073, 0.55991073)) /
                        float3(0.17883277, 0.17883277, 0.17883277)) +
                    float3(0.28466892, 0.28466892, 0.28466892)) / 12.0;
    return float3(value.x <= 0.5 ? lower.x : upper.x,
                  value.y <= 0.5 ? lower.y : upper.y,
                  value.z <= 0.5 ? lower.z : upper.z);
}

float3 bt2020ToBt709(float3 value) {
    return float3(
        1.6605 * value.r + (-0.1246) * value.g + (-0.0182) * value.b,
        (-0.5876) * value.r + 1.1329 * value.g + (-0.1006) * value.b,
        (-0.0728) * value.r + (-0.0083) * value.g + 1.1187 * value.b
    );
}

float3 postProcessRgb(float3 encodedRgb) {
    const int transferMode = (int)(colorConfig0.x + 0.5);
    const int gamutMode = (int)(colorConfig0.y + 0.5);
    const bool hdrOutputActive = colorConfig0.z >= 0.5;
    float3 working = max(encodedRgb, float3(0.0, 0.0, 0.0));
    bool linearized = false;

    if (transferMode == 1) {
        working = decodePq(working) * float3(10000.0 / 80.0, 10000.0 / 80.0, 10000.0 / 80.0);
        linearized = true;
    } else if (transferMode == 2) {
        working = decodeHlg(working) * float3(1000.0 / 80.0, 1000.0 / 80.0, 1000.0 / 80.0);
        linearized = true;
    }

    if ((gamutMode == 1 || hdrOutputActive) && !linearized) {
        working = decodeGamma22(working);
        linearized = true;
    }
    if (gamutMode == 1) {
        working = max(bt2020ToBt709(working), float3(0.0, 0.0, 0.0));
    }
    if (hdrOutputActive) {
        return max(working, float3(0.0, 0.0, 0.0));
    }
    if (transferMode != 0) {
        working = working / (float3(1.0, 1.0, 1.0) + working);
    }
    return saturate(linearized ? encodeGamma22(working) : working);
}

float4 main(PSIn input) : SV_Target {
    float y = texY.Sample(samp, input.uv).r;
    float2 uv = texUV.Sample(samp, input.uv).rg;
    float3 rgb = float3(dot(float4(y, uv.x, uv.y, 1.0), coeffR), dot(float4(y, uv.x, uv.y, 1.0), coeffG), dot(float4(y, uv.x, uv.y, 1.0), coeffB));
    return float4(postProcessRgb(rgb), 1.0);
}
)";
    static const char* kColorVs = R"(
struct VSIn { float2 pos : POSITION; float4 color : COLOR0; };
struct VSOut { float4 pos : SV_POSITION; float4 color : COLOR0; };
VSOut main(VSIn input) { VSOut output; output.pos = float4(input.pos, 0.0, 1.0); output.color = input.color; return output; }
)";
    static const char* kColorPs = R"(
struct PSIn { float4 pos : SV_POSITION; float4 color : COLOR0; };
float4 main(PSIn input) : SV_Target { return input.color; }
)";

    ComPtr<ID3DBlob> blob;
    if (FAILED(compileShader(kVideoVs, "main", "vs_4_0", blob)) ||
        FAILED(device_->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
                                           video_vertex_shader_.GetAddressOf()))) {
        return false;
    }
    const D3D11_INPUT_ELEMENT_DESC video_layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    if (FAILED(device_->CreateInputLayout(video_layout, 2, blob->GetBufferPointer(), blob->GetBufferSize(),
                                          video_input_layout_.GetAddressOf()))) {
        return false;
    }
    if (FAILED(compileShader(kYuv420Ps, "main", "ps_4_0", blob)) ||
        FAILED(device_->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
                                          yuv420_pixel_shader_.GetAddressOf()))) {
        return false;
    }
    if (FAILED(compileShader(kNv12Ps, "main", "ps_4_0", blob)) ||
        FAILED(device_->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
                                          nv12_pixel_shader_.GetAddressOf()))) {
        return false;
    }
    if (FAILED(compileShader(kColorVs, "main", "vs_4_0", blob)) ||
        FAILED(device_->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
                                           color_vertex_shader_.GetAddressOf()))) {
        return false;
    }
    const D3D11_INPUT_ELEMENT_DESC color_layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    if (FAILED(device_->CreateInputLayout(color_layout, 2, blob->GetBufferPointer(), blob->GetBufferSize(),
                                          color_input_layout_.GetAddressOf()))) {
        return false;
    }
    if (FAILED(compileShader(kColorPs, "main", "ps_4_0", blob)) ||
        FAILED(device_->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr,
                                          color_pixel_shader_.GetAddressOf()))) {
        return false;
    }

    D3D11_BUFFER_DESC video_vb_desc{};
    video_vb_desc.ByteWidth = sizeof(VideoVertex) * 6;
    video_vb_desc.Usage = D3D11_USAGE_DYNAMIC;
    video_vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    video_vb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    if (FAILED(device_->CreateBuffer(&video_vb_desc, nullptr, video_vertex_buffer_.GetAddressOf()))) {
        return false;
    }

    D3D11_BUFFER_DESC constant_desc{};
    constant_desc.ByteWidth = sizeof(ColorMatrixConstants);
    constant_desc.Usage = D3D11_USAGE_DEFAULT;
    constant_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    if (FAILED(device_->CreateBuffer(&constant_desc, nullptr, color_matrix_buffer_.GetAddressOf()))) {
        return false;
    }
    return true;
}

bool D3D11VideoRenderer::Impl::createBlendState() {
    D3D11_BLEND_DESC desc{};
    desc.RenderTarget[0].BlendEnable = TRUE;
    desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    return SUCCEEDED(device_->CreateBlendState(&desc, alpha_blend_state_.GetAddressOf()));
}

bool D3D11VideoRenderer::Impl::createTextResources() {
    D2D1_FACTORY_OPTIONS factory_options{};
#if defined(DEBUG_MODE)
    factory_options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#endif
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                   __uuidof(ID2D1Factory1),
                                   &factory_options,
                                   reinterpret_cast<void**>(d2d_factory_.GetAddressOf()));
    if (FAILED(hr) || !d2d_factory_) {
        LOG_ERROR("Failed to create D2D factory for subtitle rendering");
        return false;
    }

    ComPtr<IDXGIDevice> dxgi_device;
    if (FAILED(device_.As(&dxgi_device)) ||
        FAILED(d2d_factory_->CreateDevice(dxgi_device.Get(), d2d_device_.GetAddressOf())) ||
        FAILED(d2d_device_->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, d2d_context_.GetAddressOf()))) {
        LOG_ERROR("Failed to create D2D device context for subtitle rendering");
        return false;
    }

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                             __uuidof(IDWriteFactory),
                             reinterpret_cast<IUnknown**>(dwrite_factory_.GetAddressOf()));
    if (FAILED(hr) || !dwrite_factory_) {
        LOG_ERROR("Failed to create DirectWrite factory for subtitle rendering");
        return false;
    }

    if (FAILED(d2d_context_->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.98f), subtitle_fill_brush_.GetAddressOf())) ||
        FAILED(d2d_context_->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.82f), subtitle_shadow_brush_.GetAddressOf())) ||
        FAILED(d2d_context_->CreateSolidColorBrush(D2D1::ColorF(0.02f, 0.02f, 0.02f, 0.58f), subtitle_background_brush_.GetAddressOf()))) {
        LOG_ERROR("Failed to create subtitle brushes for D3D11 renderer");
        return false;
    }

    subtitle::SubtitleDirectWriteCollectionSummary font_collection_summary =
        subtitle::buildDirectWriteSubtitleFontCollection(dwrite_factory_.Get(),
                                                         subtitle_dwrite_font_collection_.ReleaseAndGetAddressOf());
    if (font_collection_summary.collection_created) {
        LOG_INFO("D3D11 subtitle DirectWrite custom font collection enabled: registered="
                 << font_collection_summary.registered_font_file_count
                 << ", added=" << font_collection_summary.added_font_file_count);
    } else if (font_collection_summary.registered_font_file_count > 0) {
        LOG_WARNING("D3D11 subtitle DirectWrite custom font collection unavailable: "
                    << font_collection_summary.error
                    << " (registered=" << font_collection_summary.registered_font_file_count
                    << ", added=" << font_collection_summary.added_font_file_count << ")");
    }

    subtitle_font_size_ = 0.0f;
    return true;
}

bool D3D11VideoRenderer::Impl::createTextTarget() {
    if (!swap_chain_ || !d2d_context_) {
        return false;
    }
    d2d_context_->SetTarget(nullptr);
    d2d_target_bitmap_.Reset();

    ComPtr<IDXGISurface> dxgi_surface;
    if (FAILED(swap_chain_->GetBuffer(0, IID_PPV_ARGS(dxgi_surface.GetAddressOf())))) {
        LOG_ERROR("Failed to acquire DXGI surface for subtitle rendering");
        return false;
    }

    const D2D1_BITMAP_PROPERTIES1 bitmap_props = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(current_swapchain_format_, D2D1_ALPHA_MODE_IGNORE));
    if (FAILED(d2d_context_->CreateBitmapFromDxgiSurface(dxgi_surface.Get(),
                                                         &bitmap_props,
                                                         d2d_target_bitmap_.GetAddressOf())) ||
        !d2d_target_bitmap_) {
        LOG_ERROR("Failed to create D2D target bitmap for subtitle rendering");
        return false;
    }

    d2d_context_->SetTarget(d2d_target_bitmap_.Get());
    d2d_context_->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    return true;
}

bool D3D11VideoRenderer::Impl::updateHdrPresentationLocked(const AVFrame* frame) {
    if (hdr_output_dirty_) {
        refreshOutputBindingLocked();
    }

    const bool content_hdr = frame && isHdrTransfer(frame->color_trc);
    const bool request_hdr = hdr_present_mode_ != D3D11HdrPresentMode::Off &&
                             (content_hdr || hdr_present_mode_ == D3D11HdrPresentMode::Force);
    bool enable_hdr = false;
    std::string decision = "not-evaluated";
    hdr_error_.clear();

    if (hdr_present_mode_ == D3D11HdrPresentMode::Off) {
        decision = "disabled-by-mode";
    } else if (!output_binding_.binding_succeeded) {
        decision = "display-binding-unavailable";
    } else if (!output_binding_.hdr_output.probe_succeeded ||
               !output_binding_.hdr_output.output_found ||
               !output_binding_.hdr_output.has_output6) {
        decision = "probe-unavailable";
    } else if (!(output_binding_.hdr_output.advanced_color_active || output_binding_.hdr_output.hdr_active)) {
        decision = "display-hdr-inactive";
    } else if (!content_hdr && hdr_present_mode_ != D3D11HdrPresentMode::Force) {
        decision = "sdr-content";
    } else {
        enable_hdr = true;
        decision = hdr_present_mode_ == D3D11HdrPresentMode::Force ? "force-enabled" : "auto-enabled";
    }

    DXGI_FORMAT desired_format = enable_hdr ? DXGI_FORMAT_R16G16B16A16_FLOAT
                                            : DXGI_FORMAT_B8G8R8A8_UNORM;
    DXGI_COLOR_SPACE_TYPE desired_color_space = enable_hdr
        ? DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709
        : DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

    auto ensure_swapchain_format = [&](DXGI_FORMAT format) -> bool {
        if (current_swapchain_format_ == format) {
            return true;
        }
        if (!resizeSwapChainLocked(width_.load(), height_.load(), format)) {
            return false;
        }
        hdr_state_reload_count_.fetch_add(1);
        return true;
    };

    if (!ensure_swapchain_format(desired_format)) {
        hdr_error_ = "ResizeBuffers failed";
        enable_hdr = false;
        decision = "swapchain-resize-failed";
        desired_format = DXGI_FORMAT_B8G8R8A8_UNORM;
        desired_color_space = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
        ensure_swapchain_format(desired_format);
    }

    if (!swap_chain3_) {
        if (enable_hdr) {
            hdr_error_ = "IDXGISwapChain3 unavailable";
            enable_hdr = false;
            decision = "swapchain3-unavailable";
            desired_format = DXGI_FORMAT_B8G8R8A8_UNORM;
            desired_color_space = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
            ensure_swapchain_format(desired_format);
        }
    } else {
        UINT color_space_support = 0;
        const HRESULT support_hr = swap_chain3_->CheckColorSpaceSupport(desired_color_space, &color_space_support);
        const bool present_supported = SUCCEEDED(support_hr) &&
            (color_space_support & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) != 0;
        if (!present_supported && enable_hdr) {
            hdr_error_ = "CheckColorSpaceSupport failed";
            enable_hdr = false;
            decision = "swapchain-color-space-unsupported";
            desired_format = DXGI_FORMAT_B8G8R8A8_UNORM;
            desired_color_space = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
            ensure_swapchain_format(desired_format);
        }

        if (SUCCEEDED(swap_chain3_->SetColorSpace1(desired_color_space))) {
            current_output_color_space_ = desired_color_space;
        } else if (enable_hdr) {
            hdr_error_ = "SetColorSpace1 failed";
            enable_hdr = false;
            decision = "set-colorspace-failed";
            desired_format = DXGI_FORMAT_B8G8R8A8_UNORM;
            desired_color_space = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
            ensure_swapchain_format(desired_format);
            if (swap_chain3_) {
                swap_chain3_->SetColorSpace1(desired_color_space);
            }
            current_output_color_space_ = desired_color_space;
        }
    }

    hdr_metadata_available_ = false;
    hdr_metadata_applied_ = false;
    hdr_metadata_source_ = "none";
    if (swap_chain4_) {
        DXGI_HDR_METADATA_HDR10 hdr10_metadata{};
        std::string metadata_source;
        const bool metadata_available = enable_hdr &&
            extractHdr10Metadata(frame, hdr10_metadata, metadata_source);
        hdr_metadata_available_ = metadata_available;
        hdr_metadata_source_ = metadata_available ? metadata_source : "none";
        const HRESULT metadata_hr = metadata_available
            ? swap_chain4_->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10,
                                           sizeof(hdr10_metadata),
                                           &hdr10_metadata)
            : swap_chain4_->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_NONE, 0, nullptr);
        hdr_metadata_applied_ = metadata_available && SUCCEEDED(metadata_hr);
        if (FAILED(metadata_hr) && enable_hdr && hdr_error_.empty()) {
            hdr_error_ = "SetHDRMetaData failed";
        }
    } else if (enable_hdr) {
        hdr_metadata_source_ = "swapchain4-unavailable";
    }

    hdr_present_requested_ = request_hdr;
    hdr_present_active_ = enable_hdr;
    hdr_content_detected_ = content_hdr;
    hdr_output_advanced_color_active_ = output_binding_.hdr_output.advanced_color_active;
    hdr_output_hdr_active_ = output_binding_.hdr_output.hdr_active;
    hdr_present_decision_ = decision;

    return true;
}

ID2D1Bitmap1* D3D11VideoRenderer::Impl::getCachedSubtitleBitmap(const subtitle::SubtitleBitmap& bitmap) {
    if (!d2d_context_ || bitmap.width <= 0 || bitmap.height <= 0) {
        return nullptr;
    }

    const size_t pixel_count = static_cast<size_t>(bitmap.width) * static_cast<size_t>(bitmap.height);
    if (bitmap.rgba.size() < pixel_count * 4u) {
        return nullptr;
    }

    const uint64_t key = hashSubtitleBitmap(bitmap);
    auto create_bitmap = [&](SubtitleBitmapCacheEntry& entry) -> bool {
        if (entry.premul_bgra.empty()) {
            entry.premul_bgra = premultiplySubtitleBitmapBgra(bitmap);
        }
        entry.d2d_bitmap.Reset();
        const D2D1_BITMAP_PROPERTIES1 bitmap_props = D2D1::BitmapProperties1(
            D2D1_BITMAP_OPTIONS_NONE,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            96.0f,
            96.0f);
        const UINT32 pitch = static_cast<UINT32>(static_cast<size_t>(entry.width) * 4u);
        return SUCCEEDED(d2d_context_->CreateBitmap(D2D1::SizeU(static_cast<UINT32>(entry.width),
                                                                static_cast<UINT32>(entry.height)),
                                                    entry.premul_bgra.data(),
                                                    pitch,
                                                    &bitmap_props,
                                                    entry.d2d_bitmap.GetAddressOf())) &&
               entry.d2d_bitmap != nullptr;
    };

    for (SubtitleBitmapCacheEntry& entry : subtitle_bitmap_cache_) {
        if (entry.key != key || entry.width != bitmap.width || entry.height != bitmap.height) {
            continue;
        }
        entry.last_use_token = ++subtitle_bitmap_cache_use_token_;
        if (!entry.d2d_bitmap && !create_bitmap(entry)) {
            return nullptr;
        }
        return entry.d2d_bitmap.Get();
    }

    if (subtitle_bitmap_cache_.size() >= kSubtitleBitmapCacheLimit) {
        auto lru_it = std::min_element(subtitle_bitmap_cache_.begin(),
                                       subtitle_bitmap_cache_.end(),
                                       [](const SubtitleBitmapCacheEntry& lhs, const SubtitleBitmapCacheEntry& rhs) {
                                           return lhs.last_use_token < rhs.last_use_token;
                                       });
        if (lru_it != subtitle_bitmap_cache_.end()) {
            subtitle_bitmap_cache_.erase(lru_it);
        }
    }

    SubtitleBitmapCacheEntry entry;
    entry.key = key;
    entry.width = bitmap.width;
    entry.height = bitmap.height;
    entry.last_use_token = ++subtitle_bitmap_cache_use_token_;
    if (!create_bitmap(entry)) {
        return nullptr;
    }
    subtitle_bitmap_cache_.push_back(std::move(entry));
    return subtitle_bitmap_cache_.back().d2d_bitmap.Get();
}

bool D3D11VideoRenderer::Impl::ensureSubtitleTextFormat(float font_size) {
    if (!dwrite_factory_) {
        return false;
    }
    if (subtitle_text_format_ && std::abs(subtitle_font_size_ - font_size) < 0.5f) {
        return true;
    }

    subtitle_text_format_.Reset();
    const auto create_format = [&](const wchar_t* font_family) -> HRESULT {
        HRESULT hr = E_FAIL;
        if (subtitle_dwrite_font_collection_) {
            subtitle_text_format_.Reset();
            hr = dwrite_factory_->CreateTextFormat(font_family,
                                                   subtitle_dwrite_font_collection_.Get(),
                                                   DWRITE_FONT_WEIGHT_SEMI_BOLD,
                                                   DWRITE_FONT_STYLE_NORMAL,
                                                   DWRITE_FONT_STRETCH_NORMAL,
                                                   font_size,
                                                   L"zh-CN",
                                                   subtitle_text_format_.GetAddressOf());
            if (SUCCEEDED(hr) && subtitle_text_format_) {
                return hr;
            }
        }
        subtitle_text_format_.Reset();
        return dwrite_factory_->CreateTextFormat(font_family,
                                                 nullptr,
                                                 DWRITE_FONT_WEIGHT_SEMI_BOLD,
                                                 DWRITE_FONT_STYLE_NORMAL,
                                                 DWRITE_FONT_STRETCH_NORMAL,
                                                 font_size,
                                                 L"zh-CN",
                                                 subtitle_text_format_.GetAddressOf());
    };

    HRESULT hr = create_format(L"Microsoft YaHei UI");
    if (FAILED(hr)) {
        subtitle_text_format_.Reset();
        hr = create_format(L"Segoe UI");
    }
    if (FAILED(hr) || !subtitle_text_format_) {
        LOG_ERROR("Failed to create DirectWrite subtitle text format");
        return false;
    }

    subtitle_font_size_ = font_size;
    subtitle_text_format_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    subtitle_text_format_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);
    subtitle_text_format_->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
    return true;
}


bool D3D11VideoRenderer::Impl::ensureSoftwareTextures(int width, int height) {
    if (width == software_width_ && height == software_height_ && tex_y_ && tex_u_ && tex_v_) {
        return true;
    }
    software_width_ = width;
    software_height_ = height;
    tex_y_.Reset(); tex_u_.Reset(); tex_v_.Reset();
    srv_y_.Reset(); srv_u_.Reset(); srv_v_.Reset();

    auto create_plane = [this](int plane_width, int plane_height, ComPtr<ID3D11Texture2D>& texture,
                               ComPtr<ID3D11ShaderResourceView>& srv) -> bool {
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = static_cast<UINT>(plane_width);
        desc.Height = static_cast<UINT>(plane_height);
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        if (FAILED(device_->CreateTexture2D(&desc, nullptr, texture.GetAddressOf())) ||
            FAILED(device_->CreateShaderResourceView(texture.Get(), nullptr, srv.GetAddressOf()))) {
            return false;
        }
        return true;
    };

    return create_plane(width, height, tex_y_, srv_y_) &&
           create_plane(std::max(1, (width + 1) / 2), std::max(1, (height + 1) / 2), tex_u_, srv_u_) &&
           create_plane(std::max(1, (width + 1) / 2), std::max(1, (height + 1) / 2), tex_v_, srv_v_);
}

bool D3D11VideoRenderer::Impl::ensureOverlayBuffer(size_t vertex_count) {
    if (vertex_count == 0) {
        return true;
    }
    if (overlay_vertex_buffer_ && vertex_count <= overlay_vertex_capacity_) {
        return true;
    }
    overlay_vertex_capacity_ = std::max<size_t>(vertex_count, overlay_vertex_capacity_ * 2 + 64);
    D3D11_BUFFER_DESC desc{};
    desc.ByteWidth = static_cast<UINT>(overlay_vertex_capacity_ * sizeof(ColorVertex));
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    overlay_vertex_buffer_.Reset();
    return SUCCEEDED(device_->CreateBuffer(&desc, nullptr, overlay_vertex_buffer_.GetAddressOf()));
}

void D3D11VideoRenderer::Impl::disableNativeDirectRendering(const char* reason,
                                                            const D3D11_TEXTURE2D_DESC* texture_desc,
                                                            intptr_t texture_index,
                                                            HRESULT y_plane_hr,
                                                            HRESULT uv_plane_hr) {
    native_srv_y_.Reset();
    native_srv_uv_.Reset();
    native_texture_ptr_ = nullptr;
    native_texture_index_ = 0;
    native_texture_format_ = DXGI_FORMAT_UNKNOWN;

    if (native_direct_rendering_disabled_.exchange(true)) {
        return;
    }

    LOG_WARNING("D3D11 native direct rendering disabled for current session: " << reason
                << " texture_format="
                << (texture_desc ? dxgiFormatName(texture_desc->Format) : "UNKNOWN")
                << " texture_format_id="
                << (texture_desc ? static_cast<int>(texture_desc->Format) : static_cast<int>(DXGI_FORMAT_UNKNOWN))
                << " array_size=" << (texture_desc ? texture_desc->ArraySize : 0)
                << " bind_flags=" << (texture_desc ? texture_desc->BindFlags : 0)
                << " misc_flags=" << (texture_desc ? texture_desc->MiscFlags : 0)
                << " texture_index=" << texture_index
                << " y_plane_hr=" << static_cast<long>(y_plane_hr)
                << " uv_plane_hr=" << static_cast<long>(uv_plane_hr)
                << " fallback=copyback-to-software");
}

bool D3D11VideoRenderer::Impl::ensureNativeShaderResourcesLocked(ID3D11Texture2D* texture, intptr_t index, DXGI_FORMAT format) {
    if (!device3_ || !texture) {
        return false;
    }
    if (texture == native_texture_ptr_ && index == native_texture_index_ && format == native_texture_format_ &&
        native_srv_y_ && native_srv_uv_) {
        return true;
    }
    native_srv_y_.Reset();
    native_srv_uv_.Reset();
    native_texture_ptr_ = texture;
    native_texture_index_ = index;
    native_texture_format_ = format;

    D3D11_TEXTURE2D_DESC texture_desc{};
    texture->GetDesc(&texture_desc);
    const UINT first_slice = texture_desc.ArraySize > 1 ? static_cast<UINT>(std::max<intptr_t>(0, index)) : 0;
    const bool high_bit_depth = format == DXGI_FORMAT_P010 || format == DXGI_FORMAT_P016;
    const DXGI_FORMAT y_format = high_bit_depth ? DXGI_FORMAT_R16_UNORM : DXGI_FORMAT_R8_UNORM;
    const DXGI_FORMAT uv_format = high_bit_depth ? DXGI_FORMAT_R16G16_UNORM : DXGI_FORMAT_R8G8_UNORM;

    D3D11_SHADER_RESOURCE_VIEW_DESC1 y_desc{};
    y_desc.Format = y_format;
    if (texture_desc.ArraySize > 1) {
        y_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        y_desc.Texture2DArray.MostDetailedMip = 0;
        y_desc.Texture2DArray.MipLevels = 1;
        y_desc.Texture2DArray.FirstArraySlice = first_slice;
        y_desc.Texture2DArray.ArraySize = 1;
        y_desc.Texture2DArray.PlaneSlice = 0;
    } else {
        y_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        y_desc.Texture2D.MostDetailedMip = 0;
        y_desc.Texture2D.MipLevels = 1;
        y_desc.Texture2D.PlaneSlice = 0;
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC1 uv_desc = y_desc;
    uv_desc.Format = uv_format;
    if (texture_desc.ArraySize > 1) {
        uv_desc.Texture2DArray.PlaneSlice = 1;
    } else {
        uv_desc.Texture2D.PlaneSlice = 1;
    }

    const HRESULT y_plane_hr = device3_->CreateShaderResourceView1(texture, &y_desc, native_srv_y_.GetAddressOf());
    if (FAILED(y_plane_hr)) {
        disableNativeDirectRendering("CreateShaderResourceView1 for Y plane failed", &texture_desc, index, y_plane_hr, S_OK);
        return false;
    }

    const HRESULT uv_plane_hr = device3_->CreateShaderResourceView1(texture, &uv_desc, native_srv_uv_.GetAddressOf());
    if (FAILED(uv_plane_hr)) {
        disableNativeDirectRendering("CreateShaderResourceView1 for UV plane failed", &texture_desc, index, y_plane_hr, uv_plane_hr);
        return false;
    }

    return true;
}

bool D3D11VideoRenderer::Impl::bindSoftwareFrameLocked(const AVFrame* frame, bool hdr_output_active) {
    if (!frame || !frame->data[0] || !frame->data[1] || !frame->data[2] || frame->width <= 0 || frame->height <= 0) {
        return false;
    }
    if (!ensureSoftwareTextures(frame->width, frame->height)) {
        return false;
    }
    context_->UpdateSubresource(tex_y_.Get(), 0, nullptr, frame->data[0], frame->linesize[0], 0);
    context_->UpdateSubresource(tex_u_.Get(), 0, nullptr, frame->data[1], frame->linesize[1], 0);
    context_->UpdateSubresource(tex_v_.Get(), 0, nullptr, frame->data[2], frame->linesize[2], 0);
    const ColorMatrixConstants matrix = buildColorMatrix(frame, false, hdr_output_active);
    context_->UpdateSubresource(color_matrix_buffer_.Get(), 0, nullptr, &matrix, 0, 0);

    ID3D11ShaderResourceView* srvs[] = {srv_y_.Get(), srv_u_.Get(), srv_v_.Get()};
    context_->PSSetShader(yuv420_pixel_shader_.Get(), nullptr, 0);
    context_->PSSetShaderResources(0, 3, srvs);
    context_->PSSetConstantBuffers(0, 1, color_matrix_buffer_.GetAddressOf());
    return true;
}

bool D3D11VideoRenderer::Impl::bindNativeFrameLocked(const AVFrame* frame, bool hdr_output_active) {
    if (!frame || !device3_ || frame->format != AV_PIX_FMT_D3D11 || !frame->data[0]) {
        return false;
    }
    auto* texture = reinterpret_cast<ID3D11Texture2D*>(frame->data[0]);
    const intptr_t index = reinterpret_cast<intptr_t>(frame->data[1]);
    D3D11_TEXTURE2D_DESC desc{};
    texture->GetDesc(&desc);
    const bool supported = desc.Format == DXGI_FORMAT_NV12 || desc.Format == DXGI_FORMAT_P010 || desc.Format == DXGI_FORMAT_P016;
    if (!supported) {
        disableNativeDirectRendering("decoder surface format is not supported for direct shader sampling",
                                     &desc,
                                     index,
                                     S_OK,
                                     S_OK);
        return false;
    }
    if (!ensureNativeShaderResourcesLocked(texture, index, desc.Format)) {
        return false;
    }
    const ColorMatrixConstants matrix = buildColorMatrix(frame,
                                                         desc.Format == DXGI_FORMAT_P010 || desc.Format == DXGI_FORMAT_P016,
                                                         hdr_output_active);
    context_->UpdateSubresource(color_matrix_buffer_.Get(), 0, nullptr, &matrix, 0, 0);
    ID3D11ShaderResourceView* srvs[] = {native_srv_y_.Get(), native_srv_uv_.Get()};
    context_->PSSetShader(nv12_pixel_shader_.Get(), nullptr, 0);
    context_->PSSetShaderResources(0, 2, srvs);
    context_->PSSetConstantBuffers(0, 1, color_matrix_buffer_.GetAddressOf());
    return true;
}

void D3D11VideoRenderer::Impl::appendRect(std::vector<ColorVertex>& vertices, int x, int y, int w, int h, Color color) const {
    const int window_width = std::max(1, width_.load());
    const int window_height = std::max(1, height_.load());
    if (w <= 0 || h <= 0) {
        return;
    }
    const float left = (static_cast<float>(x) / static_cast<float>(window_width)) * 2.0f - 1.0f;
    const float right = (static_cast<float>(x + w) / static_cast<float>(window_width)) * 2.0f - 1.0f;
    const float top = 1.0f - (static_cast<float>(y) / static_cast<float>(window_height)) * 2.0f;
    const float bottom = 1.0f - (static_cast<float>(y + h) / static_cast<float>(window_height)) * 2.0f;
    vertices.push_back({left, top, color.r, color.g, color.b, color.a});
    vertices.push_back({right, top, color.r, color.g, color.b, color.a});
    vertices.push_back({left, bottom, color.r, color.g, color.b, color.a});
    vertices.push_back({left, bottom, color.r, color.g, color.b, color.a});
    vertices.push_back({right, top, color.r, color.g, color.b, color.a});
    vertices.push_back({right, bottom, color.r, color.g, color.b, color.a});
}

D3D11VideoRenderer::Impl::ControlLayout D3D11VideoRenderer::Impl::computeControlLayout(int window_width, int window_height) const {
    const int safe_width = std::max(1, window_width);
    const int safe_height = std::max(1, window_height);
    ControlLayout layout{};
    layout.panel = {kControlPanelInset, std::max(0, safe_height - kControlPanelHeight - kControlPanelInset),
                    std::max(1, safe_width - (kControlPanelInset * 2)), kControlPanelHeight};
    const int track_y = layout.panel.y + (layout.panel.h - kBarHeight) / 2;
    const int volume_width = std::min(kVolumeBarWidth, std::max(56, layout.panel.w / 5));
    int progress_width = layout.panel.w - (kControlPadding * 2) - volume_width - kControlGap;
    progress_width = std::max(kMinProgressBarWidth, progress_width);
    layout.progress_track = {layout.panel.x + kControlPadding, track_y,
                             std::min(progress_width, std::max(1, layout.panel.w - (kControlPadding * 2))), kBarHeight};
    layout.volume_track = {layout.panel.x + layout.panel.w - kControlPadding - volume_width, track_y,
                           std::max(1, volume_width), kBarHeight};
    return layout;
}

void D3D11VideoRenderer::Impl::drawSubtitleLocked(const ControlLayout& layout) {
    if (!d2d_context_ || !dwrite_factory_ || !d2d_target_bitmap_) {
        return;
    }

    std::vector<subtitle::SubtitleItem> subtitle_items;
    {
        std::lock_guard<std::mutex> subtitle_lock(subtitle_mutex_);
        subtitle_items = subtitle_items_;
    }
    if (subtitle_items.empty()) {
        return;
    }

    std::stable_sort(subtitle_items.begin(), subtitle_items.end(), [](const subtitle::SubtitleItem& lhs, const subtitle::SubtitleItem& rhs) {
        if (lhs.layer != rhs.layer) {
            return lhs.layer < rhs.layer;
        }
        return lhs.index < rhs.index;
    });

    const int window_width = std::max(1, width_.load());
    const int window_height = std::max(1, height_.load());
    SDL_Rect video_rect{0, 0, window_width, window_height};
    if (current_frame_ && current_frame_->width > 0 && current_frame_->height > 0) {
        video_rect = computeRenderRect(window_width, window_height, current_frame_->width, current_frame_->height);
    }

    const float fallback_margin_x = std::max(24.0f, static_cast<float>(video_rect.w) * 0.06f);
    const float fallback_margin_y = std::max(18.0f, static_cast<float>(video_rect.h) * 0.05f);
    float bottom_limit = static_cast<float>(video_rect.y + video_rect.h) - fallback_margin_y;
    if (layout.panel.y > video_rect.y) {
        bottom_limit = std::min(bottom_limit, static_cast<float>(layout.panel.y) - fallback_margin_y);
    }
    const float top_limit = static_cast<float>(video_rect.y) + fallback_margin_y;
    if (bottom_limit <= top_limit) {
        return;
    }

    float next_bottom_y = bottom_limit;
    float next_top_y = top_limit;
    float next_middle_y = top_limit + (bottom_limit - top_limit) * 0.5f;
    const float available_height = std::max(1.0f, bottom_limit - top_limit);
    const float fallback_font_size = std::clamp(static_cast<float>(video_rect.h) * kSubtitleFontScale,
                                                kMinSubtitleFontSize,
                                                kMaxSubtitleFontSize);
    const double subtitle_clock_seconds = subtitle_clock_seconds_.load();

    d2d_context_->SetTarget(d2d_target_bitmap_.Get());
    d2d_context_->SetTransform(D2D1::Matrix3x2F::Identity());
    d2d_context_->BeginDraw();

    bool recreate_target = false;
    for (const subtitle::SubtitleItem& item : subtitle_items) {
        const double item_elapsed_seconds = subtitle_clock_seconds - item.start_seconds;
        const double item_duration_seconds = std::max(0.0, item.end_seconds - item.start_seconds);
        const subtitle::SubtitleStyle item_style = resolveAnimatedSubtitleStyle(item.style,
                                                                                item.animation,
                                                                                item_elapsed_seconds,
                                                                                item_duration_seconds);
        if (item.is_vector_drawing && !item.drawing_commands.empty()) {
            const int alignment = clampSubtitleAlignment(item_style.alignment);
            const float script_scale_x = item.play_res_x > 0 ? static_cast<float>(video_rect.w) / static_cast<float>(item.play_res_x) : 1.0f;
            const float script_scale_y = item.play_res_y > 0 ? static_cast<float>(video_rect.h) / static_cast<float>(item.play_res_y) : 1.0f;
            const SubtitleAnimationState animation_state = resolveSubtitleAnimationState(item, subtitle_clock_seconds);
            if (animation_state.opacity <= 0.001f) {
                continue;
            }
            const float glyph_scale_x = subtitlePercentScale(item_style.scale_x_percent);
            const float glyph_scale_y = subtitlePercentScale(item_style.scale_y_percent);
            const float margin_left = item.play_res_x > 0
                ? std::max(4.0f, static_cast<float>(item_style.margin_l) * script_scale_x)
                : std::max(fallback_margin_x, static_cast<float>(item_style.margin_l));
            const float margin_right = item.play_res_x > 0
                ? std::max(4.0f, static_cast<float>(item_style.margin_r) * script_scale_x)
                : std::max(fallback_margin_x, static_cast<float>(item_style.margin_r));

            float font_size = fallback_font_size;
            if (item.play_res_y > 0 && item_style.font_size > 0.0) {
                font_size = static_cast<float>(item_style.font_size) * script_scale_y;
            } else if (item_style.font_size > 0.0) {
                font_size = std::max(fallback_font_size, static_cast<float>(item_style.font_size));
            }
            font_size = std::clamp(font_size, 10.0f, std::max(36.0f, static_cast<float>(video_rect.h) * 0.35f));

            ParsedAssVectorShape drawing_shape{};
            const float unit_scale = subtitleDrawingUnitScale(item.drawing_scale);
            if (!parseAssVectorShape(item.drawing_commands,
                                     script_scale_x * unit_scale,
                                     script_scale_y * unit_scale,
                                     true,
                                     drawing_shape) ||
                !drawing_shape.valid) {
                continue;
            }

            const float content_width = std::max(1.0f, drawing_shape.bounds.width());
            const float content_height = std::max(1.0f, drawing_shape.bounds.height());
            const float anchor_local_x = subtitleAnchorLocalX(content_width, alignment);
            const float anchor_local_y = subtitleAnchorLocalY(content_height, alignment);
            subtitle::SubtitleStyle preview_style = item_style;
            preview_style.has_rotation_origin = false;
            const SubtitleAffineTransform preview_transform = buildSubtitleAffineTransform(
                preview_style,
                glyph_scale_x,
                glyph_scale_y,
                script_scale_x,
                script_scale_y,
                video_rect,
                D2D1::Point2F(anchor_local_x, anchor_local_y));
            const SubtitleLocalBounds local_bounds = computeTransformedSubtitleBounds(
                drawing_shape.bounds.left,
                drawing_shape.bounds.top,
                drawing_shape.bounds.right,
                drawing_shape.bounds.bottom,
                preview_transform);
            const float transformed_height = std::max(1.0f, local_bounds.height());
            const float gap = std::max(6.0f, std::max(font_size, content_height) * 0.22f * glyph_scale_y);

            float draw_x = static_cast<float>(video_rect.x) + margin_left;
            if (animation_state.has_position) {
                const float anchor_x = static_cast<float>(video_rect.x) + static_cast<float>(animation_state.position_x) * script_scale_x;
                draw_x = anchor_x - anchor_local_x;
            } else {
                switch (subtitleHorizontalGroup(alignment)) {
                case 2:
                    draw_x = static_cast<float>(video_rect.x + video_rect.w) - margin_right - content_width;
                    break;
                case 1:
                    draw_x = static_cast<float>(video_rect.x) + (static_cast<float>(video_rect.w) - content_width) * 0.5f;
                    break;
                default:
                    draw_x = static_cast<float>(video_rect.x) + margin_left;
                    break;
                }
            }
            draw_x = clampSubtitleCoordinate(draw_x,
                                             static_cast<float>(video_rect.x) - local_bounds.left,
                                             static_cast<float>(video_rect.x + video_rect.w) - local_bounds.right);

            float draw_y = top_limit;
            if (animation_state.has_position) {
                const float anchor_y = static_cast<float>(video_rect.y) + static_cast<float>(animation_state.position_y) * script_scale_y;
                draw_y = anchor_y - anchor_local_y;
            } else {
                switch (subtitleVerticalGroup(alignment)) {
                case 2:
                    draw_y = next_top_y - local_bounds.top;
                    next_top_y += transformed_height + gap;
                    break;
                case 1:
                    draw_y = next_middle_y - local_bounds.centerY();
                    next_middle_y += transformed_height + gap;
                    break;
                default:
                    draw_y = next_bottom_y - local_bounds.bottom;
                    next_bottom_y = draw_y + local_bounds.top - gap;
                    break;
                }
            }
            draw_y = clampSubtitleCoordinate(draw_y, top_limit - local_bounds.top, bottom_limit - local_bounds.bottom);

            const float outline_radius_x = std::max(0.0f, static_cast<float>(item_style.outline_x) * script_scale_x);
            const float outline_radius_y = std::max(0.0f, static_cast<float>(item_style.outline_y) * script_scale_y);
            const float shadow_offset_x = static_cast<float>(item_style.shadow_x) * script_scale_x;
            const float shadow_offset_y = static_cast<float>(item_style.shadow_y) * script_scale_y;
            const D2D1_POINT_2F anchor_point = D2D1::Point2F(draw_x + anchor_local_x, draw_y + anchor_local_y);
            const SubtitleAffineTransform item_transform_state = buildSubtitleAffineTransform(
                item_style,
                glyph_scale_x,
                glyph_scale_y,
                script_scale_x,
                script_scale_y,
                video_rect,
                anchor_point);
            const D2D1_MATRIX_3X2_F item_transform = subtitleTransformMatrix(item_transform_state);
            const D2D1_RECT_F item_clip_rect = D2D1::RectF(
                static_cast<float>(video_rect.x) + static_cast<float>(item_style.clip_x1) * script_scale_x,
                static_cast<float>(video_rect.y) + static_cast<float>(item_style.clip_y1) * script_scale_y,
                static_cast<float>(video_rect.x) + static_cast<float>(item_style.clip_x2) * script_scale_x,
                static_cast<float>(video_rect.y) + static_cast<float>(item_style.clip_y2) * script_scale_y);
            const std::vector<D2D1_RECT_F> item_clip_regions = item_style.has_clip
                ? buildSubtitleClipRegions(video_rect, item_clip_rect, item_style.inverse_clip)
                : std::vector<D2D1_RECT_F>{};
            const bool has_rect_item_clip = !item_clip_regions.empty();
            if (item_style.has_clip && !has_rect_item_clip) {
                continue;
            }

            ComPtr<ID2D1PathGeometry> vector_clip_path;
            ComPtr<ID2D1PathGeometry> inverse_clip_path;
            ID2D1Geometry* vector_clip_geometry = nullptr;
            const bool wants_vector_clip = item_style.has_vector_clip && !item_style.vector_clip_commands.empty();
            if (wants_vector_clip) {
                if (!buildAssClipGeometry(d2d_factory_.Get(),
                                          item_style.vector_clip_commands,
                                          item_style.vector_clip_scale,
                                          video_rect,
                                          script_scale_x,
                                          script_scale_y,
                                          vector_clip_path.GetAddressOf()) ||
                    !vector_clip_path) {
                    continue;
                }
                if (item_style.inverse_clip) {
                    if (!buildInverseClipGeometry(d2d_factory_.Get(),
                                                  vector_clip_path.Get(),
                                                  video_rect,
                                                  inverse_clip_path.GetAddressOf()) ||
                        !inverse_clip_path) {
                        continue;
                    }
                    vector_clip_geometry = inverse_clip_path.Get();
                } else {
                    vector_clip_geometry = vector_clip_path.Get();
                }
            }
            const bool has_vector_item_clip = vector_clip_geometry != nullptr;
            ComPtr<ID2D1Layer> item_clip_layer;
            if (has_vector_item_clip) {
                const HRESULT clip_hr = d2d_context_->CreateLayer(item_clip_layer.GetAddressOf());
                if (FAILED(clip_hr) || !item_clip_layer) {
                    continue;
                }
            }

            const auto push_vector_clip = [&]() {
                if (!has_vector_item_clip) {
                    return;
                }
                d2d_context_->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), vector_clip_geometry), item_clip_layer.Get());
            };
            const auto pop_vector_clip = [&]() {
                if (has_vector_item_clip) {
                    d2d_context_->PopLayer();
                }
            };

            ComPtr<ID2D1PathGeometry> drawing_geometry;
            if (!buildAssVectorGeometry(d2d_factory_.Get(), drawing_shape, draw_x, draw_y, drawing_geometry.GetAddressOf()) || !drawing_geometry) {
                continue;
            }
            ComPtr<ID2D1PathGeometry> shadow_geometry;
            const bool has_shadow = std::abs(shadow_offset_x) > 0.001f || std::abs(shadow_offset_y) > 0.001f;
            if (has_shadow &&
                (!buildAssVectorGeometry(d2d_factory_.Get(),
                                         drawing_shape,
                                         draw_x + shadow_offset_x,
                                         draw_y + shadow_offset_y,
                                         shadow_geometry.GetAddressOf()) ||
                 !shadow_geometry)) {
                continue;
            }

            const auto create_color_brush = [&](const subtitle::SubtitleColor& color,
                                                ComPtr<ID2D1SolidColorBrush>& out_brush) -> bool {
                return SUCCEEDED(d2d_context_->CreateSolidColorBrush(
                                     toD2DColor(applySubtitleOpacity(color, animation_state.opacity)),
                                     out_brush.GetAddressOf())) &&
                       out_brush;
            };

            const subtitle::SubtitleColor shadow_color = item_style.background_color.a > 0
                ? item_style.background_color
                : subtitle::SubtitleColor(0, 0, 0, 184);
            ComPtr<ID2D1SolidColorBrush> fill_brush;
            ComPtr<ID2D1SolidColorBrush> outline_brush;
            ComPtr<ID2D1SolidColorBrush> shadow_brush;
            if (!create_color_brush(item_style.primary_color, fill_brush) ||
                !create_color_brush(item_style.outline_color, outline_brush) ||
                !create_color_brush(shadow_color, shadow_brush)) {
                continue;
            }

            const float outline_stroke_width = std::max(1.0f, std::max(outline_radius_x, outline_radius_y));
            const auto draw_geometry_region = [&](ID2D1Geometry* geometry,
                                                  ID2D1Brush* brush,
                                                  bool fill_geometry,
                                                  const D2D1_RECT_F* item_clip_region) {
                d2d_context_->SetTransform(D2D1::Matrix3x2F::Identity());
                if (item_clip_region) {
                    d2d_context_->PushAxisAlignedClip(*item_clip_region, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                }
                push_vector_clip();
                d2d_context_->SetTransform(item_transform);
                if (fill_geometry) {
                    d2d_context_->FillGeometry(geometry, brush);
                } else {
                    d2d_context_->DrawGeometry(geometry, brush, outline_stroke_width);
                }
                d2d_context_->SetTransform(D2D1::Matrix3x2F::Identity());
                pop_vector_clip();
                if (item_clip_region) {
                    d2d_context_->PopAxisAlignedClip();
                }
            };

            const auto draw_geometry_all_regions = [&](ID2D1Geometry* geometry,
                                                       ID2D1Brush* brush,
                                                       bool fill_geometry) {
                if (has_rect_item_clip) {
                    for (const D2D1_RECT_F& item_clip_region : item_clip_regions) {
                        draw_geometry_region(geometry, brush, fill_geometry, &item_clip_region);
                    }
                } else {
                    draw_geometry_region(geometry, brush, fill_geometry, nullptr);
                }
            };

            if (has_shadow && shadow_geometry) {
                draw_geometry_all_regions(shadow_geometry.Get(), shadow_brush.Get(), true);
                if (outline_stroke_width > 0.0f) {
                    draw_geometry_all_regions(shadow_geometry.Get(), shadow_brush.Get(), false);
                }
            }
            if (outline_stroke_width > 0.0f) {
                draw_geometry_all_regions(drawing_geometry.Get(), outline_brush.Get(), false);
            }
            draw_geometry_all_regions(drawing_geometry.Get(), fill_brush.Get(), true);
            continue;
        }

        if (item.is_bitmap) {
            if (item.bitmap_rects.empty()) {
                continue;
            }

            const float scale_x = item.play_res_x > 0
                ? static_cast<float>(video_rect.w) / static_cast<float>(item.play_res_x)
                : 1.0f;
            const float scale_y = item.play_res_y > 0
                ? static_cast<float>(video_rect.h) / static_cast<float>(item.play_res_y)
                : 1.0f;
            d2d_context_->SetTransform(D2D1::Matrix3x2F::Identity());
            for (const subtitle::SubtitleBitmap& bitmap : item.bitmap_rects) {
                if (bitmap.width <= 0 || bitmap.height <= 0) {
                    continue;
                }

                ID2D1Bitmap1* d2d_bitmap = getCachedSubtitleBitmap(bitmap);
                if (!d2d_bitmap) {
                    continue;
                }

                const float dst_left = static_cast<float>(video_rect.x) + static_cast<float>(bitmap.x) * scale_x;
                const float dst_top = static_cast<float>(video_rect.y) + static_cast<float>(bitmap.y) * scale_y;
                const float dst_right = dst_left + static_cast<float>(bitmap.width) * scale_x;
                const float dst_bottom = dst_top + static_cast<float>(bitmap.height) * scale_y;
                if (dst_right <= dst_left + 0.5f || dst_bottom <= dst_top + 0.5f) {
                    continue;
                }

                const D2D1_RECT_F src_rect = D2D1::RectF(0.0f,
                                                         0.0f,
                                                         static_cast<float>(bitmap.width),
                                                         static_cast<float>(bitmap.height));
                const D2D1_RECT_F dst_rect = D2D1::RectF(dst_left, dst_top, dst_right, dst_bottom);
                d2d_context_->DrawBitmap(d2d_bitmap,
                                         &dst_rect,
                                         1.0f,
                                         D2D1_INTERPOLATION_MODE_LINEAR,
                                         &src_rect);
            }
            continue;
        }

        if (item.text.empty()) {
            continue;
        }

        const std::wstring text = decodeSubtitleText(item.text);
        if (text.empty()) {
            continue;
        }

        const int alignment = clampSubtitleAlignment(item_style.alignment);
        const float script_scale_x = item.play_res_x > 0 ? static_cast<float>(video_rect.w) / static_cast<float>(item.play_res_x) : 1.0f;
        const float script_scale_y = item.play_res_y > 0 ? static_cast<float>(video_rect.h) / static_cast<float>(item.play_res_y) : 1.0f;
        const SubtitleAnimationState animation_state = resolveSubtitleAnimationState(item, subtitle_clock_seconds);
        if (animation_state.opacity <= 0.001f) {
            continue;
        }
        const bool has_anchor_position = animation_state.has_position || item_style.has_position;
        const float glyph_scale_x = subtitlePercentScale(item_style.scale_x_percent);
        const float glyph_scale_y = subtitlePercentScale(item_style.scale_y_percent);

        const float margin_left = item.play_res_x > 0
            ? std::max(4.0f, static_cast<float>(item_style.margin_l) * script_scale_x)
            : std::max(fallback_margin_x, static_cast<float>(item_style.margin_l));
        const float margin_right = item.play_res_x > 0
            ? std::max(4.0f, static_cast<float>(item_style.margin_r) * script_scale_x)
            : std::max(fallback_margin_x, static_cast<float>(item_style.margin_r));

        float font_size = fallback_font_size;
        if (item.play_res_y > 0 && item_style.font_size > 0.0) {
            font_size = static_cast<float>(item_style.font_size) * script_scale_y;
        } else if (item_style.font_size > 0.0) {
            font_size = std::max(fallback_font_size, static_cast<float>(item_style.font_size));
        }
        font_size = std::clamp(font_size, 10.0f, std::max(36.0f, static_cast<float>(video_rect.h) * 0.35f));

        const float available_width = std::max(48.0f, static_cast<float>(video_rect.w) - margin_left - margin_right);
        const float layout_height = std::max(24.0f, available_height / glyph_scale_y);
        float layout_width = std::max(24.0f, available_width / glyph_scale_x);
        if (has_anchor_position) {
            layout_width = std::min(layout_width, std::max(font_size * 8.0f, (available_width * 0.45f) / glyph_scale_x));
        }

        subtitle::ensureSubtitleFontsRegistered(item.source_path);

        ComPtr<IDWriteTextFormat> text_format;
        const auto create_text_format = [&](const wchar_t* font_family) -> HRESULT {
            HRESULT hr = E_FAIL;
            if (subtitle_dwrite_font_collection_) {
                text_format.Reset();
                hr = dwrite_factory_->CreateTextFormat(font_family,
                                                       subtitle_dwrite_font_collection_.Get(),
                                                       item_style.bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
                                                       item_style.italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
                                                       DWRITE_FONT_STRETCH_NORMAL,
                                                       font_size,
                                                       L"zh-CN",
                                                       text_format.GetAddressOf());
                if (SUCCEEDED(hr) && text_format) {
                    return hr;
                }
            }
            text_format.Reset();
            return dwrite_factory_->CreateTextFormat(font_family,
                                                     nullptr,
                                                     item_style.bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
                                                     item_style.italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
                                                     DWRITE_FONT_STRETCH_NORMAL,
                                                     font_size,
                                                     L"zh-CN",
                                                     text_format.GetAddressOf());
        };

        HRESULT hr = E_FAIL;
        for (const std::wstring& candidate_family : subtitle::buildSubtitleFontFallbackFamilies(item_style.font_family)) {
            if (candidate_family.empty()) {
                continue;
            }
            text_format.Reset();
            hr = create_text_format(candidate_family.c_str());
            if (SUCCEEDED(hr) && text_format) {
                break;
            }
        }
        if (FAILED(hr) || !text_format) {
            continue;
        }

        text_format->SetTextAlignment(toTextAlignment(alignment));
        text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        text_format->SetWordWrapping(toWordWrapping(item_style.wrap_style));

        ComPtr<IDWriteTextLayout> text_layout;
        hr = dwrite_factory_->CreateTextLayout(text.c_str(),
                                               static_cast<UINT32>(text.size()),
                                               text_format.Get(),
                                               layout_width,
                                               layout_height,
                                               text_layout.GetAddressOf());
        if (FAILED(hr) || !text_layout) {
            continue;
        }

        const DWRITE_TEXT_RANGE full_range{0, static_cast<UINT32>(text.size())};
        text_layout->SetUnderline(item_style.underline, full_range);
        text_layout->SetStrikethrough(item_style.strikeout, full_range);
        ComPtr<IDWriteTextLayout1> text_layout1;
        const bool has_text_layout1 = SUCCEEDED(text_layout.As(&text_layout1)) && text_layout1;
        if (has_text_layout1) {
            const float display_spacing = item.play_res_x > 0
                ? static_cast<float>(item_style.spacing) * script_scale_x
                : static_cast<float>(item_style.spacing);
            const float layout_spacing = display_spacing / glyph_scale_x;
            text_layout1->SetCharacterSpacing(layout_spacing * 0.5f, layout_spacing * 0.5f, 0.0f, full_range);
        }

        for (const subtitle::SubtitleTextRun& run : item.runs) {
            const subtitle::SubtitleStyle run_style = resolveAnimatedSubtitleStyle(run.style,
                                                                                   item.animation,
                                                                                   item_elapsed_seconds,
                                                                                   item_duration_seconds);
            if (run.length == 0) {
                continue;
            }
            const UINT32 start_index = static_cast<UINT32>(std::min(run.start, static_cast<size_t>(text.size())));
            const UINT32 length = static_cast<UINT32>(std::min(run.length, static_cast<size_t>(text.size()) - start_index));
            if (length == 0) {
                continue;
            }
            const DWRITE_TEXT_RANGE range{start_index, length};
            for (const std::wstring& candidate_family : subtitle::buildSubtitleFontFallbackFamilies(run_style.font_family)) {
                if (!candidate_family.empty() && SUCCEEDED(text_layout->SetFontFamilyName(candidate_family.c_str(), range))) {
                    break;
                }
            }
            float run_font_size = font_size;
            if (run_style.font_size > 0.0) {
                run_font_size = item.play_res_y > 0
                    ? static_cast<float>(run_style.font_size) * script_scale_y
                    : static_cast<float>(run_style.font_size);
                run_font_size = std::clamp(run_font_size,
                                           10.0f,
                                           std::max(36.0f, static_cast<float>(video_rect.h) * 0.35f));
            }
            text_layout->SetFontSize(run_font_size, range);
            text_layout->SetFontWeight(run_style.bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL, range);
            text_layout->SetFontStyle(run_style.italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL, range);
            text_layout->SetUnderline(run_style.underline, range);
            text_layout->SetStrikethrough(run_style.strikeout, range);
            if (has_text_layout1 && std::abs(run_style.spacing - item_style.spacing) > 0.001) {
                const float run_display_spacing = item.play_res_x > 0
                    ? static_cast<float>(run_style.spacing) * script_scale_x
                    : static_cast<float>(run_style.spacing);
                const float run_layout_spacing = run_display_spacing / glyph_scale_x;
                text_layout1->SetCharacterSpacing(run_layout_spacing * 0.5f,
                                                  run_layout_spacing * 0.5f,
                                                  0.0f,
                                                  range);
            }
        }

        DWRITE_TEXT_METRICS metrics{};
        if (FAILED(text_layout->GetMetrics(&metrics))) {
            continue;
        }

        const float text_height = std::max(1.0f, metrics.height);
        const float anchor_local_x = subtitleAnchorLocalX(layout_width, alignment);
        const float anchor_local_y = subtitleAnchorLocalY(text_height, alignment);
        subtitle::SubtitleStyle preview_style = item_style;
        preview_style.has_rotation_origin = false;
        const SubtitleAffineTransform preview_transform = buildSubtitleAffineTransform(
            preview_style,
            glyph_scale_x,
            glyph_scale_y,
            script_scale_x,
            script_scale_y,
            video_rect,
            D2D1::Point2F(anchor_local_x, anchor_local_y));
        const SubtitleLocalBounds local_bounds = computeTransformedSubtitleBounds(
            metrics.left,
            metrics.top,
            metrics.left + std::max(1.0f, metrics.widthIncludingTrailingWhitespace),
            metrics.top + text_height,
            preview_transform);
        const float transformed_height = std::max(1.0f, local_bounds.height());
        const float gap = std::max(6.0f, font_size * 0.22f * glyph_scale_y);

        float draw_x = static_cast<float>(video_rect.x) + margin_left;
        if (animation_state.has_position) {
            const float anchor_x = static_cast<float>(video_rect.x) + static_cast<float>(animation_state.position_x) * script_scale_x;
            draw_x = anchor_x - anchor_local_x;
        } else {
            switch (subtitleHorizontalGroup(alignment)) {
            case 2:
                draw_x = static_cast<float>(video_rect.x + video_rect.w) - margin_right - layout_width;
                break;
            case 1:
                draw_x = static_cast<float>(video_rect.x) + (static_cast<float>(video_rect.w) - layout_width) * 0.5f;
                break;
            default:
                draw_x = static_cast<float>(video_rect.x) + margin_left;
                break;
            }
        }
        draw_x = clampSubtitleCoordinate(draw_x,
                                         static_cast<float>(video_rect.x) - local_bounds.left,
                                         static_cast<float>(video_rect.x + video_rect.w) - local_bounds.right);

        float draw_y = top_limit;
        if (animation_state.has_position) {
            const float anchor_y = static_cast<float>(video_rect.y) + static_cast<float>(animation_state.position_y) * script_scale_y;
            draw_y = anchor_y - anchor_local_y;
        } else {
            switch (subtitleVerticalGroup(alignment)) {
            case 2:
                draw_y = next_top_y - local_bounds.top;
                next_top_y += transformed_height + gap;
                break;
            case 1:
                draw_y = next_middle_y - local_bounds.centerY();
                next_middle_y += transformed_height + gap;
                break;
            default:
                draw_y = next_bottom_y - local_bounds.bottom;
                next_bottom_y = draw_y + local_bounds.top - gap;
                break;
            }
        }
        draw_y = clampSubtitleCoordinate(draw_y, top_limit - local_bounds.top, bottom_limit - local_bounds.bottom);

        const float actual_left = draw_x + metrics.left;
        const float actual_top = draw_y + metrics.top;
        const bool draw_box = item_style.border_style == 3;
        const float outline_radius_x = draw_box ? 0.0f : std::max(0.0f, static_cast<float>(item_style.outline_x) * script_scale_x);
        const float outline_radius_y = draw_box ? 0.0f : std::max(0.0f, static_cast<float>(item_style.outline_y) * script_scale_y);
        const float shadow_offset_x = static_cast<float>(item_style.shadow_x) * script_scale_x;
        const float shadow_offset_y = static_cast<float>(item_style.shadow_y) * script_scale_y;
        const subtitle::SubtitleColor box_color_source = item_style.background_color.a > 0
            ? item_style.background_color
            : subtitle::SubtitleColor(5, 5, 5, 148);
        const D2D1_COLOR_F box_color = toD2DColor(applySubtitleOpacity(box_color_source, animation_state.opacity));
        const D2D1_POINT_2F anchor_point = D2D1::Point2F(draw_x + anchor_local_x, draw_y + anchor_local_y);
        const SubtitleAffineTransform item_transform_state = buildSubtitleAffineTransform(
            item_style,
            glyph_scale_x,
            glyph_scale_y,
            script_scale_x,
            script_scale_y,
            video_rect,
            anchor_point);
        const D2D1_MATRIX_3X2_F item_transform = subtitleTransformMatrix(item_transform_state);
        const D2D1_RECT_F item_clip_rect = D2D1::RectF(
            static_cast<float>(video_rect.x) + static_cast<float>(item_style.clip_x1) * script_scale_x,
            static_cast<float>(video_rect.y) + static_cast<float>(item_style.clip_y1) * script_scale_y,
            static_cast<float>(video_rect.x) + static_cast<float>(item_style.clip_x2) * script_scale_x,
            static_cast<float>(video_rect.y) + static_cast<float>(item_style.clip_y2) * script_scale_y);
        const std::vector<D2D1_RECT_F> item_clip_regions = item_style.has_clip
            ? buildSubtitleClipRegions(video_rect, item_clip_rect, item_style.inverse_clip)
            : std::vector<D2D1_RECT_F>{};
        const bool has_rect_item_clip = !item_clip_regions.empty();
        if (item_style.has_clip && !has_rect_item_clip) {
            continue;
        }

        ComPtr<ID2D1PathGeometry> vector_clip_path;
        ComPtr<ID2D1PathGeometry> inverse_clip_path;
        ID2D1Geometry* vector_clip_geometry = nullptr;
        const bool wants_vector_clip = item_style.has_vector_clip && !item_style.vector_clip_commands.empty();
        if (wants_vector_clip) {
            if (!buildAssClipGeometry(d2d_factory_.Get(),
                                      item_style.vector_clip_commands,
                                      item_style.vector_clip_scale,
                                      video_rect,
                                      script_scale_x,
                                      script_scale_y,
                                      vector_clip_path.GetAddressOf()) ||
                !vector_clip_path) {
                continue;
            }
            if (item_style.inverse_clip) {
                if (!buildInverseClipGeometry(d2d_factory_.Get(),
                                              vector_clip_path.Get(),
                                              video_rect,
                                              inverse_clip_path.GetAddressOf()) ||
                    !inverse_clip_path) {
                    continue;
                }
                vector_clip_geometry = inverse_clip_path.Get();
            } else {
                vector_clip_geometry = vector_clip_path.Get();
            }
        }
        const bool has_vector_item_clip = vector_clip_geometry != nullptr;
        ComPtr<ID2D1Layer> item_clip_layer;
        if (has_vector_item_clip) {
            const HRESULT clip_hr = d2d_context_->CreateLayer(item_clip_layer.GetAddressOf());
            if (FAILED(clip_hr) || !item_clip_layer) {
                continue;
            }
        }

        const auto push_vector_clip = [&]() {
            if (!has_vector_item_clip) {
                return;
            }
            d2d_context_->PushLayer(D2D1::LayerParameters(D2D1::InfiniteRect(), vector_clip_geometry), item_clip_layer.Get());
        };

        const auto pop_vector_clip = [&]() {
            if (has_vector_item_clip) {
                d2d_context_->PopLayer();
            }
        };

        ComPtr<ID2D1SolidColorBrush> transparent_brush;
        hr = d2d_context_->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f), transparent_brush.GetAddressOf());
        if (FAILED(hr) || !transparent_brush) {
            continue;
        }

        struct RunBrushPass {
            DWRITE_TEXT_RANGE range{};
            ComPtr<ID2D1SolidColorBrush> fill_brush;
            ComPtr<ID2D1SolidColorBrush> outline_brush;
            ComPtr<ID2D1SolidColorBrush> shadow_brush;
            ComPtr<ID2D1SolidColorBrush> highlight_brush;
            std::vector<KaraokeHighlightClip> highlight_clips;
        };

        std::vector<RunBrushPass> run_passes;
        run_passes.reserve(std::max<size_t>(1, item.runs.size()));

        const auto create_color_brush = [&](const subtitle::SubtitleColor& color,
                                            ComPtr<ID2D1SolidColorBrush>& out_brush) -> bool {
            return SUCCEEDED(d2d_context_->CreateSolidColorBrush(
                                 toD2DColor(applySubtitleOpacity(color, animation_state.opacity)),
                                 out_brush.GetAddressOf())) &&
                   out_brush;
        };

        const auto append_run_pass = [&](const subtitle::SubtitleTextRun& run, UINT32 start_index, UINT32 length) -> bool {
            RunBrushPass pass{};
            pass.range = DWRITE_TEXT_RANGE{start_index, length};
            const subtitle::SubtitleStyle run_style = resolveAnimatedSubtitleStyle(run.style,
                                                                                   item.animation,
                                                                                   item_elapsed_seconds,
                                                                                   item_duration_seconds);

            const KaraokeRunVisualState karaoke_state = resolveKaraokeRunVisualState(run_style, run, item_elapsed_seconds);
            const subtitle::SubtitleColor shadow_color = run_style.background_color.a > 0
                ? run_style.background_color
                : subtitle::SubtitleColor(0, 0, 0, 184);
            if (!create_color_brush(karaoke_state.base_fill_color, pass.fill_brush) ||
                !create_color_brush(run_style.outline_color, pass.outline_brush) ||
                !create_color_brush(shadow_color, pass.shadow_brush)) {
                return false;
            }

            if (karaoke_state.has_highlight_overlay &&
                create_color_brush(karaoke_state.overlay_fill_color, pass.highlight_brush)) {
                std::vector<DWRITE_HIT_TEST_METRICS> hit_metrics;
                if (collectTextRangeMetrics(text_layout.Get(), start_index, length, hit_metrics)) {
                    pass.highlight_clips = buildKaraokeHighlightClips(hit_metrics,
                                                                      karaoke_state.overlay_progress,
                                                                      draw_x,
                                                                      draw_y,
                                                                      item_transform_state);
                }
            }

            run_passes.push_back(std::move(pass));
            return true;
        };

        if (!item.runs.empty()) {
            for (const subtitle::SubtitleTextRun& run : item.runs) {
                if (run.length == 0) {
                    continue;
                }
                const UINT32 start_index = static_cast<UINT32>(std::min(run.start, static_cast<size_t>(text.size())));
                const UINT32 length = static_cast<UINT32>(std::min(run.length, static_cast<size_t>(text.size()) - start_index));
                if (length == 0 || !append_run_pass(run, start_index, length)) {
                    run_passes.clear();
                    break;
                }
            }
        }
        if (run_passes.empty()) {
            subtitle::SubtitleTextRun fallback_run{};
            fallback_run.start = 0;
            fallback_run.length = text.size();
            fallback_run.style = item_style;
            if (!append_run_pass(fallback_run, 0, static_cast<UINT32>(text.size()))) {
                continue;
            }
        }

        enum class TextPassKind { Fill, Outline, Shadow, Highlight };
        const auto apply_pass_effects = [&](TextPassKind kind) {
            for (const RunBrushPass& pass : run_passes) {
                IUnknown* effect = nullptr;
                switch (kind) {
                case TextPassKind::Fill:
                    effect = pass.fill_brush.Get();
                    break;
                case TextPassKind::Outline:
                    effect = pass.outline_brush.Get();
                    break;
                case TextPassKind::Shadow:
                    effect = pass.shadow_brush.Get();
                    break;
                case TextPassKind::Highlight:
                    effect = pass.highlight_brush ? pass.highlight_brush.Get() : transparent_brush.Get();
                    break;
                }
                text_layout->SetDrawingEffect(effect, pass.range);
            }
        };

        const auto draw_text_pass = [&](float pass_offset_x,
                                        float pass_offset_y,
                                        const std::vector<KaraokeHighlightClip>* clips) {
            const auto draw_with_item_clip = [&](const D2D1_RECT_F* item_clip_region) {
                d2d_context_->SetTransform(D2D1::Matrix3x2F::Identity());
                if (item_clip_region) {
                    d2d_context_->PushAxisAlignedClip(*item_clip_region, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                }

                push_vector_clip();

                if (clips && !clips->empty()) {
                    for (const KaraokeHighlightClip& clip : *clips) {
                        d2d_context_->PushAxisAlignedClip(clip.world_rect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                        d2d_context_->SetTransform(item_transform);
                        d2d_context_->DrawTextLayout(D2D1::Point2F(draw_x + pass_offset_x, draw_y + pass_offset_y),
                                                     text_layout.Get(),
                                                     transparent_brush.Get());
                        d2d_context_->SetTransform(D2D1::Matrix3x2F::Identity());
                        d2d_context_->PopAxisAlignedClip();
                    }
                } else {
                    d2d_context_->SetTransform(item_transform);
                    d2d_context_->DrawTextLayout(D2D1::Point2F(draw_x + pass_offset_x, draw_y + pass_offset_y),
                                                 text_layout.Get(),
                                                 transparent_brush.Get());
                    d2d_context_->SetTransform(D2D1::Matrix3x2F::Identity());
                }

                pop_vector_clip();
                if (item_clip_region) {
                    d2d_context_->PopAxisAlignedClip();
                }
            };

            if (has_rect_item_clip) {
                for (const D2D1_RECT_F& item_clip_region : item_clip_regions) {
                    draw_with_item_clip(&item_clip_region);
                }
                return;
            }
            draw_with_item_clip(nullptr);
        };

        if (draw_box) {
            const auto draw_box_region = [&](const D2D1_RECT_F* item_clip_region) {
                d2d_context_->SetTransform(D2D1::Matrix3x2F::Identity());
                if (item_clip_region) {
                    d2d_context_->PushAxisAlignedClip(*item_clip_region, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
                }
                push_vector_clip();
                d2d_context_->SetTransform(item_transform);
                const D2D1_ROUNDED_RECT background = D2D1::RoundedRect(
                    D2D1::RectF(std::max(static_cast<float>(video_rect.x), actual_left - kSubtitleHorizontalPadding),
                                std::max(top_limit, actual_top - kSubtitleVerticalPadding),
                                std::min(static_cast<float>(video_rect.x + video_rect.w),
                                         actual_left + std::max(1.0f, metrics.widthIncludingTrailingWhitespace) + kSubtitleHorizontalPadding),
                                std::min(bottom_limit,
                                         actual_top + text_height + kSubtitleVerticalPadding)),
                    8.0f,
                    8.0f);
                subtitle_background_brush_->SetColor(box_color);
                d2d_context_->FillRoundedRectangle(&background, subtitle_background_brush_.Get());
                d2d_context_->SetTransform(D2D1::Matrix3x2F::Identity());
                pop_vector_clip();
                if (item_clip_region) {
                    d2d_context_->PopAxisAlignedClip();
                }
            };

            if (has_rect_item_clip) {
                for (const D2D1_RECT_F& item_clip_region : item_clip_regions) {
                    draw_box_region(&item_clip_region);
                }
            } else {
                draw_box_region(nullptr);
            }
        }


        if (outline_radius_x > 0.0f || outline_radius_y > 0.0f) {
            static const float kOutlineOffsets[8][2] = {
                {-1.0f, 0.0f},
                {1.0f, 0.0f},
                {0.0f, -1.0f},
                {0.0f, 1.0f},
                {-1.0f, -1.0f},
                {1.0f, -1.0f},
                {-1.0f, 1.0f},
                {1.0f, 1.0f},
            };
            apply_pass_effects(TextPassKind::Outline);
            for (const auto& offset : kOutlineOffsets) {
                draw_text_pass(offset[0] * outline_radius_x,
                               offset[1] * outline_radius_y,
                               nullptr);
            }
        }

        if (std::abs(shadow_offset_x) > 0.001f || std::abs(shadow_offset_y) > 0.001f) {
            apply_pass_effects(TextPassKind::Shadow);
            draw_text_pass(shadow_offset_x, shadow_offset_y, nullptr);
        }

        apply_pass_effects(TextPassKind::Fill);
        draw_text_pass(0.0f, 0.0f, nullptr);

        apply_pass_effects(TextPassKind::Highlight);
        for (const RunBrushPass& pass : run_passes) {
            if (!pass.highlight_brush || pass.highlight_clips.empty()) {
                continue;
            }
            draw_text_pass(0.0f, 0.0f, &pass.highlight_clips);
        }
    }

    const HRESULT hr = d2d_context_->EndDraw();
    if (hr == D2DERR_RECREATE_TARGET) {
        recreate_target = true;
    } else if (FAILED(hr)) {
        LOG_WARNING("D2D subtitle draw failed: " << static_cast<long>(hr));
    }
    if (recreate_target) {
        createTextTarget();
    }
}
bool D3D11VideoRenderer::Impl::renderLocked() {
    if (!initialized_ || !swap_chain_ || !render_target_view_ || minimized_.load()) {
        return false;
    }
    updateHdrPresentationLocked(frame_available_ ? current_frame_ : nullptr);
    if (!render_target_view_) {
        return false;
    }
    D3D11_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(std::max(1, width_.load()));
    viewport.Height = static_cast<float>(std::max(1, height_.load()));
    viewport.MaxDepth = 1.0f;
    context_->RSSetViewports(1, &viewport);
    context_->OMSetRenderTargets(1, render_target_view_.GetAddressOf(), nullptr);
    const float clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    context_->ClearRenderTargetView(render_target_view_.Get(), clear_color);

    if (frame_available_ && current_frame_) {
        const SDL_Rect dst = computeRenderRect(width_.load(), height_.load(), current_frame_->width, current_frame_->height);
        const float left = (static_cast<float>(dst.x) / static_cast<float>(std::max(1, width_.load()))) * 2.0f - 1.0f;
        const float right = (static_cast<float>(dst.x + dst.w) / static_cast<float>(std::max(1, width_.load()))) * 2.0f - 1.0f;
        const float top = 1.0f - (static_cast<float>(dst.y) / static_cast<float>(std::max(1, height_.load()))) * 2.0f;
        const float bottom = 1.0f - (static_cast<float>(dst.y + dst.h) / static_cast<float>(std::max(1, height_.load()))) * 2.0f;
        const VideoVertex vertices[] = {{left, top, 0.0f, 0.0f}, {right, top, 1.0f, 0.0f}, {left, bottom, 0.0f, 1.0f},
                                        {left, bottom, 0.0f, 1.0f}, {right, top, 1.0f, 0.0f}, {right, bottom, 1.0f, 1.0f}};
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(context_->Map(video_vertex_buffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            std::memcpy(mapped.pData, vertices, sizeof(vertices));
            context_->Unmap(video_vertex_buffer_.Get(), 0);
            const UINT stride = sizeof(VideoVertex);
            const UINT offset = 0;
            context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            context_->IASetInputLayout(video_input_layout_.Get());
            context_->IASetVertexBuffers(0, 1, video_vertex_buffer_.GetAddressOf(), &stride, &offset);
            context_->VSSetShader(video_vertex_shader_.Get(), nullptr, 0);
            context_->PSSetSamplers(0, 1, sampler_state_.GetAddressOf());
            const bool bound = current_frame_->format == AV_PIX_FMT_D3D11
                ? bindNativeFrameLocked(current_frame_, hdr_present_active_)
                : bindSoftwareFrameLocked(current_frame_, hdr_present_active_);
            if (bound) {
                context_->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);
                context_->Draw(6, 0);
                ID3D11ShaderResourceView* null_srvs[] = {nullptr, nullptr, nullptr};
                context_->PSSetShaderResources(0, 3, null_srvs);
            }
        }
    }
    std::vector<ColorVertex> overlay_vertices;
    const ControlLayout layout = computeControlLayout(width_.load(), height_.load());
    double progress = 0.0;
    const double duration = overlay_duration_.load();
    if (duration > 0.0) {
        progress = overlay_position_.load() / duration;
    }
    float volume = overlay_volume_.load();
    const bool paused = overlay_paused_.load();
    {
        std::lock_guard<std::mutex> request_lock(request_mutex_);
        if (seek_preview_active_) {
            progress = seek_preview_ratio_;
        }
        if (dragging_volume_) {
            volume = requested_volume_;
        }
    }
    progress = clampRatio(progress);
    volume = clampVolume(volume);
    appendRect(overlay_vertices, layout.panel.x, layout.panel.y, layout.panel.w, layout.panel.h, {0.0f, 0.0f, 0.0f, 0.55f});
    appendRect(overlay_vertices, layout.progress_track.x, layout.progress_track.y, layout.progress_track.w, layout.progress_track.h,
               {0.33f, 0.33f, 0.33f, 0.9f});
    appendRect(overlay_vertices, layout.progress_track.x, layout.progress_track.y,
               std::max(1, static_cast<int>(std::lround(progress * static_cast<double>(layout.progress_track.w)))), layout.progress_track.h,
               {0.96f, 0.96f, 0.96f, 0.92f});
    appendRect(overlay_vertices, layout.volume_track.x, layout.volume_track.y, layout.volume_track.w, layout.volume_track.h,
               {0.33f, 0.33f, 0.33f, 0.9f});
    appendRect(overlay_vertices, layout.volume_track.x, layout.volume_track.y,
               std::max(1, static_cast<int>(std::lround(volume * static_cast<double>(layout.volume_track.w)))), layout.volume_track.h,
               {0.51f, 0.86f, 0.62f, 0.92f});
    if (paused) {
        appendRect(overlay_vertices, layout.panel.x + layout.panel.w / 2 - 6, layout.panel.y + 8, 12, 6,
                   {1.0f, 0.8f, 0.4f, 0.92f});
    }
    if (!overlay_vertices.empty() && ensureOverlayBuffer(overlay_vertices.size())) {
        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (SUCCEEDED(context_->Map(overlay_vertex_buffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            std::memcpy(mapped.pData, overlay_vertices.data(), overlay_vertices.size() * sizeof(ColorVertex));
            context_->Unmap(overlay_vertex_buffer_.Get(), 0);
            const UINT stride = sizeof(ColorVertex);
            const UINT offset = 0;
            context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            context_->IASetInputLayout(color_input_layout_.Get());
            context_->IASetVertexBuffers(0, 1, overlay_vertex_buffer_.GetAddressOf(), &stride, &offset);
            context_->VSSetShader(color_vertex_shader_.Get(), nullptr, 0);
            context_->PSSetShader(color_pixel_shader_.Get(), nullptr, 0);
            context_->OMSetBlendState(alpha_blend_state_.Get(), nullptr, 0xFFFFFFFFu);
            context_->Draw(static_cast<UINT>(overlay_vertices.size()), 0);
            context_->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFFu);
        }
    }

    context_->OMSetRenderTargets(0, nullptr, nullptr);
    context_->Flush();
    drawSubtitleLocked(layout);

    const auto present_start = std::chrono::steady_clock::now();
    const HRESULT hr = swap_chain_->Present(1, 0);
    present_count_.fetch_add(1);
    updatePresentTiming(static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - present_start).count()));
    if (FAILED(hr)) {
        present_failures_.fetch_add(1);
    }
    return SUCCEEDED(hr);
}


void D3D11VideoRenderer::Impl::redrawIfPaused() {
    if (!overlay_paused_.load()) {
        return;
    }
    std::lock_guard<std::mutex> render_lock(render_mutex_);
    renderLocked();
}

void D3D11VideoRenderer::Impl::resetRequests() {
    should_quit_.store(false);
    fullscreen_.store(false);
    minimized_.store(false);
    toggle_pause_requested_ = false;
    seek_requested_ = false;
    seek_ratio_ = 0.0;
    seek_delta_requested_ = false;
    seek_delta_seconds_ = 0.0;
    volume_change_requested_ = false;
    requested_volume_ = 1.0f;
    speed_change_requested_ = false;
    speed_delta_ = 0.0;
    speed_reset_requested_ = false;
    subtitle_toggle_requested_ = false;
    ab_repeat_start_requested_ = false;
    ab_repeat_end_requested_ = false;
    ab_repeat_clear_requested_ = false;
    screenshot_requested_ = false;
    step_frame_backward_requested_ = false;
    step_frame_forward_requested_ = false;
    subtitle_delay_change_requested_ = false;
    subtitle_delay_delta_seconds_ = 0.0;
    audio_delay_change_requested_ = false;
    audio_delay_delta_seconds_ = 0.0;
    next_chapter_requested_ = false;
    previous_chapter_requested_ = false;
    next_item_requested_ = false;
    previous_item_requested_ = false;
    open_file_requested_ = false;
    open_file_path_.clear();
    last_nonzero_volume_ = 1.0f;
    dragging_seek_ = false;
    dragging_volume_ = false;
    seek_preview_active_ = false;
    seek_preview_ratio_ = 0.0;
}

void D3D11VideoRenderer::Impl::toggleFullscreen() {
    if (!window_) {
        return;
    }
    const bool next = !fullscreen_.load();
    if (SDL_SetWindowFullscreen(window_, next ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) != 0) {
        LOG_WARNING("SDL_SetWindowFullscreen failed: " << SDL_GetError());
        return;
    }
    fullscreen_.store(next);
    updateWindowSizeFromSdl(window_, width_, height_);
    std::lock_guard<std::mutex> render_lock(render_mutex_);
    markHdrOutputDirty();
    resizeSwapChainLocked(width_.load(), height_.load(), current_swapchain_format_);
    renderLocked();
}

void D3D11VideoRenderer::Impl::renderFrame(const core::VideoFrame& frame) {
    if (!frame.valid || !frame.frame) {
        return;
    }
    std::lock_guard<std::mutex> render_lock(render_mutex_);
    if (!current_frame_) {
        current_frame_ = av_frame_alloc();
        if (!current_frame_) {
            return;
        }
    }
    av_frame_unref(current_frame_);
    if (av_frame_ref(current_frame_, frame.frame) < 0) {
        return;
    }
    frame_available_ = true;
}

void D3D11VideoRenderer::Impl::present() {
    std::lock_guard<std::mutex> render_lock(render_mutex_);
    renderLocked();
}

void D3D11VideoRenderer::Impl::clear() {
    std::lock_guard<std::mutex> render_lock(render_mutex_);
    frame_available_ = false;
    if (current_frame_) {
        av_frame_unref(current_frame_);
    }
    renderLocked();
}

void D3D11VideoRenderer::Impl::handleEvents() {
    if (!window_) {
        return;
    }
    if (event_thread_id_ != std::thread::id{} && event_thread_id_ != std::this_thread::get_id()) {
        if (!event_thread_guard_reported_.exchange(true)) {
            LOG_WARNING("D3D11 renderer handleEvents called from non-event thread; ignoring event pump");
        }
        return;
    }
    bool needs_redraw = false;
    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            should_quit_.store(true);
            break;
        case SDL_KEYDOWN: {
            const SDL_Keycode key_code = event.key.keysym.sym;
            if (key_code == SDLK_ESCAPE) {
                if (fullscreen_.load()) {
                    toggleFullscreen();
                } else {
                    should_quit_.store(true);
                }
                break;
            }
            if (key_code == SDLK_RETURN || key_code == SDLK_KP_ENTER) {
                toggleFullscreen();
                break;
            }
            std::optional<input::PlayerAction> action = hotkey_manager_.actionForKey(key_code);
            if (!action && key_code == SDLK_EQUALS) action = input::PlayerAction::VolumeUp;
            if (!action && key_code == SDLK_MINUS) action = input::PlayerAction::VolumeDown;
            if (!action) break;
            std::lock_guard<std::mutex> request_lock(request_mutex_);
            switch (*action) {
            case input::PlayerAction::PlayPause: toggle_pause_requested_ = true; break;
            case input::PlayerAction::SeekBackward:
            case input::PlayerAction::SeekForward: seek_delta_seconds_ += ((*action == input::PlayerAction::SeekForward) ? 1.0 : -1.0) * (((event.key.keysym.mod & KMOD_CTRL) != 0) ? kSeekStepSecondsCtrl : kSeekStepSeconds); seek_delta_requested_ = true; break;
            case input::PlayerAction::VolumeUp:
            case input::PlayerAction::VolumeDown: requested_volume_ = clampVolume(overlay_volume_.load() + ((*action == input::PlayerAction::VolumeUp) ? kVolumeStep : -kVolumeStep)); volume_change_requested_ = true; overlay_volume_.store(requested_volume_); needs_redraw = true; break;
            case input::PlayerAction::ToggleMute: requested_volume_ = overlay_volume_.load() > 0.0001f ? 0.0f : std::max(0.5f, last_nonzero_volume_); volume_change_requested_ = true; overlay_volume_.store(requested_volume_); needs_redraw = true; break;
            case input::PlayerAction::SpeedDown:
            case input::PlayerAction::SpeedUp: speed_delta_ += (*action == input::PlayerAction::SpeedUp) ? kSpeedStep : -kSpeedStep; speed_change_requested_ = true; break;
            case input::PlayerAction::SpeedReset: speed_reset_requested_ = true; break;
            case input::PlayerAction::ToggleSubtitle: subtitle_toggle_requested_ = true; break;
            case input::PlayerAction::SetABRepeatStart: ab_repeat_start_requested_ = true; break;
            case input::PlayerAction::SetABRepeatEnd: ab_repeat_end_requested_ = true; break;
            case input::PlayerAction::ClearABRepeat: ab_repeat_clear_requested_ = true; break;
            case input::PlayerAction::TakeScreenshot: screenshot_requested_ = true; break;
            case input::PlayerAction::StepFrameBackward: step_frame_backward_requested_ = true; break;
            case input::PlayerAction::StepFrameForward: step_frame_forward_requested_ = true; break;
            case input::PlayerAction::SubtitleDelayDown:
            case input::PlayerAction::SubtitleDelayUp: if ((event.key.keysym.mod & KMOD_CTRL) != 0) { audio_delay_delta_seconds_ += (*action == input::PlayerAction::SubtitleDelayUp) ? 0.1 : -0.1; audio_delay_change_requested_ = true; } else { subtitle_delay_delta_seconds_ += (*action == input::PlayerAction::SubtitleDelayUp) ? 0.1 : -0.1; subtitle_delay_change_requested_ = true; } break;
            case input::PlayerAction::PreviousChapter: previous_chapter_requested_ = true; break;
            case input::PlayerAction::NextChapter: next_chapter_requested_ = true; break;
            case input::PlayerAction::PreviousItem: previous_item_requested_ = true; break;
            case input::PlayerAction::NextItem: next_item_requested_ = true; break;
            case input::PlayerAction::ToggleFullscreen: toggleFullscreen(); break;
            case input::PlayerAction::Quit: should_quit_.store(true); break;
            case input::PlayerAction::SeekTo10Percent: case input::PlayerAction::SeekTo20Percent: case input::PlayerAction::SeekTo30Percent: case input::PlayerAction::SeekTo40Percent: case input::PlayerAction::SeekTo50Percent: case input::PlayerAction::SeekTo60Percent: case input::PlayerAction::SeekTo70Percent: case input::PlayerAction::SeekTo80Percent: case input::PlayerAction::SeekTo90Percent: {
                const int ordinal = static_cast<int>(*action) - static_cast<int>(input::PlayerAction::SeekTo10Percent) + 1;
                seek_ratio_ = ordinal / 10.0; seek_requested_ = true; seek_preview_active_ = false; break; }
            }
            break;
        }
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_MINIMIZED || event.window.event == SDL_WINDOWEVENT_HIDDEN) {
                minimized_.store(true);
            } else if (event.window.event == SDL_WINDOWEVENT_MOVED ||
                       event.window.event == SDL_WINDOWEVENT_DISPLAY_CHANGED ||
                       event.window.event == SDL_WINDOWEVENT_RESTORED ||
                       event.window.event == SDL_WINDOWEVENT_SHOWN ||
                       event.window.event == SDL_WINDOWEVENT_MAXIMIZED ||
                       event.window.event == SDL_WINDOWEVENT_RESIZED ||
                       event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                minimized_.store(false);
                updateWindowSizeFromSdl(window_, width_, height_);
                std::lock_guard<std::mutex> render_lock(render_mutex_);
                markHdrOutputDirty();
                resizeSwapChainLocked(width_.load(), height_.load(), current_swapchain_format_);
                needs_redraw = true;
            }
            break;
        case SDL_DISPLAYEVENT:
            {
                std::lock_guard<std::mutex> render_lock(render_mutex_);
                markHdrOutputDirty();
                needs_redraw = true;
            }
            break;
        case SDL_DROPFILE:
            {
                std::lock_guard<std::mutex> request_lock(request_mutex_);
                open_file_path_ = event.drop.file ? event.drop.file : "";
                open_file_requested_ = !open_file_path_.empty();
                if (event.drop.file) {
                    SDL_free(event.drop.file);
                }
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                const ControlLayout layout = computeControlLayout(width_.load(), height_.load());
                if (pointInRect(event.button.x, event.button.y, layout.progress_track)) {
                    dragging_seek_ = true; seek_preview_active_ = true; seek_preview_ratio_ = clampRatio(static_cast<double>(event.button.x - layout.progress_track.x) / static_cast<double>(layout.progress_track.w)); needs_redraw = true;
                } else if (pointInRect(event.button.x, event.button.y, layout.volume_track)) {
                    dragging_volume_ = true; requested_volume_ = static_cast<float>(clampRatio(static_cast<double>(event.button.x - layout.volume_track.x) / static_cast<double>(layout.volume_track.w))); volume_change_requested_ = true; overlay_volume_.store(requested_volume_); needs_redraw = true;
                }
            }
            break;
        case SDL_MOUSEMOTION:
            if (dragging_seek_) { const ControlLayout layout = computeControlLayout(width_.load(), height_.load()); seek_preview_ratio_ = clampRatio(static_cast<double>(event.motion.x - layout.progress_track.x) / static_cast<double>(layout.progress_track.w)); needs_redraw = true; }
            if (dragging_volume_) { const ControlLayout layout = computeControlLayout(width_.load(), height_.load()); requested_volume_ = static_cast<float>(clampRatio(static_cast<double>(event.motion.x - layout.volume_track.x) / static_cast<double>(layout.volume_track.w))); volume_change_requested_ = true; overlay_volume_.store(requested_volume_); needs_redraw = true; }
            break;
        case SDL_MOUSEBUTTONUP:
            if (event.button.button == SDL_BUTTON_LEFT) {
                if (dragging_seek_) { seek_ratio_ = seek_preview_ratio_; seek_requested_ = true; seek_preview_active_ = false; dragging_seek_ = false; needs_redraw = true; }
                if (dragging_volume_) { dragging_volume_ = false; needs_redraw = true; }
            }
            break;
        default:
            break;
        }
    }
    if (needs_redraw) {
        redrawIfPaused();
    }
}

#define VP_CONSUME_FLAG(flag) std::lock_guard<std::mutex> request_lock(request_mutex_); if (!(flag)) return false; (flag) = false; return true;

bool D3D11VideoRenderer::Impl::consumeTogglePauseRequest() { VP_CONSUME_FLAG(toggle_pause_requested_); }
bool D3D11VideoRenderer::Impl::consumeSeekRequest(double& normalized_position) { std::lock_guard<std::mutex> request_lock(request_mutex_); if (!seek_requested_) return false; normalized_position = seek_ratio_; seek_requested_ = false; return true; }
bool D3D11VideoRenderer::Impl::consumeSeekDeltaRequest(double& delta_seconds) { std::lock_guard<std::mutex> request_lock(request_mutex_); if (!seek_delta_requested_) return false; delta_seconds = seek_delta_seconds_; seek_delta_seconds_ = 0.0; seek_delta_requested_ = false; return true; }
bool D3D11VideoRenderer::Impl::consumeVolumeChangeRequest(float& volume) { std::lock_guard<std::mutex> request_lock(request_mutex_); if (!volume_change_requested_) return false; volume = requested_volume_; volume_change_requested_ = false; if (volume > 0.0001f) last_nonzero_volume_ = volume; return true; }
bool D3D11VideoRenderer::Impl::consumeSpeedChangeRequest(double& speed_delta) { std::lock_guard<std::mutex> request_lock(request_mutex_); if (!speed_change_requested_) return false; speed_delta = speed_delta_; speed_delta_ = 0.0; speed_change_requested_ = false; return true; }
bool D3D11VideoRenderer::Impl::consumeResetSpeedRequest() { VP_CONSUME_FLAG(speed_reset_requested_); }
bool D3D11VideoRenderer::Impl::consumeToggleSubtitleRequest() { VP_CONSUME_FLAG(subtitle_toggle_requested_); }
bool D3D11VideoRenderer::Impl::consumeSetABRepeatStartRequest() { VP_CONSUME_FLAG(ab_repeat_start_requested_); }
bool D3D11VideoRenderer::Impl::consumeSetABRepeatEndRequest() { VP_CONSUME_FLAG(ab_repeat_end_requested_); }
bool D3D11VideoRenderer::Impl::consumeClearABRepeatRequest() { VP_CONSUME_FLAG(ab_repeat_clear_requested_); }
bool D3D11VideoRenderer::Impl::consumeScreenshotRequest() { VP_CONSUME_FLAG(screenshot_requested_); }
bool D3D11VideoRenderer::Impl::consumeStepFrameBackwardRequest() { VP_CONSUME_FLAG(step_frame_backward_requested_); }
bool D3D11VideoRenderer::Impl::consumeStepFrameForwardRequest() { VP_CONSUME_FLAG(step_frame_forward_requested_); }
bool D3D11VideoRenderer::Impl::consumePreviousSubtitleTrackRequest() { return false; }
bool D3D11VideoRenderer::Impl::consumeNextSubtitleTrackRequest() { return false; }
bool D3D11VideoRenderer::Impl::consumeSubtitleDelayChangeRequest(double& delta_seconds) { std::lock_guard<std::mutex> request_lock(request_mutex_); if (!subtitle_delay_change_requested_) return false; delta_seconds = subtitle_delay_delta_seconds_; subtitle_delay_delta_seconds_ = 0.0; subtitle_delay_change_requested_ = false; return true; }
bool D3D11VideoRenderer::Impl::consumeAudioDelayChangeRequest(double& delta_seconds) { std::lock_guard<std::mutex> request_lock(request_mutex_); if (!audio_delay_change_requested_) return false; delta_seconds = audio_delay_delta_seconds_; audio_delay_delta_seconds_ = 0.0; audio_delay_change_requested_ = false; return true; }
bool D3D11VideoRenderer::Impl::consumeNextChapterRequest() { VP_CONSUME_FLAG(next_chapter_requested_); }
bool D3D11VideoRenderer::Impl::consumePreviousChapterRequest() { VP_CONSUME_FLAG(previous_chapter_requested_); }
bool D3D11VideoRenderer::Impl::consumeNextItemRequest() { VP_CONSUME_FLAG(next_item_requested_); }
bool D3D11VideoRenderer::Impl::consumePreviousItemRequest() { VP_CONSUME_FLAG(previous_item_requested_); }
bool D3D11VideoRenderer::Impl::consumeOpenFileRequest(std::string& path) {
    std::lock_guard<std::mutex> request_lock(request_mutex_);
    if (!open_file_requested_) return false;
    path = open_file_path_;
    open_file_path_.clear();
    open_file_requested_ = false;
    return !path.empty();
}
#undef VP_CONSUME_FLAG

void D3D11VideoRenderer::Impl::setOverlayState(double position, double duration, float volume, bool paused) { overlay_position_.store(std::max(0.0, position)); overlay_duration_.store(std::max(0.0, duration)); overlay_volume_.store(clampVolume(volume)); overlay_paused_.store(paused); if (paused) redrawIfPaused(); }
void D3D11VideoRenderer::Impl::setSubtitleClock(double subtitle_time_seconds) {
    const double previous = subtitle_clock_seconds_.exchange(std::max(0.0, subtitle_time_seconds));
    if (overlay_paused_.load() &&
        subtitle_has_animated_content_.load() &&
        std::abs(previous - subtitle_time_seconds) > 0.0005) {
        redrawIfPaused();
    }
}
void D3D11VideoRenderer::Impl::setSubtitleText(const std::string& text) {
    if (text.empty()) {
        setSubtitleItems({});
        return;
    }

    subtitle::SubtitleItem item{};
    item.text = text;
    item.raw_text = text;
    item.style = makePlainSubtitleStyle();
    const size_t visible_length = countUtf16CodeUnits(text);
    if (visible_length > 0) {
        item.runs.push_back(subtitle::SubtitleTextRun{0, visible_length, item.style});
    }
    setSubtitleItems({item});
}
void D3D11VideoRenderer::Impl::setSubtitleItems(const std::vector<subtitle::SubtitleItem>& items) {
    {
        std::lock_guard<std::mutex> subtitle_lock(subtitle_mutex_);
        subtitle_items_ = items;
    }
    subtitle_has_animated_content_.store(subtitleItemsHaveAnimatedRuns(items));
    if (overlay_paused_.load()) {
        redrawIfPaused();
    }
}
void D3D11VideoRenderer::Impl::setHotkeyManager(const input::HotkeyManager& hotkey_manager) { std::lock_guard<std::mutex> request_lock(request_mutex_); hotkey_manager_ = hotkey_manager; }
RendererDiagnostics D3D11VideoRenderer::Impl::getDiagnostics() const {
    std::lock_guard<std::mutex> render_lock(render_mutex_);
    RendererDiagnostics diagnostics;
    diagnostics.d3d11_hdr_present_requested = hdr_present_requested_;
    diagnostics.d3d11_hdr_present_active = hdr_present_active_;
    diagnostics.d3d11_hdr_content_detected = hdr_content_detected_;
    diagnostics.d3d11_hdr_present_mode = d3d11HdrPresentModeName(hdr_present_mode_);
    diagnostics.d3d11_hdr_present_decision = hdr_present_decision_;
    diagnostics.d3d11_hdr_swapchain_format = dxgiFormatName(current_swapchain_format_);
    diagnostics.d3d11_hdr_output_color_space = dxgiColorSpaceName(current_output_color_space_);
    diagnostics.d3d11_hdr_output_display_index = output_binding_.sdl_display_index;
    diagnostics.d3d11_hdr_output_display_name = output_binding_.sdl_display_name;
    diagnostics.d3d11_hdr_output_device_name = output_binding_.output_device_name;
    diagnostics.d3d11_hdr_output_advanced_color_active = hdr_output_advanced_color_active_;
    diagnostics.d3d11_hdr_output_hdr_active = hdr_output_hdr_active_;
    diagnostics.d3d11_hdr_metadata_available = hdr_metadata_available_;
    diagnostics.d3d11_hdr_metadata_applied = hdr_metadata_applied_;
    diagnostics.d3d11_hdr_metadata_source = hdr_metadata_source_;
    diagnostics.d3d11_hdr_state_reload_count = hdr_state_reload_count_.load();
    diagnostics.d3d11_present_count = present_count_.load();
    diagnostics.d3d11_present_failures = present_failures_.load();
    diagnostics.d3d11_present_time_us_total = present_time_us_total_.load();
    diagnostics.d3d11_present_time_us_max = present_time_us_max_.load();
    diagnostics.d3d11_hdr_error = hdr_error_.empty() ? output_binding_.binding_error : hdr_error_;
    return diagnostics;
}
void D3D11VideoRenderer::Impl::resetDiagnostics() {
    std::lock_guard<std::mutex> render_lock(render_mutex_);
    resetHdrDiagnosticsLocked();
}
bool D3D11VideoRenderer::Impl::supportsNativeFrameFormat(AVPixelFormat format) const { return format == AV_PIX_FMT_D3D11 && device3_ != nullptr && !native_direct_rendering_disabled_.load(); }
void* D3D11VideoRenderer::Impl::nativeDeviceHandle() const { return device_.Get(); }

D3D11VideoRenderer::D3D11VideoRenderer() : impl_(std::make_unique<Impl>()) {}
D3D11VideoRenderer::~D3D11VideoRenderer() = default;
D3D11DiagnosticsSnapshot D3D11VideoRenderer::probeSystemDiagnostics() {
    return buildD3D11DiagnosticsSnapshot(createD3D11DeviceProbeContext());
}
bool D3D11VideoRenderer::init(const VideoRendererConfig& config) { return impl_->init(config); }
void D3D11VideoRenderer::close() { impl_->close(); }
void D3D11VideoRenderer::renderFrame(const core::VideoFrame& frame) { impl_->renderFrame(frame); }
void D3D11VideoRenderer::present() { impl_->present(); }
void D3D11VideoRenderer::clear() { impl_->clear(); }
void D3D11VideoRenderer::handleEvents() { impl_->handleEvents(); }
bool D3D11VideoRenderer::shouldQuit() const { return impl_->shouldQuit(); }
bool D3D11VideoRenderer::consumeTogglePauseRequest() { return impl_->consumeTogglePauseRequest(); }
bool D3D11VideoRenderer::consumeSeekRequest(double& normalized_position) { return impl_->consumeSeekRequest(normalized_position); }
bool D3D11VideoRenderer::consumeSeekDeltaRequest(double& delta_seconds) { return impl_->consumeSeekDeltaRequest(delta_seconds); }
bool D3D11VideoRenderer::consumeVolumeChangeRequest(float& volume) { return impl_->consumeVolumeChangeRequest(volume); }
bool D3D11VideoRenderer::consumeSpeedChangeRequest(double& speed_delta) { return impl_->consumeSpeedChangeRequest(speed_delta); }
bool D3D11VideoRenderer::consumeResetSpeedRequest() { return impl_->consumeResetSpeedRequest(); }
bool D3D11VideoRenderer::consumeToggleSubtitleRequest() { return impl_->consumeToggleSubtitleRequest(); }
bool D3D11VideoRenderer::consumeSetABRepeatStartRequest() { return impl_->consumeSetABRepeatStartRequest(); }
bool D3D11VideoRenderer::consumeSetABRepeatEndRequest() { return impl_->consumeSetABRepeatEndRequest(); }
bool D3D11VideoRenderer::consumeClearABRepeatRequest() { return impl_->consumeClearABRepeatRequest(); }
bool D3D11VideoRenderer::consumeScreenshotRequest() { return impl_->consumeScreenshotRequest(); }
bool D3D11VideoRenderer::consumeStepFrameBackwardRequest() { return impl_->consumeStepFrameBackwardRequest(); }
bool D3D11VideoRenderer::consumeStepFrameForwardRequest() { return impl_->consumeStepFrameForwardRequest(); }
bool D3D11VideoRenderer::consumePreviousSubtitleTrackRequest() { return impl_->consumePreviousSubtitleTrackRequest(); }
bool D3D11VideoRenderer::consumeNextSubtitleTrackRequest() { return impl_->consumeNextSubtitleTrackRequest(); }
bool D3D11VideoRenderer::consumeSubtitleDelayChangeRequest(double& delta_seconds) { return impl_->consumeSubtitleDelayChangeRequest(delta_seconds); }
bool D3D11VideoRenderer::consumeAudioDelayChangeRequest(double& delta_seconds) { return impl_->consumeAudioDelayChangeRequest(delta_seconds); }
bool D3D11VideoRenderer::consumeNextChapterRequest() { return impl_->consumeNextChapterRequest(); }
bool D3D11VideoRenderer::consumePreviousChapterRequest() { return impl_->consumePreviousChapterRequest(); }
bool D3D11VideoRenderer::consumeNextItemRequest() { return impl_->consumeNextItemRequest(); }
bool D3D11VideoRenderer::consumePreviousItemRequest() { return impl_->consumePreviousItemRequest(); }
bool D3D11VideoRenderer::consumeOpenFileRequest(std::string& path) { return impl_->consumeOpenFileRequest(path); }
void D3D11VideoRenderer::setOverlayState(double position, double duration, float volume, bool paused) { impl_->setOverlayState(position, duration, volume, paused); }
void D3D11VideoRenderer::setSubtitleClock(double subtitle_time_seconds) { impl_->setSubtitleClock(subtitle_time_seconds); }
void D3D11VideoRenderer::setSubtitleText(const std::string& text) { impl_->setSubtitleText(text); }
void D3D11VideoRenderer::setSubtitleItems(const std::vector<subtitle::SubtitleItem>& items) { impl_->setSubtitleItems(items); }
void D3D11VideoRenderer::setHotkeyManager(const input::HotkeyManager& hotkey_manager) { impl_->setHotkeyManager(hotkey_manager); }
RendererDiagnostics D3D11VideoRenderer::getDiagnostics() const { return impl_->getDiagnostics(); }
void D3D11VideoRenderer::resetDiagnostics() { impl_->resetDiagnostics(); }
bool D3D11VideoRenderer::supportsNativeFrameFormat(AVPixelFormat format) const { return impl_->supportsNativeFrameFormat(format); }
void* D3D11VideoRenderer::nativeDeviceHandle() const { return impl_->nativeDeviceHandle(); }
const char* D3D11VideoRenderer::rendererBackendName() const { return "D3D11"; }

}  // namespace vp::render



