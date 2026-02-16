cmake_minimum_required(VERSION 3.15)
project(modern-video-player VERSION 1.0.0 LANGUAGES C CXX)

# 设置 C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows 特定配置
if(WIN32)
    # Windows 编译选项
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    
    # 查找 SDL2
    # 方法1: 使用 vcpkg
    find_package(SDL2 CONFIG QUIET)
    
    # 方法2: 手动指定 SDL2 路径
    if(NOT SDL2_FOUND)
        set(SDL2_DIR "${CMAKE_SOURCE_DIR}/external/SDL2" CACHE PATH "SDL2 directory")
        if(EXISTS ${SDL2_DIR})
            include_directories(${SDL2_DIR}/include)
            link_directories(${SDL2_DIR}/lib/x64)
        else()
            message(FATAL_ERROR "SDL2 not found. Please install SDL2 and set SDL2_DIR")
        endif()
    endif()
    
    # 查找 FFmpeg
    # 方法1: 使用 vcpkg
    find_package(FFMPEG CONFIG QUIET)
    
    # 方法2: 手动指定 FFmpeg 路径
    if(NOT FFMPEG_FOUND)
        set(FFMPEG_DIR "${CMAKE_SOURCE_DIR}/external/ffmpeg" CACHE PATH "FFmpeg directory")
        if(EXISTS ${FFMPEG_DIR})
            include_directories(${FFMPEG_DIR}/include)
            link_directories(${FFMPEG_DIR}/lib)
        else()
            message(FATAL_ERROR "FFmpeg not found. Please install FFmpeg and set FFMPEG_DIR")
        endif()
    endif()
    
    # 查找 Quill (日志库)
    find_package(quill CONFIG QUIET)
    
    # 方法2: 手动指定 Quill 路径
    if(NOT quill_FOUND)
        set(QUILL_DIR "${CMAKE_SOURCE_DIR}/external/quill" CACHE PATH "Quill directory")
        if(EXISTS ${QUILL_DIR})
            include_directories(${QUILL_DIR}/include)
            link_directories(${QUILL_DIR}/lib)
        else()
            message(WARNING "Quill not found. Logging will be disabled.")
            set(QUILL_FOUND FALSE)
        endif()
    endif()
    
    # Windows 链接库
    set(SDL2_LIBRARIES SDL2main SDL2 SDL2)
    set(FFMPEG_LIBRARIES 
        avformat 
        avcodec 
        avutil 
        swscale 
        swresample
    )
    
    if(quill_FOUND)
        set(QUILL_LIBRARIES quill)
    endif()
    
    # 添加 Windows 子系统
    if(MSVC)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE")
    endif()
    
else()
    # Linux/macOS 配置
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    
    # 查找 FFmpeg
    pkg_check_modules(FFMPEG REQUIRED 
        libavformat
        libavcodec
        libavutil
        libswscale
        libswresample
    )
    
    # 查找 Quill
    pkg_check_modules(QUILL QUIET quill)
    
    # Linux/macOS 编译选项
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    
    # Linux/macOS 链接库
    set(SDL2_LIBRARIES ${SDL2_LIBRARIES})
    set(FFMPEG_LIBRARIES
        ${FFMPEG_LIBAVFORMAT_LIBRARIES}
        ${FFMPEG_LIBAVCODEC_LIBRARIES}
        ${FFMPEG_LIBAVUTIL_LIBRARIES}
        ${FFMPEG_LIBSWSCALE_LIBRARIES}
        ${FFMPEG_LIBSWRESAMPLE_LIBRARIES}
    )
    
    if(QUILL_FOUND)
        set(QUILL_LIBRARIES ${QUILL_LIBRARIES})
    endif()
endif()

# 包含头文件目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
    ${QUILL_INCLUDE_DIRS}
)

# 定义可执行文件
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/video_player.cpp
    src/video_decoder.cpp
    src/audio_decoder.cpp
    src/display.cpp
    src/audio_player.cpp
    src/logger.cpp
)

# 链接库
if(WIN32)
    target_link_libraries(${PROJECT_NAME}
        ${SDL2_LIBRARIES}
        ${FFMPEG_LIBRARIES}
        ${QUILL_LIBRARIES}
    )
    
    # 添加 DLL 到输出目录（Windows）
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SDL2_DIR}/bin/SDL2.dll
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
else()
    target_link_libraries(${PROJECT_NAME}
        ${SDL2_LIBRARIES}
        ${FFMPEG_LIBRARIES}
        ${QUILL_LIBRARIES}
        pthread
        m
    )
endif()

# 如果找到 Quill，定义宏
if(quill_FOUND OR QUILL_FOUND)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_QUILL_LOGGING)
endif()

# 输出构建信息
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "SDL2 found: ${SDL2_FOUND}")
message(STATUS "FFmpeg found: ${FFMPEG_FOUND}")
message(STATUS "Quill found: ${quill_FOUND}${QUILL_FOUND}")
