// Copyright 2022 The Dawn Authors
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

#ifndef SRC_DAWN_NODE_BINDING_EVENTTARGET_H_
#define SRC_DAWN_NODE_BINDING_EVENTTARGET_H_

#include <string>

#include "src/dawn/node/interop/NodeAPI.h"
#include "src/dawn/node/interop/WebGPU.h"

namespace wgpu::binding {

// Implements the machinery common to all EventTargets on the Web platform. There is a lot of
// complexity to support some of the legacy behaviors. For example setting device.onuncapturederror
// or passing a boolean for the options of AddEventListener.
class EventTarget : public virtual interop::EventTarget {
  public:
    EventTarget(Napi::Env env);
    ~EventTarget();

    void addEventListener(
        Napi::Env,
        std::string type,
        std::optional<interop::EventListener> callback,
        std::optional<std::variant<interop::AddEventListenerOptions, bool>> options) override;
    void removeEventListener(
        Napi::Env,
        std::string type,
        std::optional<interop::EventListener> callback,
        std::optional<std::variant<interop::EventListenerOptions, bool>> options) override;
    bool dispatchEvent(Napi::Env, interop::Event event) override;

  private:
    Napi::Reference<Napi::Object> eventTargetRef_;
};

}  // namespace wgpu::binding

#endif  // SRC_DAWN_NODE_BINDING_EVENTTARGET_H_
