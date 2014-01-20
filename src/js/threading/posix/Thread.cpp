/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "threading/Thread.h"

#include <assert.h>
#include <new>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#if defined(__APPLE__) && defined(__MACH__)
# include <dlfcn.h>
#endif

#if defined(__linux__)
# include <sys/prctl.h>
#endif

#include "threading/Once.h"


namespace js {

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
        assert(threadLocalId_ == Thread::none());
        threadLocalId_ = id;
    }

    static inline Thread::Id get() {
        Thread::Id id = threadLocalId_;
        if (id == Thread::none())
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

__thread Thread::Id ThreadIdManager::threadLocalId_ = 0;
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
    assert(id != none());

    ThreadTrampoline* trampoline =
        new (std::nothrow) ThreadTrampoline(entry, arg, id);
    if (!trampoline)
        return false;

    int r = pthread_create(&data->ptThread,
                           NULL,
                           ThreadTrampoline::start,
                           trampoline);
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

    id_ = none();
}


void Thread::detach() {
    assert(running());

    if (pthread_detach(platformData()->ptThread) != 0)
        abort();

    id_ = none();
}


Thread::~Thread() {
    // In theory we could just CloseHandle(handle) here if the thread is still
    // running, but that makes it more likely that bugs go unnoticed. So we
    // require people to call .detach() to "forget" about a Thread object
    // without waiting for it to finish.
    assert(!running());
}


inline Thread::PlatformData* Thread::platformData() {
    static_assert(sizeof platformData_ >= sizeof(PlatformData),
                  "platformData_ is too small");
    return reinterpret_cast<PlatformData*>(platformData_);
}


Thread::Id Thread::current() {
    return ThreadIdManager::get();
}


bool Thread::setName(const char* name) {
    assert(name);

#if (defined(__APPLE__) && defined(__MACH__)) || defined(__linux__)
    // On linux and OS X the name may not be longer than 16 bytes, including
    // the null terminator. Truncate the name to 15 characters.
    char nameBuf[16];

    strncpy(nameBuf, name, sizeof nameBuf - 1);
    nameBuf[sizeof nameBuf - 1] = '\0';
    name = nameBuf;
#endif

#if (defined(__APPLE__) && defined(__MACH__))
    // On OS X, pthread_setname_np is not always available, so try to load it
    // dynamically instead.
    int (*pthread_setname_np_dld)(const char*);

    *reinterpret_cast<void**>(&pthread_setname_np_dld) =
        dlsym(RTLD_DEFAULT, "pthread_setname_np");
    if (!pthread_setname_np_dld)
        return false;

    int r = pthread_setname_np_dld(name);
    return r == 0;

#elif defined(__FreeBSD__) || defined(__OpenBSD__)
    int r = pthread_set_name_np(pthread_self(), name);
    return r == 0;

#elif defined(__linux__)
    int r = prctl(PR_SET_NAME,
                  reinterpret_cast<unsigned long>(name),
                  0,
                  0,
                  0);
    return r == 0;

#else
    return false;

#endif
}

} // namepsace js
