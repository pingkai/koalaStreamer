//
// Created by 平凯 on 2017/8/18.
//
#include <stdlib.h>
#include <string.h>
#include "mediaFrame.h"


static void default_release(void* arg,mediaFrame * pFrame){
    if (pFrame->pBuffer)
        free(pFrame->pBuffer);
    free(pFrame);
}


mediaFrame *mediaFrameCreate(uint32_t size) {
    mediaFrame * pFrame = malloc(sizeof(mediaFrame));
    memset(pFrame,0,sizeof(mediaFrame));
    if (size){
        pFrame->pBuffer = malloc(size);
        pFrame->size = size;
    }
    return pFrame;
}

mediaFrame *mediaFrameCreateFromBuffer(uint8_t* buffer, uint32_t size) {
    mediaFrame * pFrame = malloc(sizeof(mediaFrame));
    memset(pFrame,0,sizeof(mediaFrame));
    pFrame->pBuffer = buffer;
    pFrame->size = size;
    return pFrame;
}

void mediaFrame_setRelease(mediaFrame* pFrame,FrameRelease func, void* arg){
    pFrame->release = func;
    pFrame->release_arg   = arg;
}

void mediaFrameRelease(mediaFrame *pFrame) {
    if (pFrame->release)
        return pFrame->release(pFrame->release_arg,pFrame);
    return default_release(NULL,pFrame);
}
