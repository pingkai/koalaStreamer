#include <string.h>
#include "MediaMuxer.h"
#include "../utils/logger.h"
#include "koala_muxer.h"
#include <stdlib.h>
#include "koala_common.h"
#include <libavutil/log.h>

typedef struct koala_dele_t {
    koala_muxer *pKoalaHandle;
    const char *outName;
    const char *outFormat;
    int elem;
    audio_data_callback acb;
    video_data_callback vcb;
    void *acbarg;
    void *vcbarg;
} koala_dele;

static void KoalaMuxer_close(koala_dele *pHandle);

static int audio_callback(void *arg, int cmd, void **data, int *size) {
    koala_dele *pHandle = (koala_dele *) arg;
    if (pHandle->acb) {
        return pHandle->acb(pHandle->acbarg, cmd, data, size);
    }
    return -1;

}

static int video_callback(void *arg, int cmd, void **data, int *size) {
    koala_dele *pHandle = (koala_dele *) arg;
    if (pHandle->vcb) {
        return pHandle->vcb(pHandle->vcbarg, cmd, data, size);
    }
    return -1;
}
static void callback(int level, const char *buf) {
    switch (level) {
        case AV_LOG_TRACE:
        case AV_LOG_DEBUG:
        case AV_LOG_VERBOSE:
            level = LOG_LEVEL_DEBUG;
            break;
        case AV_LOG_INFO:
            level = LOG_LEVEL_INFO;
            break;
        case AV_LOG_WARNING:
            level = LOG_LEVEL_WARNING;
            break;
        default :
            level = LOG_LEVEL_ERROR;
            break;
    }
    _log_print(level, "KOALA", "%s", buf);
    return;
}
static int probe() {
    regist_log_call_back(callback);
    koala_set_log_level(KOALA_LOG_TRACE);
    return 1;
}

static void set_callback(koala_dele *pHandle, Stream_type type, void *func, void *arg) {
    switch (type) {
        case STREAM_TYPE_AUDIO:
            pHandle->acb = func;
            pHandle->acbarg = arg;
            break;
        case STREAM_TYPE_VIDEO:
            pHandle->vcb = func;
            pHandle->vcbarg = arg;
            break;
        default:
            break;
    }
}

static void *KoalaMuxer_open(const char *outName, const char *outFormat, int elem) {
    koala_dele *pHandle = malloc(sizeof(koala_dele));
    //   int ret;
    memset(pHandle, 0, sizeof(koala_dele));
    pHandle->pKoalaHandle = koala_get_muxer_handle();
    // TODO: strdup them
    pHandle->outName = outName;
    pHandle->outFormat = outFormat;
    pHandle->elem = elem;

    return pHandle;
}

static int KoalaMuxer_prepare(koala_dele *pHandle) {
    int ret;
    int have_video = pHandle->elem & ELEMENT_VIDEO;
    int have_audio = pHandle->elem & ELEMENT_AUDIO;
    koala_muxer_reg_data_callback(pHandle->pKoalaHandle,
                                  have_audio ? audio_callback : NULL, pHandle,
                                  have_video ? video_callback : NULL, pHandle);
    TRACE;
    ret = koala_muxer_init_open(pHandle->pKoalaHandle, pHandle->outName, pHandle->outFormat);
    TRACE;
    if (ret < 0) {
        LOGE("KoalaMuxer_open error\n");
        //   KoalaMuxer_close(pHandle);

    }
    return ret;
}

static char * KoalaMuxer_getHostIp(koala_dele *pHandle){
    return koala_muxer_get_hostip(pHandle->pKoalaHandle);
}

static int KoalaMuxer_start(koala_dele *pHandle) {
    return koala_muxer_start(pHandle->pKoalaHandle);
}
static void KoalaMuxer_end(koala_dele *pHandle) {
    koala_muxer_end(pHandle->pKoalaHandle);
}

static void KoalaMuxer_close(koala_dele *pHandle) {
    if (pHandle->pKoalaHandle)
        koala_muxer_close(pHandle->pKoalaHandle);
    free(pHandle);
}

static int KoalaMuxer_setopt(koala_dele *pHandle, const char *key, const char *value) {
    if (NULL == pHandle->pKoalaHandle) {
        return -1;
    }
//    LE_LOGE("%s key:%s value:%s\n", __FUNCTION__, key, value);
    return koala_muxer_setopt(pHandle->pKoalaHandle, key, value);
}

static int KoalaMuxer_set_listener(koala_dele *pHandle, void *listener, void *arg) {
    if (NULL == pHandle->pKoalaHandle) {
        return -1;
    }
//    LE_LOGE("%s key:%s value:%s\n", __FUNCTION__, key, value);
    return koala_muxer_set_listener(pHandle->pKoalaHandle, listener, arg);
}


muxer_delegate KoalaMuxer_delegate = {
        .probe  = probe,
        .set_callback = set_callback,
        .open   = KoalaMuxer_open,
        .prepare = KoalaMuxer_prepare,
        .getHostIp = KoalaMuxer_getHostIp,
        .start  = KoalaMuxer_start,
        .end    = KoalaMuxer_end,
        .close  = KoalaMuxer_close,
        .setopt = KoalaMuxer_setopt,
        .set_listener = KoalaMuxer_set_listener,
};
