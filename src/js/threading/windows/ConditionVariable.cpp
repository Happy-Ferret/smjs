/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "threading/ConditionVariable.h"

#include <assert.h>
#include <intrin.h>
#include <stdlib.h>

#include <Windows.h>

#include "mozilla/Assertions.h"

#include "threading/Mutex.h"
#include "threading/windows/MutexPlatformData.h"

// Some versions of the Windows SDK have a bug where some interlocked functions
// are not redefined as compiler intrinsics. Fix that for the interlocked
// functions that are used in this file.
#if defined(_MSC_VER) && !defined(InterlockedExchangeAdd)
#define InterlockedExchangeAdd(addend, value) \
        _InterlockedExchangeAdd((volatile long*) (addend), (long) (value))
#endif

#if defined(_MSC_VER) && !defined(InterlockedIncrement)
#define InterlockedIncrement(addend) \
        _InterlockedIncrement((volatile long*) (addend))
#endif


namespace js {

static const uint64_t USEC_PER_MSEC = 1000000;


// Windows XP and Server 2003 don't support condition variables natively. The
// NativeImports class is responsible for detecting native support and
// retrieving the appropriate function pointers. It gets instantiated once,
// using a static initializer.
class ConditionVariableNativeImports {
  public:
    ConditionVariableNativeImports() {
        // Kernel32.dll is always loaded first, so this should be unfallible.
        HMODULE module = GetModuleHandleW(L"kernel32.dll");
        assert(module != NULL);

#define LOAD_SYMBOL(symbol) loadSymbol(module, #symbol, symbol)
        supported_ =
            LOAD_SYMBOL(InitializeConditionVariable) &&
            LOAD_SYMBOL(WakeConditionVariable) &&
            LOAD_SYMBOL(WakeAllConditionVariable) &&
            LOAD_SYMBOL(SleepConditionVariableCS);
#undef LOAD_SYMBOL
    }

    inline bool supported() const {
        return supported_;
    }

    void (WINAPI* InitializeConditionVariable)
         (CONDITION_VARIABLE* ConditionVariable);
    void (WINAPI* WakeAllConditionVariable)
         (PCONDITION_VARIABLE ConditionVariable);
    void (WINAPI* WakeConditionVariable)
         (CONDITION_VARIABLE* ConditionVariable);
    BOOL (WINAPI* SleepConditionVariableCS)
         (CONDITION_VARIABLE* ConditionVariable,
          CRITICAL_SECTION* CriticalSection,
          DWORD dwMilliseconds);

  private:
    template <typename T>
    inline bool loadSymbol(HMODULE module, const char* name, T& fn) {
        void* ptr = GetProcAddress(module, name);
        if (!ptr)
            return false;

        fn = reinterpret_cast<T>(ptr);
        return true;
    }

    bool supported_;
};

static ConditionVariableNativeImports nativeImports;


// Wrapper for native condition variable APIs.
struct ConditionVariableNative {
    inline bool initialize() {
        nativeImports.InitializeConditionVariable(&cv_);
        return true;
    }

    inline void destroy() {
        // Native condition variabled don't require cleanup.
    }

    inline void signal() {
        nativeImports.WakeConditionVariable(&cv_);
    }

    inline void broadcast() {
        nativeImports.WakeAllConditionVariable(&cv_);
    }

    inline bool wait(CRITICAL_SECTION* cs, DWORD msec) {
        return nativeImports.SleepConditionVariableCS(&cv_, cs, msec);
    }

  private:
    CONDITION_VARIABLE cv_;
};


// Fallback condition variable support for Windows XP and Server 2003.
struct ConditionVariableFallback {
    static const uint32_t WAKEUP_MODE_NONE = 0;
    static const uint32_t WAKEUP_MODE_ONE = 0x40000000;
    static const uint32_t WAKEUP_MODE_ALL = 0x80000000;

    static const uint32_t WAKEUP_MODE_MASK = WAKEUP_MODE_ONE | WAKEUP_MODE_ALL;
    static const uint32_t SLEEPERS_COUNT_MASK = ~WAKEUP_MODE_MASK;

    bool initialize() {
        BOOL r;

        // Initialize the state variable to 0 sleepers, no wakeup.
        sleepersCountAndWakeupMode_ = 0 | WAKEUP_MODE_NONE;

        // Create a semaphore that prevents threads from entering sleep,
        // or waking other threads while a wakeup is ongoing.
        SleepWakeupSemaphore_ = CreateSemaphoreW(NULL, 1, 1, NULL);
        if (!SleepWakeupSemaphore_)
            goto error3;

        // Use an auto-reset event for waking up a single sleeper.
        wakeOneEvent_ = CreateEventW(NULL, FALSE, FALSE, NULL);
        if (!wakeOneEvent_)
            goto error2;

        // Use a manual-reset event for waking up all sleepers.
        wakeAllEvent_ = CreateEventW(NULL, TRUE, FALSE, NULL);
        if (!wakeAllEvent_)
            goto error1;

        return true;

      error1:
        r = CloseHandle(wakeAllEvent_);
        assert(r);
      error2:
        r = CloseHandle(SleepWakeupSemaphore_);
        assert(r);
      error3:
        return false;
    }

    void destroy() {
        BOOL r;

        assert(sleepersCountAndWakeupMode_ == (0 | WAKEUP_MODE_NONE));

        r = CloseHandle(SleepWakeupSemaphore_);
        assert(r);

        r = CloseHandle(wakeOneEvent_);
        assert(r);

        r = CloseHandle(wakeAllEvent_);
        assert(r);
    }

  private:
    inline void wakeup(uint32_t wakeupMode, HANDLE wakeEvent) {
        // Ensure that only one thread at a time can wake up others.
        BOOL result = WaitForSingleObject(SleepWakeupSemaphore_, INFINITE);
        assert(result == WAIT_OBJECT_0);

        // Atomically set the wakeup mode and retrieve the number of sleepers.
        assert((sleepersCountAndWakeupMode_ & WAKEUP_MODE_MASK) ==
               WAKEUP_MODE_NONE);
        uint32_t wcwm = InterlockedExchangeAdd(&sleepersCountAndWakeupMode_,
                                               wakeupMode);
        uint32_t sleepersCount = wcwm & SLEEPERS_COUNT_MASK;

        if (sleepersCount > 0) {
            // If there are any sleepers, set the wake event. The (last) woken
            // up thread is responsible for releasing the semaphore.
            BOOL success = SetEvent(wakeEvent);
            assert(success);

        } else {
            // If there are no sleepers, set the wakeup mode back to 'none'
            // and release the semaphore ourselves.
            sleepersCountAndWakeupMode_ = 0 | WAKEUP_MODE_NONE;

            BOOL success = ReleaseSemaphore(SleepWakeupSemaphore_, 1, NULL);
            assert(success);
        }
    }

  public:
    void signal() {
        wakeup(WAKEUP_MODE_ONE, wakeOneEvent_);
    }

    void broadcast() {
        wakeup(WAKEUP_MODE_ALL, wakeAllEvent_);
    }

    bool wait(CRITICAL_SECTION* userLock, DWORD msec) {
        // Make sure that we can't enter sleep when there are other threads
        // that still need to wake up on either of the wake events being set.
        DWORD result = WaitForSingleObject(SleepWakeupSemaphore_, INFINITE);
        assert(result == WAIT_OBJECT_0);

        // Register ourselves as a sleeper. Use an atomic operation, because
        // if another thread times out at the same time, it will decrement the
        // sleepers count without acquiring the semaphore.
        uint32_t wcwm = InterlockedIncrement(&sleepersCountAndWakeupMode_);
        assert((wcwm & WAKEUP_MODE_MASK) == WAKEUP_MODE_NONE);

        // Now that that this thread has been enlisted as a sleeper, it is safe
        // again for other threads to do a wakeup.
        BOOL success = ReleaseSemaphore(SleepWakeupSemaphore_, 1, NULL);
        assert(success);

        // Release the caller's mutex.
        LeaveCriticalSection(userLock);

        // Wait for either event to become signaled, which happens when
        // signal() or broadcast() is called, or for a timeout.
        HANDLE handles[2] = { wakeOneEvent_, wakeAllEvent_ };
        DWORD waitResult = WaitForMultipleObjects(2, handles, FALSE, msec);

        assert(waitResult == WAIT_OBJECT_0 ||
               waitResult == WAIT_OBJECT_0 + 1 ||
               (waitResult == WAIT_TIMEOUT && msec != INFINITE));

        // Atomically decrease the sleepers count and retrieve the wakeup mode
        // and new sleepers count.
        // If the wait returned because wakeOneEvent_ was set, we are certain
        // that the wakeup mode will be WAKEUP_MODE_ONE. In that case,
        // atomically reset the wakeup mode to 'none', because if another
        // thread's sleep times out at same time and it finds that it was the
        // last sleeper, it decides whether or not to reset the wakeOneEvent_
        // based on the current wakeup mode.
        uint32_t sub;
        if (waitResult == WAIT_OBJECT_0)
            sub = 1 | WAKEUP_MODE_ONE;
        else
            sub = 1;
        // Note that InterlockedExchangeAdd returns the old value, but it's
        // easier to work with the new value.
        wcwm = InterlockedExchangeAdd(&sleepersCountAndWakeupMode_,
                                      -sub) - sub;

        uint32_t wakeupMode = wcwm & WAKEUP_MODE_MASK;
        uint32_t sleepersCount = wcwm & SLEEPERS_COUNT_MASK;

        bool releaseSleepWakeupSemaphore = false;

        if (waitResult == WAIT_OBJECT_0) {
            // The wake-one event is an auto-reset event so if we're woken by
            // it, it should already have been reset. We also already removed
            // the WAKEUP_MODE_ONE bit so the wakeup mode should now be 'none'
            // again.
            assert(wakeupMode == WAKEUP_MODE_NONE);

            // The signaling thread has acquired the enter-wakeup semaphore and
            // expects the woken (this) thread to release it again.
            releaseSleepWakeupSemaphore = true;

        } else if (waitResult == WAIT_TIMEOUT &&
                   wakeupMode == WAKEUP_MODE_ONE &&
                   sleepersCount == 0) {
            // In theory a race condition is possible where the last sleeper
            // times out right at the moment that another thread signals it.
            // If that just happened we now have a dangling signal event and
            // mode, but no threads to be woken up by it, and we need to clean
            // that up.
            BOOL success = ResetEvent(wakeOneEvent_);
            assert(success);

            // This is safe - we are certain there are no other sleepers that
            // could wake up right now, and the semaphore ensures that no
            // non-sleeping threads are messing with
            // sleepersCountAndWakeupMode_.
            sleepersCountAndWakeupMode_ = 0 | WAKEUP_MODE_NONE;

            // The signaling thread has acquired the sleep-wakeup semaphore and
            // expects the woken thread to release it. But since there are no
            // sleeping threads left this thread will do it instead.
            releaseSleepWakeupSemaphore = true;

        } else if (wakeupMode == WAKEUP_MODE_ALL &&
                   sleepersCount == 0) {
            // If this was the last thread waking up in response to a
            // broadcast, clear the wakeup mode and reset the wake-all event.
            // A race condition similar to the case described above could
            // occur, so waitResult could be WAIT_TIMEOUT, but that doesn't
            // matter for the actions that need to be taken.
            assert(waitResult = WAIT_OBJECT_0  + 1 ||
                   waitResult == WAIT_TIMEOUT);

            BOOL success = ResetEvent(wakeAllEvent_);
            assert(success);

            sleepersCountAndWakeupMode_ = 0 | WAKEUP_MODE_NONE;

            // The broadcasting thread has acquired the enter-wakeup semaphore
            // and expects the last thread that wakes up to release it.
            releaseSleepWakeupSemaphore = true;

        } else if ((waitResult == WAIT_TIMEOUT && msec != INFINITE) ||
                   (waitResult == WAIT_OBJECT_0 + 1 &&
                    wakeupMode == WAKEUP_MODE_ALL)) {
            // Either:
            //   * The wait timed out but found no active signal or broadcast
            //     the moment it decreased the wait count.
            //   * A broadcast woke up this thread but there are more threads
            //     that need to be woken up by the wake-all event.
            // These are ordinary conditions in which we don't have to do
            // anything.

        } else {
            MOZ_ASSUME_UNREACHABLE("Invalid wakeup condition");
        }

        // Release the enter-wakeup semaphore if the wakeup condition requires
        // us to do it.
        if (releaseSleepWakeupSemaphore) {
            BOOL success = ReleaseSemaphore(SleepWakeupSemaphore_, 1, NULL);
            assert(success);
        }

        // Reacquire the user mutex.
        EnterCriticalSection(userLock);

        // Return true if woken up, false when timed out.
        return waitResult != WAIT_TIMEOUT;
    }

  private:
    uint32_t sleepersCountAndWakeupMode_;
    HANDLE SleepWakeupSemaphore_;
    HANDLE wakeOneEvent_;
    HANDLE wakeAllEvent_;
};


struct ConditionVariable::PlatformData {
    union {
        ConditionVariableNative native;
        ConditionVariableFallback fallback;
    };
};


bool ConditionVariable::initialize() {
    assert(!initialized_);

    if (nativeImports.supported())
        initialized_ = platformData()->native.initialize();
    else
        initialized_ = platformData()->fallback.initialize();

    return initialized_;
}


void ConditionVariable::signal() {
    assert(initialized_);

    if (nativeImports.supported())
        platformData()->native.signal();
    else
        platformData()->fallback.signal();
}


void ConditionVariable::broadcast() {
    assert(initialized_);

    if (nativeImports.supported())
        platformData()->native.broadcast();
    else
        platformData()->fallback.broadcast();
}


void ConditionVariable::wait(Mutex& mutex) {
    assert(initialized_);
    assert(mutex.initialized_);

    CRITICAL_SECTION* cs = &mutex.platformData()->cs;
    bool r;

    if (nativeImports.supported())
        r = platformData()->native.wait(cs, INFINITE);
    else
        r = platformData()->fallback.wait(cs, INFINITE);

    if (!r)
        abort();
}


bool ConditionVariable::wait(Mutex& mutex, uint64_t usec) {
    assert(initialized_);
    assert(mutex.initialized_);

    CRITICAL_SECTION* cs = &mutex.platformData()->cs;
    DWORD msec = static_cast<DWORD>(usec / USEC_PER_MSEC);

    if (nativeImports.supported())
        return platformData()->native.wait(cs, msec);
    else
        return platformData()->fallback.wait(cs, msec);
}


ConditionVariable::~ConditionVariable() {
    if (!initialized_)
        return;

    if (nativeImports.supported())
        platformData()->native.destroy();
    else
        platformData()->fallback.destroy();
}


inline ConditionVariable::PlatformData* ConditionVariable::platformData() {
    static_assert(sizeof platformData_ >= sizeof(PlatformData),
                  "platformData_ is too small");
    return reinterpret_cast<PlatformData*>(platformData_);
}

} // namespace js
