# Modern Video Player

A modern C++17 video player using FFmpeg and SDL2, supporting both Windows and Linux.

## Features

- Video decoding and display
- Audio decoding and playback
- Audio-video synchronization
- Playback controls (play/pause/stop)
- Fullscreen support
- Volume control
- Playback speed adjustment

## Tech Stack

- **Language**: C++17
- **Video Library**: FFmpeg 8.0.1
- **Display/Audio Library**: SDL2 2.30.11
- **Logging Library**: Quill 6.0.0
- **Build System**: CMake 3.15+
- **Compilers**: 
  - Windows: MSVC 2017+ / MinGW-w64
  - Linux: GCC 9+
  - macOS: Clang 10+

## Project Structure

```
modern-video-player/
├── CMakeLists.txt          # CMake build configuration
├── README.md              # Project documentation
├── include/               # Header files
│   ├── video_player.h     # Main player class
│   ├── video_decoder.h    # Video decoder
│   ├── audio_decoder.h    # Audio decoder
│   ├── display.h          # Display window
│   └── audio_player.h     # Audio player
├── src/                   # Source files
│   ├── main.cpp           # Main program entry
│   ├── video_player.cpp   # Player implementation
│   ├── video_decoder.cpp  # Video decoder implementation
│   ├── audio_decoder.cpp  # Audio decoder implementation
│   ├── display.cpp        # Display implementation
│   └── audio_player.cpp   # Audio player implementation
└── docs/                  # Documentation
    ├── IMPLEMENTATION.md  # Step-by-step implementation guide
    └── ARCHITECTURE.md    # Architecture design document
```

## Installation

### Windows

#### Option 1: Using vcpkg (Recommended)

1. Install [vcpkg](https://github.com/microsoft/vcpkg)
2. Install dependencies:
```bash
vcpkg install sdl2 ffmpeg:x64-windows quill:x64-windows
```

3. Set vcpkg toolchain:
```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

#### Option 2: Manual Installation

1. Download and install FFmpeg:
   - Download from [gyan.dev](https://www.gyan.dev/ffmpeg/builds/)
   - Extract to `external/ffmpeg/`

2. Download and install SDL2:
   - Download from [SDL2 Releases](https://github.com/libsdl-org/SDL/releases)
   - Extract to `external/SDL2/`

3. Build:
```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Linux

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libavformat-dev \
    libavcodec-dev \
    libswscale-dev \
    libswresample-dev \
    libavutil-dev \
    libsdl2-dev

mkdir build && cd build
cmake ..
make
```

### macOS

```bash
brew install ffmpeg sdl2 cmake pkg-config

mkdir build && cd build
cmake ..
make
```

## Usage

```bash
# Windows
build\Release\modern-video-player.exe your_video.mp4

# Linux/macOS
./build/modern-video-player your_video.mp4
```

## Keyboard Shortcuts

| Key | Function |
|-----|----------|
| Space | Play/Pause |
| Enter / Alt+Enter / F | Toggle Fullscreen |
| Esc | Exit Fullscreen (or quit if windowed) |
| Q | Quit |
| Left / Right | Seek -/+5s |
| Ctrl+Left / Ctrl+Right | Seek -/+30s |
| Up / Down / +/- | Volume +5/-5 |
| M | Mute / Unmute |
| \[ / \] / R | Speed down/up/reset |
| PageUp / PageDown | Previous / Next item |
| Home / End | Previous / Next chapter |
| A / B / C | Set A / Set B / Clear A-B repeat |
| S | Save screenshot |
| V | Toggle subtitles |

## Technical Highlights

### C++17 Features

- `std::unique_ptr` - Automatic resource management
- `std::atomic` - Thread-safe atomic operations
- `std::thread` - Multi-threading support
- `std::mutex` - Mutex for shared resource protection
- `std::condition_variable` - Thread synchronization

### FFmpeg Core Concepts

1. **AVFormatContext**: Media file context containing all stream information
2. **AVCodecContext**: Codec context with decoder configuration
3. **AVPacket**: Compressed data packet
4. **AVFrame**: Decoded raw data (video YUV, audio PCM)
5. **SwsContext**: Video format conversion context
6. **SwrContext**: Audio format conversion context

### SDL2 Core Concepts

1. **SDL_Window**: Window object for displaying video
2. **SDL_Renderer**: Renderer for drawing to window
3. **SDL_Texture**: Texture for storing video frame data
4. **SDL_AudioDevice**: Audio device for playing audio

## Architecture

```
┌─────────────────────────────────────────┐
│           VideoPlayer                   │
│  ┌─────────────────────────────────┐    │
│  │      VideoDecoder              │    │
│  │  - Decode video frames         │    │
│  │  - Format conversion (YUV)     │    │
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │      AudioDecoder              │    │
│  │  - Decode audio frames         │    │
│  │  - Format conversion (S16)     │    │
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │      Display                    │    │
│  │  - SDL window management       │    │
│  │  - YUV texture rendering       │    │
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │      AudioPlayer                │    │
│  │  - SDL audio playback           │    │
│  │  - Volume control               │    │
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │      Play Thread                │    │
│  │  - Audio-video synchronization  │    │
│  │  - Frame rate control           │    │
│  └─────────────────────────────────┘    │
└─────────────────────────────────────────┘
```

## Documentation

- [Implementation Guide](docs/IMPLEMENTATION.md) - Detailed step-by-step implementation
- [Architecture Design](docs/ARCHITECTURE.md) - System architecture and design patterns
- [Version History](docs/VERSION.md) - Project version and dependency information

## Learning Roadmap

### Basic Knowledge
1. C++17 features
2. Multi-threading
3. FFmpeg basics
4. SDL2 basics

### Advanced Topics
1. Audio-video synchronization algorithms
2. Player architecture design
3. Performance optimization
4. Error handling

### Future Extensions
1. Playlist support
2. Subtitle support
3. Variable speed playback
4. Playback history
5. Hardware acceleration

## Troubleshooting

### Windows

**Problem**: SDL2.dll not found

**Solution**: Ensure SDL2.dll is in the same directory as the executable. The build script should copy it automatically.

**Problem**: FFmpeg libraries not found

**Solution**: Set `FFMPEG_DIR` to point to your FFmpeg installation:
```bash
cmake -DFFMPEG_DIR=path/to/ffmpeg ..
```

### Linux

**Problem**: pkg-config not found

**Solution**:
```bash
sudo apt-get install pkg-config
```

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

## License

MIT License

## Acknowledgments

- FFmpeg - A complete, cross-platform solution to record, convert and stream audio and video
- SDL2 - Simple DirectMedia Layer

## Author

Created as a modern C++17 video player implementation for learning and educational purposes.
