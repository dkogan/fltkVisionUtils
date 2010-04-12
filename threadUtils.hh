// -*- c++ -*-

#ifndef __THREADUTILS_HH__
#define __THREADUTILS_HH__

#include <pthread.h>
#include <iostream>

class MTmutex
{
    bool            inited;
    pthread_mutex_t mutex;

public:
    MTmutex()
        : inited(false)
    {
        if(pthread_mutex_init(&mutex, NULL) != 0)
            std::cerr << "Couldn't create mutex" << std::endl;

        inited = true;
    }

    ~MTmutex()
    {
        if(inited)
        {
            if(pthread_mutex_destroy(&mutex) != 0)
                std::cerr << "Couldn't destroy mutex" << std::endl;

            inited = false;
        }
    }

    bool lock(void)
    {
        if(pthread_mutex_lock(&mutex) != 0)
        {
            std::cerr << "Couldn't lock mutex" << std::endl;
            return false;
        }
        return true;
    }

    bool unlock(void)
    {

        if(pthread_mutex_unlock(&mutex) != 0)
        {
            std::cerr << "Couldn't unlock mutex" << std::endl;
            return false;
        }
        return true;
    }

    operator pthread_mutex_t& ()
    {
        return mutex;
    }
};

// Class for thread conditions. When a condition is triggered, bCond
// becomes true. Note this variable does not reset automatically; this
// has to be manually done with the reset() call. The reason for this
// is to allow a condition to work if setTrue() happens before
// waitForTrue()
class MTcondition
{
    bool           inited;
    bool           bCond;
    pthread_cond_t cond;
    MTmutex        mutex;

public:
    MTcondition()
    {
        if(pthread_cond_init(&cond, NULL) != 0)
        {
            std::cerr << "Couldn't create condition" << std::endl;
            inited = false;
        }

        inited = true;
        bCond = false;
    }

    ~MTcondition()
    {
        if(inited && pthread_cond_destroy(&cond) != 0)
            std::cerr << "Couldn't delete condition" << std::endl;
    }

    bool waitForTrue(void)
    {
        if(!inited)
        {
            std::cerr << __FILE__ << "[" << __LINE__ << "]: "
                      << "Tried to wait for an un-inited condition" << std::endl;
            return false;
        }
        mutex.lock();
        while(!bCond)
            pthread_cond_wait(&cond, &(pthread_mutex_t&)mutex);
        mutex.unlock();

        return true;
    }

    bool setTrue(void)
    {
        if(!inited)
        {
            std::cerr << __FILE__ << "[" << __LINE__ << "]: "
                      << "Tried to set an un-inited condition" << std::endl;
            return false;
        }

        mutex.lock();
        bCond = true;
        pthread_cond_signal(&cond);
        mutex.unlock();

        return true;
    }

    void reset(void)
    {
        bCond = false;
    }
    operator bool()
    {
        return bCond;
    }
};
