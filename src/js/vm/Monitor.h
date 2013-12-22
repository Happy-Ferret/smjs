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

#include "js/Utility.h"
#include "threading/ConditionVariable.h"
#include "threading/Mutex.h"

using js::threading::AutoMutexLock;
using js::threading::AutoMutexUnlock;
using js::threading::ConditionVariable;
using js::threading::Mutex;

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

    Mutex lock_;
    ConditionVariable condVar_;

  public:
    Monitor()
      : lock_(),
        condVar_()
    { }

    bool init();
};

class AutoLockMonitor: private AutoMutexLock
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

    void wait() {
#ifdef JS_THREADSAFE
        monitor.condVar_.wait(monitor.lock_);
#endif
    }

    void notify() {
#ifdef JS_THREADSAFE
        monitor.condVar_.signal();
#endif
    }

    void notifyAll() {
#ifdef JS_THREADSAFE
      monitor.condVar_.broadcast();
#endif
    }
};

class AutoUnlockMonitor: private AutoMutexUnlock
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
};

} // namespace js

#endif /* vm_Monitor_h */
