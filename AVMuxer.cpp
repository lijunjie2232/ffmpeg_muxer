//
// Created by li on 2023/02/11.
//
#include "AVMuxer.h"
#include "Dialog.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libavutil/timestamp.h""
#include "libavdevice/avdevice.h"
}


/*
FIX: H.264 in some container format (FLV, MP4, MKV etc.) need
"h264_mp4toannexb" bitstream filter (BSF)
  *Add SPS,PPS in front of IDR frame
  *Add start code ("0,0,0,1") in front of NALU
H.264 in some container (MPEG2TS) don't need this BSF.
*/
//'1': Use H.264 Bitstream Filter
#define USE_H264BSF 0

/*
FIX:AAC in some container format (FLV, MP4, MKV etc.) need
"aac_adtstoasc" bitstream filter (BSF)
*/
//'1': Use AAC Bitstream Filter
#define USE_AACBSF 0

AVMuxer::AVMuxer(DirUtil *dir, char *outputpath) {
    this->dir = dir;
    this->OutputDir = outputpath;
}

void AVMuxer::setDir(DirUtil *dir) { this->dir = dir; }

void AVMuxer::MuxAV(const char *OutPath) {
    if (this->dir->getFiles().size() == 0) {
        LOGI("No file to be processed in path: %s ...", this->dir->getPath());
        return;
    }
    const char *aspec = this->dir->getAudiospec();
    const char *vspec = this->dir->getVideospec();

    std::string opath;

    if (OutPath)
        opath = OutPath[0] == '/'
                ? OutPath
                : std::string(this->dir->getPath()) + "/" + OutPath;
    else
        opath = this->OutputDir[0] == '/'
                ? this->OutputDir
                : std::string(this->dir->getPath()) + "/" + this->OutputDir;

    int res = this->dir->OutEnvCheck(opath.c_str());
    if (res < 0) {
        LOGE("Error occur, OutEnvCheck exited with code %d", res);
        return;
    };

    char *vpath = new char[256];
    char *apath = new char[256];

    for (auto file: this->dir->getFiles()) {
        char *outputfile = nullptr;
        sprintf(vpath, "%s%c%s%s", this->dir->getPath(), '/', file.c_str(), vspec);
        sprintf(apath, "%s%c%s%s", this->dir->getPath(), '/', file.c_str(), aspec);
        //        this->Read((file + aspec).c_str());
        //        this->Read((file + vspec).c_str());
        outputfile = strdup((opath + "/" + file + this->dir->getVideospec()).c_str());
        LOGI("processing %s", file.c_str());
        this->Muxer(vpath, apath, outputfile);
        LOGI("%s has been been processed and output to %s", file.c_str(), outputfile);
        this->MuxerClose();
    }
    LOGI("%d media(s) has(have) been processed", this->dir->getFiles().size());
    LOGI("output dir: %s",
         (std::string(this->dir->getPath()) + "/" + opath).c_str());
    delete[] vpath;
    delete[] apath;
}


int32_t AVMuxer::Muxer(const char *in_vpath, const char *in_apath, const char *out_path) {

    AVFormatContext *ifmtCtxVideo = NULL, *ifmtCtxAudio = NULL, *ofmtCtx = NULL;
    AVPacket packet;

    int inVideoIndex = -1, inAudioIndex = -1;
    int outVideoIndex = -1, outAudioIndex = -1;
    int frameIndex = 0;

    int64_t curPstVideo = 0, curPstAudio = 0;

    int ret = 0;
    unsigned int i = 0;

    //注册设备
    avdevice_register_all();

    //打开输入视频文件
    ret = avformat_open_input(&ifmtCtxVideo, in_vpath, nullptr, nullptr);
    if (ret < 0) {
        printf("can't open input video file\n");
        return -1;
    }

    //查找输入流
    ret = avformat_find_stream_info(ifmtCtxVideo, nullptr);
    if (ret < 0) {
        printf("failed to retrieve input video stream information\n");
        return -1;
    }

    //打开输入音频文件
    ret = avformat_open_input(&ifmtCtxAudio, in_apath, 0, 0);
    if (ret < 0) {
        printf("can't open input audio file\n");
        return -1;
    }

    //查找输入流
    ret = avformat_find_stream_info(ifmtCtxAudio, 0);
    if (ret < 0) {
        printf("failed to retrieve input audio stream information\n");
        return -1;
    }

//    printf("===========Input Information==========\n");
//    av_dump_format(ifmtCtxVideo, 0, in_vpath, 0);
//    av_dump_format(ifmtCtxAudio, 0, in_apath, 0);
//    printf("======================================\n");

    //新建输出上下文
    avformat_alloc_output_context2(&ofmtCtx, NULL, NULL, out_path);
    if (!ofmtCtx) {
        printf("can't create output context\n");
        return -1;
    }

    //视频输入流
    for (i = 0; i < ifmtCtxVideo->nb_streams; ++i) {
        if (ifmtCtxVideo->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            AVStream *inStream = ifmtCtxVideo->streams[i];
            AVStream *outStream = avformat_new_stream(ofmtCtx, NULL);
            inVideoIndex = i;

            if (!outStream) {
                printf("failed to allocate output stream\n");
                return -1;
            }

            outVideoIndex = outStream->index;

            if (avcodec_parameters_copy(outStream->codecpar, inStream->codecpar) < 0) {
                printf("faild to copy context from input to output stream");
                return -1;
            }

            outStream->codecpar->codec_tag = 0;

            break;
        }
    }

    //音频输入流
    for (i = 0; i < ifmtCtxAudio->nb_streams; ++i) {
        if (ifmtCtxAudio->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            AVStream *inStream = ifmtCtxAudio->streams[i];
            AVStream *outStream = avformat_new_stream(ofmtCtx, NULL);
            inAudioIndex = i;

            if (!outStream) {
                printf("failed to allocate output stream\n");
                return -1;
            }

            outAudioIndex = outStream->index;

            if (avcodec_parameters_copy(outStream->codecpar, inStream->codecpar) < 0) {
                printf("faild to copy context from input to output stream");
                return -1;
            }

            outStream->codecpar->codec_tag = 0;

            break;
        }
    }

    LOGD("===============================Input Information===============================");
    LOGD("pxlFmt=%d, frameSize=%d*%d",
         ifmtCtxVideo->streams[inVideoIndex]->codecpar->format,
         ifmtCtxVideo->streams[inVideoIndex]->codecpar->width,
         ifmtCtxVideo->streams[inVideoIndex]->codecpar->height);
    LOGD("-------------------------------------------------------------------------------");
    LOGD("sample_fmt=%d, sampleRate=%d, channels=%d, chnl_layout=%lu, duration=%ld",
         ifmtCtxAudio->streams[inAudioIndex]->codecpar->format,
         ifmtCtxAudio->streams[inAudioIndex]->codecpar->sample_rate,
         ifmtCtxAudio->streams[inAudioIndex]->codecpar->channels,
         ifmtCtxAudio->streams[inAudioIndex]->codecpar->channel_layout,
         ifmtCtxAudio->streams[inAudioIndex]->duration);
    LOGD("===============================================================================");

//    printf("==========Output Information==========\n");
//    av_dump_format(ofmtCtx, 0, out_path, 1);
//    printf("======================================\n");

    //打开输入文件
    if (!(ofmtCtx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&ofmtCtx->pb, out_path, AVIO_FLAG_WRITE) < 0) {
            printf("can't open out file\n");
            return -1;
        }
    }

    //写文件头
    if (avformat_write_header(ofmtCtx, NULL) < 0) {
        printf("Error occurred when opening output file\n");
        return -1;
    }


#if USE_H264BSF
    AVBitStreamFilterContext* h264bsfc =  av_bitstream_filter_init("h264_mp4toannexb");
#endif
#if USE_AACBSF
    AVBitStreamFilterContext* aacbsfc =  av_bitstream_filter_init("aac_adtstoasc");
#endif
    while (1) {
        AVFormatContext *ifmtCtx = NULL;
        AVStream *inStream, *outStream;
        int streamIndex = 0;

        if (av_compare_ts(curPstVideo, ifmtCtxVideo->streams[inVideoIndex]->time_base, curPstAudio,
                          ifmtCtxAudio->streams[inAudioIndex]->time_base) < 0) {
            ifmtCtx = ifmtCtxVideo;
            streamIndex = outVideoIndex;

            if (av_read_frame(ifmtCtx, &packet) >= 0) {
                do {
                    inStream = ifmtCtx->streams[packet.stream_index];
                    outStream = ofmtCtx->streams[streamIndex];

                    if (packet.stream_index == inVideoIndex) {
                        //Fix: No PTS(Example: Raw H.264
                        //Simple Write PTS
                        if (packet.pts == AV_NOPTS_VALUE) {
                            //write PTS
                            AVRational timeBase1 = inStream->time_base;
                            //Duration between 2 frames
                            int64_t calcDuration = (double) AV_TIME_BASE / av_q2d(inStream->r_frame_rate);
                            //Parameters
                            packet.pts =
                                    (double) (frameIndex * calcDuration) / (double) (av_q2d(timeBase1) * AV_TIME_BASE);
                            packet.dts = packet.pts;
                            packet.duration = (double) calcDuration / (double) (av_q2d(timeBase1) * AV_TIME_BASE);
                            frameIndex++;
                        }

                        curPstVideo = packet.pts;
                        break;
                    }
                } while (av_read_frame(ifmtCtx, &packet) >= 0);
            } else {
                break;
            }
        } else {
            ifmtCtx = ifmtCtxAudio;
            streamIndex = outAudioIndex;

            if (av_read_frame(ifmtCtx, &packet) >= 0) {
                do {
                    inStream = ifmtCtx->streams[packet.stream_index];
                    outStream = ofmtCtx->streams[streamIndex];

                    if (packet.stream_index == inAudioIndex) {
                        //Fix: No PTS(Example: Raw H.264
                        //Simple Write PTS
                        if (packet.pts == AV_NOPTS_VALUE) {
                            //write PTS
                            AVRational timeBase1 = inStream->time_base;
                            //Duration between 2 frames
                            int64_t calcDuration = (double) AV_TIME_BASE / av_q2d(inStream->r_frame_rate);
                            //Parameters
                            packet.pts =
                                    (double) (frameIndex * calcDuration) / (double) (av_q2d(timeBase1) * AV_TIME_BASE);
                            packet.dts = packet.pts;
                            packet.duration = (double) calcDuration / (double) (av_q2d(timeBase1) * AV_TIME_BASE);
                            frameIndex++;
                        }

                        curPstAudio = packet.pts;
                        break;
                    }
                } while (av_read_frame(ifmtCtx, &packet) >= 0);
            } else {
                break;
            }
        }

        //FIX:Bitstream Filter
#if USE_H264BSF
        av_bitstream_filter_filter(h264bsfc, inStream->codec, NULL, &packet.data, &packet.size, packet.data, packet.size, 0);
#endif
#if USE_AACBSF
        av_bitstream_filter_filter(aacbsfc, outStream->codec, NULL, &packet.data, &packet.size, packet.data, packet.size, 0);
#endif

        //Convert PTS/DTS
        packet.pts = av_rescale_q_rnd(packet.pts, inStream->time_base, outStream->time_base,
                                      (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        packet.dts = av_rescale_q_rnd(packet.dts, inStream->time_base, outStream->time_base,
                                      (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        packet.duration = av_rescale_q(packet.duration, inStream->time_base, outStream->time_base);
        packet.pos = -1;
        packet.stream_index = streamIndex;

        //write
        if (av_interleaved_write_frame(ofmtCtx, &packet) < 0) {
            printf("error muxing packet");
            break;
        }

        av_packet_unref(&packet);
    }

    //Write file trailer
    av_write_trailer(ofmtCtx);

#if USE_H264BSF
    av_bitstream_filter_close(h264bsfc);
#endif
#if USE_AACBSF
    av_bitstream_filter_close(aacbsfc);
#endif

    LOGD("===============================Input Information===============================");
    LOGD("pxlFmt=%d, frameSize=%d*%d",
         ofmtCtx->streams[outVideoIndex]->codecpar->format,
         ofmtCtx->streams[outVideoIndex]->codecpar->width,
         ofmtCtx->streams[outVideoIndex]->codecpar->height);
    LOGD("-------------------------------------------------------------------------------");
    LOGD("sample_fmt=%d, sampleRate=%d, channels=%d, chnl_layout=%lu, duration=%ld",
         ofmtCtx->streams[outAudioIndex]->codecpar->format,
         ofmtCtx->streams[outAudioIndex]->codecpar->sample_rate,
         ofmtCtx->streams[outAudioIndex]->codecpar->channels,
         ofmtCtx->streams[outAudioIndex]->codecpar->channel_layout,
         ofmtCtx->streams[outAudioIndex]->duration);
    LOGD("===============================================================================");

    avformat_close_input(&ifmtCtxVideo);
    avformat_close_input(&ifmtCtxAudio);
    if (ofmtCtx && !(ofmtCtx->oformat->flags & AVFMT_NOFILE))
        avio_close(ofmtCtx->pb);

    avformat_free_context(ofmtCtx);

    return 0;
}

void AVMuxer::MuxerClose() {

}
