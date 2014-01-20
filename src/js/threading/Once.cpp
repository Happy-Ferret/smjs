/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Once.h"

#include <stdlib.h>


namespace js {

Once::Once()
  : ran_(false)
{
    if (!mutex_.initialize())
        abort();
}


bool Once::call(Callback callback) {
    if (ran_)
        return true;

    AutoMutexLock lock(mutex_);

    if (ran_)
        return true;

    if (!callback())
        return false;

    ran_ = true;
    return true;
}


bool Once::call(CallbackWithArg callback, void* arg) {
    if (ran_)
        return true;

    AutoMutexLock lock(mutex_);

    if (ran_)
        return true;

    if (!callback(arg))
        return false;

    ran_ = true;
    return true;
}

} // namepsace js
