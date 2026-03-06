cmake_minimum_required(VERSION 3.15)
set(CMAKE_SYSTEM_VERSION "10.0.26100.0")
project(modern-video-player VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(DEBUG_MODE "Enable debug logging" ON)
option(BUILD_CORE_TESTS "Build core API unit tests" OFF)

if(WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")

    find_package(SDL2 CONFIG QUIET)
    if(NOT SDL2_FOUND)
        set(SDL2_PATH "${CMAKE_SOURCE_DIR}/external/SDL2")
        if(EXISTS ${SDL2_PATH})
            include_directories(${SDL2_PATH}/include)
            link_directories(${SDL2_PATH}/lib/x64)
            set(SDL2_DIR ${SDL2_PATH} CACHE PATH "SDL2 directory" FORCE)
        else()
            message(FATAL_ERROR "SDL2 not found at ${SDL2_PATH}. Please install SDL2 and set SDL2_DIR")
        endif()
    endif()

    find_package(FFMPEG CONFIG QUIET)
    if(NOT FFMPEG_FOUND)
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
                set(FFMPEG_DIR ${FFMPEG_PATH} CACHE PATH "FFmpeg directory" FORCE)
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

    set(SDL2_LIBRARIES SDL2main SDL2 SDL2)
    if(MSVC)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE")
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

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    set(FFMPEG_LIBRARIES
        ${FFMPEG_LIBAVFORMAT_LIBRARIES}
        ${FFMPEG_LIBAVCODEC_LIBRARIES}
        ${FFMPEG_LIBAVUTIL_LIBRARIES}
        ${FFMPEG_LIBSWSCALE_LIBRARIES}
        ${FFMPEG_LIBSWRESAMPLE_LIBRARIES}
    )
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
    ${QUILL_INCLUDE_DIRS}
)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/video_player.cpp
    src/demuxer.cpp
    src/display.cpp
    src/audio_player.cpp
    src/logger.cpp
    src/core/frame.cpp
    src/core/clock.cpp
    src/core/scheduler.cpp
    src/core/player_core.cpp
    src/filters/filter_registry.cpp
    src/filters/filter_pipeline.cpp
    src/filters/brightness_filter.cpp
    src/filters/contrast_filter.cpp
    src/filters/saturation_filter.cpp
    src/filters/builtin_filters.cpp
)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

add_executable(core_frame_queue_tests EXCLUDE_FROM_ALL
    tests/core_frame_queue_tests.cpp
)
target_include_directories(core_frame_queue_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${FFMPEG_INCLUDE_DIRS}
)
target_compile_features(core_frame_queue_tests PRIVATE cxx_std_17)
if(NOT WIN32)
    target_link_libraries(core_frame_queue_tests pthread)
endif()

add_executable(core_clock_tests EXCLUDE_FROM_ALL
    tests/core_clock_tests.cpp
    src/core/clock.cpp
)
target_include_directories(core_clock_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${FFMPEG_INCLUDE_DIRS}
)
target_compile_features(core_clock_tests PRIVATE cxx_std_17)
if(NOT WIN32)
    target_link_libraries(core_clock_tests pthread)
endif()

if(BUILD_CORE_TESTS)
    add_custom_target(core_tests ALL
        DEPENDS core_frame_queue_tests core_clock_tests
    )
endif()

if(WIN32)
    target_link_libraries(${PROJECT_NAME}
        ${SDL2_LIBRARIES}
        ${FFMPEG_LIBRARIES}
        ${QUILL_LIBRARIES}
    )

    if(SDL2_DIR)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SDL2_DIR}/lib/x64/SDL2.dll
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        )
    endif()

    if(FFMPEG_DIR)
        file(GLOB_RECURSE FFMPEG_DLLS "${FFMPEG_DIR}/bin/*.dll")
        foreach(dll ${FFMPEG_DLLS})
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${dll}
                $<TARGET_FILE_DIR:${PROJECT_NAME}>
            )
        endforeach()
    endif()
else()
    target_link_libraries(${PROJECT_NAME}
        ${SDL2_LIBRARIES}
        ${FFMPEG_LIBRARIES}
        ${QUILL_LIBRARIES}
        pthread
        m
    )
endif()

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
message(STATUS "SDL2 found: ${SDL2_FOUND}")
message(STATUS "FFmpeg found: ${FFMPEG_FOUND}")

