/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef platform_Once_h
#define platform_Once_h

#include <stdlib.h>

#include "Mutex.h"

namespace js {
namespace threading {

class Once {
    typedef bool (*Callback)();
    typedef bool (*CallbackWithArg)(void* arg);

  public:
    Once();

    inline bool call(Callback callback) {
        if (ran_)
            return true;
        else
            return call2(callback);
    }

    inline bool call(CallbackWithArg callback, void* arg) {
        if (ran_)
            return true;
        else
            return call2(callback, arg);
    }

  private:
    bool call2(Callback callback);
    bool call2(CallbackWithArg, void* arg);

    Mutex mutex_;
    bool ran_;
};

} // namespace threading
} // namespace js

#endif // platform_Once_h
