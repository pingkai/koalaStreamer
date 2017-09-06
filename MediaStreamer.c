//
// Created by 平凯 on 2017/8/1.
//
#define LOG_TAG "MediaStreamer"
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "MediaStreamer.h"
#include "MediaMuxer/MediaMuxer.h"
#include "utils/logger.h"
#include "streamStatistician.h"
//#include "koala_type.h"
//#include "utils/lec_loger.h"
#define DEFAULT_AUDIO_CODEC KOALA_CODEC_ID_AAC


typedef enum mediastreamer_status_t {
    mediastreamer_status_init,
    mediastreamer_status_preparing,
    mediastreamer_status_prepared,
            mediastreamer_status_streaming,
//    mediastreamer_status_paused,
            mediastreamer_status_stoped,
//    mediastreamer_status_eos,

    mediastreamer_status_error,
} mediastreamer_status;


typedef struct streamer_t {
    muxer *mMuxer;
    MediaStreamerCB mDataCb;
    char *mOutName;
    char *mOutFormat;
    mediastreamer_status status;

    //send event to usr when pull stream err.
    mediaStreamerListener listener;
    void *listenerArg;
    pthread_mutex_t listenerMutex;
    
    streamerStatistician* pInputStastic;
    int videoInputStasticIndex;
    int audioInputStasticIndex;
 //   streamerStatistician* pOutputStastic;
 //   int videoOutputStasticIndex;
 //   int audioOutputStasticIndex;

    pthread_t prepare_thread;
    
//    lec_aIn *mAIn;
//    lec_aEnc *mAenc;
//    audio_info AInfo;
    //   audio_info AInfoEnc;
    //   int aEnc_have_data;
} mediaStreamer;
#if 0
static int Aenc_get_frame(streamer *pHandle,const void **data,int *size){
    mediaFrame *frame;
    if (!pHandle->aEnc_have_data){
        int size;
        int  got_data = 0;
        while (!got_data){
            pHandle->mDataCb->audio_callback(pHandle->mDataCb->acbarg,&frame,&size);
            got_data = lec_audio_enc_enc(pHandle->mAenc, frame->pBuffer, frame->size, frame->pts);
            if (got_data < 0){
                LOGE("audio enc error\n");
                return -1;
            }
        }
    }
    pHandle->aEnc_have_data = lec_audio_enc_get_output(pHandle->mAenc,&frame);
    *data = frame;
    *size = sizeof(mediaFrame);
    return 0;
}
#endif
// TDDO: ios only


static int audio_callback(void *arg, int cmd, void **data, int *size) {
    mediaStreamer *pHandle = (mediaStreamer *) arg;
    int ret;
#if 0
    if (pHandle->mAenc){
       switch (cmd){
       case callback_cmd_get_source_meta:
           *data = pHandle->AInfoEnc;
           *size = sizeof(audio_info);
           break;
       case callback_cmd_get_frame:
           return Aenc_get_frame(pHandle, const data,size);
       default:
           break;
       }
       return 0;
    }
#endif
    if (cmd == callback_cmd_get_frame){

        streamerStatistician_onFrameBegin(pHandle->pInputStastic, pHandle->audioInputStasticIndex, NULL);
        ret = pHandle->mDataCb.audio_callback(pHandle->mDataCb.acbarg, cmd, data, size);
        if (ret >= 0)
            streamerStatistician_onFrameEnd(pHandle->pInputStastic, pHandle->audioInputStasticIndex, (mediaFrame *) (*data));
        return ret;
    }
    return pHandle->mDataCb.audio_callback(pHandle->mDataCb.acbarg, cmd, data, size);
}

static int video_callback(void *arg, int cmd, void **data, int *size) {
    mediaStreamer *pHandle = (mediaStreamer *) arg;
    if (cmd == callback_cmd_get_frame){
        int ret;
        streamerStatistician_onFrameBegin(pHandle->pInputStastic, pHandle->videoInputStasticIndex, NULL);
        ret = pHandle->mDataCb.video_callback(pHandle->mDataCb.vcbarg, cmd, data, size);
        if (ret == 0)
            streamerStatistician_onFrameEnd(pHandle->pInputStastic, pHandle->videoInputStasticIndex, (mediaFrame *) (*data));
        
    //    uint32_t bw = mediaStreamer_getStreamBitRate(pHandle, STREAM_TYPE_VIDEO);
    //    float factor =  streamerStatistician_getStreamBandWideFactor(pHandle->pInputStastic);
   //     LOGD("video bw is %u factor is %f\n",bw,factor);
        return ret;
    }
    return pHandle->mDataCb.video_callback(pHandle->mDataCb.vcbarg, cmd, data, size);
}

mediaStreamer *create_mediaStreamer() {
    mediaStreamer *pHandle = malloc(sizeof(mediaStreamer));
    memset(pHandle, 0, sizeof(mediaStreamer));

    pHandle->mMuxer = create_muxer_handle();
    pthread_mutex_init(&pHandle->listenerMutex, NULL);
    pHandle->pInputStastic = streamerStatisticianCreate();
 //   pHandle->pOutputStastic = streamerStatisticianCreate();

    return pHandle;
}

int mediaStreamer_set_inPut(mediaStreamer *pHandle, MediaStreamerCB *pDataCb) {
    memcpy(&pHandle->mDataCb, pDataCb, sizeof(MediaStreamerCB));
    return 0;
}

int mediaStreamer_set_outPut(mediaStreamer *pHandle, const char *outName, const char *outFormat) {
    if (outName != NULL) {
        pHandle->mOutName = malloc(strlen(outName) + 1);
        memcpy(pHandle->mOutName, outName, strlen(outName) + 1);
    }
    if (outFormat != NULL) {
        pHandle->mOutFormat = malloc(strlen(outFormat) + 1);
        memcpy(pHandle->mOutFormat, outFormat, strlen(outFormat) + 1);
    }
    return 0;
}

static void Notify(mediaStreamer *pHandle, int event){
    if (pHandle->listener) {
        pHandle->listener(pHandle->listenerArg, event);
    }
}

//void (*koala_muxer_event_callback)(void* arg, int event);
static void mediastreamer_muxerListener(void *arg, int event) {
    mediaStreamer *pHandle = (mediaStreamer *) arg;
    if (NULL == pHandle) {
        LOGE("pHandle is null");
        return;
    }
    pthread_mutex_lock(&pHandle->listenerMutex);
    if (pHandle->listener) {
        pHandle->listener(pHandle->listenerArg, event);
    }
    pthread_mutex_unlock(&pHandle->listenerMutex);
}

static void* mediaStreamer_prepare_internal(mediaStreamer *pHandle) {
    int ret;
//    LE_TRACE;
    if (pHandle->status == mediastreamer_status_streaming) {
        LOGW("already prepared\n");
        return NULL;
    }
    LOGD("pHandle->mDataCb.element is %d\n", pHandle->mDataCb.element);
    ret = muxer_open(pHandle->mMuxer, pHandle->mOutName, pHandle->mOutFormat, pHandle->mDataCb.element);
    if (ret < 0) {
        LOGE("open muxer error\n");
        Notify(pHandle, PREPARE_EVENT_ERROR);
        return NULL;
    }
    TRACE;
    muxer_set_listener(pHandle->mMuxer, mediastreamer_muxerListener, pHandle);
    TRACE;
    if (pHandle->mDataCb.element & ELEMENT_AUDIO) {
#if 0
        if (pHandle->mDataCb->audio_callback == NULL){
            pHandle->mAIn = lec_ain_open(&pHandle->AInfo);
        }else{
            void * pData;
            int size;
            pHandle->mDataCb->audio_callback(pHandle->mDataCb->acbarg,callback_cmd_get_source_meta,&pData,size);
            memcpy(&pHandle->AInfo,pData,sizeof(audio_info));
        }
        memcpy(&pHandle->AInfoEnc,&pHandle->AInfo,sizeof(audio_info));

        if (pHandle->AInfo.codec == KOALA_CODEC_ID_PCM_S16LE){
            pHandle->mAenc = lec_audio_enc_create(DEFAULT_AUDIO_CODEC, pHandle->AInfo, 0);
            if (pHandle->mAenc == NULL){
                LOGE("audio encoder open error\n");
                return -1;
            }
            pHandle->AInfoEnc->codec = DEFAULT_AUDIO_CODEC;
        }
#endif
        muxer_set_callback(pHandle->mMuxer, STREAM_TYPE_AUDIO, audio_callback, pHandle);
        pHandle->audioInputStasticIndex = streamerStatistician_addStream(pHandle->pInputStastic, STREAM_TYPE_AUDIO);
    }
    TRACE;
    if (pHandle->mDataCb.element & ELEMENT_VIDEO) {
        muxer_set_callback(pHandle->mMuxer, STREAM_TYPE_VIDEO, video_callback, pHandle);
        pHandle->videoInputStasticIndex = streamerStatistician_addStream(pHandle->pInputStastic, STREAM_TYPE_VIDEO);
    }

    ret = muxer_prepare(pHandle->mMuxer);
    if (ret >= 0) {
        pHandle->status = mediastreamer_status_prepared;
        Notify(pHandle, PREPARE_EVENT_OK);
        //   LOGD("Host ip is %s\n",mediaStreamer_getHostIp(pHandle));
        mediaStreamer_start(pHandle);
    } else{
        pHandle->status = mediastreamer_status_error;
        Notify(pHandle, PREPARE_EVENT_ERROR);
    }
    return NULL;
}

int mediaStreamer_prepare(mediaStreamer *pHandle){
    if (pHandle->status == mediastreamer_status_init) {
        pHandle->status = mediastreamer_status_preparing;
        pthread_create(&pHandle->prepare_thread, NULL, (void *(*)(void *)) mediaStreamer_prepare_internal, pHandle);
    }
    return 0;
}

char * mediaStreamer_getHostIp(mediaStreamer *pHandle){
    LOGD("StatisticsReporter: Host ip is %s\n",muxer_getHostIp(pHandle->mMuxer));
    return muxer_getHostIp(pHandle->mMuxer);
    
}

int mediaStreamer_start(mediaStreamer *pHandle) {

    pHandle->status = mediastreamer_status_streaming;
    return muxer_start(pHandle->mMuxer);
}
uint32_t mediaStreamer_getStreamBitRate(mediaStreamer *pHandle, Stream_type type) {
    int index = (type == STREAM_TYPE_VIDEO) ? pHandle->videoInputStasticIndex : pHandle->audioInputStasticIndex;
    return streamerStatistician_getStreamBitRate(pHandle->pInputStastic, index);
}

float  mediaStreamer_getBandWideFactor(mediaStreamer *pHandle){
    return streamerStatistician_getStreamBandWideFactor(pHandle->pInputStastic);
}

void mediaStreamer_resetStatistician(mediaStreamer *pHandle){
    return streamerStatistician_reset(pHandle->pInputStastic);
}

void mediaStreamer_stop(mediaStreamer *pHandle) {
    if (pHandle->status > mediastreamer_status_init
            && pHandle->status < mediastreamer_status_stoped) {
        pthread_join(pHandle->prepare_thread,NULL);
        muxer_end(pHandle->mMuxer);
        muxer_close(pHandle->mMuxer);
        pHandle->status = mediastreamer_status_stoped;
    }
}

int mediaStreamer_setopt(mediaStreamer *pHandle, const char *key, const char *value) {
    if (NULL == pHandle->mMuxer) {
        LOGE("mMuxer is null\n");
        return -1;
    }
    return muxer_setopt(pHandle->mMuxer, key, value);
}

int mediaStreamer_setListener(
        mediaStreamer *pHandle, mediaStreamerListener listener, void *arg) {
    if (NULL == pHandle) {
        LOGE("streamer handle is null");
        return -1;
    }

    pthread_mutex_lock(&pHandle->listenerMutex);
    pHandle->listener = listener;
    pHandle->listenerArg = arg;
    pthread_mutex_unlock(&pHandle->listenerMutex);
    return 0;
}

void mediaStreamer_release(mediaStreamer *pHandle) {
    //mediaStreamer_stop(pHandle);
    if (pHandle->mMuxer) {
        muxer_release_handle(pHandle->mMuxer);
    }
    if (pHandle->mOutName)
        free(pHandle->mOutName);
    if (pHandle->mOutFormat)
        free(pHandle->mOutFormat);
    if (pHandle->pInputStastic)
        streamerStatisticianRelease(pHandle->pInputStastic);
 //   if(pHandle->pOutputStastic)
 //       streamerStatisticianRelease(pHandle->pOutputStastic);

    pthread_mutex_destroy(&pHandle->listenerMutex);
    free(pHandle);
    return;
}



