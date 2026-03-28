cmake_minimum_required(VERSION 3.15)
project(modern-video-player VERSION 1.0.0 LANGUAGES C CXX)
if(WIN32)
    set(CMAKE_SYSTEM_VERSION "10.0.26100.0")
endif()

set(MVP_VERSION_PRERELEASE "rc2" CACHE STRING "Prerelease suffix without leading dash")
set(MVP_VERSION_BUILD 0 CACHE STRING "Fourth component for Windows file version")
set(MVP_PRODUCT_NAME "Modern Video Player")
set(MVP_PACKAGE_VENDOR "modern-video-player")

set(MVP_VERSION_SUFFIX "")
set(MVP_VERSION_RESOURCE_FILEFLAGS "0x0L")
if(MVP_VERSION_PRERELEASE)
    set(MVP_VERSION_SUFFIX "-${MVP_VERSION_PRERELEASE}")
    set(MVP_VERSION_RESOURCE_FILEFLAGS "VS_FF_PRERELEASE")
endif()

set(MVP_VERSION_SEMVER "${PROJECT_VERSION}${MVP_VERSION_SUFFIX}")
set(MVP_VERSION_FILE_DOTTED
    "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${MVP_VERSION_BUILD}"
)
set(MVP_VERSION_FILE_COMMA
    "${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},${MVP_VERSION_BUILD}"
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(MVP_PACKAGE_ARCH "x64")
else()
    set(MVP_PACKAGE_ARCH "x86")
endif()

if(WIN32)
    set(MVP_PACKAGE_PLATFORM "windows")
elseif(APPLE)
    set(MVP_PACKAGE_PLATFORM "macos")
else()
    set(MVP_PACKAGE_PLATFORM "linux")
endif()

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")
configure_file("${CMAKE_SOURCE_DIR}/cmake/mvp_version.h.in"
               "${CMAKE_BINARY_DIR}/generated/mvp_version.h" @ONLY)
if(WIN32)
    configure_file("${CMAKE_SOURCE_DIR}/cmake/version_info.rc.in"
                   "${CMAKE_BINARY_DIR}/generated/version_info.rc" @ONLY)
    set(MVP_WIN32_RESOURCE "${CMAKE_BINARY_DIR}/generated/version_info.rc")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(DEBUG_MODE "Enable debug logging" ON)

if(WIN32)
    option(ENABLE_D3D11_RENDERER "Build D3D11 renderer backend" ON)
    option(ENABLE_D3D11VA "Enable D3D11VA decode backend capability" ON)
    option(ENABLE_DXVA2 "Enable DXVA2 decode backend capability (reserved switch)" OFF)
else()
    option(ENABLE_D3D11_RENDERER "Build D3D11 renderer backend" OFF)
    option(ENABLE_D3D11VA "Enable D3D11VA decode backend capability" OFF)
    option(ENABLE_DXVA2 "Enable DXVA2 decode backend capability (reserved switch)" OFF)
endif()
option(ENABLE_OPENGL_RENDERER "Build OpenGL renderer backend" ON)
option(ENABLE_SDL_RENDERER "Build SDL software renderer backend" ON)
if((UNIX AND NOT APPLE) OR WIN32)
    option(ENABLE_VULKAN_RENDERER "Build Vulkan renderer backend (Linux-first)" ON)
else()
    option(ENABLE_VULKAN_RENDERER "Build Vulkan renderer backend (Linux-first)" OFF)
endif()
if(APPLE)
    option(ENABLE_VIDEOTOOLBOX "Enable VideoToolbox decode backend capability (reserved switch)" ON)
else()
    option(ENABLE_VIDEOTOOLBOX "Enable VideoToolbox decode backend capability (reserved switch)" OFF)
endif()
if(UNIX AND NOT APPLE)
    option(ENABLE_VAAPI "Enable VAAPI decode backend capability (reserved switch)" ON)
else()
    option(ENABLE_VAAPI "Enable VAAPI decode backend capability (reserved switch)" OFF)
endif()

set(MVP_ENABLE_D3D11_RENDERER ${ENABLE_D3D11_RENDERER})
if(MVP_ENABLE_D3D11_RENDERER AND NOT WIN32)
    message(WARNING "ENABLE_D3D11_RENDERER requires WIN32; forcing OFF")
    set(MVP_ENABLE_D3D11_RENDERER OFF)
endif()

set(MVP_ENABLE_D3D11VA ${ENABLE_D3D11VA})
if(MVP_ENABLE_D3D11VA AND NOT WIN32)
    message(WARNING "ENABLE_D3D11VA requires WIN32; forcing OFF")
    set(MVP_ENABLE_D3D11VA OFF)
endif()

set(MVP_ENABLE_DXVA2 ${ENABLE_DXVA2})
if(MVP_ENABLE_DXVA2 AND NOT WIN32)
    message(WARNING "ENABLE_DXVA2 requires WIN32; forcing OFF")
    set(MVP_ENABLE_DXVA2 OFF)
endif()

set(MVP_ENABLE_VAAPI ${ENABLE_VAAPI})
if(MVP_ENABLE_VAAPI AND (WIN32 OR APPLE))
    message(WARNING "ENABLE_VAAPI requires Linux/Unix non-Apple; forcing OFF")
    set(MVP_ENABLE_VAAPI OFF)
endif()

set(MVP_ENABLE_VIDEOTOOLBOX ${ENABLE_VIDEOTOOLBOX})
if(MVP_ENABLE_VIDEOTOOLBOX AND NOT APPLE)
    message(WARNING "ENABLE_VIDEOTOOLBOX requires APPLE; forcing OFF")
    set(MVP_ENABLE_VIDEOTOOLBOX OFF)
endif()

set(MVP_ENABLE_OPENGL_RENDERER ${ENABLE_OPENGL_RENDERER})
set(MVP_ENABLE_SDL_RENDERER ${ENABLE_SDL_RENDERER})
set(MVP_ENABLE_VULKAN_RENDERER ${ENABLE_VULKAN_RENDERER})
if(MVP_ENABLE_VULKAN_RENDERER AND APPLE)
    message(WARNING "ENABLE_VULKAN_RENDERER requires Windows or Linux/Unix non-Apple; forcing OFF on Apple")
    set(MVP_ENABLE_VULKAN_RENDERER OFF)
endif()

set(MVP_HAVE_SOFTWARE_SDL_RENDERER 0)
if(MVP_ENABLE_SDL_RENDERER)
    set(MVP_HAVE_SOFTWARE_SDL_RENDERER 1)
endif()
set(MVP_HAVE_OPENGL_RENDERER 0)
if(MVP_ENABLE_OPENGL_RENDERER)
    set(MVP_HAVE_OPENGL_RENDERER 1)
endif()
set(MVP_HAVE_D3D11_RENDERER 0)
if(MVP_ENABLE_D3D11_RENDERER)
    set(MVP_HAVE_D3D11_RENDERER 1)
endif()
set(MVP_HAVE_D3D11VA_DECODER 0)
if(MVP_ENABLE_D3D11VA)
    set(MVP_HAVE_D3D11VA_DECODER 1)
endif()
set(MVP_HAVE_DXVA2_DECODER 0)
if(MVP_ENABLE_DXVA2)
    set(MVP_HAVE_DXVA2_DECODER 1)
endif()
set(MVP_HAVE_VAAPI_DECODER 0)
if(MVP_ENABLE_VAAPI)
    set(MVP_HAVE_VAAPI_DECODER 1)
endif()
set(MVP_HAVE_VIDEOTOOLBOX_DECODER 0)
if(MVP_ENABLE_VIDEOTOOLBOX)
    set(MVP_HAVE_VIDEOTOOLBOX_DECODER 1)
endif()
set(MVP_HAVE_VULKAN_RENDERER 0)
set(MVP_VULKAN_DEPENDENCY_SOURCE "disabled")

message(STATUS "Feature switches: D3D11_RENDERER=${MVP_ENABLE_D3D11_RENDERER} OPENGL_RENDERER=${MVP_ENABLE_OPENGL_RENDERER} SDL_RENDERER=${MVP_ENABLE_SDL_RENDERER} VULKAN_RENDERER=${MVP_ENABLE_VULKAN_RENDERER} D3D11VA=${MVP_ENABLE_D3D11VA} DXVA2=${MVP_ENABLE_DXVA2} VAAPI=${MVP_ENABLE_VAAPI} VIDEOTOOLBOX=${MVP_ENABLE_VIDEOTOOLBOX}")

set(PLATFORM_EXTRA_INCLUDE_DIRS "")
set(PLATFORM_EXTRA_LIBRARIES "")

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set(SDL2_EXTERNAL_DIR "")
    set(FFMPEG_EXTERNAL_DIR "")

    set(SDL2_LIBRARIES "")
    find_package(SDL2 CONFIG QUIET)
    if(SDL2_FOUND)
        if(TARGET SDL2::SDL2)
            list(APPEND SDL2_LIBRARIES SDL2::SDL2)
            if(TARGET SDL2::SDL2main)
                list(PREPEND SDL2_LIBRARIES SDL2::SDL2main)
            endif()
        endif()
    endif()

    if(NOT SDL2_FOUND OR NOT SDL2_LIBRARIES)
        set(SDL2_PATH "${CMAKE_SOURCE_DIR}/external/SDL2")
        if(EXISTS ${SDL2_PATH})
            include_directories(${SDL2_PATH}/include)
            link_directories(${SDL2_PATH}/lib/x64)
            set(SDL2_LIBRARIES SDL2main SDL2 SDL2)
            set(SDL2_EXTERNAL_DIR ${SDL2_PATH})
            set(SDL2_FOUND TRUE)
        else()
            message(FATAL_ERROR "SDL2 not found at ${SDL2_PATH}. Please install SDL2 and set SDL2_DIR")
        endif()
    endif()

    set(FFMPEG_LIBRARIES "")
    find_package(FFMPEG CONFIG QUIET)
    if(FFMPEG_FOUND)
        if(TARGET FFMPEG::avformat)
            set(FFMPEG_LIBRARIES
                FFMPEG::avformat
                FFMPEG::avcodec
                FFMPEG::avutil
                FFMPEG::swscale
                FFMPEG::swresample
            )
        endif()
    endif()

    if(NOT FFMPEG_FOUND OR NOT FFMPEG_LIBRARIES)
        find_package(unofficial-ffmpeg CONFIG QUIET)
        if(TARGET unofficial::ffmpeg::avformat)
            set(FFMPEG_LIBRARIES
                unofficial::ffmpeg::avformat
                unofficial::ffmpeg::avcodec
                unofficial::ffmpeg::avutil
                unofficial::ffmpeg::swscale
                unofficial::ffmpeg::swresample
            )
            set(FFMPEG_FOUND TRUE)
        endif()
    endif()

    if(NOT FFMPEG_FOUND OR NOT FFMPEG_LIBRARIES)
        set(FFMPEG_PATH "${CMAKE_SOURCE_DIR}/external/ffmpeg")
        if(EXISTS ${FFMPEG_PATH})
            set(FFMPEG_INCLUDE_DIRS ${FFMPEG_PATH}/include)
            include_directories(${FFMPEG_INCLUDE_DIRS})

            find_library(AVFORMAT_LIBRARY avformat PATHS ${FFMPEG_PATH}/lib NO_DEFAULT_PATH)
            find_library(AVCODEC_LIBRARY avcodec PATHS ${FFMPEG_PATH}/lib NO_DEFAULT_PATH)
            find_library(AVUTIL_LIBRARY avutil PATHS ${FFMPEG_PATH}/lib NO_DEFAULT_PATH)
            find_library(SWSCALE_LIBRARY swscale PATHS ${FFMPEG_PATH}/lib NO_DEFAULT_PATH)
            find_library(SWRESAMPLE_LIBRARY swresample PATHS ${FFMPEG_PATH}/lib NO_DEFAULT_PATH)

            if(AVFORMAT_LIBRARY AND AVCODEC_LIBRARY AND AVUTIL_LIBRARY AND SWSCALE_LIBRARY AND SWRESAMPLE_LIBRARY)
                set(FFMPEG_LIBRARIES
                    ${AVFORMAT_LIBRARY}
                    ${AVCODEC_LIBRARY}
                    ${AVUTIL_LIBRARY}
                    ${SWSCALE_LIBRARY}
                    ${SWRESAMPLE_LIBRARY}
                )
                set(FFMPEG_FOUND TRUE)
                set(FFMPEG_EXTERNAL_DIR ${FFMPEG_PATH})
            else()
                message(FATAL_ERROR "FFmpeg libraries not found in ${FFMPEG_PATH}/lib")
            endif()
        else()
            message(FATAL_ERROR "FFmpeg directory not found: ${FFMPEG_PATH}")
        endif()
    endif()

    set(QUILL_ROOT "${CMAKE_SOURCE_DIR}/external/quill" CACHE PATH "Quill installation directory")
    find_package(quill CONFIG QUIET PATHS ${QUILL_ROOT})
    if(quill_FOUND)
        set(QUILL_FOUND TRUE)
        if(TARGET quill::quill)
            set(QUILL_LIBRARIES quill::quill)
        else()
            set(QUILL_LIBRARIES "")
        endif()
    else()
        if(EXISTS ${QUILL_ROOT})
            include_directories(${QUILL_ROOT}/include)
            set(QUILL_LIBRARIES "")
            set(QUILL_FOUND TRUE)
        else()
            set(QUILL_FOUND FALSE)
        endif()
    endif()

    if(MSVC)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE")
    endif()
    if(MVP_ENABLE_VULKAN_RENDERER)
        set(VULKAN_PACKAGE_HAS_TARGET OFF)
        set(VULKAN_PACKAGE_HAS_LIBRARIES OFF)
        set(VULKAN_PACKAGE_HAS_INCLUDES OFF)
        set(VULKAN_PACKAGE_LIBRARIES "")
        set(VULKAN_PACKAGE_INCLUDE_DIRS "")
        find_package(Vulkan QUIET)
        if(Vulkan_FOUND)
            if(TARGET Vulkan::Vulkan)
                set(VULKAN_PACKAGE_HAS_TARGET ON)
                set(VULKAN_PACKAGE_HAS_LIBRARIES ON)
                set(VULKAN_PACKAGE_HAS_INCLUDES ON)
                list(APPEND VULKAN_PACKAGE_LIBRARIES Vulkan::Vulkan)
            elseif(Vulkan_LIBRARIES)
                set(VULKAN_PACKAGE_HAS_LIBRARIES ON)
                list(APPEND VULKAN_PACKAGE_LIBRARIES ${Vulkan_LIBRARIES})
            endif()
            if(Vulkan_INCLUDE_DIRS)
                set(VULKAN_PACKAGE_HAS_INCLUDES ON)
                list(APPEND VULKAN_PACKAGE_INCLUDE_DIRS ${Vulkan_INCLUDE_DIRS})
            elseif(Vulkan_INCLUDE_DIR)
                set(VULKAN_PACKAGE_HAS_INCLUDES ON)
                list(APPEND VULKAN_PACKAGE_INCLUDE_DIRS ${Vulkan_INCLUDE_DIR})
            endif()
        endif()

        if(Vulkan_FOUND AND VULKAN_PACKAGE_HAS_LIBRARIES AND VULKAN_PACKAGE_HAS_INCLUDES)
            list(APPEND PLATFORM_EXTRA_LIBRARIES ${VULKAN_PACKAGE_LIBRARIES})
            list(APPEND PLATFORM_EXTRA_INCLUDE_DIRS ${VULKAN_PACKAGE_INCLUDE_DIRS})
            set(MVP_VULKAN_DEPENDENCY_SOURCE "find_package")
        elseif(DEFINED ENV{VULKAN_SDK} AND NOT "$ENV{VULKAN_SDK}" STREQUAL "")
            set(VULKAN_SDK_ROOT "$ENV{VULKAN_SDK}")
            set(VULKAN_SDK_INCLUDE_DIR "${VULKAN_SDK_ROOT}/Include")
            set(VULKAN_SDK_LIBRARY "${VULKAN_SDK_ROOT}/Lib/vulkan-1.lib")
            if(EXISTS "${VULKAN_SDK_INCLUDE_DIR}/vulkan/vulkan.h" AND EXISTS "${VULKAN_SDK_LIBRARY}")
                list(APPEND PLATFORM_EXTRA_INCLUDE_DIRS "${VULKAN_SDK_INCLUDE_DIR}")
                list(APPEND PLATFORM_EXTRA_LIBRARIES "${VULKAN_SDK_LIBRARY}")
                set(MVP_VULKAN_DEPENDENCY_SOURCE "vulkan_sdk_fallback")
                if(Vulkan_FOUND)
                    message(WARNING "ENABLE_VULKAN_RENDERER requested on Windows and find_package(Vulkan) returned incomplete package info; using VULKAN_SDK fallback (${VULKAN_SDK_ROOT})")
                else()
                    message(STATUS "Vulkan fallback probe: using VULKAN_SDK=${VULKAN_SDK_ROOT}")
                endif()
            else()
                message(WARNING "ENABLE_VULKAN_RENDERER requested on Windows but find_package(Vulkan) failed or was incomplete and VULKAN_SDK fallback is incomplete (expected ${VULKAN_SDK_INCLUDE_DIR}/vulkan/vulkan.h and ${VULKAN_SDK_LIBRARY}); forcing OFF")
                set(MVP_ENABLE_VULKAN_RENDERER OFF)
                set(MVP_VULKAN_DEPENDENCY_SOURCE "disabled")
            endif()
        else()
            if(Vulkan_FOUND)
                message(WARNING "ENABLE_VULKAN_RENDERER requested on Windows but find_package(Vulkan) returned incomplete package info and no VULKAN_SDK fallback is available; forcing OFF")
            else()
                message(WARNING "ENABLE_VULKAN_RENDERER requested on Windows but Vulkan SDK/runtime package is missing (find_package(Vulkan)); forcing OFF")
            endif()
            set(MVP_ENABLE_VULKAN_RENDERER OFF)
            set(MVP_VULKAN_DEPENDENCY_SOURCE "disabled")
        endif()

        if(MVP_ENABLE_VULKAN_RENDERER)
            message(STATUS "Windows Vulkan dependency source: ${MVP_VULKAN_DEPENDENCY_SOURCE}")
        endif()
    endif()
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(FFMPEG REQUIRED
        libavformat
        libavcodec
        libavutil
        libswscale
        libswresample
    )
    if(UNIX AND NOT APPLE)
        pkg_check_modules(LIBASS REQUIRED libass)
        pkg_check_modules(FONTCONFIG REQUIRED fontconfig)
        pkg_check_modules(FREETYPE2 REQUIRED freetype2)
        list(APPEND PLATFORM_EXTRA_INCLUDE_DIRS
            ${LIBASS_INCLUDE_DIRS}
            ${FONTCONFIG_INCLUDE_DIRS}
            ${FREETYPE2_INCLUDE_DIRS}
        )
        list(APPEND PLATFORM_EXTRA_LIBRARIES
            ${LIBASS_LIBRARIES}
            ${FONTCONFIG_LIBRARIES}
            ${FREETYPE2_LIBRARIES}
        )
        if(MVP_ENABLE_VULKAN_RENDERER)
            pkg_check_modules(VULKAN QUIET vulkan)
            if(VULKAN_FOUND)
                list(APPEND PLATFORM_EXTRA_INCLUDE_DIRS ${VULKAN_INCLUDE_DIRS})
                list(APPEND PLATFORM_EXTRA_LIBRARIES ${VULKAN_LIBRARIES})
            else()
                message(WARNING "ENABLE_VULKAN_RENDERER requested but Vulkan package is missing (pkg-config: vulkan); forcing OFF")
                set(MVP_ENABLE_VULKAN_RENDERER OFF)
            endif()
        endif()
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    set(FFMPEG_LIBRARIES
        ${FFMPEG_LIBAVFORMAT_LIBRARIES}
        ${FFMPEG_LIBAVCODEC_LIBRARIES}
        ${FFMPEG_LIBAVUTIL_LIBRARIES}
        ${FFMPEG_LIBSWSCALE_LIBRARIES}
        ${FFMPEG_LIBSWRESAMPLE_LIBRARIES}
    )
endif()

if(MVP_ENABLE_VULKAN_RENDERER)
    set(MVP_HAVE_VULKAN_RENDERER 1)
endif()

if(NOT MVP_ENABLE_D3D11_RENDERER AND NOT MVP_ENABLE_OPENGL_RENDERER AND NOT MVP_ENABLE_SDL_RENDERER AND NOT MVP_ENABLE_VULKAN_RENDERER)
    message(FATAL_ERROR "At least one renderer must be enabled: ENABLE_D3D11_RENDERER / ENABLE_OPENGL_RENDERER / ENABLE_SDL_RENDERER / ENABLE_VULKAN_RENDERER")
endif()

include_directories(
    ${CMAKE_BINARY_DIR}/generated
    ${CMAKE_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
    ${QUILL_INCLUDE_DIRS}
    ${PLATFORM_EXTRA_INCLUDE_DIRS}
)

set(MVP_PLAYER_SOURCES
    src/main.cpp
    src/video_player.cpp
    src/demuxer.cpp
    src/display.cpp
    src/audio_player.cpp
    src/logger.cpp
    src/core/frame.cpp
    src/core/clock.cpp
    src/core/frame_pool.cpp
    src/core/decoder_thread.cpp
    src/core/scheduler.cpp
    src/core/playback_strategy.cpp
    src/core/player_core.cpp
    src/platform/platform_capabilities.cpp
    src/platform/hw_device_factory.cpp
    src/render/renderer_factory.cpp
    src/render/output_color_profile.cpp
    src/audio/audio_equalizer.cpp
    src/audio/audio_mixer.cpp
    src/decoder/decoder_factory.cpp
    src/subtitle/subtitle_parser.cpp
    src/subtitle/srt_parser.cpp
    src/subtitle/ass_parser.cpp
    src/subtitle/libass_probe.cpp
    src/subtitle/embedded_subtitle_loader.cpp
    src/subtitle/subtitle_font_registry.cpp
    src/subtitle/subtitle_timeline.cpp
    src/playlist/playlist_manager.cpp
    src/config/settings_manager.cpp
    src/input/hotkey_manager.cpp
    src/media/format_support.cpp
    src/streaming/http_stream_downloader.cpp
    src/streaming/hls_manifest_parser.cpp
    src/streaming/dash_manifest_parser.cpp
    src/streaming/adaptive_bitrate_selector.cpp
    src/ui/skin_engine.cpp
    src/plugin/plugin_manager.cpp
    src/filters/filter_registry.cpp
    src/filters/filter_pipeline.cpp
    src/filters/video_filter_chain.cpp
    src/filters/audio_filter_chain.cpp
    src/filters/brightness_filter.cpp
    src/filters/contrast_filter.cpp
    src/filters/saturation_filter.cpp
    src/filters/volume_balance_filter.cpp
    src/filters/builtin_filters.cpp
)
if(MVP_ENABLE_SDL_RENDERER)
    list(APPEND MVP_PLAYER_SOURCES src/render/sdl_video_renderer.cpp)
endif()
if(MVP_ENABLE_D3D11_RENDERER)
    list(APPEND MVP_PLAYER_SOURCES src/render/d3d11_video_renderer.cpp)
endif()
if(MVP_ENABLE_OPENGL_RENDERER)
    list(APPEND MVP_PLAYER_SOURCES src/render/opengl_video_renderer.cpp)
endif()
if(MVP_ENABLE_VULKAN_RENDERER)
    list(APPEND MVP_PLAYER_SOURCES src/render/vulkan_video_renderer.cpp)
endif()
if(WIN32 AND MVP_WIN32_RESOURCE)
    list(APPEND MVP_PLAYER_SOURCES ${MVP_WIN32_RESOURCE})
endif()

add_executable(${PROJECT_NAME} ${MVP_PLAYER_SOURCES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /utf-8 /external:anglebrackets /external:W0)
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE
    MVP_HAVE_SOFTWARE_SDL_RENDERER=${MVP_HAVE_SOFTWARE_SDL_RENDERER}
    MVP_HAVE_OPENGL_RENDERER=${MVP_HAVE_OPENGL_RENDERER}
    MVP_HAVE_D3D11_RENDERER=${MVP_HAVE_D3D11_RENDERER}
    MVP_HAVE_VULKAN_RENDERER=${MVP_HAVE_VULKAN_RENDERER}
    MVP_VULKAN_DEPENDENCY_SOURCE="${MVP_VULKAN_DEPENDENCY_SOURCE}"
    MVP_HAVE_D3D11VA_DECODER=${MVP_HAVE_D3D11VA_DECODER}
    MVP_HAVE_DXVA2_DECODER=${MVP_HAVE_DXVA2_DECODER}
    MVP_HAVE_VAAPI_DECODER=${MVP_HAVE_VAAPI_DECODER}
    MVP_HAVE_VIDEOTOOLBOX_DECODER=${MVP_HAVE_VIDEOTOOLBOX_DECODER}
)

add_library(sample_logger_plugin SHARED
    src/plugin/sample_logger_plugin.cpp
)
target_compile_features(sample_logger_plugin PRIVATE cxx_std_17)
if(MSVC)
    target_compile_options(sample_logger_plugin PRIVATE /utf-8 /external:anglebrackets /external:W0)
endif()
target_include_directories(sample_logger_plugin PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
    ${QUILL_INCLUDE_DIRS}
)
target_compile_definitions(sample_logger_plugin PRIVATE VP_PLUGIN_BUILD)

add_custom_command(TARGET sample_logger_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:sample_logger_plugin>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/plugins/
)

if(NOT WIN32 AND MVP_ENABLE_OPENGL_RENDERER)
    find_package(OpenGL REQUIRED)
endif()

if(WIN32)
    set(MVP_WIN32_EXTRA_LIBS
        dwrite
        ole32
    )
    if(MVP_ENABLE_D3D11_RENDERER OR MVP_ENABLE_OPENGL_RENDERER)
        list(APPEND MVP_WIN32_EXTRA_LIBS
            d3d11
            d2d1
            dxgi
            d3dcompiler
        )
    endif()
    if(MVP_ENABLE_OPENGL_RENDERER)
        list(APPEND MVP_WIN32_EXTRA_LIBS opengl32)
    endif()
    target_link_libraries(${PROJECT_NAME}
        ${SDL2_LIBRARIES}
        ${FFMPEG_LIBRARIES}
        ${QUILL_LIBRARIES}
        ${PLATFORM_EXTRA_LIBRARIES}
        ${MVP_WIN32_EXTRA_LIBS}
    )

    if(SDL2_EXTERNAL_DIR)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SDL2_EXTERNAL_DIR}/lib/x64/SDL2.dll
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        )
    endif()

    if(FFMPEG_EXTERNAL_DIR)
        file(GLOB_RECURSE FFMPEG_DLLS "${FFMPEG_EXTERNAL_DIR}/bin/*.dll")
        foreach(dll ${FFMPEG_DLLS})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${dll}
                $<TARGET_FILE_DIR:${PROJECT_NAME}>
            )
        endforeach()
    endif()
else()
    set(MVP_UNIX_EXTRA_LIBS ${PLATFORM_EXTRA_LIBRARIES})
    if(MVP_ENABLE_OPENGL_RENDERER)
        list(APPEND MVP_UNIX_EXTRA_LIBS OpenGL::GL)
    endif()
    target_link_libraries(${PROJECT_NAME}
        ${SDL2_LIBRARIES}
        ${FFMPEG_LIBRARIES}
        ${QUILL_LIBRARIES}
        ${MVP_UNIX_EXTRA_LIBS}
        pthread
        m
    )
endif()

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION .
    BUNDLE DESTINATION .
)
install(TARGETS sample_logger_plugin
    RUNTIME DESTINATION plugins
    LIBRARY DESTINATION plugins
)

if(SDL2_EXTERNAL_DIR AND EXISTS "${SDL2_EXTERNAL_DIR}/lib/x64/SDL2.dll")
    install(FILES "${SDL2_EXTERNAL_DIR}/lib/x64/SDL2.dll" DESTINATION .)
endif()

if(FFMPEG_DLLS)
    install(FILES ${FFMPEG_DLLS} DESTINATION .)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/docs/reports/V1_0_0_RC1_RELEASE_NOTES.md")
    install(FILES "${CMAKE_SOURCE_DIR}/docs/reports/V1_0_0_RC1_RELEASE_NOTES.md"
            DESTINATION .
            RENAME RELEASE_NOTES.md)
endif()

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "${MVP_PACKAGE_VENDOR}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Modern Video Player")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}-${MVP_VERSION_SEMVER}")
set(CPACK_PACKAGE_FILE_NAME
    "${PROJECT_NAME}-${MVP_VERSION_SEMVER}-${MVP_PACKAGE_PLATFORM}-${MVP_PACKAGE_ARCH}"
)
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
elseif(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB;TGZ")
    set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${MVP_PACKAGE_VENDOR}")
    set(CPACK_DEBIAN_PACKAGE_SECTION "video")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "libsdl2-2.0-0, libavformat60, libavcodec60, libavutil58, libswscale7, libswresample4, libass9, libfontconfig1, libfreetype6"
    )
endif()
set(CPACK_VERBATIM_VARIABLES YES)

if(QUILL_FOUND)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_QUILL_LOGGING)
    target_compile_definitions(${PROJECT_NAME} PRIVATE QUILL_DISABLE_ASSERTIONS)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/logs")
    message(STATUS "Quill: enabled")
else()
    message(STATUS "Quill: disabled (using std::cout)")
endif()

if(DEBUG_MODE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG_MODE)
    message(STATUS "Debug mode: ON")
else()
    message(STATUS "Debug mode: OFF")
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Version: ${MVP_VERSION_SEMVER}")
message(STATUS "SDL2 found: ${SDL2_FOUND}")
message(STATUS "FFmpeg found: ${FFMPEG_FOUND}")

include(CPack)





