#include "render/vulkan_video_renderer.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#if __has_include(<SDL2/SDL_vulkan.h>)
#include <SDL2/SDL_vulkan.h>
#elif __has_include(<SDL_vulkan.h>)
#include <SDL_vulkan.h>
#else
#error "SDL2 Vulkan headers not found"
#endif
#elif __has_include(<SDL.h>)
#include <SDL.h>
#if __has_include(<SDL_vulkan.h>)
#include <SDL_vulkan.h>
#else
#error "SDL2 Vulkan headers not found"
#endif
#else
#error "SDL2 headers not found"
#endif

#include <vulkan/vulkan.h>

extern "C" {
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

#include "logger.h"

namespace vp::render {

namespace {

constexpr Uint32 kRequestedSdlSubsystems = SDL_INIT_VIDEO | SDL_INIT_AUDIO;
constexpr int kDefaultWindowWidth = 1280;
constexpr int kDefaultWindowHeight = 720;
constexpr int kMinWindowWidth = 320;
constexpr int kMinWindowHeight = 180;
constexpr uint32_t kInvalidQueueFamily = std::numeric_limits<uint32_t>::max();
constexpr std::array<const char*, 1> kRequiredDeviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
constexpr uint64_t kFrameFenceWaitTimeoutNs = 250000000ULL;

enum class VulkanPresentModeRequest {
    Auto,
    Fifo,
    Mailbox,
    Immediate,
    FifoRelaxed
};

std::string toLowerAscii(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return text;
}

std::string trimAscii(std::string text) {
    const auto is_space = [](unsigned char ch) {
        return std::isspace(ch) != 0;
    };
    auto begin = text.begin();
    while (begin != text.end() && is_space(static_cast<unsigned char>(*begin))) {
        ++begin;
    }
    auto end = text.end();
    while (end != begin && is_space(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }
    return std::string(begin, end);
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
    std::string copy(value);
    std::free(value);
    return copy;
#else
    const char* value = std::getenv(key);
    if (!value) {
        return std::nullopt;
    }
    return std::string(value);
#endif
}

struct VulkanPresentModeParseResult {
    VulkanPresentModeRequest request{VulkanPresentModeRequest::Auto};
    bool recognized{true};
    std::string normalized{"auto"};
};

VulkanPresentModeParseResult parseVulkanPresentModeRequest(const std::optional<std::string>& value) {
    VulkanPresentModeParseResult result;
    if (!value) {
        return result;
    }

    result.normalized = toLowerAscii(trimAscii(*value));
    if (result.normalized.empty() || result.normalized == "auto" || result.normalized == "paced") {
        result.request = VulkanPresentModeRequest::Auto;
        return result;
    }
    if (result.normalized == "fifo" || result.normalized == "vsync") {
        result.request = VulkanPresentModeRequest::Fifo;
        return result;
    }
    if (result.normalized == "mailbox") {
        result.request = VulkanPresentModeRequest::Mailbox;
        return result;
    }
    if (result.normalized == "immediate" || result.normalized == "tearing") {
        result.request = VulkanPresentModeRequest::Immediate;
        return result;
    }
    if (result.normalized == "fifo_relaxed" || result.normalized == "relaxed") {
        result.request = VulkanPresentModeRequest::FifoRelaxed;
        return result;
    }

    result.recognized = false;
    result.request = VulkanPresentModeRequest::Auto;
    return result;
}

const char* presentModeRequestName(VulkanPresentModeRequest request) {
    switch (request) {
    case VulkanPresentModeRequest::Auto:
        return "auto";
    case VulkanPresentModeRequest::Fifo:
        return "fifo";
    case VulkanPresentModeRequest::Mailbox:
        return "mailbox";
    case VulkanPresentModeRequest::Immediate:
        return "immediate";
    case VulkanPresentModeRequest::FifoRelaxed:
        return "fifo_relaxed";
    default:
        return "unknown";
    }
}

const char* presentModeName(VkPresentModeKHR present_mode) {
    switch (present_mode) {
    case VK_PRESENT_MODE_IMMEDIATE_KHR:
        return "immediate";
    case VK_PRESENT_MODE_MAILBOX_KHR:
        return "mailbox";
    case VK_PRESENT_MODE_FIFO_KHR:
        return "fifo";
    case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
        return "fifo_relaxed";
#ifdef VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR
    case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR:
        return "shared_demand_refresh";
#endif
#ifdef VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR
    case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR:
        return "shared_continuous_refresh";
#endif
    default:
        return "unknown";
    }
}

bool hasPresentMode(const std::vector<VkPresentModeKHR>& present_modes, VkPresentModeKHR mode) {
    return std::find(present_modes.begin(), present_modes.end(), mode) != present_modes.end();
}

const char* vkResultName(VkResult result) {
    switch (result) {
    case VK_SUCCESS:
        return "VK_SUCCESS";
    case VK_NOT_READY:
        return "VK_NOT_READY";
    case VK_TIMEOUT:
        return "VK_TIMEOUT";
    case VK_EVENT_SET:
        return "VK_EVENT_SET";
    case VK_EVENT_RESET:
        return "VK_EVENT_RESET";
    case VK_INCOMPLETE:
        return "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
        return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:
        return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED:
        return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT:
        return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:
        return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_SURFACE_LOST_KHR:
        return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    default:
        return "VK_UNKNOWN_ERROR";
    }
}

std::string sdlErrorText(const char* prefix) {
    const char* sdl_error = SDL_GetError();
    std::string message(prefix ? prefix : "SDL error");
    message += ": ";
    message += (sdl_error && sdl_error[0] != '\0') ? sdl_error : "unknown";
    return message;
}

bool ensureSdlSubsystems(Uint32 requested_flags, Uint32& owned_flags) {
    owned_flags = 0;
    const Uint32 current_flags = SDL_WasInit(requested_flags);
    const Uint32 missing_flags = requested_flags & ~current_flags;
    if (missing_flags == 0) {
        return true;
    }

    const int init_result =
        SDL_WasInit(0) == 0 ? SDL_Init(missing_flags) : SDL_InitSubSystem(missing_flags);
    if (init_result != 0) {
        return false;
    }

    owned_flags = missing_flags;
    return true;
}

void releaseSdlSubsystems(Uint32& owned_flags) {
    if (owned_flags != 0) {
        SDL_QuitSubSystem(owned_flags);
        owned_flags = 0;
    }
}

struct QueueFamilyIndices {
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;

    bool complete() const {
        return graphics.has_value() && present.has_value();
    }
};

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physical_device, VkSurfaceKHR surface) {
    QueueFamilyIndices indices;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
    if (queue_family_count == 0) {
        return indices;
    }

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families.data());

    for (uint32_t i = 0; i < queue_family_count; ++i) {
        const VkQueueFamilyProperties& props = queue_families[i];
        if (props.queueCount > 0 && (props.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0u) {
            indices.graphics = i;
        }

        VkBool32 present_support = VK_FALSE;
        if (vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support) == VK_SUCCESS &&
            present_support == VK_TRUE) {
            indices.present = i;
        }

        if (indices.complete()) {
            break;
        }
    }

    return indices;
}

bool hasRequiredDeviceExtensions(VkPhysicalDevice physical_device) {
    uint32_t extension_count = 0;
    if (vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count, nullptr) != VK_SUCCESS) {
        return false;
    }

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    if (extension_count > 0 &&
        vkEnumerateDeviceExtensionProperties(
            physical_device, nullptr, &extension_count, available_extensions.data()) != VK_SUCCESS) {
        return false;
    }

    for (const char* required : kRequiredDeviceExtensions) {
        bool found = false;
        for (const VkExtensionProperties& extension : available_extensions) {
            if (std::strcmp(required, extension.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }

    return true;
}

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;

    bool complete() const {
        return !formats.empty() && !present_modes.empty();
    }
};

bool querySwapchainSupport(VkPhysicalDevice physical_device,
                           VkSurfaceKHR surface,
                           SwapchainSupportDetails& details) {
    details = {};
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &details.capabilities) != VK_SUCCESS) {
        return false;
    }

    uint32_t format_count = 0;
    if (vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, nullptr) != VK_SUCCESS) {
        return false;
    }
    if (format_count > 0) {
        details.formats.resize(format_count);
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &format_count, details.formats.data()) !=
            VK_SUCCESS) {
            return false;
        }
    }

    uint32_t present_mode_count = 0;
    if (vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, nullptr) !=
        VK_SUCCESS) {
        return false;
    }
    if (present_mode_count > 0) {
        details.present_modes.resize(present_mode_count);
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(
                physical_device, surface, &present_mode_count, details.present_modes.data()) != VK_SUCCESS) {
            return false;
        }
    }

    return true;
}

VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
    for (const VkSurfaceFormatKHR& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }
    return formats.front();
}

VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& present_modes,
                                   VulkanPresentModeRequest request,
                                   bool& exact_match) {
    exact_match = false;

    auto preferredAutoMode = [&present_modes]() -> VkPresentModeKHR {
        if (hasPresentMode(present_modes, VK_PRESENT_MODE_MAILBOX_KHR)) {
            return VK_PRESENT_MODE_MAILBOX_KHR;
        }
        if (hasPresentMode(present_modes, VK_PRESENT_MODE_FIFO_KHR)) {
            return VK_PRESENT_MODE_FIFO_KHR;
        }
        if (hasPresentMode(present_modes, VK_PRESENT_MODE_IMMEDIATE_KHR)) {
            return VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
        return present_modes.empty() ? VK_PRESENT_MODE_FIFO_KHR : present_modes.front();
    };

    VkPresentModeKHR requested_mode = VK_PRESENT_MODE_FIFO_KHR;
    switch (request) {
    case VulkanPresentModeRequest::Auto:
        return preferredAutoMode();
    case VulkanPresentModeRequest::Fifo:
        requested_mode = VK_PRESENT_MODE_FIFO_KHR;
        break;
    case VulkanPresentModeRequest::Mailbox:
        requested_mode = VK_PRESENT_MODE_MAILBOX_KHR;
        break;
    case VulkanPresentModeRequest::Immediate:
        requested_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        break;
    case VulkanPresentModeRequest::FifoRelaxed:
        requested_mode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
        break;
    default:
        return preferredAutoMode();
    }

    if (hasPresentMode(present_modes, requested_mode)) {
        exact_match = true;
        return requested_mode;
    }

    if (request == VulkanPresentModeRequest::FifoRelaxed && hasPresentMode(present_modes, VK_PRESENT_MODE_FIFO_KHR)) {
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    return preferredAutoMode();
}

VkExtent2D chooseSwapExtent(SDL_Window* window, const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int width = 0;
    int height = 0;
    SDL_Vulkan_GetDrawableSize(window, &width, &height);
    const uint32_t clamped_width = std::clamp(
        static_cast<uint32_t>(std::max(1, width)),
        capabilities.minImageExtent.width,
        capabilities.maxImageExtent.width);
    const uint32_t clamped_height = std::clamp(
        static_cast<uint32_t>(std::max(1, height)),
        capabilities.minImageExtent.height,
        capabilities.maxImageExtent.height);

    return {clamped_width, clamped_height};
}

std::string physicalDeviceName(VkPhysicalDevice physical_device) {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physical_device, &properties);
    if (properties.deviceName[0] == '\0') {
        return "unknown";
    }
    return std::string(properties.deviceName);
}

}  // namespace

struct VulkanVideoRenderer::VulkanState {
    Uint32 owned_sdl_subsystems{0};
    SDL_Window* window{nullptr};
    uint32_t window_id{0};
    bool should_quit{false};
    bool initialized{false};

    VkInstance instance{VK_NULL_HANDLE};
    VkSurfaceKHR surface{VK_NULL_HANDLE};
    VkPhysicalDevice physical_device{VK_NULL_HANDLE};
    VkDevice device{VK_NULL_HANDLE};
    VkQueue graphics_queue{VK_NULL_HANDLE};
    VkQueue present_queue{VK_NULL_HANDLE};
    uint32_t graphics_queue_family{kInvalidQueueFamily};
    uint32_t present_queue_family{kInvalidQueueFamily};
    VkSwapchainKHR swapchain{VK_NULL_HANDLE};
    VkFormat swapchain_image_format{VK_FORMAT_UNDEFINED};
    VkExtent2D swapchain_extent{};
    std::vector<VkImage> swapchain_images;
    VulkanPresentModeRequest present_mode_requested{VulkanPresentModeRequest::Auto};
    VkPresentModeKHR present_mode_active{VK_PRESENT_MODE_FIFO_KHR};
    uint64_t frame_present_submitted{0};
    uint64_t frame_presented{0};
    uint64_t fence_wait_time_ns_total{0};
    uint64_t fence_wait_time_ns_max{0};
    uint64_t fence_wait_timeout_count{0};
    uint64_t acquire_out_of_date_count{0};
    uint64_t present_out_of_date_count{0};
    bool swapchain_recreate_requested{false};

    VkCommandPool command_pool{VK_NULL_HANDLE};
    VkCommandBuffer command_buffer{VK_NULL_HANDLE};
    VkSemaphore image_available_semaphore{VK_NULL_HANDLE};
    VkSemaphore render_finished_semaphore{VK_NULL_HANDLE};
    VkFence in_flight_fence{VK_NULL_HANDLE};
    bool frame_submission_in_flight{false};
    std::array<float, 4> clear_color{{0.04f, 0.04f, 0.06f, 1.0f}};

    SwsContext* upload_sws{nullptr};
    int upload_sws_src_width{0};
    int upload_sws_src_height{0};
    AVPixelFormat upload_sws_src_format{AV_PIX_FMT_NONE};
    int upload_sws_dst_width{0};
    int upload_sws_dst_height{0};

    std::mutex frame_upload_mutex;
    std::vector<uint8_t> frame_upload_rgba;
    uint32_t frame_upload_width{0};
    uint32_t frame_upload_height{0};
    uint32_t frame_upload_stride{0};
    bool frame_upload_ready{false};

    VkBuffer frame_upload_staging_buffer{VK_NULL_HANDLE};
    VkDeviceMemory frame_upload_staging_memory{VK_NULL_HANDLE};
    VkDeviceSize frame_upload_staging_size{0};

    input::HotkeyManager hotkey_manager{};
    std::mutex request_mutex;
    bool open_file_requested{false};
    std::string open_file_path;
};

namespace {

bool queryDrawableSize(SDL_Window* window, int& width, int& height) {
    width = 0;
    height = 0;
    if (!window) {
        return false;
    }
    SDL_Vulkan_GetDrawableSize(window, &width, &height);
    return width > 0 && height > 0;
}

VkImageSubresourceRange colorImageRange() {
    VkImageSubresourceRange range{};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;
    return range;
}

uint32_t findMemoryTypeIndex(VkPhysicalDevice physical_device,
                             uint32_t type_filter,
                             VkMemoryPropertyFlags required_properties) {
    VkPhysicalDeviceMemoryProperties memory_properties{};
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);
    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i) {
        const bool type_supported = (type_filter & (1u << i)) != 0u;
        const bool property_match =
            (memory_properties.memoryTypes[i].propertyFlags & required_properties) == required_properties;
        if (type_supported && property_match) {
            return i;
        }
    }
    return kInvalidQueueFamily;
}

template <typename StateT>
void destroySwapchain(StateT& state) {
    if (state.swapchain != VK_NULL_HANDLE && state.device != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(state.device, state.swapchain, nullptr);
        state.swapchain = VK_NULL_HANDLE;
    }
    state.swapchain_images.clear();
    state.swapchain_image_format = VK_FORMAT_UNDEFINED;
    state.swapchain_extent = {};
}

template <typename StateT>
bool createSwapchain(StateT& state, VkSwapchainKHR old_swapchain) {
    if (!state.window || state.device == VK_NULL_HANDLE || state.physical_device == VK_NULL_HANDLE ||
        state.surface == VK_NULL_HANDLE) {
        LOG_ERROR("Vulkan swapchain create failed: renderer state is incomplete");
        return false;
    }

    int drawable_width = 0;
    int drawable_height = 0;
    if (!queryDrawableSize(state.window, drawable_width, drawable_height)) {
        LOG_WARNING("Vulkan swapchain create deferred: drawable size is zero");
        return false;
    }

    SwapchainSupportDetails swapchain_support;
    if (!querySwapchainSupport(state.physical_device, state.surface, swapchain_support) || !swapchain_support.complete()) {
        LOG_ERROR("Vulkan swapchain create failed: swapchain support query incomplete");
        return false;
    }

    const VkSurfaceFormatKHR surface_format = chooseSurfaceFormat(swapchain_support.formats);
    bool present_mode_exact_match = false;
    const VkPresentModeKHR present_mode = choosePresentMode(
        swapchain_support.present_modes, state.present_mode_requested, present_mode_exact_match);
    const VkExtent2D extent = chooseSwapExtent(state.window, swapchain_support.capabilities);

    uint32_t image_count = swapchain_support.capabilities.minImageCount + 1;
    if (swapchain_support.capabilities.maxImageCount > 0 &&
        image_count > swapchain_support.capabilities.maxImageCount) {
        image_count = swapchain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchain_info{};
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface = state.surface;
    swapchain_info.minImageCount = image_count;
    swapchain_info.imageFormat = surface_format.format;
    swapchain_info.imageColorSpace = surface_format.colorSpace;
    swapchain_info.imageExtent = extent;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    const uint32_t queue_family_indices[] = {state.graphics_queue_family, state.present_queue_family};
    if (state.graphics_queue_family != state.present_queue_family) {
        swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_info.queueFamilyIndexCount = 2;
        swapchain_info.pQueueFamilyIndices = queue_family_indices;
    } else {
        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    swapchain_info.preTransform = swapchain_support.capabilities.currentTransform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode = present_mode;
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.oldSwapchain = old_swapchain;

    VkSwapchainKHR new_swapchain = VK_NULL_HANDLE;
    const VkResult create_result = vkCreateSwapchainKHR(state.device, &swapchain_info, nullptr, &new_swapchain);
    if (create_result != VK_SUCCESS) {
        LOG_ERROR("Vulkan swapchain create failed: vkCreateSwapchainKHR returned "
                  << vkResultName(create_result) << " (" << static_cast<int>(create_result) << ")");
        return false;
    }

    uint32_t swapchain_image_count = 0;
    VkResult result = vkGetSwapchainImagesKHR(state.device, new_swapchain, &swapchain_image_count, nullptr);
    if (result != VK_SUCCESS || swapchain_image_count == 0) {
        LOG_ERROR("Vulkan swapchain create failed: vkGetSwapchainImagesKHR(count) returned "
                  << vkResultName(result) << " (" << static_cast<int>(result) << ")");
        vkDestroySwapchainKHR(state.device, new_swapchain, nullptr);
        return false;
    }

    std::vector<VkImage> swapchain_images(swapchain_image_count);
    result = vkGetSwapchainImagesKHR(state.device, new_swapchain, &swapchain_image_count, swapchain_images.data());
    if (result != VK_SUCCESS) {
        LOG_ERROR("Vulkan swapchain create failed: vkGetSwapchainImagesKHR(list) returned "
                  << vkResultName(result) << " (" << static_cast<int>(result) << ")");
        vkDestroySwapchainKHR(state.device, new_swapchain, nullptr);
        return false;
    }

    if (old_swapchain != VK_NULL_HANDLE && state.device != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(state.device, old_swapchain, nullptr);
    }

    state.swapchain = new_swapchain;
    state.swapchain_images = std::move(swapchain_images);
    state.swapchain_image_format = surface_format.format;
    state.swapchain_extent = extent;
    state.present_mode_active = present_mode;

    LOG_INFO("Vulkan swapchain ready"
             << " images=" << state.swapchain_images.size()
             << " extent=" << state.swapchain_extent.width << "x" << state.swapchain_extent.height
             << " drawable=" << drawable_width << "x" << drawable_height
             << " present_mode_requested=" << presentModeRequestName(state.present_mode_requested)
             << " present_mode_active=" << presentModeName(state.present_mode_active)
             << " present_mode_exact_match=" << (present_mode_exact_match ? "true" : "false"));
    return true;
}

template <typename StateT>
bool recreateSwapchain(StateT& state) {
    if (state.device == VK_NULL_HANDLE) {
        return false;
    }

    int drawable_width = 0;
    int drawable_height = 0;
    if (!queryDrawableSize(state.window, drawable_width, drawable_height)) {
        state.swapchain_recreate_requested = true;
        return false;
    }

    const VkResult idle_result = vkDeviceWaitIdle(state.device);
    if (idle_result != VK_SUCCESS) {
        LOG_WARNING("Vulkan swapchain recreate: vkDeviceWaitIdle returned "
                    << vkResultName(idle_result) << " (" << static_cast<int>(idle_result) << ")");
    }
    state.frame_submission_in_flight = false;

    const VkSwapchainKHR old_swapchain = state.swapchain;
    if (!createSwapchain(state, old_swapchain)) {
        state.swapchain_recreate_requested = true;
        return false;
    }

    state.swapchain_recreate_requested = false;
    return true;
}

template <typename StateT>
void destroyFrameUploadStaging(StateT& state) {
    if (state.device != VK_NULL_HANDLE) {
        if (state.frame_upload_staging_buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(state.device, state.frame_upload_staging_buffer, nullptr);
            state.frame_upload_staging_buffer = VK_NULL_HANDLE;
        }
        if (state.frame_upload_staging_memory != VK_NULL_HANDLE) {
            vkFreeMemory(state.device, state.frame_upload_staging_memory, nullptr);
            state.frame_upload_staging_memory = VK_NULL_HANDLE;
        }
    } else {
        state.frame_upload_staging_buffer = VK_NULL_HANDLE;
        state.frame_upload_staging_memory = VK_NULL_HANDLE;
    }
    state.frame_upload_staging_size = 0;
}

template <typename StateT>
bool ensureFrameUploadStaging(StateT& state, VkDeviceSize required_size) {
    if (required_size == 0) {
        return false;
    }
    if (state.frame_upload_staging_buffer != VK_NULL_HANDLE && state.frame_upload_staging_size >= required_size) {
        return true;
    }

    destroyFrameUploadStaging(state);

    VkBufferCreateInfo buffer_info{};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = required_size;
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(state.device, &buffer_info, nullptr, &state.frame_upload_staging_buffer);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Vulkan frame upload staging create failed: vkCreateBuffer returned "
                  << vkResultName(result) << " (" << static_cast<int>(result) << ")");
        destroyFrameUploadStaging(state);
        return false;
    }

    VkMemoryRequirements memory_requirements{};
    vkGetBufferMemoryRequirements(state.device, state.frame_upload_staging_buffer, &memory_requirements);
    const uint32_t memory_type_index = findMemoryTypeIndex(
        state.physical_device,
        memory_requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (memory_type_index == kInvalidQueueFamily) {
        LOG_ERROR("Vulkan frame upload staging create failed: no compatible host-visible/coherent memory type");
        destroyFrameUploadStaging(state);
        return false;
    }

    VkMemoryAllocateInfo allocate_info{};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = memory_requirements.size;
    allocate_info.memoryTypeIndex = memory_type_index;
    result = vkAllocateMemory(state.device, &allocate_info, nullptr, &state.frame_upload_staging_memory);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Vulkan frame upload staging create failed: vkAllocateMemory returned "
                  << vkResultName(result) << " (" << static_cast<int>(result) << ")");
        destroyFrameUploadStaging(state);
        return false;
    }

    result = vkBindBufferMemory(state.device, state.frame_upload_staging_buffer, state.frame_upload_staging_memory, 0);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Vulkan frame upload staging create failed: vkBindBufferMemory returned "
                  << vkResultName(result) << " (" << static_cast<int>(result) << ")");
        destroyFrameUploadStaging(state);
        return false;
    }

    state.frame_upload_staging_size = required_size;
    return true;
}

template <typename StateT>
void destroyFrameUploadResources(StateT& state) {
    if (state.upload_sws) {
        sws_freeContext(state.upload_sws);
        state.upload_sws = nullptr;
    }
    state.upload_sws_src_width = 0;
    state.upload_sws_src_height = 0;
    state.upload_sws_src_format = AV_PIX_FMT_NONE;
    state.upload_sws_dst_width = 0;
    state.upload_sws_dst_height = 0;

    {
        std::lock_guard<std::mutex> lock(state.frame_upload_mutex);
        state.frame_upload_rgba.clear();
        state.frame_upload_width = 0;
        state.frame_upload_height = 0;
        state.frame_upload_stride = 0;
        state.frame_upload_ready = false;
    }

    destroyFrameUploadStaging(state);
}

template <typename StateT>
bool ensureFrameUploadSwsContext(StateT& state,
                                 int src_width,
                                 int src_height,
                                 AVPixelFormat src_format,
                                 int dst_width,
                                 int dst_height) {
    const bool context_matches = state.upload_sws &&
                                 state.upload_sws_src_width == src_width &&
                                 state.upload_sws_src_height == src_height &&
                                 state.upload_sws_src_format == src_format &&
                                 state.upload_sws_dst_width == dst_width &&
                                 state.upload_sws_dst_height == dst_height;
    if (context_matches) {
        return true;
    }

    if (state.upload_sws) {
        sws_freeContext(state.upload_sws);
        state.upload_sws = nullptr;
    }

    state.upload_sws = sws_getContext(src_width,
                                      src_height,
                                      src_format,
                                      dst_width,
                                      dst_height,
                                      AV_PIX_FMT_RGBA,
                                      SWS_BILINEAR,
                                      nullptr,
                                      nullptr,
                                      nullptr);
    if (!state.upload_sws) {
        LOG_ERROR("Vulkan frame upload swscale init failed"
                  << " src=" << src_width << "x" << src_height
                  << " src_format=" << static_cast<int>(src_format)
                  << " dst=" << dst_width << "x" << dst_height);
        return false;
    }

    state.upload_sws_src_width = src_width;
    state.upload_sws_src_height = src_height;
    state.upload_sws_src_format = src_format;
    state.upload_sws_dst_width = dst_width;
    state.upload_sws_dst_height = dst_height;
    return true;
}

template <typename StateT>
void destroyFrameResources(StateT& state) {
    if (state.device == VK_NULL_HANDLE) {
        state.command_pool = VK_NULL_HANDLE;
        state.command_buffer = VK_NULL_HANDLE;
        state.image_available_semaphore = VK_NULL_HANDLE;
        state.render_finished_semaphore = VK_NULL_HANDLE;
        state.in_flight_fence = VK_NULL_HANDLE;
        state.frame_submission_in_flight = false;
        return;
    }

    if (state.image_available_semaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(state.device, state.image_available_semaphore, nullptr);
        state.image_available_semaphore = VK_NULL_HANDLE;
    }
    if (state.render_finished_semaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(state.device, state.render_finished_semaphore, nullptr);
        state.render_finished_semaphore = VK_NULL_HANDLE;
    }
    if (state.in_flight_fence != VK_NULL_HANDLE) {
        vkDestroyFence(state.device, state.in_flight_fence, nullptr);
        state.in_flight_fence = VK_NULL_HANDLE;
    }
    if (state.command_pool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(state.device, state.command_pool, nullptr);
        state.command_pool = VK_NULL_HANDLE;
    }
    state.command_buffer = VK_NULL_HANDLE;
    state.frame_submission_in_flight = false;
}

template <typename StateT>
bool createFrameResources(StateT& state) {
    if (state.device == VK_NULL_HANDLE) {
        return false;
    }

    VkCommandPoolCreateInfo command_pool_info{};
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_info.queueFamilyIndex = state.graphics_queue_family;

    VkResult result = vkCreateCommandPool(state.device, &command_pool_info, nullptr, &state.command_pool);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Vulkan frame resource init failed: vkCreateCommandPool returned "
                  << vkResultName(result) << " (" << static_cast<int>(result) << ")");
        destroyFrameResources(state);
        return false;
    }

    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = state.command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = 1;
    result = vkAllocateCommandBuffers(state.device, &command_buffer_allocate_info, &state.command_buffer);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Vulkan frame resource init failed: vkAllocateCommandBuffers returned "
                  << vkResultName(result) << " (" << static_cast<int>(result) << ")");
        destroyFrameResources(state);
        return false;
    }

    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    result = vkCreateSemaphore(state.device, &semaphore_info, nullptr, &state.image_available_semaphore);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Vulkan frame resource init failed: vkCreateSemaphore(image_available) returned "
                  << vkResultName(result) << " (" << static_cast<int>(result) << ")");
        destroyFrameResources(state);
        return false;
    }

    result = vkCreateSemaphore(state.device, &semaphore_info, nullptr, &state.render_finished_semaphore);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Vulkan frame resource init failed: vkCreateSemaphore(render_finished) returned "
                  << vkResultName(result) << " (" << static_cast<int>(result) << ")");
        destroyFrameResources(state);
        return false;
    }

    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    result = vkCreateFence(state.device, &fence_info, nullptr, &state.in_flight_fence);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Vulkan frame resource init failed: vkCreateFence returned "
                  << vkResultName(result) << " (" << static_cast<int>(result) << ")");
        destroyFrameResources(state);
        return false;
    }

    state.frame_submission_in_flight = false;
    return true;
}

template <typename StateT>
bool waitForInFlightFrame(StateT& state) {
    if (!state.frame_submission_in_flight || state.in_flight_fence == VK_NULL_HANDLE) {
        return true;
    }

    const auto wait_begin = std::chrono::steady_clock::now();
    const VkResult wait_result =
        vkWaitForFences(state.device, 1, &state.in_flight_fence, VK_TRUE, kFrameFenceWaitTimeoutNs);
    const uint64_t wait_elapsed_ns = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - wait_begin).count());
    state.fence_wait_time_ns_total += wait_elapsed_ns;
    state.fence_wait_time_ns_max = std::max(state.fence_wait_time_ns_max, wait_elapsed_ns);

    if (wait_result == VK_TIMEOUT) {
        ++state.fence_wait_timeout_count;
        LOG_WARNING("Vulkan frame wait timeout: timeout_ns=" << kFrameFenceWaitTimeoutNs
                                                             << " waited_ns=" << wait_elapsed_ns
                                                             << " timeout_count=" << state.fence_wait_timeout_count
                                                             << " present_mode_active="
                                                             << presentModeName(state.present_mode_active));
        const VkResult idle_result = vkDeviceWaitIdle(state.device);
        if (idle_result != VK_SUCCESS) {
            LOG_WARNING("Vulkan frame wait timeout recovery: vkDeviceWaitIdle returned "
                        << vkResultName(idle_result) << " (" << static_cast<int>(idle_result) << ")");
        }
        state.frame_submission_in_flight = false;
        state.swapchain_recreate_requested = true;
        return false;
    }

    if (wait_result != VK_SUCCESS) {
        LOG_ERROR("Vulkan frame wait failed: vkWaitForFences returned "
                  << vkResultName(wait_result) << " (" << static_cast<int>(wait_result) << ")");
        state.frame_submission_in_flight = false;
        state.swapchain_recreate_requested = true;
        return false;
    }

    state.frame_submission_in_flight = false;
    return true;
}

template <typename StateT>
bool recordPresentCommandBuffer(StateT& state,
                                uint32_t image_index,
                                bool has_frame_upload,
                                uint32_t upload_width,
                                uint32_t upload_height,
                                uint32_t upload_stride) {
    if (state.command_buffer == VK_NULL_HANDLE || image_index >= state.swapchain_images.size()) {
        return false;
    }

    VkResult result = vkResetCommandBuffer(state.command_buffer, 0);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Vulkan command reset failed: vkResetCommandBuffer returned "
                  << vkResultName(result) << " (" << static_cast<int>(result) << ")");
        return false;
    }

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    result = vkBeginCommandBuffer(state.command_buffer, &begin_info);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Vulkan command begin failed: vkBeginCommandBuffer returned "
                  << vkResultName(result) << " (" << static_cast<int>(result) << ")");
        return false;
    }

    VkImageMemoryBarrier to_transfer_barrier{};
    to_transfer_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    to_transfer_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    to_transfer_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    to_transfer_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    to_transfer_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    to_transfer_barrier.image = state.swapchain_images[image_index];
    to_transfer_barrier.subresourceRange = colorImageRange();
    to_transfer_barrier.srcAccessMask = 0;
    to_transfer_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(state.command_buffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &to_transfer_barrier);

    VkClearColorValue clear_color{};
    clear_color.float32[0] = state.clear_color[0];
    clear_color.float32[1] = state.clear_color[1];
    clear_color.float32[2] = state.clear_color[2];
    clear_color.float32[3] = state.clear_color[3];
    const VkImageSubresourceRange clear_range = colorImageRange();
    vkCmdClearColorImage(state.command_buffer,
                         state.swapchain_images[image_index],
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         &clear_color,
                         1,
                         &clear_range);

    if (has_frame_upload && state.frame_upload_staging_buffer != VK_NULL_HANDLE && upload_width > 0 &&
        upload_height > 0 && upload_stride >= upload_width * 4) {
        VkBufferImageCopy copy_region{};
        copy_region.bufferOffset = 0;
        copy_region.bufferRowLength = upload_stride / 4;
        copy_region.bufferImageHeight = upload_height;
        copy_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy_region.imageSubresource.mipLevel = 0;
        copy_region.imageSubresource.baseArrayLayer = 0;
        copy_region.imageSubresource.layerCount = 1;
        copy_region.imageOffset = {0, 0, 0};
        copy_region.imageExtent = {
            std::min(upload_width, state.swapchain_extent.width),
            std::min(upload_height, state.swapchain_extent.height),
            1};
        vkCmdCopyBufferToImage(state.command_buffer,
                               state.frame_upload_staging_buffer,
                               state.swapchain_images[image_index],
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &copy_region);
    }

    VkImageMemoryBarrier to_present_barrier{};
    to_present_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    to_present_barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    to_present_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    to_present_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    to_present_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    to_present_barrier.image = state.swapchain_images[image_index];
    to_present_barrier.subresourceRange = colorImageRange();
    to_present_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    to_present_barrier.dstAccessMask = 0;
    vkCmdPipelineBarrier(state.command_buffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &to_present_barrier);

    result = vkEndCommandBuffer(state.command_buffer);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Vulkan command end failed: vkEndCommandBuffer returned "
                  << vkResultName(result) << " (" << static_cast<int>(result) << ")");
        return false;
    }

    return true;
}

}  // namespace

VulkanVideoRenderer::VulkanVideoRenderer() : state_(std::make_unique<VulkanState>()) {}

VulkanVideoRenderer::~VulkanVideoRenderer() {
    close();
}

bool VulkanVideoRenderer::init(const VideoRendererConfig& config) {
    close();

    Uint32 owned_subsystems = 0;
    if (!ensureSdlSubsystems(kRequestedSdlSubsystems, owned_subsystems)) {
        LOG_ERROR("Vulkan renderer init failed: " << sdlErrorText("SDL init failed"));
        return false;
    }
    state_->owned_sdl_subsystems = owned_subsystems;

    const int window_width = std::max(kMinWindowWidth, config.width > 0 ? config.width : kDefaultWindowWidth);
    const int window_height = std::max(kMinWindowHeight, config.height > 0 ? config.height : kDefaultWindowHeight);
    const std::string window_title = config.title.empty() ? "Video Player" : config.title;

    Uint32 window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN;
#if defined(SDL_WINDOW_ALLOW_HIGHDPI)
    window_flags |= SDL_WINDOW_ALLOW_HIGHDPI;
#endif
    state_->window = SDL_CreateWindow(window_title.c_str(),
                                      SDL_WINDOWPOS_CENTERED,
                                      SDL_WINDOWPOS_CENTERED,
                                      window_width,
                                      window_height,
                                      window_flags);
    if (!state_->window) {
        LOG_ERROR("Vulkan renderer init failed: " << sdlErrorText("SDL_CreateWindow failed"));
        close();
        return false;
    }
    state_->window_id = SDL_GetWindowID(state_->window);

    const VulkanPresentModeParseResult present_mode_parse =
        parseVulkanPresentModeRequest(readEnvVar("MVP_VULKAN_PRESENT_MODE"));
    state_->present_mode_requested = present_mode_parse.request;
    if (!present_mode_parse.recognized) {
        LOG_WARNING("Vulkan present mode override invalid, falling back to auto:"
                    << " MVP_VULKAN_PRESENT_MODE=\"" << present_mode_parse.normalized << "\"");
    }
    LOG_INFO("Vulkan present mode request: " << presentModeRequestName(state_->present_mode_requested));

    std::vector<const char*> instance_extensions;
    uint32_t extension_count = 0;
    if (SDL_Vulkan_GetInstanceExtensions(state_->window, &extension_count, nullptr) != SDL_TRUE ||
        extension_count == 0) {
        LOG_ERROR("Vulkan renderer init failed: " << sdlErrorText("SDL_Vulkan_GetInstanceExtensions(count) failed"));
        close();
        return false;
    }
    instance_extensions.resize(extension_count);
    if (SDL_Vulkan_GetInstanceExtensions(state_->window, &extension_count, instance_extensions.data()) != SDL_TRUE) {
        LOG_ERROR("Vulkan renderer init failed: "
                  << sdlErrorText("SDL_Vulkan_GetInstanceExtensions(names) failed"));
        close();
        return false;
    }

    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = window_title.c_str();
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "modern-video-player";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instance_info{};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size());
    instance_info.ppEnabledExtensionNames = instance_extensions.data();

    VkResult result = vkCreateInstance(&instance_info, nullptr, &state_->instance);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Vulkan renderer init failed: vkCreateInstance failed with "
                  << vkResultName(result) << " (" << static_cast<int>(result) << ")");
        close();
        return false;
    }

    if (SDL_Vulkan_CreateSurface(state_->window, state_->instance, &state_->surface) != SDL_TRUE) {
        LOG_ERROR("Vulkan renderer init failed: " << sdlErrorText("SDL_Vulkan_CreateSurface failed"));
        close();
        return false;
    }

    uint32_t physical_device_count = 0;
    result = vkEnumeratePhysicalDevices(state_->instance, &physical_device_count, nullptr);
    if (result != VK_SUCCESS || physical_device_count == 0) {
        LOG_ERROR("Vulkan renderer init failed: vkEnumeratePhysicalDevices(count) failed with "
                  << vkResultName(result) << " (" << static_cast<int>(result) << ")");
        close();
        return false;
    }

    std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
    result = vkEnumeratePhysicalDevices(state_->instance, &physical_device_count, physical_devices.data());
    if (result != VK_SUCCESS) {
        LOG_ERROR("Vulkan renderer init failed: vkEnumeratePhysicalDevices(list) failed with "
                  << vkResultName(result) << " (" << static_cast<int>(result) << ")");
        close();
        return false;
    }

    QueueFamilyIndices selected_queue_families;
    for (VkPhysicalDevice candidate : physical_devices) {
        const QueueFamilyIndices queue_families = findQueueFamilies(candidate, state_->surface);
        if (!queue_families.complete()) {
            continue;
        }
        if (!hasRequiredDeviceExtensions(candidate)) {
            continue;
        }

        SwapchainSupportDetails swapchain_support;
        if (!querySwapchainSupport(candidate, state_->surface, swapchain_support) || !swapchain_support.complete()) {
            continue;
        }

        state_->physical_device = candidate;
        selected_queue_families = queue_families;
        break;
    }

    if (state_->physical_device == VK_NULL_HANDLE || !selected_queue_families.complete()) {
        LOG_ERROR("Vulkan renderer init failed: no suitable physical device with graphics/present/swapchain support");
        close();
        return false;
    }
    state_->graphics_queue_family = *selected_queue_families.graphics;
    state_->present_queue_family = *selected_queue_families.present;

    const std::set<uint32_t> unique_queue_families = {
        state_->graphics_queue_family,
        state_->present_queue_family};
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    queue_create_infos.reserve(unique_queue_families.size());

    const float queue_priority = 1.0f;
    for (uint32_t queue_family : unique_queue_families) {
        VkDeviceQueueCreateInfo queue_info{};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.queueFamilyIndex = queue_family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_info);
    }

    VkPhysicalDeviceFeatures device_features{};
    VkDeviceCreateInfo device_info{};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    device_info.pQueueCreateInfos = queue_create_infos.data();
    device_info.pEnabledFeatures = &device_features;
    device_info.enabledExtensionCount = static_cast<uint32_t>(kRequiredDeviceExtensions.size());
    device_info.ppEnabledExtensionNames = kRequiredDeviceExtensions.data();

    result = vkCreateDevice(state_->physical_device, &device_info, nullptr, &state_->device);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Vulkan renderer init failed: vkCreateDevice failed with "
                  << vkResultName(result) << " (" << static_cast<int>(result) << ")");
        close();
        return false;
    }

    vkGetDeviceQueue(state_->device, state_->graphics_queue_family, 0, &state_->graphics_queue);
    vkGetDeviceQueue(state_->device, state_->present_queue_family, 0, &state_->present_queue);

    if (!createFrameResources(*state_)) {
        LOG_ERROR("Vulkan renderer init failed: frame resources initialization failed");
        close();
        return false;
    }

    if (!createSwapchain(*state_, VK_NULL_HANDLE)) {
        LOG_ERROR("Vulkan renderer init failed: swapchain initialization failed");
        close();
        return false;
    }
    state_->swapchain_recreate_requested = false;
    state_->should_quit = false;
    state_->initialized = true;

    LOG_INFO("Vulkan renderer initialized"
             << " device=\"" << physicalDeviceName(state_->physical_device) << "\""
             << " graphics_queue_family=" << state_->graphics_queue_family
             << " present_queue_family=" << state_->present_queue_family
             << " swapchain_images=" << state_->swapchain_images.size()
             << " extent=" << state_->swapchain_extent.width << "x" << state_->swapchain_extent.height
             << " present_mode_requested=" << presentModeRequestName(state_->present_mode_requested)
             << " present_mode_active=" << presentModeName(state_->present_mode_active));
    return true;
}

void VulkanVideoRenderer::close() {
    if (!state_) {
        return;
    }

    if (state_->device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(state_->device);
    }

    destroySwapchain(*state_);
    destroyFrameResources(*state_);
    destroyFrameUploadResources(*state_);

    if (state_->device != VK_NULL_HANDLE) {
        vkDestroyDevice(state_->device, nullptr);
        state_->device = VK_NULL_HANDLE;
    }
    state_->graphics_queue = VK_NULL_HANDLE;
    state_->present_queue = VK_NULL_HANDLE;
    state_->graphics_queue_family = kInvalidQueueFamily;
    state_->present_queue_family = kInvalidQueueFamily;
    state_->physical_device = VK_NULL_HANDLE;

    if (state_->surface != VK_NULL_HANDLE && state_->instance != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(state_->instance, state_->surface, nullptr);
        state_->surface = VK_NULL_HANDLE;
    }

    if (state_->instance != VK_NULL_HANDLE) {
        vkDestroyInstance(state_->instance, nullptr);
        state_->instance = VK_NULL_HANDLE;
    }

    if (state_->window) {
        SDL_DestroyWindow(state_->window);
        state_->window = nullptr;
        state_->window_id = 0;
    }

    releaseSdlSubsystems(state_->owned_sdl_subsystems);

    {
        std::lock_guard<std::mutex> lock(state_->request_mutex);
        state_->open_file_requested = false;
        state_->open_file_path.clear();
    }
    state_->should_quit = false;
    state_->initialized = false;
    state_->swapchain_recreate_requested = false;
    state_->frame_submission_in_flight = false;
    state_->present_mode_active = VK_PRESENT_MODE_FIFO_KHR;
    state_->frame_present_submitted = 0;
    state_->frame_presented = 0;
    state_->fence_wait_time_ns_total = 0;
    state_->fence_wait_time_ns_max = 0;
    state_->fence_wait_timeout_count = 0;
    state_->acquire_out_of_date_count = 0;
    state_->present_out_of_date_count = 0;
}

void VulkanVideoRenderer::renderFrame(const core::VideoFrame& frame) {
    if (!state_ || !state_->initialized || !frame.valid || !frame.frame) {
        return;
    }

    const AVFrame* av_frame = frame.frame;
    if (!av_frame || av_frame->width <= 0 || av_frame->height <= 0) {
        return;
    }

    const AVPixelFormat src_format = static_cast<AVPixelFormat>(av_frame->format);
    if (src_format != AV_PIX_FMT_YUV420P && src_format != AV_PIX_FMT_NV12) {
        return;
    }

    const int dst_width = state_->swapchain_extent.width > 0
                              ? static_cast<int>(state_->swapchain_extent.width)
                              : std::max(1, av_frame->width);
    const int dst_height = state_->swapchain_extent.height > 0
                               ? static_cast<int>(state_->swapchain_extent.height)
                               : std::max(1, av_frame->height);
    if (dst_width <= 0 || dst_height <= 0) {
        return;
    }

    if (!ensureFrameUploadSwsContext(*state_,
                                     av_frame->width,
                                     av_frame->height,
                                     src_format,
                                     dst_width,
                                     dst_height)) {
        return;
    }

    std::vector<uint8_t> rgba(static_cast<size_t>(dst_width) * static_cast<size_t>(dst_height) * 4u);
    uint8_t* dst_planes[4] = {rgba.data(), nullptr, nullptr, nullptr};
    int dst_linesize[4] = {dst_width * 4, 0, 0, 0};

    const int scaled_height = sws_scale(state_->upload_sws,
                                        av_frame->data,
                                        av_frame->linesize,
                                        0,
                                        av_frame->height,
                                        dst_planes,
                                        dst_linesize);
    if (scaled_height <= 0) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(state_->frame_upload_mutex);
        state_->frame_upload_rgba = std::move(rgba);
        state_->frame_upload_width = static_cast<uint32_t>(dst_width);
        state_->frame_upload_height = static_cast<uint32_t>(dst_height);
        state_->frame_upload_stride = static_cast<uint32_t>(dst_linesize[0]);
        state_->frame_upload_ready = true;
    }
}

void VulkanVideoRenderer::present() {
    if (!state_ || !state_->initialized || state_->device == VK_NULL_HANDLE || state_->swapchain == VK_NULL_HANDLE ||
        state_->command_buffer == VK_NULL_HANDLE || state_->graphics_queue == VK_NULL_HANDLE ||
        state_->present_queue == VK_NULL_HANDLE) {
        return;
    }

    if (state_->swapchain_recreate_requested && !recreateSwapchain(*state_)) {
        return;
    }

    int drawable_width = 0;
    int drawable_height = 0;
    if (!queryDrawableSize(state_->window, drawable_width, drawable_height)) {
        state_->swapchain_recreate_requested = true;
        return;
    }

    if (!waitForInFlightFrame(*state_)) {
        return;
    }

    uint32_t image_index = 0;
    VkResult result = vkAcquireNextImageKHR(
        state_->device, state_->swapchain, UINT64_MAX, state_->image_available_semaphore, VK_NULL_HANDLE, &image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        ++state_->acquire_out_of_date_count;
        LOG_WARNING("Vulkan acquire reported out-of-date: count=" << state_->acquire_out_of_date_count);
        state_->swapchain_recreate_requested = true;
        return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        LOG_ERROR("Vulkan present failed: vkAcquireNextImageKHR returned "
                  << vkResultName(result) << " (" << static_cast<int>(result) << ")");
        state_->swapchain_recreate_requested = true;
        return;
    }
    if (result == VK_SUBOPTIMAL_KHR) {
        LOG_INFO("Vulkan acquire returned suboptimal; scheduling swapchain recreate");
        state_->swapchain_recreate_requested = true;
    }

    bool has_frame_upload = false;
    uint32_t upload_width = 0;
    uint32_t upload_height = 0;
    uint32_t upload_stride = 0;
    VkDeviceSize upload_data_size = 0;
    std::vector<uint8_t> upload_rgba;
    {
        std::lock_guard<std::mutex> lock(state_->frame_upload_mutex);
        if (state_->frame_upload_ready && state_->frame_upload_width > 0 && state_->frame_upload_height > 0 &&
            state_->frame_upload_stride > 0) {
            const size_t min_stride = static_cast<size_t>(state_->frame_upload_width) * 4u;
            const size_t required_size =
                static_cast<size_t>(state_->frame_upload_stride) * static_cast<size_t>(state_->frame_upload_height);
            if (state_->frame_upload_stride >= min_stride && state_->frame_upload_rgba.size() >= required_size) {
                upload_width = state_->frame_upload_width;
                upload_height = state_->frame_upload_height;
                upload_stride = state_->frame_upload_stride;
                upload_rgba = state_->frame_upload_rgba;
                upload_data_size = static_cast<VkDeviceSize>(required_size);
                has_frame_upload = true;
            }
        }
    }

    if (has_frame_upload) {
        if (!ensureFrameUploadStaging(*state_, upload_data_size)) {
            has_frame_upload = false;
        } else {
            void* mapped_data = nullptr;
            result = vkMapMemory(state_->device, state_->frame_upload_staging_memory, 0, upload_data_size, 0, &mapped_data);
            if (result != VK_SUCCESS || !mapped_data) {
                LOG_ERROR("Vulkan frame upload failed: vkMapMemory returned "
                          << vkResultName(result) << " (" << static_cast<int>(result) << ")");
                has_frame_upload = false;
            } else {
                std::memcpy(mapped_data, upload_rgba.data(), static_cast<size_t>(upload_data_size));
                vkUnmapMemory(state_->device, state_->frame_upload_staging_memory);
            }
        }
    }

    if (!recordPresentCommandBuffer(*state_, image_index, has_frame_upload, upload_width, upload_height, upload_stride)) {
        return;
    }

    result = vkResetFences(state_->device, 1, &state_->in_flight_fence);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Vulkan present failed: vkResetFences returned "
                  << vkResultName(result) << " (" << static_cast<int>(result) << ")");
        return;
    }

    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &state_->image_available_semaphore;
    submit_info.pWaitDstStageMask = &wait_stage;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &state_->command_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &state_->render_finished_semaphore;

    result = vkQueueSubmit(state_->graphics_queue, 1, &submit_info, state_->in_flight_fence);
    if (result != VK_SUCCESS) {
        LOG_ERROR("Vulkan present failed: vkQueueSubmit returned "
                  << vkResultName(result) << " (" << static_cast<int>(result) << ")");
        return;
    }
    ++state_->frame_present_submitted;
    state_->frame_submission_in_flight = true;

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &state_->render_finished_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &state_->swapchain;
    present_info.pImageIndices = &image_index;

    result = vkQueuePresentKHR(state_->present_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        ++state_->present_out_of_date_count;
        LOG_WARNING("Vulkan present requires swapchain recreate: result="
                    << vkResultName(result) << " count=" << state_->present_out_of_date_count);
        state_->swapchain_recreate_requested = true;
        return;
    }
    if (result != VK_SUCCESS) {
        LOG_ERROR("Vulkan present failed: vkQueuePresentKHR returned "
                  << vkResultName(result) << " (" << static_cast<int>(result) << ")");
        state_->swapchain_recreate_requested = true;
        return;
    }

    ++state_->frame_presented;
    if (state_->frame_presented % 120 == 0) {
        const double wait_avg_ms = state_->frame_presented > 0
                                       ? static_cast<double>(state_->fence_wait_time_ns_total) /
                                             static_cast<double>(state_->frame_presented) / 1000000.0
                                       : 0.0;
        const double wait_max_ms = static_cast<double>(state_->fence_wait_time_ns_max) / 1000000.0;
        LOG_INFO("Vulkan pacing stats:"
                 << " present_mode=" << presentModeName(state_->present_mode_active)
                 << " submitted=" << state_->frame_present_submitted
                 << " presented=" << state_->frame_presented
                 << " wait_avg_ms=" << wait_avg_ms
                 << " wait_max_ms=" << wait_max_ms
                 << " wait_timeouts=" << state_->fence_wait_timeout_count);
    }
}

void VulkanVideoRenderer::clear() {
    present();
}

void VulkanVideoRenderer::handleEvents() {
    if (!state_ || !state_->window) {
        return;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        switch (event.type) {
        case SDL_QUIT:
            state_->should_quit = true;
            break;
        case SDL_WINDOWEVENT:
            if (event.window.windowID == state_->window_id) {
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    state_->should_quit = true;
                } else if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                           event.window.event == SDL_WINDOWEVENT_RESIZED ||
                           event.window.event == SDL_WINDOWEVENT_RESTORED ||
                           event.window.event == SDL_WINDOWEVENT_MAXIMIZED ||
                           event.window.event == SDL_WINDOWEVENT_MINIMIZED) {
                    state_->swapchain_recreate_requested = true;
                }
            }
            break;
        case SDL_DROPFILE:
            if (event.drop.file) {
                std::lock_guard<std::mutex> lock(state_->request_mutex);
                state_->open_file_requested = true;
                state_->open_file_path = event.drop.file;
                SDL_free(event.drop.file);
            }
            break;
        case SDL_KEYDOWN:
            if (event.key.repeat == 0) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    state_->should_quit = true;
                } else {
                    const auto action = state_->hotkey_manager.actionForKey(event.key.keysym.sym);
                    if (action && *action == input::PlayerAction::Quit) {
                        state_->should_quit = true;
                    }
                }
            }
            break;
        default:
            break;
        }
    }
}

bool VulkanVideoRenderer::shouldQuit() const {
    return state_ ? state_->should_quit : false;
}

bool VulkanVideoRenderer::consumeTogglePauseRequest() {
    return false;
}

bool VulkanVideoRenderer::consumeSeekRequest(double& normalized_position) {
    (void)normalized_position;
    return false;
}

bool VulkanVideoRenderer::consumeSeekDeltaRequest(double& delta_seconds) {
    (void)delta_seconds;
    return false;
}

bool VulkanVideoRenderer::consumeVolumeChangeRequest(float& volume) {
    (void)volume;
    return false;
}

bool VulkanVideoRenderer::consumeSpeedChangeRequest(double& speed_delta) {
    (void)speed_delta;
    return false;
}

bool VulkanVideoRenderer::consumeResetSpeedRequest() {
    return false;
}

bool VulkanVideoRenderer::consumeToggleSubtitleRequest() {
    return false;
}

bool VulkanVideoRenderer::consumeSetABRepeatStartRequest() {
    return false;
}

bool VulkanVideoRenderer::consumeSetABRepeatEndRequest() {
    return false;
}

bool VulkanVideoRenderer::consumeClearABRepeatRequest() {
    return false;
}

bool VulkanVideoRenderer::consumeScreenshotRequest() {
    return false;
}

bool VulkanVideoRenderer::consumeStepFrameBackwardRequest() {
    return false;
}

bool VulkanVideoRenderer::consumeStepFrameForwardRequest() {
    return false;
}

bool VulkanVideoRenderer::consumePreviousSubtitleTrackRequest() {
    return false;
}

bool VulkanVideoRenderer::consumeNextSubtitleTrackRequest() {
    return false;
}

bool VulkanVideoRenderer::consumeSubtitleDelayChangeRequest(double& delta_seconds) {
    (void)delta_seconds;
    return false;
}

bool VulkanVideoRenderer::consumeAudioDelayChangeRequest(double& delta_seconds) {
    (void)delta_seconds;
    return false;
}

bool VulkanVideoRenderer::consumeNextChapterRequest() {
    return false;
}

bool VulkanVideoRenderer::consumePreviousChapterRequest() {
    return false;
}

bool VulkanVideoRenderer::consumeNextItemRequest() {
    return false;
}

bool VulkanVideoRenderer::consumePreviousItemRequest() {
    return false;
}

bool VulkanVideoRenderer::consumeOpenFileRequest(std::string& path) {
    if (!state_) {
        return false;
    }

    std::lock_guard<std::mutex> lock(state_->request_mutex);
    if (!state_->open_file_requested) {
        return false;
    }

    state_->open_file_requested = false;
    path = std::move(state_->open_file_path);
    state_->open_file_path.clear();
    return !path.empty();
}

void VulkanVideoRenderer::setOverlayState(double position, double duration, float volume, bool paused) {
    (void)position;
    (void)duration;
    (void)volume;
    (void)paused;
}

void VulkanVideoRenderer::setSubtitleText(const std::string& text) {
    (void)text;
}

void VulkanVideoRenderer::setHotkeyManager(const input::HotkeyManager& hotkey_manager) {
    if (state_) {
        state_->hotkey_manager = hotkey_manager;
    }
}

bool VulkanVideoRenderer::supportsDirectFrameFormat(AVPixelFormat format) const {
    return format == AV_PIX_FMT_YUV420P || format == AV_PIX_FMT_NV12;
}

const char* VulkanVideoRenderer::rendererBackendName() const {
    return "Vulkan";
}

}  // namespace vp::render
