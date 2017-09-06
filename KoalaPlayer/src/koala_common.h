
#ifndef KOALA_COMMON_H
#define KOALA_COMMON_H
#include "../include/koala_type.h"
void regist_log_call_back(void (*callback)(int, const char*));
void koala_init_ffmpeg();
#define KOALA_LOG_FATAL   1
#define KOALA_LOG_ERROR   2
#define KOALA_LOG_WARNING 4
#define KOALA_LOG_INFO    8
#define KOALA_LOG_DEBUG   16
#define KOALA_LOG_TRACE   32
void koala_log(int prio, const char *fmt, ...);

#define koala_logd(...) koala_log(KOALA_LOG_DEBUG,__VA_ARGS__)

#define koala_trace koala_logd("%s:%d",__FILE__,__LINE__);

int koala_set_log_level(int level);

enum AVCodecID koalaCodecID2AVCodecID(enum KoalaCodecID codec);

#endif
