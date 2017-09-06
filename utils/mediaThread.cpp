//
// Created by 平凯 on 2017/8/21.
//
#include <zconf.h>
#include <cerrno>
#include <sys/time.h>
#include <stdlib.h>
#include "mediaThread.h"
#include "logger.h"

static int make_abstime_latems(struct timespec *pAbstime, uint32_t ms) {
    struct timeval delta;
    gettimeofday(&delta, NULL);
    pAbstime->tv_sec = delta.tv_sec + (ms / 1000);
    pAbstime->tv_nsec = (delta.tv_usec + (ms % 1000) * 1000) * 1000;
    if (pAbstime->tv_nsec > 1000000000l) {
        pAbstime->tv_sec += 1;
        pAbstime->tv_nsec -= 1000000000l;
    }
    return 0;
}

mediaFrameQueue::mediaFrameQueue(int capacity) : mCapacity(capacity),mQueue(capacity) {
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&push_cond, NULL);
    pthread_cond_init(&pop_cond, NULL);
}

mediaFrameQueue::~mediaFrameQueue() {
    clean();
    pthread_cond_destroy(&pop_cond);
    pthread_cond_destroy(&push_cond);
    pthread_mutex_destroy(&mutex);
}

int mediaFrameQueue::getSize() {
    return mQueue.size();
}

int mediaFrameQueue::enqueue(mediaFrame *pFrame) {
    pthread_mutex_lock(&mutex);
    if (mQueue.IsFull()) {
        int ret;
        struct timespec abstime;
        make_abstime_latems(&abstime, 10);
        ret = pthread_cond_timedwait(&push_cond, &mutex, &abstime);
        if (ret == ETIMEDOUT) {
            pthread_mutex_unlock(&mutex);
            //LOGW("ETIMEDOUT\n");
            return -ETIMEDOUT;
        }
    }
    mQueue.put(pFrame);
    pthread_cond_signal(&pop_cond);
    pthread_mutex_unlock(&mutex);
    return 0;
}

mediaFrame *mediaFrameQueue::dequeue() {
    pthread_mutex_lock(&mutex);
    if (mQueue.IsEmpty()) {
        struct timespec abstime;
        int ret;
        make_abstime_latems(&abstime, 10);
        ret = pthread_cond_timedwait(&pop_cond, &mutex, &abstime);
        if (ret == ETIMEDOUT) {
            pthread_mutex_unlock(&mutex);
            return NULL;
        }
    }
    mediaFrame *pFrame = mQueue.get();
    pthread_cond_signal(&push_cond);
    pthread_mutex_unlock(&mutex);
    return pFrame;
}

void mediaFrameQueue::clean() {
    return cleanIf(NULL);
}

void mediaFrameQueue::cleanIf(cleanIt func, void* arg) {
    int flag = 0;
    if (func == NULL)
        flag = CLEAN_FLAG_CLEAN;
    pthread_mutex_lock(&mutex);
    while (mQueue.size() > 0) {
        mediaFrame *pFrame = mQueue.peak();
        if (func)
            flag = func(arg,pFrame);
        if (flag & CLEAN_FLAG_CLEAN) {
            mQueue.get();
            mediaFrameRelease(pFrame);
        }
        if (flag & CLEAN_FLAG_STOP)
            break;
    }
    pthread_cond_signal(&push_cond);
    pthread_mutex_unlock(&mutex);
}

mediaThread::mediaThread(const char *name) : mStatus(threadStatusNULL), mFrameInputQueue(NULL),
                                             mFrameOutPutQueue(NULL) {
    mPFrameStack = NULL;

}

mediaThread::~mediaThread() {
    stop();
}

void *mediaThread::run(void *arg) {
    mediaThread *thread = reinterpret_cast<mediaThread *>(arg);
    while (thread->mStatus != threadStatusStop) {
        if (thread->mStatus == threadStatusPause) {
            usleep(10000);
            continue;
        }
        thread->func();
    }
    return nullptr;
}

int mediaThread::start() {
    if (mStatus == threadStatusNULL) {
        mStatus = threadStatusRuning;
        return pthread_create(&mThreadId, NULL, run, this);
    }
    return -1;
}

int mediaThread::pause() {
    if (mStatus == threadStatusRuning)
        mStatus = threadStatusPause;
    return 0;
}

int mediaThread::stop() {
    if (mStatus == threadStatusPause || mStatus == threadStatusRuning) {
        mStatus = threadStatusStop;
        pthread_join(mThreadId, NULL);
    }
    return 0;
}

void mediaThread::func() {

    mediaFrame *pFrame = NULL;
    pFrame = mFrameInputQueue->dequeue();
    if (pFrame) {
        pFrame = deal(pFrame);
    }

    while (pFrame != NULL) {
        int ret;
        ret = mFrameOutPutQueue->enqueue(pFrame);
        if (ret < 0) {
            if (ret == -ETIMEDOUT && mStatus != threadStatusStop) {
              //  LOGW("enqueue buffer time out");
            } else {
                mediaFrameRelease(pFrame);
                pFrame = NULL;
            }
        } else
            pFrame = NULL;
    }
}

void mediaThread::setInputQueue(mediaFrameQueue *pQueue) {
    mFrameInputQueue = pQueue;
    return;
}

void mediaThread::setOutputQueue(mediaFrameQueue *pQueue) {
    mFrameOutPutQueue = pQueue;
    return;
}

void mediaThread::setMediaFrameStack(StreamerStack<mediaFrame *> *pFrameStack) {
    mPFrameStack = pFrameStack;

}

mediaFrame *mediaThread::getMediaFrame() {
    mediaFrame *pFrame = NULL;
    if (mPFrameStack)
        pFrame = mPFrameStack->pop();
    if (pFrame == NULL){
        pFrame = mediaFrameCreate(0);
    }
    return pFrame;
}

void mediaThread::putMediaFrame(mediaFrame *pFrame) {
    if (!mPFrameStack || mPFrameStack->push(pFrame) < 0)
        free(pFrame);
}
