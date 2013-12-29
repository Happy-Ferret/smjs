/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <assert.h>
#include <new>
#include <pthread.h>
#include <stdint.h>

#include "threading/Once.h"
#include "threading/Thread.h"


namespace js {
namespace threading {

struct Thread::PlatformData {
    pthread_t ptThread;
};


// The thread id is stored in a thread local storage slot. When a thread is
// created with Thread::start, the creator thread decides what the new
// id of the new thread is going to be. The ThreadTrampoline::start method
// stores the thread id into a TLS slot before invoking the entry function.
class ThreadIdManager {
  public:
    static inline void set(Thread::Id id) {
        assert(threadLocalId_ == Thread::NONE);
        threadLocalId_ = id;
    }

    static inline Thread::Id get() {
        Thread::Id id = threadLocalId_;
        if (id == Thread::NONE)
            threadLocalId_ = id = newId();
        return id;
    }

    static inline Thread::Id newId() {
        return __sync_add_and_fetch(&idCounter_, 1);
    }

  private:
    static __thread Thread::Id threadLocalId_;
    static Thread::Id idCounter_;
};

__thread Thread::Id ThreadIdManager::threadLocalId_ = Thread::NONE;
Thread::Id ThreadIdManager::idCounter_ = 0;


class ThreadTrampoline {
  public:
    inline ThreadTrampoline(Thread::Entry entry, void* arg, Thread::Id id)
      : entry_(entry),
        arg_(arg),
        id_(id)
    { }

    static void* start(void* trampolinePtr) {
        ThreadTrampoline* trampoline =
            reinterpret_cast<ThreadTrampoline*>(trampolinePtr);

        void* arg = trampoline->arg_;
        Thread::Entry entry = trampoline->entry_;

        ThreadIdManager::set(trampoline->id_);

        delete trampoline;

        entry(arg);
        return NULL;
    }

  private:
    Thread::Entry entry_;
    void* arg_;
    Thread::Id id_;
};


bool Thread::start(Thread::Entry entry, void* arg) {
    assert(!running());

    PlatformData* data = platformData();

    Thread::Id id = ThreadIdManager::newId();
    assert(id != NONE);

    ThreadTrampoline* trampoline =
        new (std::nothrow) ThreadTrampoline(entry, arg, id);
    if (!trampoline)
        return false;

    int r = pthread_create(&data->ptThread,
                           NULL,
                           ThreadTrampoline::start,
                           data);
    if (r != 0) {
        delete trampoline;
        return false;
    }

    id_ = id;

    return true;
}


void Thread::join() {
    assert(running());

    if (pthread_join(platformData()->ptThread, NULL) != 0)
        abort();

    id_ = NONE;
}


void Thread::detach() {
    assert(running());

    if (pthread_detach(platformData()->ptThread) != 0)
        abort();

    id_ = NONE;
}


Thread::~Thread() {
    // In theory we could just CloseHandle(handle) here if the thread is still
    // running, but that makes it more likely that bugs go unnoticed. So we
    // require people to call .detach() to "forget" about a Thread object
    // without waiting for it to finish.
    assert(!running());
}


inline Thread::PlatformData* Thread::platformData() {
    static_assert(sizeof platform_data_ >= sizeof(PlatformData),
                  "platform_data_ is too small");
    return reinterpret_cast<PlatformData*>(platform_data_);
}


Thread::Id Thread::current() {
    return ThreadIdManager::get();
}


void Thread::setName(const char* name) {
    // XXX implement me
}

} // namespace threading
} // namepsace js
