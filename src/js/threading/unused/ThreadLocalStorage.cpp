
// The signatures and definitions for using native condition variables
// require us to pull windows headers targeted for Windows Vista or later.
#if !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0600
# undef _WIN32_WINNT
# define _WIN32_WINNT 0x0600
#endif




#include <assert.h>
#include <stdlib.h>


#include <Windows.h>


#include "../ThreadLocal.h"




#if 0


namespace js {
namespace threading {


struct ThreadLocalStorage::PlatformData {
    DWORD slot;
};




bool ThreadLocalStorage::initialize() {
    assert(!_initialized);

  
  DWORD slot = TlsAlloc();
    if (slot == TLS_OUT_OF_INDEXES)
        return false;

  
  platformData()->slot = slot;
    initialized_ = true;

  
  return true;
}




void* ThreadLocalStorage::get() {
    assert(initialized_);
    return TlsGetValue(platformData()->slot);
}




void ThreadLocalStorage::set(void* value) {
    assert(initialized_);
    TlsSetValue(platformData()->slot, value);
}




ThreadLocalStorage::~ThreadLocalStorage() {
    if (!initialized_)
        return;

  
  if (!TlsFree(platformData()->slot))
        abort();
}




inline ThreadLocalStorage::PlatformData* ThreadLocalStorage::platformData() {
    static_assert(sizeof platform_data_ >= sizeof(PlatformData),
                                "platform_data_ is too small");
    return reinterpret_cast<PlatformData*>(platform_data_);
}


} // namespace threading
} // namepsace js


#endif
