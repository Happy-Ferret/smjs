/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
  * vim: set ts=8 sts=4 et sw=4 tw=99:
  * This Source Code Form is subject to the terms of the Mozilla Public
  * License, v. 2.0. If a copy of the MPL was not distributed with this
  * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef platform_ThreadLocal_h
#define platform_ThreadLocal_h


#if 0
#include <stdint.h>




namespace js {
namespace threading {


class ThreadLocalStorage {
    struct PlatformData;

 
 public:
    ThreadLocalStorage()
        : initialized_(false)
    { };

  
  ~ThreadLocalStorage();

  
  bool initialize();
 
   
  void* get();
    void set(void* value);

 
 private:
    PlatformData* platformData();

  
  void* platform_data_[1];
    bool initialized_;
};




template <typename T>
class ThreadLocal: public ThreadLocalPtr {
  public:
    inline T get() {
        void* value = ThreadLocalPtr::get();
        static_assert(sizeof value >= sizeof(T));
        return *reinterpret_cast<T*>(&value);
    }

  
  inline void set(T value) {
        void* value;
        static_assert(sizeof value >= sizeof(T));
        value = *reinterpret_cast<void**>(&value);
        ThreadLocalPtr::set(value);
    }
}




} // namespace threading
} // namespace js
#endif


#endif // platform_ThreadLocal_h
