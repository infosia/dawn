// Copyright 2025 The Dawn & Tint Authors
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef SRC_DAWN_NATIVE_DEVICEGUARD_H_
#define SRC_DAWN_NATIVE_DEVICEGUARD_H_

#include <mutex>

#include "dawn/common/Defer.h"
#include "dawn/common/MutexProtected.h"
#include "dawn/common/Ref.h"

namespace dawn::native {

class DeviceBase;

namespace detail {

struct DeviceMutexTraits {
    using MutexType = Ref<RecursiveMutex>;
    using LockType = RecursiveMutex::AutoLock;

    static RecursiveMutex* GetMutex(MutexType& m) { return m.Get(); }
};

}  // namespace detail

// DeviceGuard is the equivalent to a Guard when using MutexProtected specifically designed for the
// device-wide lock. Usually, Guards are scoped via lambda functions, but for the device-wide lock,
// we provide ways to get this guard from the Device. Since the device-wide lock is a temporary
// solution, this provides a way get the lock without adding lambda scopes everywhere.
class DeviceGuard : private ::dawn::detail::Guard<DeviceBase, detail::DeviceMutexTraits> {
  public:
    using Base = ::dawn::detail::Guard<DeviceBase, detail::DeviceMutexTraits>;
    using Base::Defer;

  private:
    friend class DeviceBase;
    using Base::Base;
};

}  // namespace dawn::native

#endif  // SRC_DAWN_NATIVE_DEVICEGUARD_H_
