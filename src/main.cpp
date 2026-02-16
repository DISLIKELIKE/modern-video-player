#include "video_player.h"
#include "logger.h"
#include <iostream>
#include <csignal>
#include <memory>

using namespace vp;

static std::unique_ptr<VideoPlayer> g_player;

void signalHandler(int signal) {
    if (g_player) {
        g_player->stop();
    }
    Logger::shutdown();
    exit(0);
}

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " <video_file>" << std::endl;
    std::cout << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  SPACE - Play/Pause" << std::endl;
    std::cout << "  Q/ESC - Quit" << std::endl;
    std::cout << "  F - Toggle Fullscreen" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    // 初始化日志系统
    Logger::init();
    
    if (argc < 2) {
        printUsage(argv[0]);
        Logger::shutdown();
        return 1;
    }
    
    std::cout << "========================================" << std::endl;
    std::cout << "  Video Player - FFmpeg + SDL2 + C++17" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    Logger::info("Starting Modern Video Player");
    
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    g_player = std::make_unique<VideoPlayer>();
    
    std::string filename(argv[1]);
    Logger::info("Opening file: " + filename);
    
    if (!g_player->open(filename)) {
        Logger::error("Could not open video file");
        Logger::shutdown();
        return 1;
    }
    
    Logger::info("Starting playback...");
    std::cout << "Playing video..." << std::endl;
    std::cout << std::endl;
    
    g_player->play();
    
    while (g_player->isPlaying()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    Logger::info("Playback finished");
    g_player->stop();
    g_player->close();
    
    Logger::shutdown();
    
    std::cout << std::endl;
    std::cout << "Playback finished." << std::endl;
    
    return 0;
}
