// Copyright 2026 The Dawn & Tint Authors
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

#ifndef SRC_DAWN_NATIVE_CONCURRENTUSEGUARDED_H_
#define SRC_DAWN_NATIVE_CONCURRENTUSEGUARDED_H_

#include "dawn/common/NonCopyable.h"
#include "dawn/native/Error.h"
#include "partition_alloc/pointers/raw_ptr.h"

namespace dawn::native {

// An interface for objects that can be used concurrently and need to be guarded against
// simultaneous use, particularly when being used on a queue.
class ConcurrentUseGuardedBase {
  public:
    // Calls FinishUse() on the guarded object when it goes out of scope.
    // Caller is responsible for ensuring the guarded object's lifetime is longer than ScopedUse.
    class ScopedUse : public NonCopyable {
      public:
        ScopedUse();
        explicit ScopedUse(ConcurrentUseGuardedBase* guarded);
        ~ScopedUse();

        ScopedUse(ScopedUse&& other);
        ScopedUse& operator=(ScopedUse&& other);

        // Caller will handle calling FinishUse() on the guarded object.
        // Can't be called on an empty ScopedUse.
        void Release();

      private:
        raw_ptr<ConcurrentUseGuardedBase> mGuarded = nullptr;
    };

    // Checks that the object is ready for use on queue. If successful, changes its state to denote
    // it's in use and returns `ScopedUse` which resets the state when it goes out of scope.
    // Returns a validation error on failure.
    virtual ResultOrError<ScopedUse> ValidateCanUseOnQueueNow() = 0;

    // Called when the object is done being used, on queue or internally.
    // This should only be called from ScopedUse or if ScopedUse was released from calling this.
    virtual void FinishUse() = 0;

  protected:
    virtual ~ConcurrentUseGuardedBase() = default;
};

// A concrete implementation of ConcurrentUseGuardedBase that uses an atomic flag to manage the
// in-use state.
class ConcurrentUseGuarded : public ConcurrentUseGuardedBase {
  public:
    ResultOrError<ScopedUse> ValidateCanUseOnQueueNow() override;
    void FinishUse() override;

  protected:
    ResultOrError<ScopedUse> TryUse();

  private:
    std::atomic<bool> mInUse{false};
};

}  // namespace dawn::native

#endif  // SRC_DAWN_NATIVE_CONCURRENTUSEGUARDED_H_
