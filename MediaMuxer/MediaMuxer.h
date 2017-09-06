//
// Created by 平凯 on 2017/8/1.
//

#ifndef STREAMER_MEDIAMUXER_H
#define STREAMER_MEDIAMUXER_H

#include "../KoalaPlayer/include/koala_type.h"

#include "../MediaStreamer.h"

#define ELEMENT_AUDIO 1<< 0
#define ELEMENT_VIDEO 1<< 1
#define ELEMENT_ALL ELEMENT_AUDIO|ELEMENT_VIDEO
#define ELEMENT_NONE 0
typedef struct koala_dele_t koala_dele;

typedef void delcnt;
typedef struct muxer_delegate_t {

    int (*probe)();

    delcnt *(*open)(const char *outName, const char *outFormat, int elem);

    void (*set_callback)(koala_dele *pHandle, Stream_type type, void *func, void *arg);

    int (*prepare)(koala_dele *pHandle);
    
    char * (*getHostIp)(koala_dele *pHandle);

    int (*start)(koala_dele *pHandle);

    void (*end)(koala_dele *pHandle);

    void (*close)(koala_dele *pHandle);

    int (*setopt)(koala_dele *pHandle, const char *key, const char *value);

    int (*set_listener)(koala_dele *pHandle, void *listener, void *arg);
} muxer_delegate;

typedef struct muxer_t muxer;

muxer *create_muxer_handle();

int muxer_setopt(muxer *pHandle, const char *key, const char *value);

int muxer_open(muxer *pHandle, const char *outName, const char *outFormat, int elem);

int muxer_prepare(muxer *pHandle);

char* muxer_getHostIp(muxer *pHandle);

int muxer_start(muxer *pHandle);

void muxer_end(muxer *pHandle);

void muxer_close(muxer *pHandle);


void muxer_set_callback(muxer *pHandle, Stream_type type, void *func, void *arg);

int muxer_set_listener(muxer *pHandle, void *listener, void *arg);

void muxer_release_handle(muxer *pHandle);

#endif //STREAMER_MEDIAMUXER_H
