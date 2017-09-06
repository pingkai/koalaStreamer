//
// Created by 平凯 on 2017/8/21.
//

#ifndef STREAMER_MEDIATHREAD_H
#define STREAMER_MEDIATHREAD_H


//#include <sys/_pthread/_pthread_t.h>
#include <pthread.h>
#include "mediaFrame.h"
#include "StreamerStack.h"
#include <list>
using namespace std;

#define CLEAN_FLAG_CLEAN 1 << 0
#define CLEAN_FLAG_STOP  1 << 1


typedef  int (*cleanIt)(void* arg,mediaFrame *pFrame);

class mediaFrameQueue{
public:
    mediaFrameQueue(int capacity);
    ~mediaFrameQueue();
    int getSize();
    int enqueue(mediaFrame *pFrame);
    mediaFrame * dequeue();
    void clean();
    void cleanIf(cleanIt func,void*arg = NULL);

private:
    StreamerFiFo <mediaFrame *> mQueue;
    int mCapacity;

    pthread_mutex_t mutex;
    pthread_cond_t push_cond;
    pthread_cond_t pop_cond;
};
typedef enum {
    threadStatusNULL,
    threadStatusRuning,
    threadStatusPause,
    threadStatusStop,
}threadStatus;

class mediaThread {
public:
    mediaThread(const char*name);
    void setMediaFrameStack(StreamerStack<mediaFrame *> *pFrameStac);
    int start();
    int pause();
    int stop();

    void setInputQueue(mediaFrameQueue *pQueue);
    void setOutputQueue(mediaFrameQueue *pQueue);
    virtual ~mediaThread();

    void putMediaFrame(mediaFrame *pFrame);

private:
    pthread_t mThreadId;
    static void * run(void* arg);
    void func();
    virtual mediaFrame* deal(mediaFrame *pFrame) = 0;

    threadStatus mStatus;
    mediaFrameQueue *mFrameInputQueue;
    mediaFrameQueue *mFrameOutPutQueue;
    StreamerStack <mediaFrame *> * mPFrameStack;
protected:
    mediaFrame * getMediaFrame();
};


#endif //STREAMER_MEDIATHREAD_H
