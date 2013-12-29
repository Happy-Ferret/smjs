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

struct Thread::PlatformData {
    HANDLE handle;
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
        void* arg = trampoline->arg_;
        Thread::Entry entry = trampoline->entry_;
        delete trampoline;

        entry(arg);
        return 0;
    }

  private:
    Thread::Entry entry_;
    void* arg_;
};


bool Thread::start(Thread::Entry entry, void* arg) {
    assert(!running());

    PlatformData* data = platformData();

    ThreadTrampoline* trampoline =
        new (std::nothrow) ThreadTrampoline(entry, arg);
    if (!trampoline)
        return false;

    // Use _beginthreadex and not CreateThread, because threads that are
    // created with the latter leak a small amount of memory when they use
    // certain msvcrt functions and then exit.
    HANDLE handle;
    unsigned int id;
    handle = _beginthreadex(NULL,
                            0,
                            ThreadTrampoline::start,
                            trampoline,
                            0,
                            &id);
    if (handle == NULL) {
        delete trampoline;
        return false;
    }

    data->handle = (HANDLE) handle;

    assert(id != NONE);
    id_ = id;

    return true;
}


void Thread::join() {
    assert(running());

    HANDLE handle = platformData()->handle;
    if (WaitForSingleObject(handle, INFINITE) != WAIT_OBJECT_0)
        abort();

    BOOL success = CloseHandle(handle);
    assert(success);

    id_ = NONE;
}


void Thread::detach() {
    assert(running());

    BOOL success = CloseHandle(platformData()->handle);
    assert(success);

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
    Thread::Id id = GetCurrentThreadId();
    assert(id != NONE);

    return id;
}


void Thread::setName(const char* name) {
    // Setting the thread name requires compiler support for structured
    // exceptions, so this only works when compiled with MSVC.
#ifdef _MSC_VER
    static const DWORD THREAD_NAME_EXCEPTION = 0x406D1388;
    static const DWORD THREAD_NAME_INFO_TYPE = 0x1000;

    #pragma pack(push, 8)
    struct THREADNAME_INFO  {
        DWORD dwType;
        LPCSTR szName;
        DWORD dwThreadID;
        DWORD dwFlags;
    };
    #pragma pack(pop)

    THREADNAME_INFO info;
    info.dwType = THREAD_NAME_INFO_TYPE;
    info.szName = name;
    info.dwThreadID = GetCurrentThreadId();
    info.dwFlags = 0;

    __try {
        RaiseException(THREAD_NAME_EXCEPTION,
                       0,
                       sizeof(info) / sizeof(ULONG_PTR),
                       (ULONG_PTR*) &info);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        // Do nothing.
    }
#endif // _MSC_VER
}

} // namespace threading
} // namepsace js
