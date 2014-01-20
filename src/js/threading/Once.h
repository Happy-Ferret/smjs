/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef threading_Once_h
#define threading_Once_h

#include <stdlib.h>

#include "mozilla/Attributes.h"

#include "threading/Mutex.h"


namespace js {

class Once {
    typedef bool (*Callback)();
    typedef bool (*CallbackWithArg)(void* arg);

  public:
    Once();

    bool call(Callback callback);
    bool call(CallbackWithArg, void* arg);

  private:
    // Disallow copy and assign.
    Once(const Once& that) MOZ_DELETE;
    void operator=(const Once& that) MOZ_DELETE;

    Mutex mutex_;
    bool ran_;
};

} // namespace js

#endif // threading_Once_h
