/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef platform_Thread_h
#define platform_Thread_h

#include <stdint.h>


namespace js {
namespace threading {

class Thread {
  public:
    typedef uintptr_t Id;
    static const Id NONE = 0;

    typedef void (*Entry)(void* arg);

    struct PlatformData;

    Thread()
      : id_(NONE)
    { };

    ~Thread();

    bool start(Entry entry, void* arg);
    void detach();
    void join();

    inline bool running() const {
        return id_ != NONE;
    }

    inline Thread::Id id() const {
        return id_;
    }

    static Thread::Id current();
    static void setName(const char* name);

  private:
    PlatformData* platformData();

    Thread::Id id_;
    void* platform_data_[1];
};


} // namespace threading
} // namespace js

#endif // platform_Thread_h
