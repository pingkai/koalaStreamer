//
//  streamStatistician.c
//
//  Created by 平凯 on 2017/8/9.
//  Copyright © 2017年 PingKai all rights reserved.
//

#include "streamStatistician.h"
#include "utils/logger.h"
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

typedef struct statistics_data_t {
  //  int32_t  bit_rate;
  //  float bwFactor;
    uint64_t size;
    uint64_t frames;
    uint64_t wait_ms;
    uint64_t wait_times;
    int64_t  start_time;
}statistics_data;

typedef struct stream_statistics_t{
    statistics_data total;
    statistics_data statistics;
    int64_t frame_start_time;
    Stream_type type;
    
}stream_statistics;

typedef struct streamerStatistician_t{
    int streamerNum;
    stream_statistics* streamers[MAX_STREAMES];
    int64_t  start_time;
    
}streamerStatistician;

static int64_t get_time(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t) tv.tv_sec * 1000000 + tv.tv_usec;
}


streamerStatistician *streamerStatisticianCreate(){
    streamerStatistician *pHandle = malloc(sizeof(streamerStatistician));
    memset(pHandle,0,sizeof(streamerStatistician));
    return pHandle;
}

int streamerStatistician_addStream(streamerStatistician *pHandle, Stream_type type){
    if (pHandle->streamerNum >= MAX_STREAMES)
        return -1;
    pHandle->streamers[pHandle->streamerNum] = malloc(sizeof(stream_statistics));
    memset(pHandle->streamers[pHandle->streamerNum], 0, sizeof(stream_statistics));
    pHandle->streamers[pHandle->streamerNum]->type = type;
    return pHandle->streamerNum++;
}

void streamerStatistician_onFrameBegin(streamerStatistician *pHandle, int index, mediaFrame *frame){
    if (index >= MAX_STREAMES)
        return;
    (void) frame;
    stream_statistics* pStream = pHandle->streamers[index];
    if (pStream == NULL)
        return;
    if (pHandle->start_time == 0)
        pHandle->start_time = get_time();
    if (pStream->statistics.start_time == 0)
        pStream->statistics.start_time = get_time();
    pStream->frame_start_time = get_time();
    return;
}

void streamerStatistician_onFrameEnd(streamerStatistician *pHandle, int index, mediaFrame *frame){
    if (index >= MAX_STREAMES || frame == NULL)
        return;
    stream_statistics* pStream = pHandle->streamers[index];
    if (pStream == NULL)
        return;
    int64_t timeNow = get_time();
    int64_t delayTimeMs = (timeNow - pStream->frame_start_time)/1000;
    
    pStream->statistics.frames++;
    pStream->statistics.size += frame->size;
    
    // TODO: use it?
    if (delayTimeMs >= 10){
        pStream->statistics.wait_times++;
        pStream->statistics.wait_ms += delayTimeMs;
    }
    return;
}

void streamerStatisticianRelease(streamerStatistician *pHandle){
    int i;
    if (pHandle == NULL)
        return;
    for(i = 0; i < pHandle->streamerNum; i++){
        if(pHandle->streamers[i] != NULL)
            free(pHandle->streamers[i]);
    }
    free(pHandle);
    return;
}

uint32_t streamerStatistician_getStreamBitRate(streamerStatistician *pHandle, int index) {
    stream_statistics* pStream = pHandle->streamers[index];
    if (pStream == NULL){
        TRACE;
        return 0;
    }
    int64_t  timeNow = get_time();
    float time = (timeNow - pStream->statistics.start_time)/1000000;
    if (time == 0)
        return 0;
    return (uint32_t) ((float)pStream->statistics.size * 8 / time);
}

float streamerStatistician_getStreamBandWideFactor(streamerStatistician *pHandle) {
    uint64_t  wait_ms = 0;
    int64_t  start_time = 0;
    for (int i = 0; i < pHandle->streamerNum; ++i) {
        if (pHandle->streamers[i] != NULL) {
            wait_ms += pHandle->streamers[i]->statistics.wait_ms;
            start_time = pHandle->streamers[i]->statistics.start_time;
        }
    }
    int64_t time = get_time() - start_time;

    return  1.0f  - (((float)wait_ms /1000) /((float)time/1000000));
}

void streamerStatistician_reset(streamerStatistician *pHandle){
    for (int i = 0; i < pHandle->streamerNum; ++i)
        if (pHandle->streamers[i] != NULL) {
            pHandle->streamers[i]->total.size        += pHandle->streamers[i]->statistics.size;
            pHandle->streamers[i]->total.frames      += pHandle->streamers[i]->statistics.frames;
            pHandle->streamers[i]->total.wait_ms     += pHandle->streamers[i]->statistics.wait_ms;
            pHandle->streamers[i]->total.wait_times  += pHandle->streamers[i]->statistics.wait_times;

            memset(&(pHandle->streamers[i]->statistics), 0, sizeof(statistics_data));
            pHandle->streamers[i]->statistics.start_time = get_time();
        }
    return;


}

