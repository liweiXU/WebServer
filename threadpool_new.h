#ifndef THREADPOOL_NEW_H
#define THREADPOOL_NEW_H

#include <pthread.h>
#include <list>
#include "locker.h"
#include <exception>
#include <cstdio>

template <typename T>
class threadpool{
public:
    threadpool(int thread_num = 9, int max_request = 10000);
    ~threadpool();
    bool append(T* request);

private: 
    static void* worker(void* arg);
    void run();

private:
    
}