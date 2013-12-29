/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef platform_Mutex_h
#define platform_Mutex_h

#include <stdint.h>


namespace js {
namespace threading {


class Mutex {
  public:
    struct PlatformData;

    inline Mutex()
      : initialized_(false)
    { };

    ~Mutex();

    bool initialize();

    void lock();
    void unlock();

  private:
    friend class ConditionVariable;
    PlatformData* platformData();

    bool initialized_;

#if defined(__linux__)
    char platform_data[40];
#elif defined(__APPLE__) && defined(__MACH__)
    char platform_data[24];
#elif defined(_WIN32)
    void* platform_data_[6];
#else
#   error "Mutex platform data size isn't known for this platform"
#endif
};


class AutoMutexLock {
  public:
    inline AutoMutexLock(Mutex& mutex)
      : mutex_(&mutex)
    {
        mutex_->lock();
    }

    inline ~AutoMutexLock() {
        mutex_->unlock();
    }

  private:
    Mutex* mutex_;
};


class AutoMutexUnlock {
  public:
    inline AutoMutexUnlock(Mutex& mutex)
      : mutex_(&mutex)
    {
        mutex_->unlock();
    }

    inline ~AutoMutexUnlock() {
        mutex_->lock();
    }

  private:
    Mutex* mutex_;
};

} // namespace threading
} // namespace js

#endif // platform_Mutex_h
