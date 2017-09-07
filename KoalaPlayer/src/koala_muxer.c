

/*
 * Copyright (c) 2015 pingkai@letv.com
 *
 *
 */
#include "koala_common.h"
#include "koala_muxer.h"
#include "../../utils/mediaFrame.h"
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
//#include <libavformat/internal.h>
#include <pthread.h>
typedef struct koala_muxer_t{
    AVFormatContext *oc;
    audio_data_callback acb;
    void *acb_arg;
    video_data_callback vcb;
    void *vcb_arg;

  //  OutputStream video_st;
  //  OutputStream audio_st;
    AVCodec *audio_codec;
    AVCodec *video_codec;

    void *audio_frame;
    void *video_frame;
    pthread_t mux_id;
    int end;

    int vide_stream;
    int audio_stream;

    int64_t last_audio_pts;
    int64_t last_video_pts;

    AVBitStreamFilterContext * aac_bsf;
    AVStream * pAudioStream;

    int64_t first_pts;
    int req_i_frame;

    AVRational conAbase;
    AVRational conVbase;
    AVIOInterruptCB mAVIOInterruptCB;
    pthread_mutex_t apiMutex;
    int io_timeout_ms;

    koala_muxer_listener listener;
    void* listenerArg;
    pthread_mutex_t listenerMutex;
    char *hostip;
}koala_muxer;

typedef enum stream_type_e {
    stream_type_video,
    stream_type_audio,
}stream_type;

koala_muxer * koala_get_muxer_handle(){
    koala_muxer *pHandle = malloc(sizeof(koala_muxer));
    memset(pHandle,0 ,sizeof(koala_muxer));
    pthread_mutex_init(&pHandle->listenerMutex, NULL);
    pHandle->first_pts = -1;
    pthread_mutex_init(&pHandle->apiMutex, NULL);
    koala_init_ffmpeg();
    return pHandle;
}
// TODO: interrupt
void koala_muxer_reg_data_callback(koala_muxer * pHandle,
                                       audio_data_callback acb,void *acb_arg,
                                       video_data_callback vcb,void *vcb_arg){

    pHandle->acb     = acb;
    pHandle->acb_arg = acb_arg;
    pHandle->vcb     = vcb;
    pHandle->vcb_arg = vcb_arg;
}
static void sendEvent(koala_muxer *pHandle, int event){
    if(NULL==pHandle){
        return ;
    }
    pthread_mutex_lock(&pHandle->listenerMutex);
    if(pHandle->listener){
        pHandle->listener(pHandle->listenerArg, event);
    }
    pthread_mutex_unlock(&pHandle->listenerMutex);
}
static int get_audio_frame(koala_muxer *pHandle,mediaFrame **frame){
    if (pHandle->acb){
        int size;
        return pHandle->acb(pHandle->acb_arg,callback_cmd_get_frame,(void **)frame,&size);
    }
    return -1;
}
static int get_video_frame(koala_muxer *pHandle,mediaFrame **frame){
    if (pHandle->vcb){
        int size;
        return pHandle->vcb(pHandle->vcb_arg,callback_cmd_get_frame,(void **)frame,&size);
    }
    return -1;
}

static int get_audio_source_meta(koala_muxer *pHandle,Stream_meta**meta){
    if (pHandle->acb){
        int size;
        return pHandle->acb(pHandle->acb_arg,callback_cmd_get_source_meta,(void **)meta,&size);
    }
    return -1;
}

static int get_video_source_meta(koala_muxer *pHandle,Stream_meta **meta){
    if (pHandle->vcb){
        int size;
        return pHandle->vcb(pHandle->vcb_arg,callback_cmd_get_source_meta,(void **)meta,&size);
    }
    return -1;
}
static int have_video(koala_muxer *pHandle){
    if (pHandle->vcb)
        return 1;
    return 0;
}

static int have_audio(koala_muxer *pHandle){
    if (pHandle->acb)
        return 1;
    return 0;
}


static int encode_interrupt_cb(void* opaque){
    if(NULL==opaque){
        return 0;
    }
    koala_muxer* pHandle=(koala_muxer*)opaque;
    return 1==pHandle->end?1:0;
}

int koala_muxer_init_open(koala_muxer *pHandle, const char* outName,const char * outFormat){

    AVOutputFormat *fmt;
    int ret;
    pthread_mutex_lock(&pHandle->apiMutex);
    // TODO: rtmp is flv
    if (outName != NULL){
        avformat_alloc_output_context2(&pHandle->oc, NULL, outFormat, outName);
        if (pHandle->oc == NULL){
            koala_logd("Can't alloc_output_context\n");
            pthread_mutex_unlock(&pHandle->apiMutex);
            return -1;
        }
    }else {
        // TODO: use out data call back to out put data
        koala_logd("no outName, not support Now\n");
        sendEvent(pHandle, ret);
        pthread_mutex_unlock(&pHandle->apiMutex);
        return -1;
    }

    fmt = pHandle->oc->oformat;

    if (have_audio(pHandle)){
        Stream_meta* meta;
        koala_logd("mux audio data\n");
        int ret = get_audio_source_meta(pHandle,&meta);
        if (ret < 0){
            koala_log(KOALA_LOG_ERROR,"get_audio_source_meta error \n");
            goto fail;
        }
        koala_trace;
      //  AVCodec *codec = avcodec_find_decoder(koalaCodecID2AVCodecID(meta->codec));
        AVStream *st = avformat_new_stream(pHandle->oc, NULL);
        st->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
        st->codecpar->codec_id = koalaCodecID2AVCodecID(meta->codec);
        st->codecpar->sample_rate = meta->samplerate;
        st->codecpar->channel_layout = (uint64_t) av_get_channel_layout_nb_channels(meta->channels);
        st->codecpar->channels = meta->channels;
        /* take first format from list of supported formats */
        st->codec->sample_fmt = (enum AVSampleFormat)meta->sample_fmt;
        st->codec->time_base = (AVRational){1, st->codec->sample_rate};
        st->codec->frame_size = meta->frame_size;
        pHandle->audio_stream = st->index;
        pHandle->pAudioStream = st;
        if (st->codecpar->codec_id == AV_CODEC_ID_AAC){
            // TODO: check adts header
        //    pHandle->aac_bsf = av_bitstream_filter_init("aac_adtstoasc");
        //wlj    ff_stream_add_bitstream_filter(st, "aac_adtstoasc", NULL);
        }
        st->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
        if (meta->extradata_size > 0){
            st->codecpar->extradata = malloc(meta->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
            memcpy(st->codecpar->extradata,meta->extradata,meta->extradata_size);
        }
        st->codecpar->extradata_size = meta->extradata_size;
        if (!strncmp(fmt->name,"flv",3))
            pHandle->conAbase = (AVRational){1,1000};
        else
            pHandle->conAbase = (AVRational){meta->samplerate,1000000};
    }

    if (have_video(pHandle)){
        Stream_meta* meta;
        koala_logd("mux video data\n");
        AVStream *st = avformat_new_stream(pHandle->oc, NULL);
        int ret = get_video_source_meta(pHandle, &meta);
        if (ret < 0){
            koala_log(KOALA_LOG_ERROR,"get_audio_source_meta error \n");
            goto fail;
        }

        st->codecpar->height = meta->height;
        st->codecpar->width = meta->width;
        st->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
        st->codecpar->codec_id = koalaCodecID2AVCodecID(meta->codec);
        if (meta->extradata_size > 0){
            st->codecpar->extradata = malloc(meta->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
            memcpy(st->codecpar->extradata,meta->extradata,meta->extradata_size);
        }
        st->codecpar->extradata_size = meta->extradata_size;
        st->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

        // TODO: get it
        AVRational r = {1,1};
        st->codecpar->sample_aspect_ratio = r;
        /* take first format from list of supported formats */

        // TODO: get it
        st->codec->pix_fmt = AV_PIX_FMT_YUV420P;
        st->avg_frame_rate = (AVRational){(int)meta->avg_fps, 1};
        /* video time_base can be set to whatever is handy and supported by encoder */
        st->codec->time_base = (AVRational){1,AV_TIME_BASE};
        pHandle->vide_stream = st->index;
        if (!strncmp(fmt->name,"flv",3))
            pHandle->conVbase = (AVRational){1,1000};
        else
            pHandle->conVbase = (AVRational){1,1};
    }

    av_dump_format(pHandle->oc, 0, outName, 1);


    if (!(pHandle->oc->oformat->flags & AVFMT_NOFILE)) {
#if 1
        pHandle->mAVIOInterruptCB.callback=encode_interrupt_cb;
        pHandle->mAVIOInterruptCB.opaque=(void*)pHandle;
        pHandle->oc->interrupt_callback=pHandle->mAVIOInterruptCB;
        AVDictionary* streamOpt = NULL;
        av_dict_set_int(&streamOpt, "io_timeout_ms", pHandle->io_timeout_ms, 0);
        ret = avio_open2(&(pHandle->oc->pb), outName, AVIO_FLAG_WRITE, &pHandle->mAVIOInterruptCB, &streamOpt);
        uint8_t *ip = NULL;
        av_opt_get(pHandle->oc->pb, "hostip_addr", AV_OPT_SEARCH_CHILDREN, &pHandle->hostip);
        av_dict_free(& streamOpt);
#else
        ret = avio_open(&(pHandle->oc->pb), outName, AVIO_FLAG_WRITE);
#endif
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'", outName);
            goto fail;
        }
    }

    /* init muxer, write output file header */
    ret = avformat_write_header(pHandle->oc, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file\n");
        goto fail;
    }

    pthread_mutex_unlock(&pHandle->apiMutex);
    return 0;

fail:
    if (pHandle->oc && !(pHandle->oc->oformat->flags & AVFMT_NOFILE))
        avio_close(pHandle->oc->pb);
    avformat_free_context(pHandle->oc);
    pHandle->oc=NULL;
    pthread_mutex_unlock(&pHandle->apiMutex);
    sendEvent(pHandle, ret);
    return -1;
}

/*
 * return 1 when finished, 0 otherwise
 */
static int write_frame(koala_muxer *pHandle,stream_type stream){
    mediaFrame *frame = NULL;
    int ret;
    AVPacket pkt = {0, };
    av_init_packet(&pkt);
    if (stream == stream_type_video)
        ret = get_video_frame(pHandle,&frame);
    else
        ret = get_audio_frame(pHandle,&frame);
    if (ret < 0){
        koala_logd("%d frame end\n",stream);
        return 1;
    }

    if (pHandle->req_i_frame){
        if (stream == stream_type_video && frame->flag){
            pHandle->req_i_frame = 0;
            pHandle->last_video_pts = 0;
            pHandle->last_audio_pts = 0;
        }
        else{
            if (stream == stream_type_video){
                pHandle->last_video_pts = frame->dts;
                koala_logd("drop a video frame %lld\n",frame->pts);
            }
            else{
                pHandle->last_audio_pts = frame->dts;
                koala_logd("drop a auido frame %lld\n",frame->pts);
            }
            mediaFrameRelease(frame);
            return 0;
        }
    }
    if (pHandle->first_pts < 0){
        pHandle->first_pts = frame->dts;
    }
    if (pHandle->first_pts > 0){
        frame->dts -= pHandle->first_pts;
        frame->pts -= pHandle->first_pts;
    }
    pkt.data = frame->pBuffer;
    pkt.pts  = frame->pts;
    pkt.size = frame->size;
    pkt.stream_index = (stream == stream_type_video) ? pHandle->vide_stream :  pHandle->audio_stream;
    pkt.dts   = frame->dts;
    pkt.flags = frame->flag;

    if (stream == stream_type_audio ){
        pkt.dts = pkt.pts = av_rescale(frame->pts,pHandle->conAbase.num,pHandle->conAbase.den);
    }else if (stream == stream_type_video){
        pkt.pts = av_rescale(frame->pts,pHandle->conVbase.num,pHandle->conVbase.den);
        pkt.dts = av_rescale(frame->dts,pHandle->conVbase.num,pHandle->conVbase.den);
    }

    if (stream == stream_type_video)
        //wljpHandle->last_video_pts = frame->dts;
        pHandle->last_video_pts = frame->pts;
    else
        pHandle->last_audio_pts = frame->pts;

    pHandle->oc->max_interleave_delta = 0;
    ret = av_interleaved_write_frame(pHandle->oc, &pkt);
    if(AVERROR(ETIMEDOUT)==ret|| AVERROR(EPIPE)==ret|| AVERROR(ENOTRECOVERABLE)==ret){
        sendEvent(pHandle, ret);
    }
    mediaFrameRelease(frame);
    return ret;
}
static void* mux_thread(void *arg){
    koala_muxer *pHandle = (koala_muxer *)arg;
    int encode_video = have_video(pHandle);
    int encode_audio = have_audio(pHandle);
    AVRational time_base = {1,AV_TIME_BASE};
    if (encode_video) {
      //  pHandle->req_i_frame = 1;
    }
    

    while (encode_video || encode_audio) {
        /* select the stream to encode */
        if (encode_video &&
            (!encode_audio || av_compare_ts(pHandle->last_video_pts, time_base,
                                            pHandle->last_audio_pts, time_base) <= 0)) {
            encode_video = !write_frame(pHandle,stream_type_video);
        } else {
            encode_audio = !write_frame(pHandle,stream_type_audio);
        }

        if (pHandle->end)
            break;
    }
    av_write_trailer(pHandle->oc);
    return NULL;
}

int koala_muxer_start(koala_muxer *pHandle){
    pthread_mutex_lock(&pHandle->apiMutex);
    if (pHandle->mux_id == 0)
        pthread_create(&(pHandle->mux_id), NULL, mux_thread, pHandle);
    pthread_mutex_unlock(&pHandle->apiMutex);
    return 0;
}
char *koala_muxer_get_hostip(koala_muxer *pHandle){
    return pHandle->hostip;
}

int koala_muxer_end(koala_muxer *pHandle){
    pHandle->end = 1;
    pthread_mutex_lock(&pHandle->apiMutex);
    if(pHandle->mux_id!=0) {
        pthread_join(pHandle->mux_id, NULL);
    }
    pHandle->first_pts = -1;
    pHandle->mux_id = 0;
    pthread_mutex_unlock(&pHandle->apiMutex);
    return 0;
}

int koala_muxer_setopt(koala_muxer *pHandle, const char* key, const char* value){
    if(NULL==pHandle){
        return -1;
    }
    int retval=-2;
    pthread_mutex_lock(&pHandle->apiMutex);
    koala_logd("%s  key:%s value:%s\n", __FUNCTION__, key, value);
    if(!strcasecmp("io_timeout_ms", key)){
        int timeout_ms=atoi(value);
        pHandle->io_timeout_ms=timeout_ms;
        retval=0;
    }


    pthread_mutex_unlock(&pHandle->apiMutex);
    return retval;
}


int koala_muxer_set_listener(
        koala_muxer *pHandle, koala_muxer_listener listener, void *arg){
    if(NULL==pHandle){
        koala_logd("phandle is null");
        return -1;
    }

    pthread_mutex_lock(&pHandle->listenerMutex);
    pHandle->listener=listener;
    pHandle->listenerArg=arg;
    pthread_mutex_unlock(&pHandle->listenerMutex);
    return 0;
}


void koala_muxer_close(koala_muxer *pHandle){
    pthread_mutex_lock(&pHandle->apiMutex);
    if (pHandle->oc && !(pHandle->oc->oformat->flags & AVFMT_NOFILE))
        avio_close(pHandle->oc->pb);
    avformat_free_context(pHandle->oc);
    pHandle->oc=NULL;
    pthread_mutex_unlock(&pHandle->apiMutex);
    pthread_mutex_destroy(&pHandle->apiMutex);
    pthread_mutex_destroy(&pHandle->listenerMutex);
    free(pHandle);
}
