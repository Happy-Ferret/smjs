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
    bool wait(Mutex& mutex, uint64_t timeout);

  private:
    PlatformData* platformData();

    void* platform_data_[9];
    bool initialized_;
};

} // namespace threading
} // namespace js

#endif // platform_ConditionVariable_h
