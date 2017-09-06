//
// Created by 平凯 on 2017/8/1.
//

#ifndef STREAMER_HFMEDIASTREAMER_H
#define STREAMER_HFMEDIASTREAMER_H

#define ELEMENT_AUDIO 1<< 0
#define ELEMENT_VIDEO 1<< 1
#define ELEMENT_ALL ELEMENT_AUDIO|ELEMENT_VIDEO
#define ELEMENT_NONE 0

#include <stdint.h>
#include "KoalaPlayer/include/koala_type.h"
#include "utils/mediaFrame.h"

#define PREPARE_EVENT_BASE  0x1000

#define PREPARE_EVENT_OK  PREPARE_EVENT_BASE
#define PREPARE_EVENT_ERROR  PREPARE_EVENT_BASE +1

typedef struct streamer_t mediaStreamer;

typedef struct MediaStreamerCB_t{
    int element;
    int (*audio_callback)(void *arg,int cmd,void **data,int *size);
    void * acbarg;
    int (*video_callback)(void *arg,int cmd,void **data,int *size);
    void *vcbarg;
}MediaStreamerCB;
#ifdef __cplusplus
extern "C"{
#endif
mediaStreamer * create_mediaStreamer();

int mediaStreamer_set_inPut(mediaStreamer *pHandle, MediaStreamerCB *pDataCb);

int mediaStreamer_set_outPut(mediaStreamer *pHandle, const char *outName, const char *outFormat);

int mediaStreamer_prepare(mediaStreamer *pHandle);

char * mediaStreamer_getHostIp(mediaStreamer *pHandle);

int mediaStreamer_start(mediaStreamer *pHandle);

uint32_t  mediaStreamer_getStreamBitRate(mediaStreamer *pHandle, Stream_type type);

float  mediaStreamer_getBandWideFactor(mediaStreamer *pHandle);

void mediaStreamer_resetStatistician(mediaStreamer *pHandle);

void mediaStreamer_stop(mediaStreamer *pHandle);

void mediaStreamer_release(mediaStreamer *pHandle);

int mediaStreamer_setopt(mediaStreamer *pHandle, const char *key, const char *value);

typedef void (*mediaStreamerListener)(void *arg, int event);

int mediaStreamer_setListener(mediaStreamer *pHandle, mediaStreamerListener listener, void *arg);
#ifdef __cplusplus
}
#endif
#endif //STREAMER_HFSTREAMER_H
