/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef threading_Mutex_h
#define threading_Mutex_h

#include <stdint.h>

#include "mozilla/Attributes.h"


namespace js {

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
    // Disallow copy and assign.
    Mutex(const Mutex& that) MOZ_DELETE;
    void operator=(const Mutex& that) MOZ_DELETE;

    PlatformData* platformData();

    bool initialized_;

    // Linux and maybe other platforms define the storage size of
    // pthread_mutex_t in bytes. However we must define it as an array of void
    // pointers to ensure proper alignment.
#if defined(__APPLE__) && defined(__MACH__) && defined(__i386__)
    void* platformData_[11];
#elif defined(__APPLE__) && defined(__MACH__) && defined(__amd64__)
    void* platformData_[8];
#elif defined(__linux__)
    void* platformData_[40 / sizeof(void*)];
#elif defined(_WIN32)
    void* platformData_[6];
#else
#   error "Mutex platform data size isn't known for this platform"
#endif
};

} // namespace js

#endif // threading_Mutex_h
