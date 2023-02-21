//
// Created by li on 2023/02/11.
//
#include "AVMuxer.h"
#include "Dialog.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include <libavutil/timestamp.h>
}

/*
FIX: H.264 in some container format (FLV, MP4, MKV etc.) need
"h264_mp4toannexb" bitstream filter (BSF)
  *Add SPS,PPS in front of IDR frame
  *Add start code ("0,0,0,1") in front of NALU
H.264 in some container (MPEG2TS) don't need this BSF.
*/
//'1': Use H.264 Bitstream Filter
#define USE_H264BSF (0)

/*
FIX:AAC in some container format (FLV, MP4, MKV etc.) need
"aac_adtstoasc" bitstream filter (BSF)
*/
//'1': Use AAC Bitstream Filter
#define USE_AACBSF (0)

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
        this->Read(vpath); // this->dir->getPath() + '/' + file + aspec
        this->Read(apath);
        if (this->m_pVFormatCtx == nullptr | this->m_pAFormatCtx == nullptr)
            continue;
        else {
            outputfile = strdup((opath + "/" + file + this->dir->getVideospec()).c_str());
            LOGI("processing %s", file.c_str());
            this->MuxerDo(outputfile);
            this->MuxerClose();
            LOGI("%s has been been processed and output to %s", file.c_str(),
                 outputfile);
        }
        this->Close();
    }
    LOGI("%d media(s) has(have) been processed", this->dir->getFiles().size());
    LOGI("output dir: %s",
         (std::string(this->dir->getPath()) + "/" + opath).c_str());
    delete[] vpath;
    delete[] apath;
}

int32_t AVMuxer::Read(const char *pszFilePath) {
    AVFormatContext *m_pAvFormatCtx = nullptr;
//    AVCodec *pDecoder = nullptr;
    AVFormatContext *m_pAFormatCtx0 = nullptr;
    int res = 0;

    LOGI("Media files: %s", pszFilePath);

    // 打开媒体文件
    res = avformat_open_input(&m_pAvFormatCtx, pszFilePath, nullptr, nullptr);
    if (m_pAvFormatCtx == nullptr) {
        LOGE("Avformat fail to open files, avformat_open_input() exit with res %d",
             res);
        return res;
    }

    // 查找所有媒体流信息
    res = avformat_find_stream_info(m_pAvFormatCtx, nullptr);
    if (res == AVERROR_EOF) {
        LOGD("reached to files end");
        this->Close();
        return -1;
    }

    LOGD("number of streams: %d", m_pAvFormatCtx->nb_streams);
    // 遍历所有的媒体流信息
    for (unsigned int i = 0; i < m_pAvFormatCtx->nb_streams; i++) {
        AVStream *pAvStream = m_pAvFormatCtx->streams[i];
        if (pAvStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {

            LOGD("video stream");
            if ((pAvStream->codecpar->width <= 0) ||
                (pAvStream->codecpar->height <= 0)) {
                LOGE("invalid resolution, streamIndex=%d", i);
                continue;
            }

            AVCodec *pDecoder =
                    avcodec_find_decoder(pAvStream->codecpar->codec_id); // 找到视频解码器
            if (pDecoder == nullptr) {
                LOGE("can not find video codec");
                continue;
            } else {
                this->m_pVidDecodeCtx = avcodec_alloc_context3(pDecoder);
                if (this->m_pVidDecodeCtx == nullptr) {
                    LOGE("fail to video avcodec_alloc_context3()");
                    this->Close();
                    return -1;
                }
                res = avcodec_parameters_to_context(
                        this->m_pVidDecodeCtx,
                        m_pAvFormatCtx->streams[this->m_nVidStreamIndex]->codecpar);

                res = avcodec_open2(this->m_pVidDecodeCtx, nullptr, nullptr);
                if (res != 0) {
                    LOGE("fail to video avcodec_open2(), res=%d", res);
                    this->Close();
                    return -1;
                }
            }

            this->m_nVidStreamIndex = i;
            avformat_close_input(&m_pAvFormatCtx);
            m_pAvFormatCtx = nullptr;
            res = avformat_open_input(&this->m_pVFormatCtx, pszFilePath, nullptr, nullptr);
            if (this->m_pVFormatCtx == nullptr) {
                LOGE("Avformat fail to open files, avformat_open_input() exit with res %d",
                     res);
                return res;
            }
            // 查找所有媒体流信息
            res = avformat_find_stream_info(this->m_pVFormatCtx, nullptr);
            if (res == AVERROR_EOF) {
                LOGD("reached to files end");
                this->Close();
                return -1;
            }
            LOGD("============Input Information=========");
            LOGD("pxlFmt=%d, frameSize=%d*%d",
                 (int) this->m_pVFormatCtx->streams[this->m_nVidStreamIndex]->codecpar->format,
                 this->m_pVFormatCtx->streams[this->m_nVidStreamIndex]->codecpar->width,
                 this->m_pVFormatCtx->streams[this->m_nVidStreamIndex]->codecpar->height);
            LOGD("======================================");
            break;
        } else if (pAvStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            avformat_open_input(&this->m_pAFormatCtx, pszFilePath, nullptr, nullptr);
            LOGD("audio stream");
            if ((pAvStream->codecpar->channels <= 0) ||
                (pAvStream->codecpar->sample_rate <= 0)) {
                LOGE("invalid resolution, streamIndex=%d", i);
                continue;
            }

            AVCodec *pDecoder =
                    avcodec_find_decoder(pAvStream->codecpar->codec_id); // 找到音频解码器
            if (pDecoder == nullptr) {
                LOGE("can not find Audio codec");
                continue;
            } else {
                this->m_pAudDecodeCtx = avcodec_alloc_context3(pDecoder);
                if (this->m_pAudDecodeCtx == nullptr) {
                    LOGE("fail to audio avcodec_alloc_context3()");
                    this->Close();
                    return -1;
                }
                res = avcodec_parameters_to_context(
                        this->m_pAudDecodeCtx,
                        m_pAvFormatCtx->streams[this->m_nAudStreamIndex]->codecpar);

                res = avcodec_open2(this->m_pAudDecodeCtx, nullptr, nullptr);
                if (res != 0) {
                    LOGE("fail to audio avcodec_open2(), res=%d", res);
                    this->Close();
                    return -1;
                }
            }

            this->m_nAudStreamIndex = i;

            avformat_close_input(&m_pAvFormatCtx);
            m_pAvFormatCtx = nullptr;
            res = avformat_open_input(&this->m_pAFormatCtx, pszFilePath, nullptr, nullptr);
            if (this->m_pAFormatCtx == nullptr) {
                LOGE("Avformat fail to open files, avformat_open_input() exit with res %d",
                     res);
                return res;
            }
            // 查找所有媒体流信息
            res = avformat_find_stream_info(this->m_pAFormatCtx, nullptr);
            if (res == AVERROR_EOF) {
                LOGD("reached to files end");
                this->Close();
                return -1;
            }
            LOGD("============Input Information=========");
            LOGD("sample_fmt=%d, sampleRate=%d, channels=%d, chnl_layout=%lu, duration=%ld",
                 (int) this->m_pAFormatCtx->streams[this->m_nAudStreamIndex]->codecpar->format,
                 this->m_pAFormatCtx->streams[this->m_nAudStreamIndex]->codecpar->sample_rate,
                 this->m_pAFormatCtx->streams[this->m_nAudStreamIndex]->codecpar->channels,
                 this->m_pAFormatCtx->streams[this->m_nAudStreamIndex]->codecpar->channel_layout,
                 this->m_pAFormatCtx->streams[this->m_nAudStreamIndex]->duration);
            LOGD("======================================");
            break;
        }
    }
//    this->Close();
    if (m_pAvFormatCtx != nullptr) {
        avformat_close_input(&m_pAvFormatCtx);
        m_pAvFormatCtx = nullptr;
    }
    return 0;
}

//
// 关闭媒体文件，关闭对应的解码器
//
void AVMuxer::Close() {
    // 关闭媒体文件解析
//    if (m_pAvFormatCtx != nullptr) {
//        avformat_close_input(&m_pAvFormatCtx);
//        m_pAvFormatCtx = nullptr;
//    }
    if (this->m_pVFormatCtx != nullptr) {
        avformat_close_input(&this->m_pVFormatCtx);
        this->m_pVFormatCtx = nullptr;
    }
    if (this->m_pAFormatCtx != nullptr) {
        avformat_close_input(&this->m_pAFormatCtx);
        this->m_pAFormatCtx = nullptr;
    }
}

int32_t AVMuxer::MuxerDo(const char *pszFilePath) {
    int res = 0;

    // 创建输出流格式上下文
    res = avformat_alloc_output_context2(&this->m_pFormatCtx, nullptr, nullptr,
                                         pszFilePath);
    if (nullptr == this->m_pFormatCtx || res < 0) {
        LOGE("<MuxerDo> [ERROR] fail to avformat_alloc_output_context2()\n");
        return -1;
    }

    // Add video stream to output steam
    LOGD("Add video stream to output steam");
    AVStream *in_vstream = this->m_pVFormatCtx->streams[this->m_nVidStreamIndex];
    AVStream *out_vstream =
            avformat_new_stream(this->m_pFormatCtx, in_vstream->codec->codec);
    if (!out_vstream) {
        LOGE("Failed allocating output stream\n");
        this->Close();
        return -1;
    }

    int videoindex_out = out_vstream->index;

    // Copy the settings of AVCodecContext
    LOGD("Copy the settings of AVCodecContext");
    if (avcodec_parameters_from_context(out_vstream->codecpar,
                                        in_vstream->codec) < 0) {
        LOGE("Failed to copy context from input to output stream codec context");
        return -1;
    }
    out_vstream->codec->codec_tag = 0;
    if (this->m_pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
        out_vstream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // Add audio stream to output steam
    AVStream *in_astream = this->m_pAFormatCtx->streams[m_nAudStreamIndex];
    AVStream *out_astream =
            avformat_new_stream(this->m_pFormatCtx, in_astream->codec->codec);
    if (!out_astream) {
        LOGE("Failed allocating output stream");
        return 1;
    }

    int audioindex_out = out_astream->index;

    LOGD("videoindex_out: %d", videoindex_out);
    LOGD("audioindex_out: %d", audioindex_out);

    // Copy the settings of AVCodecContext
    LOGD("Copy the settings of AVCodecContext");
    if (avcodec_parameters_from_context(out_astream->codecpar,
                                        in_astream->codec) < 0) {
        LOGE("Failed to copy context from input to output stream codec context");
        return -1;
    }
    out_astream->codec->codec_tag = 0;
    if (this->m_pFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
        out_astream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // 打开文件IO上下文
    res = avio_open(&this->m_pFormatCtx->pb, pszFilePath, AVIO_FLAG_WRITE);
    if (res < 0) {
        LOGE("<MuxerDo> [ERROR] fail to avio_open(), res=%d\n", res);
        avformat_free_context(this->m_pFormatCtx);
        return -2;
    }

    AVDictionary *opts = NULL;
    int fragmented_mp4_options = 0;
    if (fragmented_mp4_options) {
        // https://developer.mozilla.org/en-US/docs/Web/API/Media_Source_Extensions_API/Transcoding_assets_for_MSE
        av_dict_set(&opts, "movflags", "frag_keyframe+empty_moov+default_base_moof", 0);
    }
    // 设置参数
    av_dict_set(&opts, "c:v", "copy", 0);
    av_dict_set(&opts, "c:a", "copy", 0);

    //
    // 写入文件头信息
    //
    res = avformat_write_header(this->m_pFormatCtx, &opts);
    if (res < 0) {
        LOGE("<MuxerDo> [ERROR] fail to FF_avformat_write_header(), res=%d\n", res);
        avformat_free_context(this->m_pFormatCtx);
        return -3;
    }

    int frame_index = 0;
    int64_t cur_pts_v = 0, cur_pts_a = 0;

#if USE_H264BSF
    AVBitStreamFilterContext *h264bsfc =
        av_bitstream_filter_init("h264_mp4toannexb");
#endif
#if USE_AACBSF
    AVBitStreamFilterContext *aacbsfc = av_bitstream_filter_init("aac_adtstoasc");
#endif

    AVPacket pkt;
    AVStream *in_stream, *out_stream;
    AVFormatContext *ifmt_ctx;
//    AVFormatContext *m_pFormatCtx_tmp = nullptr;
    int stream_index = 0;
//    int lastdts = 0;

//    res = avformat_open_input(&m_pFormatCtx_tmp, pszFilePath, nullptr, nullptr);
//    if (m_pFormatCtx_tmp == nullptr) {
//        LOGE("Avformat fail to open files, avformat_open_input() exit with res %d",
//             res);
//        return res;
//    }

    LOGD("READ PKT");

    while (1) {

        // Get an AVPacket
        res = av_compare_ts(
                cur_pts_v,
                in_vstream->time_base,
                cur_pts_a,
                in_astream->time_base);
        if (res < 0) {
            ifmt_ctx = this->m_pVFormatCtx;
            stream_index = videoindex_out;


            res = av_read_frame(ifmt_ctx, &pkt);
            if (res >= 0) {
//                do {
//                    if (pkt.stream_index == this->m_nVidStreamIndex) {
//                        cur_pts_v = pkt.pts;
//                        break;
//                    }
//                } while (av_read_frame(ifmt_ctx, &pkt) >= 0);
                do {
                    in_stream = ifmt_ctx->streams[pkt.stream_index];
                    out_stream = this->m_pFormatCtx->streams[stream_index];

                    if (pkt.stream_index == this->m_nVidStreamIndex) {
                        //Fix: No PTS(Example: Raw H.264
                        //Simple Write PTS
                        if (pkt.pts == AV_NOPTS_VALUE) {
                            //write PTS
                            AVRational timeBase1 = in_stream->time_base;
                            //Duration between 2 frames
                            int64_t calcDuration = (double) AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
                            //Parameters
                            pkt.pts = (double) (stream_index * calcDuration) /
                                      (double) (av_q2d(timeBase1) * AV_TIME_BASE);
                            pkt.dts = pkt.pts;
                            pkt.duration = (double) calcDuration / (double) (av_q2d(timeBase1) * AV_TIME_BASE);
                            stream_index++;
                        }

                        cur_pts_v = pkt.pts;
                        break;
                    }
                } while (av_read_frame(ifmt_ctx, &pkt) >= 0);
            } else {
                break;
            }
        } else {
            ifmt_ctx = this->m_pAFormatCtx;
            stream_index = audioindex_out;

            res = av_read_frame(ifmt_ctx, &pkt);
            if (res >= 0) {
//                do {
//                    if (pkt.stream_index == this->m_nAudStreamIndex) {
//                        cur_pts_a = pkt.pts;
//                        break;
//                    }
//                } while (av_read_frame(ifmt_ctx, &pkt) >= 0);
                do {
                    in_stream = ifmt_ctx->streams[pkt.stream_index];
                    out_stream = this->m_pFormatCtx->streams[stream_index];
                    if (pkt.stream_index == this->m_nAudStreamIndex) {
                        //Fix: No PTS(Example: Raw H.264
                        //Simple Write PTS
                        if (pkt.pts == AV_NOPTS_VALUE) {
                            //write PTS
                            AVRational timeBase1 = in_stream->time_base;
                            //Duration between 2 frames
                            int64_t calcDuration = (double) AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
                            //Parameters
                            pkt.pts =
                                    (double) (stream_index * calcDuration) /
                                    (double) (av_q2d(timeBase1) * AV_TIME_BASE);
                            pkt.dts = pkt.pts;
                            pkt.duration = (double) calcDuration / (double) (av_q2d(timeBase1) * AV_TIME_BASE);
                            stream_index++;
                        }

                        cur_pts_a = pkt.pts;
                        break;
                    }
                } while (av_read_frame(ifmt_ctx, &pkt) >= 0);

            } else {
                break;
            }
        }


#if USE_H264BSF
        av_bitstream_filter_filter(h264bsfc, in_stream->codec, NULL, &pkt.data,
                                   &pkt.size, pkt.data, pkt.size, 0);
#endif
#if USE_AACBSF
        av_bitstream_filter_filter(aacbsfc, in_stream->codec, NULL, &pkt.data,
                                   &pkt.size, pkt.data, pkt.size, 0);
#endif
        if (pkt.pts == AV_NOPTS_VALUE) {
            // Write PTS
            AVRational time_base1 = in_stream->time_base;
            // Duration between 2 frames (us)
            int64_t calc_duration =
                    (double) AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
            // Parameters
            pkt.pts = (double) (frame_index * calc_duration) /
                      (double) (av_q2d(time_base1) * AV_TIME_BASE);
            pkt.dts = pkt.pts;
            pkt.duration =
                    (double) calc_duration / (double) (av_q2d(time_base1) * AV_TIME_BASE);
            frame_index++;
        }
        /* copy packet */
        // Convert PTS/DTS
        pkt.pts = av_rescale_q_rnd(
                pkt.pts, in_stream->time_base, out_stream->time_base,
                (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(
                pkt.dts, in_stream->time_base, out_stream->time_base,
                (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.duration =
                av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        pkt.stream_index = stream_index;

//        LOGD("Write 1 Packet. size:%5d\tpts:%8d", pkt.size, pkt.pts);
        // Write
//        LOGD("dts:%ld, pts:%ld", pkt.dts, pkt.pts);
        res = av_interleaved_write_frame(this->m_pFormatCtx, &pkt);
        if (res < 0) {
//            LOGD("Error muxing a packet");
            continue;
        }
//    av_free_packet(&pkt);
        av_packet_unref(&pkt);
    }
    // Write file trailer
    LOGD("Write file trailer");
    if (this->m_pFormatCtx != nullptr) {
        av_write_trailer(this->m_pFormatCtx);
    }

    LOGD("============Output Information=========");
    LOGD("pxlFmt=%d, frameSize=%d*%d", (int) m_pFormatCtx->streams[0]->codecpar->format,
         m_pFormatCtx->streams[0]->codecpar->width, m_pFormatCtx->streams[0]->codecpar->height);
    LOGD("****************************************");
    LOGD("sample_fmt=%d, sampleRate=%d, channels=%d, chnl_layout=%lu, duration=%d",
         (int) m_pFormatCtx->streams[1]->codecpar->format, m_pFormatCtx->streams[1]->codecpar->sample_rate,
         m_pFormatCtx->streams[1]->codecpar->channels, m_pFormatCtx->streams[1]->codecpar->channel_layout,
         m_pFormatCtx->streams[1]->duration);
    LOGD("======================================");

#if USE_H264BSF
    av_bitstream_filter_close(h264bsfc);
#endif
#if USE_AACBSF
    av_bitstream_filter_close(aacbsfc);
#endif

    this->Close();
    this->MuxerClose();

    res = avformat_open_input(&this->m_pFormatCtx, pszFilePath, nullptr, nullptr);
    if (this->m_pFormatCtx == nullptr) {
        LOGE("Avformat fail to open files, avformat_open_input() exit with res %d", res);
        return res;
    }
    res = avformat_find_stream_info(this->m_pFormatCtx, nullptr);
    if (res == AVERROR_EOF) {
        LOGD("reached to files end");
        this->Close();
        return -1;
    }
    LOGD("============Output Information=========");
    LOGD("pxlFmt=%d, frameSize=%d*%d, duration=%ld", (int) m_pFormatCtx->streams[0]->codecpar->format,
         m_pFormatCtx->streams[0]->codecpar->width, m_pFormatCtx->streams[0]->codecpar->height,
         m_pFormatCtx->streams[0]->duration);
    LOGD("****************************************");
    LOGD("sample_fmt=%d, sampleRate=%d, channels=%d, chnl_layout=%lu, duration=%ld",
         (int) m_pFormatCtx->streams[1]->codecpar->format, m_pFormatCtx->streams[1]->codecpar->sample_rate,
         m_pFormatCtx->streams[1]->codecpar->channels, m_pFormatCtx->streams[1]->codecpar->channel_layout,
         m_pFormatCtx->streams[1]->duration);
    LOGD("======================================");

    this->MuxerClose();


    return 0;
}

void AVMuxer::MuxerClose() {

    // 先关IO上下文
    if (this->m_pFormatCtx && this->m_pFormatCtx->pb != nullptr) {
        avio_closep(&this->m_pFormatCtx->pb);
        this->m_pFormatCtx->pb = nullptr;
    }

    // 再释放媒体格式上下文
    if (this->m_pFormatCtx != nullptr) {
        avformat_free_context(this->m_pFormatCtx);
        this->m_pFormatCtx = nullptr;
    }

    // 流文件直接在 avformat_free_context()内部已经销毁了

    avformat_close_input(&m_pVFormatCtx);
    avformat_close_input(&m_pAFormatCtx);

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
