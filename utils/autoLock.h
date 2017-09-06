//
// Created by 平凯 on 2017/8/25.
//

#ifndef STREAMER_AUTOLOCK_H
#define STREAMER_AUTOLOCK_H
#include <pthread.h>
#include <mutex>

class autoLock{

public:
    autoLock(pthread_mutex_t &mutex) : mMutex(mutex) {
        pthread_mutex_lock(&mutex);
    }
    ~autoLock(){
        pthread_mutex_unlock(&mMutex);
    }

private:
    pthread_mutex_t &mMutex;
};

#endif //STREAMER_AUTOLOCK_H
