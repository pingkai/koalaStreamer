//
// Created by 平凯 on 2017/8/18.
//

#ifndef STREAMER_MEDIAFRAME_H
#define STREAMER_MEDIAFRAME_H

#include <ntsid.h>
#include <stdint.h>

typedef struct mediaFrame_t mediaFrame;

typedef void (*FrameRelease)(void *arg,mediaFrame* pFrame);
typedef struct mediaFrame_t{
    uint8_t *pBuffer;
    int size;
    int streamIndex;
    int64_t pts;
    int64_t dts;
    int flag;
    int duration;
    int64_t pos;

    FrameRelease release;
    void* release_arg;

    void *pAppending;
}mediaFrame;
#ifdef __cplusplus
extern "C"{
#endif

mediaFrame *mediaFrameCreate(uint32_t size);

void mediaFrame_setRelease(mediaFrame *pFrame, FrameRelease func, void *arg);

mediaFrame *mediaFrameCreateFromBuffer(uint8_t *buffer, uint32_t size);

void mediaFrameRelease(mediaFrame *pFrame);

#ifdef __cplusplus
}
#endif

#endif //STREAMER_MEDIAFRAME_H
