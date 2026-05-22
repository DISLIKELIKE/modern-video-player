#include "platform/hw_device_factory.h"

#include <utility>

extern "C" {
#include <libavutil/error.h>
#include <libavutil/hwcontext.h>
}

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <d3d11.h>

extern "C" {
#include <libavutil/hwcontext_d3d11va.h>
}

#endif

namespace vp::platform {

namespace {

std::string avErrorToString(int error_code) {
    char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
    if (av_strerror(error_code, buffer, sizeof(buffer)) == 0) {
        return std::string(buffer);
    }
    return "unknown";
}

AVHWDeviceType backendToDeviceType(decoder::DecoderBackend backend) {
    switch (backend) {
    case decoder::DecoderBackend::D3D11VA:
        return AV_HWDEVICE_TYPE_D3D11VA;
    case decoder::DecoderBackend::VAAPI:
        return AV_HWDEVICE_TYPE_VAAPI;
    default:
        return AV_HWDEVICE_TYPE_NONE;
    }
}

const char* backendName(decoder::DecoderBackend backend) {
    switch (backend) {
    case decoder::DecoderBackend::D3D11VA:
        return "D3D11VA";
    case decoder::DecoderBackend::VAAPI:
        return "VAAPI";
    default:
        return "Unknown";
    }
}

}  // namespace

bool HwDeviceFactory::supportsBackend(decoder::DecoderBackend backend) {
    return backendToDeviceType(backend) != AV_HWDEVICE_TYPE_NONE;
}

bool HwDeviceFactory::findCodecHardwarePixelFormat(const AVCodec* codec,
                                                   decoder::DecoderBackend backend,
                                                   AVPixelFormat* out_pixel_format,
                                                   std::string* detail) {
    if (!out_pixel_format) {
        return false;
    }
    *out_pixel_format = AV_PIX_FMT_NONE;
    if (detail) {
        detail->clear();
    }

    if (!codec) {
        if (detail) {
            *detail = "codec-is-null";
        }
        return false;
    }

    const AVHWDeviceType device_type = backendToDeviceType(backend);
    if (device_type == AV_HWDEVICE_TYPE_NONE) {
        if (detail) {
            *detail = std::string("unsupported-backend:") + backendName(backend);
        }
        return false;
    }

    for (int index = 0;; ++index) {
        const AVCodecHWConfig* hw_config = avcodec_get_hw_config(codec, index);
        if (!hw_config) {
            break;
        }
        if ((hw_config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) &&
            hw_config->device_type == device_type) {
            *out_pixel_format = hw_config->pix_fmt;
            if (detail) {
                *detail = "ok";
            }
            return true;
        }
    }

    if (detail) {
        *detail = "codec-has-no-matching-hw-config";
    }
    return false;
}

bool HwDeviceFactory::createHardwareDeviceContext(const HardwareDeviceCreateRequest& request,
                                                  HardwareDeviceCreateResult* out_result) {
    if (!out_result) {
        return false;
    }
    out_result->device_context = nullptr;
    out_result->detail.clear();

    const AVHWDeviceType device_type = backendToDeviceType(request.backend);
    if (device_type == AV_HWDEVICE_TYPE_NONE) {
        out_result->detail = std::string("unsupported-backend:") + backendName(request.backend);
        return false;
    }

#if defined(_WIN32)
    if (request.backend == decoder::DecoderBackend::D3D11VA && request.native_device_handle) {
        AVBufferRef* device_ref = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_D3D11VA);
        if (!device_ref) {
            out_result->detail = "av_hwdevice_ctx_alloc-failed";
            return false;
        }

        auto* device_ctx = reinterpret_cast<AVHWDeviceContext*>(device_ref->data);
        auto* d3d11_ctx = device_ctx ? reinterpret_cast<AVD3D11VADeviceContext*>(device_ctx->hwctx) : nullptr;
        if (!device_ctx || !d3d11_ctx) {
            av_buffer_unref(&device_ref);
            out_result->detail = "empty-d3d11-device-context";
            return false;
        }

        auto* d3d11_device = static_cast<ID3D11Device*>(request.native_device_handle);
        d3d11_device->AddRef();
        d3d11_ctx->device = d3d11_device;

        const int init_ret = av_hwdevice_ctx_init(device_ref);
        if (init_ret < 0) {
            av_buffer_unref(&device_ref);
            out_result->detail = std::string("av_hwdevice_ctx_init-failed:") + avErrorToString(init_ret);
            return false;
        }

        out_result->device_context = device_ref;
        out_result->detail = "shared-renderer-device";
        return true;
    }
#endif

    AVBufferRef* device_ref = nullptr;
    const int create_ret = av_hwdevice_ctx_create(
        &device_ref,
        device_type,
        (request.device_name && request.device_name[0] != '\0') ? request.device_name : nullptr,
        nullptr,
        0);
    if (create_ret < 0 || !device_ref) {
        out_result->detail = std::string("av_hwdevice_ctx_create-failed:") + avErrorToString(create_ret);
        return false;
    }

    out_result->device_context = device_ref;
    out_result->detail = "ffmpeg-managed-device";
    return true;
}

bool HwDeviceFactory::probeRuntimeAvailability(decoder::DecoderBackend backend, std::string* detail) {
    if (detail) {
        detail->clear();
    }

    if (!supportsBackend(backend)) {
        if (detail) {
            *detail = "unsupported-backend";
        }
        return false;
    }

    HardwareDeviceCreateResult result{};
    const bool create_ok = createHardwareDeviceContext({backend, nullptr, nullptr}, &result);
    if (create_ok && result.device_context) {
        av_buffer_unref(&result.device_context);
        if (detail) {
            *detail = "ok";
        }
        return true;
    }

    if (detail) {
        *detail = result.detail.empty() ? "probe-failed" : result.detail;
    }
    return false;
}

}  // namespace vp::platform
