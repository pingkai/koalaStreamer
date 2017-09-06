//
// Created by 平凯 on 2017/8/1.
//

#ifndef STREAMER_LOGGER_H
#define STREAMER_LOGGER_H


#include <stdint.h>

#define LOG_LEVEL_DEBUG   1
#define LOG_LEVEL_INFO    2
#define LOG_LEVEL_WARNING 4
#define LOG_LEVEL_ERROR   8
#define LOG_LEVEL_FATAL   16

#ifndef LOG_TAG
#define LOG_TAG "LEC"
#endif

typedef void(*log_back)(int prio, char *buf);
//#define printf "can't use printf,use LE_LOG* please"
#ifdef __cplusplus
extern "C"{
#endif

int _log_print(int prio, const char *tag, const char *fmt, ...);

//void lec_log_set_back(lec_log_back func, void *arg);
//
//void lec_log_set_back2(lec_log_back func, void *arg);

void log_set_level(int level, int disable_console);

#ifdef __cplusplus
}

#endif

#ifndef NOLELOGI
#define LOGI(...) _log_print(LOG_LEVEL_INFO,LOG_TAG,__VA_ARGS__)
#else
#define LOGI(...)
#endif
#ifndef NOLELOGD
#define LOGD(...) _log_print(LOG_LEVEL_DEBUG,LOG_TAG,__VA_ARGS__)
#else
#define LOGD(...)
#endif
#ifndef NOLELOGW
#define LOGW(...) _log_print(LOG_LEVEL_WARNING,LOG_TAG,__VA_ARGS__)
#else
#define LOGW(...)
#endif
#define LOGE(...) _log_print(LOG_LEVEL_ERROR,LOG_TAG,__VA_ARGS__)
#define LOGF(...) _log_print(LOG_LEVEL_FATAL,LOG_TAG,__VA_ARGS__)

#define TRACE do { LOGD("%s:%d\n",__func__,__LINE__);} while(0)

#define DUMP_INT(LINE) LOGD("%s is %d\n",#LINE,LINE)

#ifdef __cplusplus
extern "C"{
#endif

static inline void lec_hex_dump(uint8_t *pBuffer, int size) {
    int i;
    for (i = 0; i < size; i++)
        LOGD("%02x ", pBuffer[i]);
    LOGD("\n");
}

#ifdef __cplusplus
}
#endif

#endif //STREAMER_LOGGER_H
