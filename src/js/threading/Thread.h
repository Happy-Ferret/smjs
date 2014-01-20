/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef threading_Thread_h
#define threading_Thread_h

#include <stdint.h>

#include "mozilla/Attributes.h"


namespace js {

class Thread {
  public:
    typedef uintptr_t Id;

    typedef void (*Entry)(void* arg);

    struct PlatformData;

    Thread()
      : id_(none())
    { };

    ~Thread();

    bool start(Entry entry, void* arg);
    void detach();
    void join();

    inline bool running() const {
        return id_ != none();
    }

    // Returns the id of this thread. The thread ID is guaranteed to uniquely
    // identify a thread and can be compared with the == operator.
    inline Id id() const {
        return id_;
    }

    // Returns a thread id which means "no thread".
    inline static Id none() {
        // It'd be nice to make this a const member instead, but it's much
        // harder to get it to link properly when using gcc.
        return 0;
    }

    // Returns the thread id of the calling thread.
    static Id current();

    // Set the current thread name. Returns true if successful. Note that
    // setting the thread name may not be available on all platforms; on these
    // platforms setName() should always return false.
    static bool setName(const char* name);

  private:
    // Disallow copy and assign.
    Thread(const Thread& that) MOZ_DELETE;
    void operator=(const Thread& that) MOZ_DELETE;

    PlatformData* platformData();

    Thread::Id id_;
    void* platformData_[1];
};

} // namespace js

#endif // threading_Thread_h
