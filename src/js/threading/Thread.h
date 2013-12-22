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
    class Id;
    typedef void (*Entry)(void* arg);
    struct PlatformData;

    Thread()
      : running_(false)
    { };

    ~Thread();

    bool start(Entry entry, void* arg);
    void detach();
    void join();

    inline bool running() const {
        return running_;
    }

    Thread::Id id() const;

    static void setName(const char* name);

    static Id current();
    static Id none();

  private:
    PlatformData* platformData() const;

    void* platform_data_[2];
    bool running_;
};


class Thread::Id {
  public:
    inline Id(const Id& that):
        data_(that.data_)
    { }

    bool operator ==(const Id& that);
    bool operator !=(const Id& that);
    bool operator !();

    // Not recommended, because we can't ensure that two thread ids have the
    // same integral value if they refer to the same thread. Ideally we'd mark
    // this cast operator as explicit, but compiler support for this C++11
    // feature is still very limited.
    inline operator uintptr_t() {
      if (!*this)
          return 0;
      else
          return data_;
    }

  private:
    friend class Thread;
    inline explicit Id(uintptr_t data)
        : data_(data)
    { }

    Id();

    uintptr_t data_;
};

} // namespace threading
} // namespace js

#endif // platform_Thread_h
