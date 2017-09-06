

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#ifdef __cplusplus
}
#endif

#include "MediaStreamer.h"
#include "utils/logger.h"
#include "utils/mediaThread.h"
#include "utils/StreamerStack.h"
#include "KoalaPlayer/include/koala_type.h"
//#include "KoalaPlayer/include/koala_type.h"


class FFEncoderThread : public mediaThread {
public:
    FFEncoderThread(const char *name);

    ~FFEncoderThread();

    int initThread(AVCodecID codecID, AVStream *pStream, int64_t bit_rate);
    int getExradata(uint8_t **data);

private:
    mediaFrame *deal(mediaFrame *pFrame);

    mediaFrame *encoderVideo(mediaFrame *pFrame);

    mediaFrame *encoderAudio(mediaFrame *pFrame);

private:
    AVCodec *mEnc;
    AVCodecContext *mCodecCtx;
    AVStream *mpStream;
    uint8_t *mPCMBuffer;
    int mPcmSize;
};

FFEncoderThread::FFEncoderThread(const char *name) : mediaThread(name) {
    mPCMBuffer = NULL;
    mPcmSize = 0;
}

FFEncoderThread::~FFEncoderThread() {
    stop();
    avcodec_close(mCodecCtx);
    avcodec_free_context(&mCodecCtx);
    if (mPCMBuffer)
        free(mPCMBuffer);
}

int FFEncoderThread::initThread(AVCodecID codecID, AVStream *pStream, int64_t bit_rate) {
    AVDictionary *format_opts = NULL;
    mpStream             = pStream;
    mEnc                 = avcodec_find_encoder(codecID);
    mCodecCtx            = avcodec_alloc_context3(mEnc);
    mCodecCtx->time_base = mpStream->codec->time_base;
    mCodecCtx->bit_rate  = bit_rate;
    if (mpStream->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
        mCodecCtx->pix_fmt  = mpStream->codec->pix_fmt;
        mCodecCtx->height   = mpStream->codec->height;
        mCodecCtx->width    = mpStream->codec->width;
        mCodecCtx->flags   |= AV_CODEC_FLAG_GLOBAL_HEADER;
        av_dict_set_int(&format_opts, "realtime", 1, 0);
    } else if (mpStream->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
        if (pStream &&
            (pStream->codec->codec_id    >= AV_CODEC_ID_FIRST_AUDIO)
            && (pStream->codec->codec_id <= AV_CODEC_ID_PCM_S16BE_PLANAR)) {
            mEnc = avcodec_find_encoder_by_name("aac_at");
            if (mEnc != NULL) {
                printf("audio encoder is %s\n", mEnc->name);
            }
            mCodecCtx = avcodec_alloc_context3(mEnc);
            mCodecCtx->sample_fmt     = pStream->codec->sample_fmt;
            printf("sample_fmt is %d\n", mCodecCtx->sample_fmt);
            mCodecCtx->sample_rate    = pStream->codec->sample_rate;
            mCodecCtx->time_base.den  = pStream->codec->time_base.den;
            mCodecCtx->time_base.num  = pStream->codec->time_base.num;
            mCodecCtx->channels       = pStream->codec->channels;
            mCodecCtx->channel_layout = pStream->codec->channel_layout;
            mPCMBuffer                = (uint8_t *) malloc(1024 * 8);
            mPcmSize                  = 0;
        }
    }

    if (avcodec_open2(mCodecCtx, mEnc, format_opts == NULL ? NULL : &format_opts) < 0)
        printf("open %s error\n", mEnc->name);
    else
        printf("open %s ok\n", mEnc->name);

    LOGD("extra data size is %d",mCodecCtx->extradata_size);

    av_opt_free(format_opts);

    return 0;
}
static mediaFrame * getMediaFrame(StreamerStack <mediaFrame *> * pFrameStack){
    mediaFrame *pFrame = pFrameStack->pop();
    if (pFrame == NULL)
        pFrame = mediaFrameCreate(0);
    return pFrame;
}
static void putMediaFrame(StreamerStack <mediaFrame *> * pFrameStack,mediaFrame *pFrame){
    if (pFrameStack->push(pFrame) < 0)
        free(pFrame);
}
static void release_pkt_buffer(void *arg, mediaFrame *pFrame) {
    AVPacket *pkt = (AVPacket *) arg;
    av_packet_free(&pkt);
    mediaThread * pThis = (mediaThread *) pFrame->pAppending;
    pThis->putMediaFrame(pFrame);
}

mediaFrame *FFEncoderThread::encoderVideo(mediaFrame *pFrame) {
    int got = 0;
    AVFrame *frame = av_frame_alloc();
    AVPacket *pkt = (AVPacket *) pFrame->pBuffer;
    frame->data[0] = pkt->data + 64;
    frame->data[1] = frame->data[0] + (mCodecCtx->height *
                                       mCodecCtx->width);
    frame->linesize[0] = mCodecCtx->width;
    frame->linesize[1] = mCodecCtx->width;
    frame->linesize[2] = 0;
    //printf("size is %d\n",streamer->pkt->size);
    frame->height = mCodecCtx->height;
    frame->width  = mCodecCtx->width;
    frame->format = mCodecCtx->pix_fmt;
    frame->pts    = pkt->pts;
    frame->color_range = AVCOL_RANGE_JPEG;
    pkt = av_packet_alloc();
    av_init_packet(pkt);
    avcodec_encode_video2(mCodecCtx, pkt, frame, &got);
    av_frame_free(&frame);
    mediaFrameRelease(pFrame);
    if (got) {
        pFrame = getMediaFrame();
        pFrame->pAppending = this;
        mediaFrame_setRelease(pFrame, release_pkt_buffer, pkt);
        pFrame->size    = pkt->size;
        pFrame->pBuffer = pkt->data;
        pFrame->pts     = pkt->pts;
        pFrame->dts     = pkt->dts;
        pFrame->flag    = pkt->flags & AV_PKT_FLAG_KEY;
        return pFrame;
    }
    return NULL;
}

mediaFrame *FFEncoderThread::encoderAudio(mediaFrame *pFrame) {
    int got = 0;
    AVPacket *pkt = (AVPacket *) pFrame->pBuffer;
    memcpy(mPCMBuffer + mPcmSize, pkt->data, (size_t) pkt->size);
    mPcmSize += pkt->size;
    if (mPcmSize < 1024 * 8) {
        mediaFrameRelease(pFrame);
        return NULL;
    }
    AVFrame *frame = av_frame_alloc();
    //  LOGD("pkt->size is %d pts is %lld, frame size is %d\n",pkt->size,pkt->pts,mCodecCtx->frame_size);
    frame->nb_samples        = mPcmSize / av_get_bytes_per_sample(mCodecCtx->sample_fmt) / mCodecCtx->channels;
    frame->channel_layout    = mCodecCtx->channel_layout;
    frame->format            = mCodecCtx->sample_fmt;
    frame->sample_rate       = mCodecCtx->sample_rate;
    frame->channels          = mCodecCtx->channels;
    frame->data[0]           = mPCMBuffer;
    frame->extended_data     = &frame->data[0];
    frame->linesize[0]       = mPcmSize;
    frame->pts               = pkt->pts;
    pkt                      = av_packet_alloc();
    av_init_packet(pkt);
    int ret = avcodec_encode_audio2(mCodecCtx, pkt, frame, &got);
    frame->data[0]           = NULL;
    frame->extended_data     = &frame->data[0];
    av_frame_free(&frame);
    mediaFrameRelease(pFrame);
    mPcmSize = 0;
    if (ret < 0) {
        LOGE("avcodec_encode_audio2 error %d\n", ret);
    }
    if (got) {
        pFrame = getMediaFrame();
        pFrame->pAppending = this;
        mediaFrame_setRelease(pFrame, release_pkt_buffer, pkt);
        pFrame->size       = pkt->size;
        pFrame->pBuffer    = pkt->data;
        pFrame->pts        = pkt->pts;
        pFrame->dts        = pkt->dts;
        pFrame->flag       = pkt->flags & AV_PKT_FLAG_KEY;
        return pFrame;
    }
    return NULL;
}

mediaFrame *FFEncoderThread::deal(mediaFrame *pFrame) {
    if (mEnc->type == AVMEDIA_TYPE_VIDEO)
        return encoderVideo(pFrame);
    else if (mEnc->type == AVMEDIA_TYPE_AUDIO)
        return encoderAudio(pFrame);
    else {
        LOGE("UnSupport type");
        mediaFrameRelease(pFrame);
        return NULL;
    }

}

int FFEncoderThread::getExradata(uint8_t **data) {

    //TODO: if 264 video no extradata, return sps pps
    *data = mCodecCtx->extradata;
    return mCodecCtx->extradata_size;
}


typedef struct streamer_cont_t {
    mediaStreamer *streamer;
    bool stop;
    pthread_t ff_devices_thread;
    AVFormatContext *pFormatCtx;
    AVInputFormat *ifmt;
    Stream_meta meta;
    Stream_meta metaA;
    AVStream *pVideoStream;
    int video_index;
    int audio_index;
    AVStream *pAudioStream;
  //  AVFrame *frame;

    FFEncoderThread *mpThreadVE;
    mediaFrameQueue *pYUVQueue;
    mediaFrameQueue *pAEQueue;  //audio es

    FFEncoderThread *mpThreadAE;
    mediaFrameQueue *pPCMQueue;
    mediaFrameQueue *pVEQueue;  //video es
    StreamerStack <mediaFrame *> * pFrameStack;
} streamer_cont;

static void MyMediaStreamerListener(void *arg, int event) {
    streamer_cont *pHandle = (streamer_cont *) arg;
    if (event < 0) {
        LOGE("streamer error\n");
        pHandle->stop = true;
    }
    else if (event == PREPARE_EVENT_ERROR) {
        LOGE("PREPARE_EVENT_ERROR\n");
        pHandle->stop = true;
    } else if (event == PREPARE_EVENT_OK)
        LOGD("PREPARE_EVENT_OK\n");
}

void *ff_read_devices(void *arg);

static int audio_callback(void *arg, int cmd, void **data, int *size) {
    streamer_cont *streamer = (streamer_cont *) arg;
    switch (cmd) {
        case callback_cmd_get_source_meta:
            memset(&streamer->metaA, 0, sizeof(Stream_meta));
            streamer->metaA.codec      = KOALA_CODEC_ID_AAC;
            streamer->metaA.channels   = streamer->pAudioStream->codec->channels;
            streamer->metaA.samplerate = streamer->pAudioStream->codec->sample_rate;
            streamer->metaA.extradata_size = streamer->mpThreadAE->getExradata(&(streamer->metaA.extradata));

            *data = &streamer->metaA;
            *size = sizeof(Stream_meta);
            return 0;
        case callback_cmd_get_frame: {
            mediaFrame *pMediaFrame = streamer->pAEQueue->dequeue();
            while (pMediaFrame == NULL) {
                pMediaFrame = streamer->pAEQueue->dequeue();
                if (streamer->stop) {
                    if (pMediaFrame)
                        mediaFrameRelease(pMediaFrame);
                    return -1;
                }
            }

            *data = pMediaFrame;
            *size = sizeof(mediaFrame);
            return 0;
        }
        default:
            break;
    }
    return 0;
}

static int video_callback(void *arg, int cmd, void **data, int *size) {
    streamer_cont *streamer = (streamer_cont *) arg;
    switch (cmd) {
        case callback_cmd_get_source_meta:
            //LOGD("video callback_cmd_get_source_meta\n");
     //       pthread_create(&(streamer->ff_devices_thread), NULL, ff_read_devices, streamer);
            memset(&streamer->meta, 0, sizeof(Stream_meta));
            streamer->meta.codec   = KOALA_CODEC_ID_H264;
            printf("xxxxx width = %d\n", streamer->pVideoStream->codec->width);
            streamer->meta.width   = streamer->pVideoStream->codec->width;
            streamer->meta.height  = streamer->pVideoStream->codec->height;
            streamer->meta.avg_fps = 30;
            streamer->meta.type = STREAM_TYPE_VIDEO;
            streamer->meta.extradata_size = streamer->mpThreadVE->getExradata(&(streamer->meta.extradata));
            *data = &(streamer->meta);
            *size = sizeof(Stream_meta);
            return 0;
        case callback_cmd_get_frame: {
            mediaFrame *pMediaFrame = streamer->pVEQueue->dequeue();
            while (pMediaFrame == NULL) {
                pMediaFrame = streamer->pVEQueue->dequeue();
                if (streamer->stop) {
                    if (pMediaFrame)
                        mediaFrameRelease(pMediaFrame);
                    return -1;
                }
            }

            *data = pMediaFrame;
            *size = sizeof(mediaFrame);
            return 0;
        }
        default:
            break;
    }
    return 0;
}

static int open_input(streamer_cont *pHandle, const char *url) {

    (void) url;
    av_register_all();
    avdevice_register_all();
    AVDictionary *format_opts;
    av_dict_set_int(&format_opts, "pixel_format", AV_PIX_FMT_NV12, 0);
    av_dict_set_int(&format_opts, "framerate", 30, 0);
    pHandle->pFormatCtx = avformat_alloc_context();
    pHandle->ifmt       = av_find_input_format("avfoundation");

    if (avformat_open_input(&(pHandle->pFormatCtx), "0:0", pHandle->ifmt, &format_opts) != 0) {
        printf("open avfoundation error \n");
        return -1;
    }
    if (avformat_find_stream_info(pHandle->pFormatCtx, NULL) < 0) {
        printf("Couldn't find stream information.\n");
        return -1;
    }

    int i;
    for (i = 0; i < pHandle->pFormatCtx->nb_streams; i++) {
        if (pHandle->pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            printf("found a video stream\n");
            pHandle->pVideoStream = pHandle->pFormatCtx->streams[i];
            pHandle->video_index  = i;
            if (pHandle->pVideoStream->codec->codec_id == AV_CODEC_ID_RAWVIDEO) {
                printf("pix fmt is %d AV_PIX_FMT_NV12 is %d\n", pHandle->pVideoStream->codec->pix_fmt, AV_PIX_FMT_NV12);
                printf("%d X %d\n", pHandle->pVideoStream->codec->height, pHandle->pVideoStream->codec->width);
            }
        } else if (pHandle->pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            printf("found a Audio stream\n");
            pHandle->pAudioStream = pHandle->pFormatCtx->streams[i];
            LOGD("audio codec is %x AV_CODEC_ID_PCM_F32LE is %x\n", pHandle->pAudioStream->codec->codec_id,
                 AV_CODEC_ID_PCM_F32LE);
            pHandle->audio_index = i;
            LOGD("Audio channel is %d\n", pHandle->pFormatCtx->streams[i]->codec->channels);
        }
    }

    return 0;
}

static void close_input(streamer_cont *pHandle) {

    avformat_close_input(&(pHandle->pFormatCtx));

}

static int initMediaThread(streamer_cont *pHandle) {
    if (pHandle->video_index >= 0) {
        pHandle->pYUVQueue  = new mediaFrameQueue(5);
        pHandle->mpThreadVE = new FFEncoderThread("VE");
        pHandle->mpThreadVE->setInputQueue(pHandle->pYUVQueue);
        pHandle->pVEQueue   = new mediaFrameQueue(100);
        pHandle->mpThreadVE->setOutputQueue(pHandle->pVEQueue);
        pHandle->mpThreadVE->setMediaFrameStack(pHandle->pFrameStack);
        pHandle->mpThreadVE->initThread(AV_CODEC_ID_H264, pHandle->pVideoStream, 2 * 1024 * 1024);
    }
    if (pHandle->audio_index >= 0) {
        pHandle->pPCMQueue  = new mediaFrameQueue(20);
        pHandle->mpThreadAE = new FFEncoderThread("AE");
        pHandle->mpThreadAE->setInputQueue(pHandle->pPCMQueue);
        pHandle->pAEQueue   = new mediaFrameQueue(400);
        pHandle->mpThreadAE->setOutputQueue(pHandle->pAEQueue);
        pHandle->mpThreadAE->setMediaFrameStack(pHandle->pFrameStack);
        pHandle->mpThreadAE->initThread(AV_CODEC_ID_AAC, pHandle->pAudioStream, 64*1024);
    }

    return 0;
}

static void destroyMediaThread(streamer_cont *pHandle) {
    if (pHandle->video_index >= 0) {
        pHandle->mpThreadVE->stop();
        delete pHandle->mpThreadVE;
        delete pHandle->pYUVQueue;
        delete pHandle->pVEQueue;
    }
    if (pHandle->audio_index >= 0) {
        pHandle->mpThreadAE->stop();
        delete pHandle->mpThreadAE;
        delete pHandle->pPCMQueue;
        delete pHandle->pAEQueue;
    }

}

static int myCleanQueueV(void *arg, mediaFrame *pFrame) {
    int size = *(int *) arg;
//    LOGD("clean size is %d\n",size);
    if (pFrame->flag == 0 || size == 0) {
        (*(int *) arg)++;
        return CLEAN_FLAG_CLEAN;
    }
    return CLEAN_FLAG_STOP;
}

static int myCleanQueueA(void *arg, mediaFrame *pFrame) {
    int size = *(int *) arg;
    (void) pFrame;
  //  LOGD("clean size is %d\n",size);
    if (size < 100) {
        (*(int *) arg)++;
        return CLEAN_FLAG_CLEAN;
    }
    return CLEAN_FLAG_STOP;
}

void ReleasePkt(void *arg, mediaFrame *pFrame) {
    streamer_cont *pHandle = (streamer_cont *) arg;
    AVPacket *pkt = (AVPacket *) pFrame->pBuffer;
    av_packet_free(&pkt);
    putMediaFrame(pHandle->pFrameStack,pFrame);
    return;
}

void *ff_read_devices(void *arg) {
    streamer_cont *pHandle = (streamer_cont *) arg;
    AVPacket *pkt;
    while (!pHandle->stop) {
        pkt = av_packet_alloc();
        av_read_frame(pHandle->pFormatCtx, pkt);
        mediaFrame *pFrame = getMediaFrame(pHandle->pFrameStack);
        pFrame->pBuffer = (uint8_t *) pkt;
        mediaFrame_setRelease(pFrame, ReleasePkt, pHandle);
        if (pkt->stream_index == pHandle->video_index) {
//        LOGD("enqueue a pkt %p\n",pFrame->pBuffer);
            pHandle->pYUVQueue->enqueue(pFrame);
        } else if (pkt->stream_index == pHandle->audio_index) {
//      LOGD("enqueue a pkt %p\n",pFrame->pBuffer);
            pHandle->pPCMQueue->enqueue(pFrame);
        } else {
            mediaFrameRelease(pFrame);
        }
        //TODO: move to another thread
        {
            int size;
            if (pHandle->pVEQueue) {
                size = pHandle->pVEQueue->getSize();
                if (size > 80) {
                    size = 0;
                    pHandle->pVEQueue->cleanIf(myCleanQueueV, &size);
                }
                // TODO: if the queue is empty,we must req a key frame from encoder
            }
            if (pHandle->pAEQueue) {
                size = pHandle->pAEQueue->getSize();
                if (size > 300) {
                    size = 0;
                    pHandle->pAEQueue->cleanIf(myCleanQueueA, &size);
                }
            }
        }
    }
    return NULL;
}


int main(int argc, const char **argv) {
    int ret;
    streamer_cont *pHandle = (streamer_cont *) malloc(sizeof(streamer_cont));
    memset(pHandle, 0, sizeof(streamer_cont));
    pHandle->video_index = pHandle->audio_index = -1;
    pHandle->pFrameStack = new StreamerStack <mediaFrame*>(500);
    MediaStreamerCB myCb = {0,};
    ret = open_input(pHandle, NULL);
    if (ret < 0){
        LOGE("open input error\n");
        free(pHandle);
        return ret;
    }
    initMediaThread(pHandle);
    pthread_create(&(pHandle->ff_devices_thread), NULL, ff_read_devices, pHandle);
    if (pHandle->mpThreadVE) {
        pHandle->mpThreadVE->start();
    }
    if (pHandle->mpThreadAE) {
        pHandle->mpThreadAE->start();

    }
    myCb.element = ELEMENT_ALL;
    myCb.audio_callback = audio_callback;
    myCb.video_callback = video_callback;
    myCb.acbarg = pHandle;
    myCb.vcbarg = pHandle;
    pHandle->streamer = create_mediaStreamer();

    mediaStreamer_set_inPut(pHandle->streamer, &myCb);
    mediaStreamer_set_outPut(pHandle->streamer, argv[1], "flv");
    mediaStreamer_setListener(pHandle->streamer, MyMediaStreamerListener, pHandle);
    mediaStreamer_prepare(pHandle->streamer);
    char c;
    while (!pHandle->stop) {
        c = (char) getchar();
        switch (c) {
            case 'q':
                pHandle->stop = true;
                break;
            case 'i': {
                int size;
                if (pHandle->pVEQueue) {
                    size = pHandle->pVEQueue->getSize();
                    LOGI("VE buffer size is %d\n", size);
                }
                if (pHandle->pAEQueue) {
                    size = pHandle->pAEQueue->getSize();
                    LOGI("AE buffer size is %d\n", size);
                }
                LOGD("factor is %f\n",    mediaStreamer_getBandWideFactor(pHandle->streamer));
                LOGD("video bw  is %u\n", mediaStreamer_getStreamBitRate(pHandle->streamer, STREAM_TYPE_VIDEO));
                LOGD("audio bw  is %u\n", mediaStreamer_getStreamBitRate(pHandle->streamer, STREAM_TYPE_AUDIO));
                mediaStreamer_resetStatistician(pHandle->streamer);
            }
            default:
                break;
        }
    }
    destroyMediaThread(pHandle);
    pthread_join(pHandle->ff_devices_thread,NULL);
    mediaStreamer_stop(pHandle->streamer);
    mediaStreamer_release(pHandle->streamer);
    close_input(pHandle);

    mediaFrame *pFrame;
    LOGD("frame StreamerStack size is %d\n",pHandle->pFrameStack->size());
    do{
        pFrame = pHandle->pFrameStack->pop();
        if (pFrame != NULL) {
            free(pFrame);
        }
    }while (pFrame != NULL);
    delete pHandle->pFrameStack;

    free(pHandle);
    return 0;
}