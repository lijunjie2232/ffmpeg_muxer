//
// Created by li on 2023/02/11.
//

#ifndef FFMPEG_PROJECT_MEDIAFILE_H
#define FFMPEG_PROJECT_MEDIAFILE_H
extern "C" {
#include "libavformat/avformat.h"
}

class MediaFile {
    AVFormatContext *m_pAvFormatCtx; // 流文件解析上下文
    AVCodecContext *m_pVidDecodeCtx; // 视频解码器上下文
    uint32_t m_nVidStreamIndex;      // 视频流索引值
    AVCodecContext *m_pAudDecodeCtx; // 音频解码器上下文
    uint32_t m_nAudStreamIndex;      // 音频流索引值
public:
    MediaFile(const char *pszFilePath);

    ~MediaFile();

    int32_t Open(const char *pszFilePath);

    void Close();

    int32_t ReadFrame();

    int32_t DecodePktToFrame(
            AVCodecContext *pDecoderCtx,
            AVPacket *pInPacket,
            AVFrame **ppOutFrame);

    void Info();

};


#endif //FFMPEG_PROJECT_MEDIAFILE_H
