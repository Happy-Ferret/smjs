/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef vm_Monitor_h
#define vm_Monitor_h

#ifdef JS_THREADSAFE
#include "mozilla/DebugOnly.h"
#endif

#include <stddef.h>

#include "jslock.h"

#include "js/Utility.h"

#ifdef JS_THREADSAFE
#include "threading/ConditionVariable.h"
#include "threading/Mutex.h"
#endif

namespace js {

// A base class used for types intended to be used in a parallel
// fashion, such as the workers in the |ThreadPool| class.  Combines a
// lock and a condition variable.  You can acquire the lock or signal
// the condition variable using the |AutoLockMonitor| type.

class Monitor
{
  protected:
    friend class AutoLockMonitor;
    friend class AutoUnlockMonitor;

#ifdef JS_THREADSAFE
    Mutex lock_;
    ConditionVariable condVar_;
#endif

  public:
    Monitor()
#ifdef JS_THREADSAFE
      : lock_(),
        condVar_()
#endif
    { }

    bool init();
};

class AutoLockMonitor
#ifdef JS_THREADSAFE
  : private AutoMutexLock
#endif
{
  private:
#ifdef JS_THREADSAFE
    Monitor &monitor;
#endif

  public:
    AutoLockMonitor(Monitor &monitor)
#ifdef JS_THREADSAFE
      : AutoMutexLock(monitor.lock_),
        monitor(monitor)
#endif
    { }

    bool isFor(Monitor &other) const {
#ifdef JS_THREADSAFE
        return &monitor.lock_ == &other.lock_;
#else
        return true;
#endif
    }

    void wait(ConditionVariable& condVar) {
#ifdef JS_THREADSAFE
        condVar.wait(monitor.lock_);
#endif
    }

    void wait() {
#ifdef JS_THREADSAFE
        wait(monitor.condVar_);
#endif
    }

    void notify(ConditionVariable& condVar) {
#ifdef JS_THREADSAFE
        condVar.signal();
#endif
    }

    void notify() {
#ifdef JS_THREADSAFE
        notify(monitor.condVar_);
#endif
    }

    void notifyAll(ConditionVariable& condVar) {
#ifdef JS_THREADSAFE
        condVar.broadcast();
#endif
    }

    void notifyAll() {
#ifdef JS_THREADSAFE
        notifyAll(monitor.condVar_);
#endif
    }
};

class AutoUnlockMonitor
#ifdef JS_THREADSAFE
  : private AutoMutexUnlock
#endif
{
  private:
#ifdef JS_THREADSAFE
    Monitor &monitor;
#endif

  public:
    AutoUnlockMonitor(Monitor &monitor)
#ifdef JS_THREADSAFE
      : AutoMutexUnlock(monitor.lock_),
        monitor(monitor)
#endif
    { }

    bool isFor(Monitor &other) const {
#ifdef JS_THREADSAFE
        return &monitor.lock_ == &other.lock_;
#else
        return true;
#endif
    }
};

} // namespace js

#endif /* vm_Monitor_h */
