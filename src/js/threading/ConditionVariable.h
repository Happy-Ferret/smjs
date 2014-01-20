/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef threading_ConditionVariable_h
#define threading_ConditionVariable_h

#include <stdint.h>

#include "mozilla/Attributes.h"

#include "threading/Mutex.h"


namespace js {

class ConditionVariable {
  public:
    struct PlatformData;

    ConditionVariable()
      : initialized_(false)
    { };

    ~ConditionVariable();

    bool initialize();

    void signal();
    void broadcast();
    void wait(Mutex& mutex);

    // Wait for with a relative timeout specified in microseconds. Returns true
    // if woken up by broadcast() or signal(), or false when timed out.
    bool wait(Mutex& mutex, uint64_t usec);

  private:
    // Disallow copy and assign.
    ConditionVariable(const ConditionVariable& that) MOZ_DELETE;
    void operator=(const ConditionVariable& that) MOZ_DELETE;

    PlatformData* platformData();

    bool initialized_;

    // Linux and maybe other platforms define the storage size of
    // pthread_cond_t in bytes. However we must define it as array of void
    // pointers to ensure proper alignment.
#if defined(__APPLE__) && defined(__MACH__) && defined(__i386__)
    void* platformData_[7];
#elif defined(__APPLE__) && defined(__MACH__) && defined(__amd64__)
    void* platformData_[6];
#elif defined(__linux__)
    void* platformData_[48 / sizeof(void*)];
#elif defined(_WIN32)
    void* platformData_[4];
#else
#   error "Mutex platform data size isn't known for this platform"
#endif
};

} // namespace js

#endif // threading_ConditionVariable_h
