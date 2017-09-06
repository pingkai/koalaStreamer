//
// Created by 平凯 on 2017/8/1.
//

#include <string.h>
// TODO: reg data call back
#include <stdlib.h>
#include "MediaMuxer.h"
#include "../utils/logger.h"
#include "../KoalaPlayer/include/koala_type.h"

typedef struct OptNode_ {
    char *key;
    char *value;
    struct OptNode_ *next;
} OptNode;

static OptNode *getOpt(OptNode **opts, const char *key) {
    OptNode *opt = *opts;
//    LOGE("opt is %p\n", opt);
    for (; NULL != opt; opt = opt->next) {
        if (!strcasecmp(key, opt->key)) {
            return opt;
        }
    }
    return NULL;
}

static int setOpt(OptNode **opts, const char *key, const char *value) {
    OptNode *opt = getOpt(opts, key);
    if (NULL == opt) {
        opt = (OptNode *) malloc(sizeof(OptNode));
        if (NULL == opt) {
            LOGE("alloc memory for opt failed!\n");
            return -1;
        }
        opt->key = strdup(key);
        opt->value = strdup(value);
        opt->next = *opts;
        *opts = opt;
    } else {
        if (opt->value) free(opt->value);
        opt->value = strdup(value);
    }
    return 0;
}

static int releaseOpts(OptNode **opts) {
    OptNode *opt = *opts;
    for (; NULL != opt;) {
        if (opt->key) free(opt->key);
        if (opt->value) free(opt->value);
        OptNode *tmp = opt;
        free(opt);
        opt = tmp->next;
    }
    return 0;
}


typedef struct muxer_t {
    muxer_delegate *mMuxer;
    delcnt *pDelcnt;
    OptNode *opts;
} muxer;

extern muxer_delegate KoalaMuxer_delegate;

muxer *create_muxer_handle() {
    muxer *pHandle;
    pHandle = malloc(sizeof(muxer));
    memset(pHandle, 0, sizeof(muxer));
    pHandle->opts = NULL;
    return pHandle;
}

int muxer_open(muxer *pHandle, const char *outName, const char *outFormat, int elem) {
    int ret;
    muxer_delegate *pDelegate = &KoalaMuxer_delegate;
    ret = pDelegate->probe();

    if (ret < 0) {
        LOGE("muxer not support\n");
        return ret;
    }
    pHandle->mMuxer = pDelegate;

    pHandle->pDelcnt = pDelegate->open(outName, outFormat, elem);

    if (pHandle->pDelcnt == NULL) {
        LOGE("lec_muxer_open open error\n");
        return -1;
    }

    OptNode *opt = pHandle->opts;
    for (; NULL != opt; opt = opt->next) {
        LOGE("%s key:%s value:%s\n", __FUNCTION__, opt->key, opt->value);
        pHandle->mMuxer->setopt(pHandle->pDelcnt, opt->key, opt->value);
    }
    TRACE;
    return 0;
}

int muxer_prepare(muxer *pHandle){

    return pHandle->mMuxer->prepare(pHandle->pDelcnt);

}

int muxer_setopt(muxer *pHandle, const char *key, const char *value) {
    if (NULL == pHandle) {
        LOGE("pHandle is null\n");
        return -1;
    }

    if (NULL == key || NULL == value) {
        LOGE("key:%p value:%p include null\n", key, value);
        return -2;
    }

//    LOGE("%s key:%s value:%s\n", __FUNCTION__, key, value);
    return setOpt(&pHandle->opts, key, value);
}


int muxer_set_listener(muxer *pHandle, void *listener, void *arg) {
    if (NULL == pHandle) {
        LOGE("pHandle is null");
        return -1;
    }
    return pHandle->mMuxer->set_listener(pHandle->pDelcnt, listener, arg);
}


void muxer_set_callback(muxer *pHandle, Stream_type type, void *func, void *arg) {
    if (pHandle->mMuxer && pHandle->pDelcnt)
        pHandle->mMuxer->set_callback(pHandle->pDelcnt, type, func, arg);
}

char* muxer_getHostIp(muxer *pHandle){
    if (pHandle->mMuxer && pHandle->pDelcnt)
        return pHandle->mMuxer->getHostIp(pHandle->pDelcnt);
    return NULL;
}

int muxer_start(muxer *pHandle) {
    if (pHandle->mMuxer && pHandle->pDelcnt)
        return pHandle->mMuxer->start(pHandle->pDelcnt);
    return 0;
}

void muxer_end(muxer *pHandle) {
    if (pHandle->mMuxer && pHandle->pDelcnt)
        return pHandle->mMuxer->end(pHandle->pDelcnt);
}

void muxer_close(muxer *pHandle) {
    if (pHandle->mMuxer && pHandle->pDelcnt) {
        pHandle->mMuxer->close(pHandle->pDelcnt);
        pHandle->pDelcnt = NULL;
    }
}

void muxer_release_handle(muxer *pHandle) {
    if (pHandle->pDelcnt)
        muxer_close(pHandle);

    releaseOpts(&pHandle->opts);
    free(pHandle);
}
