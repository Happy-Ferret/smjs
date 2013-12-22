/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <assert.h>
#include <new.h>
#include <process.h>
#include <stdint.h>

#include <Windows.h>

#include "threading/Thread.h"


namespace js {
namespace threading {

static const uintptr_t ThreadIdNone = ~static_cast<uintptr_t>(0);


struct Thread::PlatformData {
    HANDLE handle;
    unsigned int id;
};


// On 32-bit windows different calling conventions exist. This class makes
// it possible to use a function with an unspecified calling convention as the
// thread entry point.
class ThreadTrampoline {
  public:
    inline ThreadTrampoline(Thread::Entry entry, void* arg)
      : entry_(entry),
        arg_(arg)
    { }

    static unsigned int __stdcall start(void* trampolinePtr) {
        ThreadTrampoline* trampoline =
            reinterpret_cast<ThreadTrampoline*>(trampolinePtr);

        // Copy information out of the trampoline object onto the stack, and
        // free the trampoline object.
        void* arg = trampoline->arg();
        Thread::Entry entry = trampoline->entry();
        delete trampoline;

        entry(arg);

        return 0;
    }

  private:
    inline Thread::Entry entry() const {
        return entry_;
    }

    inline void* arg() const {
        return arg_;
    }

    Thread::Entry entry_;
    void* arg_;
};


bool Thread::start(Thread::Entry entry, void* arg) {
    PlatformData* data;
    ThreadTrampoline* trampoline;
    uintptr_t handle;
    unsigned int id;

    assert(!running_);

    data = platformData();

    trampoline = new (std::nothrow) ThreadTrampoline(entry, arg);
    if (!trampoline)
        return false;

    // Use _beginthreadex and not CreateThread, because threads that are
    // created with the latter leak a small amount of memory when they use
    // certain msvcrt functions and then exit.
    handle = _beginthreadex(NULL,
                            0,
                            ThreadTrampoline::start,
                            trampoline,
                            0,
                            &id);

    if (handle == 0) {
        delete trampoline;
        return false;
    }

    data->handle = (HANDLE) handle;
    data->id = id;

    running_ = true;
    return true;
}


void Thread::join() {
    assert(running_);

    HANDLE handle = platformData()->handle;
    if (WaitForSingleObject(handle, INFINITE) != WAIT_OBJECT_0)
        abort();

    BOOL success = CloseHandle(handle);
    assert(success);

    running_ = false;
}


void Thread::detach() {
    assert(running_);

    BOOL success = CloseHandle(platformData()->handle);
    assert(success);

    running_ = false;
}


Thread::Id Thread::id() const {
    assert(running_);
    return Id(platformData()->id);
}


Thread::Id Thread::none() {
    return Id(ThreadIdNone);
}


Thread::Id Thread::current() {
    return Id(GetCurrentThreadId());
}


Thread::~Thread() {
    // In theory we could just CloseHandle(handle) here if the thread is still
    // running, but that makes it more likely that bugs go unnoticed. So we
    // require people to call .detach() to "forget" about a Thread object
    // without waiting for it to finish.
    assert(!running_);
}


inline Thread::PlatformData* Thread::platformData() const {
    static_assert(sizeof platform_data_ >= sizeof(PlatformData),
                  "platform_data_ is too small");
    const void** ptr = const_cast<const void**>(platform_data_);
    return reinterpret_cast<PlatformData*>(ptr);
}


void Thread::setName(const char* name) {
    // Setting the thread name is only supported when compiled with MSVC.
#ifdef _MSC_VER
    #pragma pack(push, 8)
    struct THREADNAME_INFO  {
        DWORD dwType;
        LPCSTR szName;
        DWORD dwThreadID;
        DWORD dwFlags;
    };
    #pragma pack(pop)

    static const DWORD setThreadNameException = 0x406D1388;

    THREADNAME_INFO  info;
    info.dwType = 0x1000;
    info.szName = name;
    info.dwThreadID = GetCurrentThreadId();
    info.dwFlags = 0;

    __try {
        RaiseException(setThreadNameException,
                       0,
                       sizeof(info) / sizeof(ULONG_PTR),
                       (ULONG_PTR*) &info);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        // Do nothing.
    }
#endif // _MSC_VER
}


bool Thread::Id::operator ==(const Thread::Id& that) {
    return data_ == that.data_;
}


bool Thread::Id::operator !=(const Thread::Id& that) {
    return data_ != that.data_;
}


bool Thread::Id::operator !() {
    return data_ == ThreadIdNone;
}


} // namespace threading
} // namepsace js