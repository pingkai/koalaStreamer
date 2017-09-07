cmake_minimum_required(VERSION 3.5)
project(streamer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(SOURCE_FILES MacRTMPStreamer.cpp
        MediaStreamer.c
        MediaMuxer/MediaMuxer.c
        MediaMuxer/KoalaMuxer.c
        utils/logger.c
        KoalaPlayer/src/koala_muxer.c
        KoalaPlayer/src/koala_common.c
        streamStatistician.c
        utils/mediaFrame.c utils/mediaFrame.h utils/mediaThread.cpp utils/mediaThread.h utils/StreamerStack.h utils/autoLock.h)
include_directories(KoalaPlayer/src /usr/local/include/)
link_directories(/usr/local/lib)
#link_directories(extern/ffmpeg/libavcodec extern/ffmpeg/libavformat extern/ffmpeg/libavutil extern/ffmpeg/libavfilter extern/ffmpeg/libswresample)
add_executable(MacRTMPStreamer ${SOURCE_FILES})
target_link_libraries(MacRTMPStreamer avcodec avformat swresample avutil avdevice)
