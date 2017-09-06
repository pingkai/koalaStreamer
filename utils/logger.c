//
// Created by 平凯 on 2017/8/1.
//


#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#ifdef ANDROID
#include <android/log.h>
#define ANDROID_APP_TAG "LECMediaPlayer"
#endif

#include "logger.h"
//#include "../version.h"
#define VERSION "--"

#undef printf

#define NONE         "\033[m"
#define RED          "\033[0;32;31m"
#define LIGHT_RED    "\033[1;31m"
#define GREEN        "\033[0;32;32m"
#define LIGHT_GREEN  "\033[1;32m"
#define BLUE         "\033[0;32;34m"
#define LIGHT_BLUE   "\033[1;34m"
#define DARY_GRAY    "\033[1;30m"
#define CYAN         "\033[0;36m"
#define LIGHT_CYAN   "\033[1;36m"
#define PURPLE       "\033[0;35m"
#define LIGHT_PURPLE "\033[1;35m"
#define BROWN        "\033[0;33m"
#define YELLOW       "\033[1;33m"
#define LIGHT_GRAY   "\033[0;37m"
#define WHITE        "\033[1;37m"


typedef struct log_ctrl_t {
    int log_level;
    int disable_console;

    void (*log_out)(void *arg, int prio, char *buf);

    void *log_out_arg;
    int disabled;

    void (*log_out2)(void *arg, int prio, char *buf);

    void *log_out_arg2;
    int disabled2;
} log_ctrl;

static log_ctrl logCtrl = {0,};

static int inited = 0;
static pthread_mutex_t log_mutex;

static const char *gitVer = NULL;

#ifndef ANDROID

pid_t gettid(void) {
#if (defined TARGET_IOS || defined MACOSX)
    return pthread_mach_thread_np(pthread_self());
#else
    return syscall(SYS_gettid);
#endif

}

#endif

#ifdef ANDROID

static int  lec_lev2android_lev(int prio){
    int lev;
    switch (prio){
    case LOG_LEVEL_DEBUG:
        lev = ANDROID_LOG_DEBUG;
    //    break;                //letv phone can't print debug info
    case LOG_LEVEL_INFO:
        lev = ANDROID_LOG_INFO;
        break;
    case LOG_LEVEL_WARNING:
        lev = ANDROID_LOG_WARN;
        break;
    case LOG_LEVEL_ERROR:
        lev = ANDROID_LOG_ERROR;
        break;
    case LOG_LEVEL_FATAL:
        lev = ANDROID_LOG_FATAL;
        break;
    default:
        lev = ANDROID_LOG_DEFAULT;
        break;
    }
    return lev;
}
#endif


static void get_local_time(char *buffer) {
    struct timeval t;
    gettimeofday(&t, NULL);
    struct tm *ptm = localtime(&t.tv_sec);
    sprintf(buffer, "%02d-%02d %02d:%02d:%02d.%03d",
            ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec, t.tv_usec / 1000);
    return;

}

static char get_char_lev(int lev, char **ctrl) {
    *ctrl = NONE;
    switch (lev) {
        case LOG_LEVEL_INFO:
            return 'I';
        case LOG_LEVEL_DEBUG:
            return 'D';
        case LOG_LEVEL_WARNING:
            *ctrl = YELLOW;
            return 'W';
        case LOG_LEVEL_ERROR:
            *ctrl = RED;
            return 'E';
        case LOG_LEVEL_FATAL:
            *ctrl = LIGHT_RED;
            return 'F';
        default:
            return ' ';
    }
    return ' ';
}

#ifndef ANDROID

static void linux_print(int prio, char *printf_buf) {
    char time_buffer[32];
    char lev_c;
    int tid = gettid();
    char *ctr;
    get_local_time(time_buffer);
    lev_c = get_char_lev(prio, &ctr);
#ifndef TARGET_IOS
    printf(ctr);
#endif
    printf("%s", printf_buf);
#ifndef TARGET_IOS
    printf(NONE);
#endif
}

#endif

static void format_log(int prio, const char *tag, char *cont_buf, char *out_buf) {
    char time_buffer[32];
    char lev_c;
    int tid = gettid();
    int pid = getpid();
    char *ctr;
    get_local_time(time_buffer);
    lev_c = get_char_lev(prio, &ctr);
    sprintf(out_buf, "%s %d %d %c [%s] [%s]: %s", time_buffer, pid, tid, lev_c, gitVer, tag, cont_buf);
    int len = (int) strlen(out_buf);
    if (out_buf[len - 1] != '\n') {
        out_buf[len] = '\n';
        out_buf[len + 1] = '\0';
    }
    return;
}

int _log_print(int prio, const char *tag, const char *fmt, ...) {

    if (prio < LOG_LEVEL_ERROR && prio < logCtrl.log_level)
        return 0;

    if (!inited) {
        pthread_mutex_init(&log_mutex, NULL);
        gitVer = strrchr(VERSION, 'g');
        if (gitVer == NULL)
            gitVer = VERSION;
        logCtrl.log_level = LOG_LEVEL_DEBUG;
        inited = 1;
    }

    pthread_mutex_lock(&log_mutex);
#ifdef ANDROID
    int  lev = lec_lev2android_lev(prio);
#endif
    static char printf_buf[1024];
    static char out_buf[1024 + 256];
    va_list args;
    int printed;
    va_start(args, fmt);
    printed = vsnprintf(printf_buf, 1023, fmt, args);
    va_end(args);
    format_log(prio, tag, printf_buf, out_buf);
    if (logCtrl.log_out) {
        logCtrl.log_out(logCtrl.log_out_arg, prio, out_buf);
    }
    if (logCtrl.log_out2)
        logCtrl.log_out2(logCtrl.log_out_arg2, prio, out_buf);
    if (!logCtrl.disable_console) {

#ifdef ANDROID
        __android_log_print(lev,ANDROID_APP_TAG,"[%s] [%s] :%s",gitVer,tag,printf_buf);
#else
        linux_print(prio, out_buf);
#endif
    }
    pthread_mutex_unlock(&log_mutex);
    return 0;
}

//void lec_log_set_back(lec_log_back func, void *arg) {
//    logCtrl.log_out = func;
//    logCtrl.log_out_arg = arg;
//}
//
//void lec_log_set_back2(lec_log_back func, void *arg) {
//    logCtrl.log_out2 = func;
//    logCtrl.log_out_arg2 = arg;
//}

void log_set_level(int level, int disable_console) {
    logCtrl.log_level = level;
    logCtrl.disable_console = disable_console;
    return;
}

