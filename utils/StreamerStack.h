//
// Created by 平凯 on 2017/8/24.
//

#ifndef STREAMER_STACK_H
#define STREAMER_STACK_H

#include "autoLock.h"

//TODO: add thread LOCK
template<class T>

class StreamerStack {
public:
    StreamerStack(int capacity) {
        mPData = new T[capacity];
        mSize = 0;
        mCapacity = capacity;
        pthread_mutex_init(&mMutex, NULL);
    }

    ~StreamerStack() {
        delete[] mPData;
        pthread_mutex_destroy(&mMutex);
    }

    int push(T data) {
        autoLock lock(mMutex);
        if (mSize == mCapacity)
            return -1;
        mPData[mSize++] = data;
        return 0;
    }

    int size() {
        return mSize;
    }

    T pop() {
        autoLock lock(mMutex);
        if (mSize == 0)
            return nullptr;
        return mPData[--mSize];
    }

private:
    T *mPData;
    int mCapacity;
    int mSize;
    pthread_mutex_t mMutex;
};


template <class T>
class StreamerFiFo{
public:
    StreamerFiFo(int capacity = 100);
    ~StreamerFiFo();
    int size();
    bool IsEmpty() {
        return mSize == 0;
    }

    bool IsFull() {
        return mSize == mCapacity;
    }

    int put(T data);
    T get();
    T peak();

private:
    int mCapacity;
    int mHeader;
    int mTail;
    T *mData;
    int mSize;
};
template <class T>
StreamerFiFo<T>::StreamerFiFo(int capacity) {
    mHeader = mTail = mSize =0;
    mCapacity = capacity;
    mData = new T[mCapacity];
}
template <class T>
StreamerFiFo<T>::~StreamerFiFo() {
    delete [] mData;
}
template <class T>
int StreamerFiFo<T>::size() {
    return mSize;
}
template <class T>
int StreamerFiFo<T>::put(T data) {
    if (IsFull())
        return -1;
    mData[mTail] = data;
    mTail++;
    mTail %= mCapacity;
    mSize ++;
    return 0;
}
template <class T>
T StreamerFiFo<T>::get() {
    if (IsEmpty())
        return NULL;
    T data = mData[mHeader];
    mHeader++;
    mHeader %= mCapacity;
    mSize--;
    return data;
}
template <class T>
T StreamerFiFo<T>::peak() {
    if (IsEmpty())
        return NULL;
    return mData[mHeader];
}

#endif //STREAMER_STACK_H
