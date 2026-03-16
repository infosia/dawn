// Copyright 2026 The Dawn & Tint Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SRC_DAWN_NATIVE_OBJECTLABEL_H_
#define SRC_DAWN_NATIVE_OBJECTLABEL_H_

#include <mutex>
#include <string>
#include <string_view>
#include <utility>

#include "dawn/common/Ref.h"
#include "dawn/common/RefCounted.h"
#include "dawn/common/StringViewUtils.h"

namespace dawn::native {

class ObjectLabel final : public RefCounted {
  public:
    explicit ObjectLabel(std::string label) : mLabel(std::move(label)) {}

    const std::string& GetLabel() const { return mLabel; }
    operator const std::string&() const { return mLabel; }

    // Holds a Ref<ObjectLabel> so the string stays alive for the caller's scope.
    // Implicit conversions cover nearly all call sites.
    class View {
      public:
        explicit View(Ref<ObjectLabel> ref) : ref(std::move(ref)) {}

        operator const std::string&() const { return ref->GetLabel(); }
        operator std::string_view() const { return ref->GetLabel(); }

        // Mimics std::string's helper methods.
        const char* c_str() const { return ref->GetLabel().c_str(); }
        bool empty() const { return ref->GetLabel().empty(); }
        size_t find(std::string_view s, size_t pos = 0) const {
            return ref->GetLabel().find(s, pos);
        }

      private:
        Ref<ObjectLabel> ref;
    };

    // A thread-safe container for a Ref<ObjectLabel> that allows atomic updates and
    // retrieval of a View.
    //
    // We cannot use MutexProtected<Ref<ObjectLabel>> because MutexProtected has a
    // partial specialization for Ref<T> that unwraps the Ref and only gives access
    // to the underlying object (T* or T&). This specialization is intended for
    // protecting the *contents* of a reference-counted object, but it prevents
    // us from updating the Ref itself (assigning a new ObjectLabel).
    class AtomicRef {
      public:
        AtomicRef() = default;
        explicit AtomicRef(Ref<ObjectLabel> ref) : mRef(std::move(ref)) {}

        void Set(Ref<ObjectLabel> newRef) {
            std::scoped_lock lock(mMutex);
            mRef = std::move(newRef);
        }

        View Get() const {
            std::scoped_lock lock(mMutex);
            if (!mRef) {
                static Ref<ObjectLabel> kNullLabel = AcquireRef(new ObjectLabel(""));
                return View(kNullLabel);
            }
            return View(mRef);
        }

      private:
        mutable std::mutex mMutex;
        Ref<ObjectLabel> mRef;
    };

  private:
    const std::string mLabel;
};

}  // namespace dawn::native

namespace dawn {

inline WGPUStringView ToOutputStringView(const native::ObjectLabel::View& label) {
    return ToOutputStringView(static_cast<std::string_view>(label));
}

}  // namespace dawn

#endif  // SRC_DAWN_NATIVE_OBJECTLABEL_H_
