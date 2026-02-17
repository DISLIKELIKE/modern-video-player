cmake_minimum_required(VERSION 3.15)
set(CMAKE_SYSTEM_VERSION "10.0.26100.0")
project(modern-video-player VERSION 1.0.0 LANGUAGES C CXX)

# 设置 C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows 特定配置
if(WIN32)
    # Windows 编译选项
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    
    # 查找 SDL2
find_package(SDL2 CONFIG QUIET)

if(NOT SDL2_FOUND)
    # 直接使用普通变量保存路径，避免缓存变量作用域问题
    set(SDL2_PATH "${CMAKE_SOURCE_DIR}/external/SDL2")
    message(STATUS "Looking for SDL2 at: ${SDL2_PATH}")
    
    if(EXISTS ${SDL2_PATH})
        include_directories(${SDL2_PATH}/include)
        link_directories(${SDL2_PATH}/lib/x64)
        # 将路径存入缓存，以便用户修改（可选）
        set(SDL2_DIR ${SDL2_PATH} CACHE PATH "SDL2 directory" FORCE)
    else()
        message(FATAL_ERROR "SDL2 not found at ${SDL2_PATH}. Please install SDL2 and set SDL2_DIR")
    endif()
endif()
    
    # 查找 FFmpeg
find_package(FFMPEG CONFIG QUIET)
if(NOT FFMPEG_FOUND)
    # 直接使用普通变量保存路径，避免缓存变量作用域问题
    set(FFMPEG_PATH "${CMAKE_SOURCE_DIR}/external/ffmpeg")
    message(STATUS "Looking for FFmpeg at: ${FFMPEG_PATH}")
    
    if(EXISTS ${FFMPEG_PATH})
        set(FFMPEG_INCLUDE_DIRS ${FFMPEG_PATH}/include)
        include_directories(${FFMPEG_INCLUDE_DIRS})
        
        # 查找各个库（使用find_library精确定位）
        find_library(AVFORMAT_LIBRARY avformat PATHS ${FFMPEG_PATH}/lib NO_DEFAULT_PATH)
        find_library(AVCODEC_LIBRARY avcodec PATHS ${FFMPEG_PATH}/lib NO_DEFAULT_PATH)
        find_library(AVUTIL_LIBRARY avutil PATHS ${FFMPEG_PATH}/lib NO_DEFAULT_PATH)
        find_library(SWSCALE_LIBRARY swscale PATHS ${FFMPEG_PATH}/lib NO_DEFAULT_PATH)
        find_library(SWRESAMPLE_LIBRARY swresample PATHS ${FFMPEG_PATH}/lib NO_DEFAULT_PATH)
        
        if(AVFORMAT_LIBRARY AND AVCODEC_LIBRARY AND AVUTIL_LIBRARY AND SWSCALE_LIBRARY AND SWRESAMPLE_LIBRARY)
            set(FFMPEG_LIBRARIES ${AVFORMAT_LIBRARY} ${AVCODEC_LIBRARY} ${AVUTIL_LIBRARY} ${SWSCALE_LIBRARY} ${SWRESAMPLE_LIBRARY})
            set(FFMPEG_FOUND TRUE)
            # 将路径存入缓存，以便用户修改
            set(FFMPEG_DIR ${FFMPEG_PATH} CACHE PATH "FFmpeg directory" FORCE)
            message(STATUS "Found FFmpeg libraries: ${FFMPEG_LIBRARIES}")
        else()
            message(FATAL_ERROR "FFmpeg libraries not found in ${FFMPEG_PATH}/lib")
        endif()
    else()
        message(FATAL_ERROR "FFmpeg directory not found: ${FFMPEG_PATH}")
    endif()
endif()
    
    # 查找 Quill (日志库) - header-only，无需链接库
    # 注意: 由于 Quill v6.x API 变化，暂不启用 Quill 支持
    # 如果需要启用，请确保 Quill 版本兼容
find_package(quill CONFIG QUIET)

if(quill_FOUND AND FALSE)  # 暂时禁用 Quill
    # 如果找到 CMake 配置，通常 quill 会提供目标 quill::quill (也是头文件仅用)
    if(TARGET quill::quill)
        set(QUILL_LIBRARIES quill::quill)
    else()
        set(QUILL_LIBRARIES "")
    endif()
else()
    # 手动指定路径模式
    set(QUILL_PATH "${CMAKE_SOURCE_DIR}/external/quill")
    message(STATUS "Looking for Quill at: ${QUILL_PATH}")
    if(EXISTS ${QUILL_PATH})
        include_directories(${QUILL_PATH}/include)
        set(QUILL_LIBRARIES "")
        # set(quill_FOUND TRUE)  # 暂时禁用
        # 将路径存入缓存，以便用户修改（可选）
        set(QUILL_DIR ${QUILL_PATH} CACHE PATH "Quill directory" FORCE)
    else()
        message(STATUS "Quill not found at ${QUILL_PATH}. Using std::cout for logging.")
        set(quill_FOUND FALSE)
    endif()
endif()
    
    # Windows 链接库
    set(SDL2_LIBRARIES SDL2main SDL2 SDL2)
    
    
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
    
    # 查找 Quill - 暂时禁用，因为 v6.x API 不兼容
    # pkg_check_modules(QUILL QUIET quill)
    
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

# 如果找到 Quill，定义宏 - 暂时禁用
# if(quill_FOUND OR QUILL_FOUND)
#     target_compile_definitions(${PROJECT_NAME} PRIVATE USE_QUILL_LOGGING)
# endif()

# 输出构建信息
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "SDL2 found: ${SDL2_FOUND}")
message(STATUS "FFmpeg found: ${FFMPEG_FOUND}")
message(STATUS "Quill found: disabled (using std::cout)")
