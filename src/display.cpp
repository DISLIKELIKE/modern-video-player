#include "display.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <iostream>
#include <sstream>
#include <vector>

extern "C" {
#include <libavutil/frame.h>
}

namespace vp {

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

struct WindowSize {
    int width;
    int height;
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

using Glyph5x7 = std::array<uint8_t, 7>;

const Glyph5x7& glyphFor(char ch) {
    static const Glyph5x7 kSpace{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    static const Glyph5x7 kUnknown{0x1F, 0x11, 0x15, 0x15, 0x15, 0x11, 0x1F};
    static const Glyph5x7 kDot{0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C};
    static const Glyph5x7 kComma{0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x08};
    static const Glyph5x7 kColon{0x00, 0x04, 0x00, 0x00, 0x04, 0x00, 0x00};
    static const Glyph5x7 kSemicolon{0x00, 0x04, 0x00, 0x00, 0x04, 0x04, 0x08};
    static const Glyph5x7 kExclamation{0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04};
    static const Glyph5x7 kQuestion{0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04};
    static const Glyph5x7 kMinus{0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00};
    static const Glyph5x7 kUnderscore{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F};
    static const Glyph5x7 kSlash{0x01, 0x02, 0x04, 0x08, 0x10, 0x00, 0x00};
    static const Glyph5x7 kPercent{0x19, 0x19, 0x02, 0x04, 0x08, 0x13, 0x13};
    static const Glyph5x7 kPlus{0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00};
    static const Glyph5x7 kEqual{0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00, 0x00};
    static const Glyph5x7 kApostrophe{0x04, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00};
    static const Glyph5x7 kQuote{0x0A, 0x0A, 0x05, 0x00, 0x00, 0x00, 0x00};
    static const Glyph5x7 kLeftParen{0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02};
    static const Glyph5x7 kRightParen{0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08};

    switch (std::toupper(static_cast<unsigned char>(ch))) {
    case ' ':
        return kSpace;
    case '.':
        return kDot;
    case ',':
        return kComma;
    case ':':
        return kColon;
    case ';':
        return kSemicolon;
    case '!':
        return kExclamation;
    case '?':
        return kQuestion;
    case '-':
        return kMinus;
    case '_':
        return kUnderscore;
    case '/':
        return kSlash;
    case '%':
        return kPercent;
    case '+':
        return kPlus;
    case '=':
        return kEqual;
    case '\'':
        return kApostrophe;
    case '"':
        return kQuote;
    case '(':
        return kLeftParen;
    case ')':
        return kRightParen;
    case '0': {
        static const Glyph5x7 v{0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E};
        return v;
    }
    case '1': {
        static const Glyph5x7 v{0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E};
        return v;
    }
    case '2': {
        static const Glyph5x7 v{0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F};
        return v;
    }
    case '3': {
        static const Glyph5x7 v{0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E};
        return v;
    }
    case '4': {
        static const Glyph5x7 v{0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
        return v;
    }
    case '5': {
        static const Glyph5x7 v{0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E};
        return v;
    }
    case '6': {
        static const Glyph5x7 v{0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E};
        return v;
    }
    case '7': {
        static const Glyph5x7 v{0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
        return v;
    }
    case '8': {
        static const Glyph5x7 v{0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};
        return v;
    }
    case '9': {
        static const Glyph5x7 v{0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E};
        return v;
    }
    case 'A': {
        static const Glyph5x7 v{0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
        return v;
    }
    case 'B': {
        static const Glyph5x7 v{0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E};
        return v;
    }
    case 'C': {
        static const Glyph5x7 v{0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E};
        return v;
    }
    case 'D': {
        static const Glyph5x7 v{0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E};
        return v;
    }
    case 'E': {
        static const Glyph5x7 v{0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F};
        return v;
    }
    case 'F': {
        static const Glyph5x7 v{0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10};
        return v;
    }
    case 'G': {
        static const Glyph5x7 v{0x0E, 0x11, 0x10, 0x10, 0x13, 0x11, 0x0E};
        return v;
    }
    case 'H': {
        static const Glyph5x7 v{0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11};
        return v;
    }
    case 'I': {
        static const Glyph5x7 v{0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E};
        return v;
    }
    case 'J': {
        static const Glyph5x7 v{0x01, 0x01, 0x01, 0x01, 0x11, 0x11, 0x0E};
        return v;
    }
    case 'K': {
        static const Glyph5x7 v{0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11};
        return v;
    }
    case 'L': {
        static const Glyph5x7 v{0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F};
        return v;
    }
    case 'M': {
        static const Glyph5x7 v{0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11};
        return v;
    }
    case 'N': {
        static const Glyph5x7 v{0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11};
        return v;
    }
    case 'O': {
        static const Glyph5x7 v{0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
        return v;
    }
    case 'P': {
        static const Glyph5x7 v{0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10};
        return v;
    }
    case 'Q': {
        static const Glyph5x7 v{0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D};
        return v;
    }
    case 'R': {
        static const Glyph5x7 v{0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11};
        return v;
    }
    case 'S': {
        static const Glyph5x7 v{0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E};
        return v;
    }
    case 'T': {
        static const Glyph5x7 v{0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
        return v;
    }
    case 'U': {
        static const Glyph5x7 v{0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E};
        return v;
    }
    case 'V': {
        static const Glyph5x7 v{0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04};
        return v;
    }
    case 'W': {
        static const Glyph5x7 v{0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A};
        return v;
    }
    case 'X': {
        static const Glyph5x7 v{0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11};
        return v;
    }
    case 'Y': {
        static const Glyph5x7 v{0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04};
        return v;
    }
    case 'Z': {
        static const Glyph5x7 v{0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F};
        return v;
    }
    default:
        return kUnknown;
    }
}

void drawGlyph5x7(SDL_Renderer* renderer, char ch, int x, int y, int scale, SDL_Color color) {
    if (!renderer || scale <= 0) {
        return;
    }

    const Glyph5x7& glyph = glyphFor(ch);
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 5; ++col) {
            if ((glyph[row] & (1 << (4 - col))) == 0) {
                continue;
            }
            SDL_Rect pixel_rect{
                x + col * scale,
                y + row * scale,
                scale,
                scale
            };
            SDL_RenderFillRect(renderer, &pixel_rect);
        }
    }
}

std::vector<std::string> splitSubtitleLines(const std::string& text, size_t max_lines) {
    std::vector<std::string> lines;
    std::stringstream stream(text);
    std::string line;

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::string normalized;
        normalized.reserve(line.size());
        for (size_t i = 0; i < line.size();) {
            const unsigned char ch = static_cast<unsigned char>(line[i]);
            if (ch < 0x80) {
                normalized.push_back(static_cast<char>(ch));
                ++i;
                continue;
            }

            normalized.push_back('?');
            if ((ch & 0xE0) == 0xC0) {
                i += 2;
            } else if ((ch & 0xF0) == 0xE0) {
                i += 3;
            } else if ((ch & 0xF8) == 0xF0) {
                i += 4;
            } else {
                ++i;
            }
        }

        lines.push_back(std::move(normalized));
        if (lines.size() >= max_lines) {
            break;
        }
    }

    return lines;
}

} // namespace

Display::Display()
    : window_(nullptr)
    , renderer_(nullptr)
    , texture_(nullptr)
    , width_(0)
    , height_(0)
    , should_quit_(false)
    , toggle_pause_requested_(false)
    , fullscreen_(false)
    , initialized_(false)
    , seek_requested_(false)
    , seek_ratio_(0.0)
    , seek_delta_requested_(false)
    , seek_delta_seconds_(0.0)
    , volume_change_requested_(false)
    , requested_volume_(1.0f)
    , speed_change_requested_(false)
    , speed_delta_(0.0)
    , speed_reset_requested_(false)
    , subtitle_toggle_requested_(false)
    , next_item_requested_(false)
    , previous_item_requested_(false)
    , last_nonzero_volume_(1.0f)
    , dragging_seek_(false)
    , dragging_volume_(false)
    , seek_preview_active_(false)
    , seek_preview_ratio_(0.0)
    , overlay_position_(0.0)
    , overlay_duration_(0.0)
    , overlay_volume_(1.0f)
    , overlay_paused_(false) {
}

Display::~Display() {
    close();
}

bool Display::init(int width, int height, const std::string& title) {
    if (initialized_) {
        close();
    }
    
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "Error: Could not initialize SDL: " << SDL_GetError() << std::endl;
        return false;
    }
    
    const WindowSize window_size = computeInitialWindowSize(width, height);
    width_.store(window_size.width);
    height_.store(window_size.height);
    
    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width_.load(),
        height_.load(),
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!window_) {
        std::cerr << "Error: Could not create SDL window: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_SetWindowMinimumSize(window_, kMinWindowWidth, kMinWindowHeight);
    
    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer_) {
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer_) {
        std::cerr << "Error: Could not create SDL renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        return false;
    }
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    
    if (!createTexture(width, height)) {
        SDL_DestroyRenderer(renderer_);
        SDL_DestroyWindow(window_);
        renderer_ = nullptr;
        window_ = nullptr;
        return false;
    }
    
    initialized_ = true;
    should_quit_.store(false);
    toggle_pause_requested_.store(false);
    {
        std::lock_guard<std::mutex> lock(request_mutex_);
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
        next_item_requested_ = false;
        previous_item_requested_ = false;
        last_nonzero_volume_ = 1.0f;
        dragging_seek_ = false;
        dragging_volume_ = false;
        seek_preview_active_ = false;
        seek_preview_ratio_ = 0.0;
    }
    
    std::cout << "Display initialized: window " << width_.load() << "x" << height_.load()
              << " (source " << width << "x" << height << ")" << std::endl;
    
    return true;
}

void Display::close() {
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    
    if (renderer_) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
    
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    
    SDL_Quit();
    
    width_.store(0);
    height_.store(0);
    initialized_ = false;
}

bool Display::createTexture(int width, int height) {
    if (texture_) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    
    texture_ = SDL_CreateTexture(
        renderer_,
        SDL_PIXELFORMAT_IYUV,
        SDL_TEXTUREACCESS_STREAMING,
        width,
        height
    );
    
    if (!texture_) {
        std::cerr << "Error: Could not create SDL texture: " << SDL_GetError() << std::endl;
        return false;
    }
    
    return true;
}

bool Display::updateTexture(const uint8_t* data, int width, int height) {
    (void)width;
    (void)height;
    if (!texture_ || !data) {
        return false;
    }
    
    const AVFrame* frame = reinterpret_cast<const AVFrame*>(data);
    
    const int ret = SDL_UpdateYUVTexture(
        texture_,
        nullptr,
        frame->data[0], frame->linesize[0],
        frame->data[1], frame->linesize[1],
        frame->data[2], frame->linesize[2]
    );
    
    if (ret < 0) {
        std::cerr << "Error: Could not update texture: " << SDL_GetError() << std::endl;
        return false;
    }
    
    return true;
}

void Display::renderFrame(const uint8_t* data, int width, int height) {
    if (!renderer_ || !texture_ || !data) {
        return;
    }

    std::lock_guard<std::mutex> lock(sdl_mutex_);
    SDL_RenderClear(renderer_);
    if (!updateTexture(data, width, height)) {
        return;
    }
    
    const SDL_Rect dst_rect = computeRenderRect(width_.load(), height_.load(), width, height);
    SDL_RenderCopy(renderer_, texture_, nullptr, &dst_rect);
    drawSubtitleOverlay(width_.load(), height_.load());
    drawControls(width_.load(), height_.load());
}

void Display::present() {
    if (renderer_) {
        std::lock_guard<std::mutex> lock(sdl_mutex_);
        SDL_RenderPresent(renderer_);
    }
}

void Display::clear() {
    if (renderer_) {
        std::lock_guard<std::mutex> lock(sdl_mutex_);
        SDL_RenderClear(renderer_);
    }
}

void Display::handleEvents() {
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                should_quit_.store(true);
                break;
                
            case SDL_KEYDOWN:
            {
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
                if (!action && key_code == SDLK_EQUALS) {
                    action = input::PlayerAction::VolumeUp;
                } else if (!action && key_code == SDLK_MINUS) {
                    action = input::PlayerAction::VolumeDown;
                }
                if (!action) {
                    break;
                }

                switch (*action) {
                case input::PlayerAction::PlayPause:
                    toggle_pause_requested_.store(true);
                    break;
                case input::PlayerAction::SeekBackward:
                case input::PlayerAction::SeekForward: {
                    const bool ctrl_pressed = (event.key.keysym.mod & KMOD_CTRL) != 0;
                    const double delta = ctrl_pressed ? kSeekStepSecondsCtrl : kSeekStepSeconds;
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    seek_delta_seconds_ += (*action == input::PlayerAction::SeekForward) ? delta : -delta;
                    seek_delta_requested_ = true;
                    break;
                }
                case input::PlayerAction::VolumeUp:
                case input::PlayerAction::VolumeDown: {
                    const float direction = (*action == input::PlayerAction::VolumeUp) ? 1.0f : -1.0f;
                    const float next = clampVolume(overlay_volume_.load() + direction * kVolumeStep);
                    {
                        std::lock_guard<std::mutex> lock(request_mutex_);
                        requested_volume_ = next;
                        volume_change_requested_ = true;
                    }
                    overlay_volume_.store(next);
                    break;
                }
                case input::PlayerAction::ToggleMute: {
                    const float current = clampVolume(overlay_volume_.load());
                    float next = current;
                    if (current > 0.0001f) {
                        last_nonzero_volume_ = current;
                        next = 0.0f;
                    } else {
                        next = clampVolume(last_nonzero_volume_);
                        if (next < 0.0001f) {
                            next = 0.5f;
                        }
                    }
                    {
                        std::lock_guard<std::mutex> lock(request_mutex_);
                        requested_volume_ = next;
                        volume_change_requested_ = true;
                    }
                    overlay_volume_.store(next);
                    break;
                }
                case input::PlayerAction::SpeedDown:
                case input::PlayerAction::SpeedUp: {
                    const double delta = (*action == input::PlayerAction::SpeedUp) ? kSpeedStep : -kSpeedStep;
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    speed_delta_ += delta;
                    speed_change_requested_ = true;
                    break;
                }
                case input::PlayerAction::SpeedReset: {
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    speed_reset_requested_ = true;
                    break;
                }
                case input::PlayerAction::PreviousItem: {
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    previous_item_requested_ = true;
                    break;
                }
                case input::PlayerAction::NextItem: {
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    next_item_requested_ = true;
                    break;
                }
                case input::PlayerAction::ToggleSubtitle: {
                    std::lock_guard<std::mutex> lock(request_mutex_);
                    subtitle_toggle_requested_ = true;
                    break;
                }
                case input::PlayerAction::ToggleFullscreen:
                    toggleFullscreen();
                    break;
                case input::PlayerAction::Quit:
                    should_quit_.store(true);
                    break;
                }
                break;
            }
                
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                    event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                    event.window.event == SDL_WINDOWEVENT_MAXIMIZED ||
                    event.window.event == SDL_WINDOWEVENT_RESTORED) {
                    const int new_width = event.window.data1;
                    const int new_height = event.window.data2;
                    width_.store(std::max(1, new_width));
                    height_.store(std::max(1, new_height));
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button != SDL_BUTTON_LEFT) {
                    break;
                }
                if (pointInRect(event.button.x, event.button.y, computeControlLayout(width_.load(), height_.load()).progress_track)) {
                    dragging_seek_ = true;
                    updateSeekFromMouse(event.button.x, false);
                } else if (pointInRect(event.button.x, event.button.y, computeControlLayout(width_.load(), height_.load()).volume_track)) {
                    dragging_volume_ = true;
                    updateVolumeFromMouse(event.button.x);
                }
                break;

            case SDL_MOUSEMOTION:
                if (dragging_seek_) {
                    updateSeekFromMouse(event.motion.x, false);
                }
                if (dragging_volume_) {
                    updateVolumeFromMouse(event.motion.x);
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (event.button.button != SDL_BUTTON_LEFT) {
                    break;
                }
                if (dragging_seek_) {
                    updateSeekFromMouse(event.button.x, true);
                    dragging_seek_ = false;
                }
                if (dragging_volume_) {
                    updateVolumeFromMouse(event.button.x);
                    dragging_volume_ = false;
                }
                break;
                
            default:
                break;
        }
    }
}

bool Display::consumeTogglePauseRequest() {
    return toggle_pause_requested_.exchange(false);
}

bool Display::consumeSeekRequest(double& normalized_position) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!seek_requested_) {
        return false;
    }
    normalized_position = seek_ratio_;
    seek_requested_ = false;
    return true;
}

bool Display::consumeSeekDeltaRequest(double& delta_seconds) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!seek_delta_requested_) {
        return false;
    }
    delta_seconds = seek_delta_seconds_;
    seek_delta_seconds_ = 0.0;
    seek_delta_requested_ = false;
    return true;
}

bool Display::consumeVolumeChangeRequest(float& volume) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!volume_change_requested_) {
        return false;
    }
    volume = requested_volume_;
    volume_change_requested_ = false;
    return true;
}

bool Display::consumeSpeedChangeRequest(double& speed_delta) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!speed_change_requested_) {
        return false;
    }
    speed_delta = speed_delta_;
    speed_delta_ = 0.0;
    speed_change_requested_ = false;
    return true;
}

bool Display::consumeResetSpeedRequest() {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!speed_reset_requested_) {
        return false;
    }
    speed_reset_requested_ = false;
    return true;
}

bool Display::consumeToggleSubtitleRequest() {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!subtitle_toggle_requested_) {
        return false;
    }
    subtitle_toggle_requested_ = false;
    return true;
}

bool Display::consumeNextItemRequest() {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!next_item_requested_) {
        return false;
    }
    next_item_requested_ = false;
    return true;
}

bool Display::consumePreviousItemRequest() {
    std::lock_guard<std::mutex> lock(request_mutex_);
    if (!previous_item_requested_) {
        return false;
    }
    previous_item_requested_ = false;
    return true;
}

void Display::setOverlayState(double position, double duration, float volume, bool paused) {
    overlay_position_.store(std::max(0.0, position));
    overlay_duration_.store(std::max(0.0, duration));
    const float clamped_volume = clampVolume(volume);
    overlay_volume_.store(clamped_volume);
    if (clamped_volume > 0.0001f) {
        last_nonzero_volume_ = clamped_volume;
    }
    overlay_paused_.store(paused);
}

void Display::setSubtitleText(const std::string& text) {
    std::lock_guard<std::mutex> lock(subtitle_mutex_);
    subtitle_text_ = text;
}

void Display::setHotkeyManager(const input::HotkeyManager& hotkey_manager) {
    std::lock_guard<std::mutex> lock(request_mutex_);
    hotkey_manager_ = hotkey_manager;
}

void Display::drawSubtitleOverlay(int window_width, int window_height) {
    if (!renderer_) {
        return;
    }

    std::string subtitle_text;
    {
        std::lock_guard<std::mutex> lock(subtitle_mutex_);
        subtitle_text = subtitle_text_;
    }

    if (subtitle_text.empty()) {
        return;
    }

    constexpr size_t kMaxSubtitleLines = 3;
    auto lines = splitSubtitleLines(subtitle_text, kMaxSubtitleLines);
    if (lines.empty()) {
        return;
    }

    const int scale = std::max(1, std::min(3, window_width / 520));
    const int glyph_width = 5 * scale;
    const int glyph_height = 7 * scale;
    const int glyph_spacing = scale;
    const int line_spacing = scale * 2;
    const int horizontal_margin = std::max(16, scale * 8);
    const int char_step = glyph_width + glyph_spacing;
    const size_t max_chars = std::max<size_t>(8, static_cast<size_t>(std::max(1, window_width - horizontal_margin * 2) / std::max(1, char_step)));

    int content_width = 0;
    for (std::string& line : lines) {
        if (line.size() > max_chars) {
            if (max_chars > 3) {
                line = line.substr(0, max_chars - 3) + "...";
            } else {
                line = line.substr(0, max_chars);
            }
        }
        const int line_width = static_cast<int>(line.empty() ? 0 : line.size() * char_step - glyph_spacing);
        content_width = std::max(content_width, line_width);
    }

    const int content_height =
        static_cast<int>(lines.size()) * glyph_height + static_cast<int>(std::max<size_t>(0, lines.size() - 1)) * line_spacing;

    if (content_width <= 0 || content_height <= 0) {
        return;
    }

    const ControlLayout controls = computeControlLayout(window_width, window_height);
    const int panel_padding = scale * 4;
    const int panel_width = content_width + panel_padding * 2;
    const int panel_height = content_height + panel_padding * 2;
    const int panel_x = std::max(0, (window_width - panel_width) / 2);
    const int panel_bottom = std::max(0, controls.panel.y - scale * 3);
    const int panel_y = std::max(0, panel_bottom - panel_height);

    SDL_Rect subtitle_panel{panel_x, panel_y, panel_width, panel_height};
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 150);
    SDL_RenderFillRect(renderer_, &subtitle_panel);

    SDL_Color shadow{0, 0, 0, 230};
    SDL_Color front{242, 242, 242, 245};

    for (size_t line_index = 0; line_index < lines.size(); ++line_index) {
        const std::string& line = lines[line_index];
        const int line_width = static_cast<int>(line.empty() ? 0 : line.size() * char_step - glyph_spacing);
        int x = panel_x + (panel_width - line_width) / 2;
        const int y = panel_y + panel_padding + static_cast<int>(line_index) * (glyph_height + line_spacing);
        for (char ch : line) {
            drawGlyph5x7(renderer_, ch, x + 1, y + 1, scale, shadow);
            drawGlyph5x7(renderer_, ch, x, y, scale, front);
            x += char_step;
        }
    }
}

Display::ControlLayout Display::computeControlLayout(int window_width, int window_height) const {
    const int safe_width = std::max(1, window_width);
    const int safe_height = std::max(1, window_height);

    ControlLayout layout{};
    layout.panel = {
        kControlPanelInset,
        std::max(0, safe_height - kControlPanelHeight - kControlPanelInset),
        std::max(1, safe_width - (kControlPanelInset * 2)),
        kControlPanelHeight
    };

    const int track_y = layout.panel.y + (layout.panel.h - kBarHeight) / 2;
    const int volume_width = std::min(kVolumeBarWidth, std::max(56, layout.panel.w / 5));
    int progress_width = layout.panel.w - (kControlPadding * 2) - volume_width - kControlGap;
    progress_width = std::max(kMinProgressBarWidth, progress_width);

    layout.progress_track = {
        layout.panel.x + kControlPadding,
        track_y,
        std::min(progress_width, std::max(1, layout.panel.w - (kControlPadding * 2))),
        kBarHeight
    };
    layout.volume_track = {
        layout.panel.x + layout.panel.w - kControlPadding - volume_width,
        track_y,
        std::max(1, volume_width),
        kBarHeight
    };
    return layout;
}

void Display::drawControls(int window_width, int window_height) {
    if (!renderer_) {
        return;
    }

    const ControlLayout layout = computeControlLayout(window_width, window_height);
    double progress = 0.0;
    const double duration = overlay_duration_.load();
    if (duration > 0.0) {
        progress = overlay_position_.load() / duration;
    }

    float volume = overlay_volume_.load();
    bool paused = overlay_paused_.load();

    {
        std::lock_guard<std::mutex> lock(request_mutex_);
        if (seek_preview_active_) {
            progress = seek_preview_ratio_;
        }
        if (dragging_volume_) {
            volume = requested_volume_;
        }
    }

    progress = clampRatio(progress);
    volume = clampVolume(volume);

    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 140);
    SDL_RenderFillRect(renderer_, &layout.panel);

    SDL_SetRenderDrawColor(renderer_, 85, 85, 85, 230);
    SDL_RenderFillRect(renderer_, &layout.progress_track);

    SDL_Rect progress_fill = layout.progress_track;
    progress_fill.w = std::max(1, static_cast<int>(std::lround(progress * static_cast<double>(layout.progress_track.w))));
    SDL_SetRenderDrawColor(renderer_, 245, 245, 245, 235);
    SDL_RenderFillRect(renderer_, &progress_fill);

    SDL_SetRenderDrawColor(renderer_, 85, 85, 85, 230);
    SDL_RenderFillRect(renderer_, &layout.volume_track);

    SDL_Rect volume_fill = layout.volume_track;
    volume_fill.w = std::max(1, static_cast<int>(std::lround(volume * static_cast<double>(layout.volume_track.w))));
    SDL_SetRenderDrawColor(renderer_, 130, 220, 160, 235);
    SDL_RenderFillRect(renderer_, &volume_fill);

    if (paused) {
        SDL_Rect pause_flag{
            layout.panel.x + layout.panel.w / 2 - 6,
            layout.panel.y + 8,
            12,
            6
        };
        SDL_SetRenderDrawColor(renderer_, 255, 204, 102, 235);
        SDL_RenderFillRect(renderer_, &pause_flag);
    }
}

void Display::updateSeekFromMouse(int mouse_x, bool commit) {
    const ControlLayout layout = computeControlLayout(width_.load(), height_.load());
    if (layout.progress_track.w <= 0) {
        return;
    }
    const double ratio = clampRatio(
        static_cast<double>(mouse_x - layout.progress_track.x) / static_cast<double>(layout.progress_track.w));

    std::lock_guard<std::mutex> lock(request_mutex_);
    seek_preview_active_ = true;
    seek_preview_ratio_ = ratio;
    if (commit) {
        seek_ratio_ = ratio;
        seek_requested_ = true;
        seek_preview_active_ = false;
    }
}

void Display::updateVolumeFromMouse(int mouse_x) {
    const ControlLayout layout = computeControlLayout(width_.load(), height_.load());
    if (layout.volume_track.w <= 0) {
        return;
    }
    const float next_volume = static_cast<float>(clampRatio(
        static_cast<double>(mouse_x - layout.volume_track.x) / static_cast<double>(layout.volume_track.w)));
    {
        std::lock_guard<std::mutex> lock(request_mutex_);
        requested_volume_ = next_volume;
        volume_change_requested_ = true;
    }
    overlay_volume_.store(next_volume);
}

void Display::toggleFullscreen() {
    std::lock_guard<std::mutex> lock(sdl_mutex_);
    if (!window_) {
        return;
    }

    const bool next_fullscreen = !fullscreen_.load();
    fullscreen_.store(next_fullscreen);

    if (next_fullscreen) {
        SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        SDL_SetWindowFullscreen(window_, 0);
    }

    int updated_width = width_.load();
    int updated_height = height_.load();
    SDL_GetWindowSize(window_, &updated_width, &updated_height);
    width_.store(std::max(1, updated_width));
    height_.store(std::max(1, updated_height));
}

} // namespace vp
