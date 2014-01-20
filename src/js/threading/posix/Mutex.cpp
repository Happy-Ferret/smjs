/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "threading/Mutex.h"

#include <assert.h>
#include <pthread.h>

#include "threading/posix/MutexPlatformData.h"


namespace js {

bool Mutex::initialize() {
    assert(!initialized_);

    if (pthread_mutex_init(&platformData()->ptMutex, NULL) != 0)
        return false;

    initialized_ = true;
    return true;
}


void Mutex::lock() {
    int r = pthread_mutex_lock(&platformData()->ptMutex);
    assert(r == 0);
}


void Mutex::unlock() {
    int r = pthread_mutex_unlock(&platformData()->ptMutex);
    assert(r == 0);
}


Mutex::~Mutex() {
    if (!initialized_)
        return;

    int r = pthread_mutex_destroy(&platformData()->ptMutex);
    assert(r == 0);
}


Mutex::PlatformData* Mutex::platformData() {
    static_assert(sizeof platformData_ >= sizeof(PlatformData),
                  "platformData_ is too small");
    return reinterpret_cast<PlatformData*>(platformData_);
}

} // namepsace js
