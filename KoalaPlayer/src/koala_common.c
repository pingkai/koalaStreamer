
#include <libavformat/avformat.h>
#include "koala_common.h"
#include <pthread.h>
//#include "libavformat/url.h"
//#ifdef ANDROID
//extern URLProtocol ff_curl_http_protocol;
//extern URLProtocol ff_curl_https_protocol;
extern AVInputFormat koala_rtsp_demuxer;
//#endif
static void (*av_log_callback)(int, const char*) = NULL;

static int av_log_level_cb    = AV_LOG_INFO;
static int koala_log_level = KOALA_LOG_INFO;

static int levelKoala2av(int koala_level){
    switch (koala_level){
        case KOALA_LOG_FATAL:
            return AV_LOG_FATAL;
        case KOALA_LOG_ERROR:
            return AV_LOG_ERROR;
        case KOALA_LOG_WARNING:
            return AV_LOG_WARNING;
        case KOALA_LOG_INFO:
            return AV_LOG_INFO;
        case KOALA_LOG_DEBUG:
            return AV_LOG_DEBUG;
        case KOALA_LOG_TRACE:
            return AV_LOG_TRACE;
        default :
            return AV_LOG_INFO;
    }
}

int koala_set_log_level(int level){
    koala_log_level = level;
    av_log_level_cb = levelKoala2av(level);
    return 0;
}

pthread_mutex_t * creat_mutex(){
  //  printf("%s\n",__func__);
    pthread_mutex_t * pMute = (pthread_mutex_t *) malloc (sizeof(pthread_mutex_t));
    if (pMute == NULL)
        return NULL;
    pthread_mutex_init(pMute, NULL);
    return pMute;
}

static int lock_mutex(pthread_mutex_t *pMute){
  //  printf("%s\n",__func__);
    return -pthread_mutex_lock(pMute);
}

static int unlock_mutex(pthread_mutex_t *pMute){
  //  printf("%s\n",__func__);
    return pthread_mutex_unlock(pMute);
}
static void destroy_mutex(pthread_mutex_t *pMute){
//    printf("%s\n",__func__);
    pthread_mutex_destroy(pMute);
    free(pMute);
    pMute = NULL;
}

static int lockmgr(void **mtx, enum AVLockOp op)
{
   switch(op) {
      case AV_LOCK_CREATE:
          *mtx = creat_mutex();
          if(!*mtx)
              return 1;
          return 0;
      case AV_LOCK_OBTAIN:
          return !!lock_mutex(*mtx);
      case AV_LOCK_RELEASE:
          return !!unlock_mutex(*mtx);
      case AV_LOCK_DESTROY:
          destroy_mutex(*mtx);
          return 0;
   }
   return 1;
}

void regist_log_call_back(void (*callback)(int, const char*)){
    av_log_callback = callback;
    return;
}

static void log_back (void *ptr, int level, const char *fmt, va_list vl){
    static char line[1024];
    AVClass* avc = ptr ? *(AVClass **) ptr : NULL;
    if (av_log_callback == NULL)
        return;
    if (level >av_log_level_cb)
        return;
    line[0] = 0;
    if (avc) {
        if (avc->parent_log_context_offset) {
            AVClass** parent = *(AVClass ***) (((uint8_t *) ptr) +
                                   avc->parent_log_context_offset);
            if (parent && *parent) {
                snprintf(line, sizeof(line), "[%s @ %p] ",
                         (*parent)->item_name(parent), parent);
            }
        }
        snprintf(line + strlen(line), sizeof(line) - strlen(line), "[%s @ %p] ",
                 avc->item_name(ptr), ptr);
    }

    vsnprintf(line + strlen(line), sizeof(line) - strlen(line), fmt, vl);

    av_log_callback(level,line);
}
void koala_log(int prio, const char *fmt, ...){
    char printf_buf[1024];
    if (av_log_callback == NULL)
        return;

    if (prio > koala_log_level)
        return;
    va_list args;
    int printed;
    va_start(args, fmt);
    printed = vsprintf(printf_buf, fmt, args);
    va_end(args);
    av_log_callback(prio,printf_buf);

    return;
}
void koala_init_ffmpeg(){
    static int inited = 0;
    if (!inited){
        av_lockmgr_register(lockmgr);
        av_log_set_level(AV_LOG_VERBOSE);
        av_log_set_callback(log_back);
#ifndef _NOEXTERN_LIB
//        ffurl_register_protocol(&ff_curl_http_protocol);
//        ffurl_register_protocol(&ff_curl_https_protocol);
//        av_register_input_format(&koala_rtsp_demuxer);
#endif
        av_register_all();
        avformat_network_init();
        inited = 1;
    }
    return;

}
typedef struct codec_paire_t{
    enum KoalaCodecID  klId;
    enum AVCodecID     avId;
}codec_paire;

static  codec_paire codec_paire_table[]=
{
    {KOALA_CODEC_ID_AAC,AV_CODEC_ID_AAC},
    {KOALA_CODEC_ID_AC3,AV_CODEC_ID_AC3},
    {KOALA_CODEC_ID_EAC3,AV_CODEC_ID_EAC3},
    {KOALA_CODEC_ID_MP3,AV_CODEC_ID_MP3},
    {KOALA_CODEC_ID_MP2,AV_CODEC_ID_MP2},
    {KOALA_CODEC_ID_MP1,AV_CODEC_ID_MP1},
    {KOALA_CODEC_ID_PCM_MULAW,AV_CODEC_ID_PCM_MULAW},
    {KOALA_CODEC_ID_APE,AV_CODEC_ID_APE},
    {KOALA_CODEC_ID_FLAC,AV_CODEC_ID_FLAC},
    {KOALA_CODEC_ID_H264,AV_CODEC_ID_H264},
    {KOALA_CODEC_ID_HEVC,AV_CODEC_ID_HEVC},
    {KOALA_CODEC_ID_NONE,AV_CODEC_ID_NONE},
    {KOALA_CODEC_ID_PCM_S16LE,AV_CODEC_ID_PCM_S16LE},
};

enum AVCodecID koalaCodecID2AVCodecID(enum KoalaCodecID codec){

    int num = sizeof(codec_paire_table)/sizeof(codec_paire_table[0]);
    int i;

    for (i = 0; i< num; i++){
        if (codec_paire_table[i].klId == codec)
            return codec_paire_table[i].avId;
    }
    koala_logd("koala codec %d not found\n",codec);
    return AV_CODEC_ID_NONE;
}
