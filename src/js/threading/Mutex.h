/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef platform_Mutex_h
#define platform_Mutex_h

#include <stdint.h>


namespace js {
namespace threading {


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
    friend class ConditionVariable;
    PlatformData* platformData();

    void* platform_data_[6];
    bool initialized_;
};

} // namespace threading
} // namespace js

#endif // platform_Mutex_h
