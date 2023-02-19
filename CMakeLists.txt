cmake_minimum_required(VERSION 3.23)
project(../ffmpeg_project)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILE
        main.cpp
        DirUtil.h
        DirUtil.cpp
        )

set(ZLIB_INCLUDE /usr/include/ffmpeg4.4)
include_directories(
        ${ZLIB_INCLUDE}
)


set(ZLIB_LIBRARIES /usr/lib/ffmpeg4.4)
link_directories(
        ${ZLIB_LIBRARIES}
)


add_executable(ffmpeg_project main.cpp DirUtil.h DirUtil.cpp AVMuxer.h MediaFile.h MediaFile.cpp Dialog.h AVMuxer.cpp)

target_link_libraries(ffmpeg_project
        swscale swresample avcodec avutil avdevice avfilter avformat
        )
