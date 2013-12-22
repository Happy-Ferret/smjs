/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <assert.h>

#include <Windows.h>

#include "MutexPlatformData.h"
#include "threading/Mutex.h"


namespace js {
namespace threading {

bool Mutex::initialize() {
    assert(!initialized_);

    InitializeCriticalSection(&platformData()->cs);

    initialized_ = true;
    return true;
}


void Mutex::lock() {
    EnterCriticalSection(&platformData()->cs);
}


void Mutex::unlock() {
    LeaveCriticalSection(&platformData()->cs);
}


Mutex::~Mutex() {
    if (initialized_)
        DeleteCriticalSection(&platformData()->cs);
}


inline Mutex::PlatformData* Mutex::platformData() {
    static_assert(sizeof platform_data_ >= sizeof(PlatformData),
                  "platform_data_ is too small");
    return reinterpret_cast<PlatformData*>(platform_data_);
}

} // namespace threading
} // namepsace js
