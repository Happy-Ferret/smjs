/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef platform_ConditionVariable_h
#define platform_ConditionVariable_h

#include <stdint.h>
#include "Mutex.h"


namespace js {
namespace threading {

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
    bool wait(Mutex& mutex, uint64_t usec);

  private:
    PlatformData* platformData();

    bool initialized_;

#if defined(__linux__)
    char platform_data[48]
#elif defined(__APPLE__) && defined(__MACH__)
    char platform_data[44];
#elif defined(_WIN32)
    void* platform_data_[9];
#else
#   error "Mutex platform data size isn't known for this platform"
#endif
};

} // namespace threading
} // namespace js

#endif // platform_ConditionVariable_h
