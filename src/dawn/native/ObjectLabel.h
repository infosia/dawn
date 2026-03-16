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
#include <utility>

namespace dawn::native {

// Holds a label string with a mutex so the string can be updated and read safely from multiple
// threads without a device-wide lock.
class ObjectLabel final {
  public:
    ObjectLabel() = default;

    void SetValue(std::string label) {
        std::scoped_lock lock(mMutex);
        mLabel = std::move(label);
    }

    std::string GetValue() const {
        std::scoped_lock lock(mMutex);
        return mLabel;
    }

  private:
    mutable std::mutex mMutex;
    std::string mLabel;
};

}  // namespace dawn::native

#endif  // SRC_DAWN_NATIVE_OBJECTLABEL_H_
