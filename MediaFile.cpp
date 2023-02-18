//
// Created by li on 2023/02/11.
//
#include "MediaFile.h"
#include "Dialog.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
}

MediaFile::MediaFile(const char *pszFilePath) {
    this->m_pAvFormatCtx = nullptr;
    this->m_pVidDecodeCtx = nullptr;
    this->m_pAudDecodeCtx = nullptr;
    this->Open(pszFilePath);
}

MediaFile::~MediaFile() {
    this->Close();
//    LOGD("~MediaFile");
}

void MediaFile::Info(){
    if (this->m_pVidDecodeCtx != nullptr) {
        LOGI("[Video stream info] width: %d, height: %d, pix_fmt:%d", this->m_pVidDecodeCtx->width, this->m_pVidDecodeCtx->height, this->m_pVidDecodeCtx->pix_fmt);
    }else{
        LOGI("no video stream in this media");
    }
    if (this->m_pAudDecodeCtx != nullptr) {
        LOGI("[Audio stream info] sample_rate: %d, channels: %d, sample_fmt: %d", this->m_pAudDecodeCtx->sample_rate, this->m_pAudDecodeCtx->channels, this->m_pAudDecodeCtx->sample_rate);
    }else{
        LOGI("no audio stream in this media");
    }
}

//
// 打开媒体文件,解析码流信息,并且创建和打开对应的解码器
//
int32_t MediaFile::Open(const char *pszFilePath) {
    AVCodec *pVidDecoder = nullptr;
    AVCodec *pAudDecoder = nullptr;
    int res = 0;

    LOGI("Media files: %s", pszFilePath);

    // 打开媒体文件
    res = avformat_open_input(&this->m_pAvFormatCtx, pszFilePath, nullptr, nullptr);
    if (this->m_pAvFormatCtx == nullptr) {
        LOGE("Avformat fail to open files, avformat_open_input() exit with res %d", res);
        return res;
    }

    // 查找所有媒体流信息
    res = avformat_find_stream_info(this->m_pAvFormatCtx, nullptr);
    if (res == AVERROR_EOF) {
        LOGD("reached to files end");
        this->Close();
        return -1;
    }

    LOGD("number of streams: %d", this->m_pAvFormatCtx->nb_streams);
    // 遍历所有的媒体流信息
    for (unsigned int i = 0; i < this->m_pAvFormatCtx->nb_streams; i++) {
        AVStream *pAvStream = this->m_pAvFormatCtx->streams[i];
        if (pAvStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            LOGD("find a video stream");
            if ((pAvStream->codecpar->width <= 0) || (pAvStream->codecpar->height <= 0)) {
                LOGE("invalid resolution, streamIndex=%d", i);
                continue;
            }

            pVidDecoder = avcodec_find_decoder(pAvStream->codecpar->codec_id);  // 找到视频解码器
            if (pVidDecoder == nullptr) {
                LOGE("can not find video codec");
                continue;
            }

            this->m_nVidStreamIndex = (uint32_t) i;
            LOGD("pxlFmt=%d, frameSize=%d*%d",
                 (int) pAvStream->codecpar->format,
                 pAvStream->codecpar->width,
                 pAvStream->codecpar->height);

        } else if (pAvStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            LOGD("find an audio stream");
            if ((pAvStream->codecpar->channels <= 0) || (pAvStream->codecpar->sample_rate <= 0)) {
                LOGE("invalid resolution, streamIndex=%d", i);
                continue;
            }

            pAudDecoder = avcodec_find_decoder(pAvStream->codecpar->codec_id);  // 找到音频解码器
            if (pAudDecoder == nullptr) {
                LOGE("can not find Audio codec");
                continue;
            }

            this->m_nAudStreamIndex = (uint32_t) i;
            LOGD("sample_fmt=%d, sampleRate=%d, channels=%d, chnl_layout=%lu",
                 (int) pAvStream->codecpar->format,
                 pAvStream->codecpar->sample_rate,
                 pAvStream->codecpar->channels,
                 pAvStream->codecpar->channel_layout);
        }
    }

    // 创建视频解码器并且打开
    if (pVidDecoder != nullptr) {
        this->m_pVidDecodeCtx = avcodec_alloc_context3(pVidDecoder);
        if (this->m_pVidDecodeCtx == nullptr) {
            LOGE("fail to video avcodec_alloc_context3()");
            this->Close();
            return -1;
        }
        res = avcodec_parameters_to_context(this->m_pVidDecodeCtx,
                                            this->m_pAvFormatCtx->streams[this->m_nVidStreamIndex]->codecpar);

        res = avcodec_open2(this->m_pVidDecodeCtx, nullptr, nullptr);
        if (res != 0) {
            LOGE("fail to video avcodec_open2(), res=%d", res);
            this->Close();
            return -1;
        }
    } else {
        LOGI("no video stream in this media");
    }

    // 创建音频解码器并且打开
    if (pAudDecoder != nullptr) {
        this->m_pAudDecodeCtx = avcodec_alloc_context3(pAudDecoder);
        if (this->m_pAudDecodeCtx == nullptr) {
            LOGE("fail to audio avcodec_alloc_context3()");
            this->Close();
            return -1;
        }
        res = avcodec_parameters_to_context(this->m_pAudDecodeCtx,
                                            this->m_pAvFormatCtx->streams[this->m_nAudStreamIndex]->codecpar);

        res = avcodec_open2(this->m_pAudDecodeCtx, nullptr, nullptr);
        if (res != 0) {
            LOGE("fail to audio avcodec_open2(), res=%d", res);
            this->Close();
            return -1;
        }
    } else {
        LOGI("no audio stream in this media")
    }
    return 0;
}

//
// 关闭媒体文件，关闭对应的解码器
//
void MediaFile::Close() {
    // 关闭媒体文件解析
    if (this->m_pAvFormatCtx != nullptr) {
        avformat_close_input(&this->m_pAvFormatCtx);
        this->m_pAvFormatCtx = nullptr;
    }

    // 关闭视频解码器
    if (this->m_pVidDecodeCtx != nullptr) {
        avcodec_close(this->m_pVidDecodeCtx);
        avcodec_free_context(&this->m_pVidDecodeCtx);
        this->m_pVidDecodeCtx = nullptr;
    }

    // 关闭音频解码器
    if (this->m_pAudDecodeCtx != nullptr) {
        avcodec_close(this->m_pAudDecodeCtx);
        avcodec_free_context(&this->m_pAudDecodeCtx);
        this->m_pAudDecodeCtx = nullptr;
    }
}

//
// 循环不断的读取音视频数据包进行解码处理
//
int32_t MediaFile::ReadFrame() {
    int res = 0;

    while (true) {
        AVPacket *pPacket = av_packet_alloc();

        // 依次读取数据包
        res = av_read_frame(this->m_pAvFormatCtx, pPacket);
        if (res == AVERROR_EOF)  // 正常读取到文件尾部退出
        {
            LOGE("reached media files end");
            break;
        } else if (res < 0) // 其他小于0的返回值是数据包读取错误
        {
            LOGE("fail av_read_frame(), res=%d", res);
            break;
        }

        if (pPacket->stream_index == this->m_nVidStreamIndex)       // 读取到视频包
        {
            // 这里进行视频包解码操作,详细下一章节讲解
            AVFrame *pVideoFrame = nullptr;
            res = DecodePktToFrame(this->m_pVidDecodeCtx, pPacket, &pVideoFrame);
            if (res == 0 && pVideoFrame != nullptr) {
//                Enqueue(pVideoFrame);  // 解码成功后的视频帧插入队列
            }
        } else if (pPacket->stream_index == this->m_nAudStreamIndex)  // 读取到音频包
        {
            // 这里进行音频包解码操作,详细下一章节讲解
            AVFrame *pAudioFrame = nullptr;
            res = DecodePktToFrame(this->m_pAudDecodeCtx, pPacket, &pAudioFrame);
            if (res == 0 && pAudioFrame != nullptr) {
//                Enqueue(pAudioFrame);  // 解码成功后的音频帧插入队列
            }
        }

        av_packet_free(&pPacket);  // 数据包用完了可以释放了
    }
    return 0;

}

//
// 功能: 送入一个数据包进行解码,获取解码后的音视频帧
//       注意: 即使正常解码情况下,不是每次都有解码后的帧输出的
//
// 参数: pDecoderCtx ---- 解码器上下文信息
//       pInPacket   ---- 输入的数据包, 可以为nullptr来刷新解码缓冲区
//       ppOutFrame  ---- 输出解码后的音视频帧, 即使返回0也可能无输出
//
// 返回: 0:           解码正常;
//       AVERROR_EOF: 解码全部完成;
//       other:       解码出错
//
int32_t MediaFile::DecodePktToFrame(
        AVCodecContext *pDecoderCtx,        // 解码器上下文信息
        AVPacket *pInPacket,          // 输入的数据包
        AVFrame **ppOutFrame)     // 解码后生成的视频帧或者音频帧
{
    AVFrame *pOutFrame = nullptr;
    int res = 0;

    // 送入要解码的数据包
    res = avcodec_send_packet(pDecoderCtx, pInPacket);
    if (res == AVERROR(EAGAIN)) // 没有数据送入,但是可以继续可以从内部缓冲区读取编码后的视频包
    {
        //    LOGD("<DecodePktToFrame> avcodec_send_frame() EAGAIN");
    } else if (res == AVERROR_EOF) // 数据包送入结束不再送入,但是可以继续可以从内部缓冲区读取编码后的视频包
    {
        //    LOGD("<DecodePktToFrame> avcodec_send_frame() AVERROR_EOF");
    } else if (res < 0)  // 送入输入数据包失败
    {
        LOGE("fail to avcodec_send_frame(), res=%d", res);
        return res;
    }


    // 获取解码后的视频帧或者音频帧
    pOutFrame = av_frame_alloc();
    res = avcodec_receive_frame(pDecoderCtx, pOutFrame);
    if (res == AVERROR(EAGAIN)) // 当前这次没有解码后的音视频帧输出,但是后续可以继续读取
    {
        LOGD("<CAvVideoDecComp::DecodeVideoPkt> no data output");
        av_frame_free(&pOutFrame);
        (*ppOutFrame) = nullptr;
        return 0;
    } else if (res == AVERROR_EOF) // 解码缓冲区已经刷新完成,后续不再有数据输出
    {
        LOGD("avcodec_receive_packet() EOF");
        av_frame_free(&pOutFrame);
        (*ppOutFrame) = nullptr;
        return AVERROR_EOF;
    } else if (res < 0) {
        LOGE("fail to avcodec_receive_packet(), res=%d", res);
        av_frame_free(&pOutFrame);
        (*ppOutFrame) = nullptr;
        return res;
    }


    (*ppOutFrame) = pOutFrame;
    return 0;
}