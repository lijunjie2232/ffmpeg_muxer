//
// Created by li on 2023/02/10.
//
/*
对于编码后的音视频包写入mp4文件的处理，混流所有的API函数都属于libavformat 库。音视频混流操作的流程比较简单：

1、创建一个新的媒体格式上下文 avformat_alloc_output_context2()

2、根据音视频编码器信息，分别创建音频流 和 视频流 avformat_new_stream() 和 avcodec_parameters_from_context()

3、打开文件IO操作 avio_open()

4、写入文件头信息 avformat_write_header()

5、循环交错调用 av_interleaved_write_frame() 写入音视频帧数据。音视频数据包写入都是通过这个函数，需要注意的是AVPacket中 stream_index 流索引值要设置对。在具体项目中通常都是交错调用这个函数分别写入的（并不需要1对1的交错，通常是写入一个视频包，写入几个音频包的交错）

6、写入文件尾信息 av_write_trailer()

7、关闭文件IO操作 avio_closep()、释放媒体格式上下文 avformat_free_context()
 */

#ifndef FFMPEG_PROJECT_AVMUXER_H
#define FFMPEG_PROJECT_AVMUXER_H

#include "DirUtil.h"
extern "C" {
#include "libavformat/avformat.h"
};

class AVMuxer {
public:

    AVMuxer(DirUtil *dir, char *outputpath = "output");

    void setDir(DirUtil *dir);

    void MuxAV(const char *OutPath = nullptr);

private:
    // input media info
    AVFormatContext *m_pAvFormatCtx; // 流文件解析上下文
    AVFormatContext *m_pVFormatCtx; // 视频流文件解析上下文
    AVFormatContext *m_pAFormatCtx; // 音频流文件解析上下文
    AVCodecContext *m_pVidDecodeCtx; // 视频解码器上下文
    AVCodecContext *m_pAudDecodeCtx; // 音频解码器上下文
    uint32_t m_nVidStreamIndex;      // 视频流索引值
    uint32_t m_nAudStreamIndex;      // 音频流索引值



    AVFormatContext *m_pFormatCtx = nullptr;      // 媒体格式上下文
//    bool              m_bGlobalHeader = true;         // 音视频编解码器是否需要标记 AV_CODEC_FLAG_GLOBAL_HEADER
//    AVStream *m_pVideoStream = nullptr;      // 视频流信息
//    AVStream *m_pAudioStream = nullptr;      // 音频流信息

//    AVCodecContext *m_pVidOutCtx; // 视频解码器上下文
//    uint32_t m_nVidOutStreamIndex;      // 视频流索引值
//    AVCodecContext *m_pAudOutCtx; // 音频解码器上下文
//    uint32_t m_nAudOutStreamIndex;      // 音频流索引值


    DirUtil *dir = nullptr;

    char *OutputDir = nullptr;

    int32_t Read(const char *pszFilePath);

    void Close();

    int32_t MuxerDo(const char *pszFilePath);

    void MuxerClose();

//    int32_t MuxerWrite(bool bVideoPkt, AVPacket *pInPacket);
};

#endif //FFMPEG_PROJECT_AVMUXER_H
