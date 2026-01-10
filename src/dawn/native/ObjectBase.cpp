// Copyright 2018 The Dawn & Tint Authors
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

#include <atomic>
#include <mutex>
#include <utility>
#include <vector>

#include "absl/strings/str_format.h"
#include "dawn/native/Adapter.h"
#include "dawn/native/Device.h"
#include "dawn/native/ObjectBase.h"
#include "dawn/native/ObjectType_autogen.h"
#include "dawn/native/Queue.h"
#include "dawn/native/utils/WGPUHelpers.h"

namespace dawn::native {

// The ref count payload holds 3 potential states:
// NotInitialized: The object is still in the process of initialization and does not know if
//                 it will be an error yet.
// InitializedError: The object is initialized and is an error.
// InitializedNotError: The object is initialized and is valid.
//
// The following state flows are possible:
//  * NotInitialized -> InitializedError
//  * NotInitialized -> InitializedNoError
//
// All states are valid initial states.
//
// The values of the states are chosen so that valid transitions can be made with atomic AND and OR
// operations on the existing payload.  NotInitialized is chosen to be all 1s so that the target
// state values can be ANDed to make the transition.
static constexpr uint64_t kNotInitializedPayload = 0b11;
static constexpr uint64_t kInitializedErrorPayload = 0b00;
static constexpr uint64_t kInitializedNoErrorPayload = 0b10;

static constexpr uint64_t kInitializedMask = 0b1;
static constexpr uint64_t kInitialized = 0b0;

ErrorMonad::ErrorMonad() : RefCounted(kInitializedNoErrorPayload) {}
ErrorMonad::ErrorMonad(ErrorTag) : RefCounted(kInitializedErrorPayload) {}
ErrorMonad::ErrorMonad(DelayedInitializationTag) : RefCounted(kNotInitializedPayload) {}

bool ErrorMonad::IsInitialized() const {
    return (GetRefCountPayload() & kInitializedMask) == kInitialized;
}

bool ErrorMonad::IsError() const {
    uint64_t payload = GetRefCountPayload();
    // ASSERT that the error state is initialized but also return that this is an error if it has
    // not been initialized in release builds. This makes sure that invalid accesses to initializing
    // objects are avoided.
    DAWN_ASSERT((payload & kInitializedMask) == kInitialized);
    return payload != kInitializedNoErrorPayload;
}

void ErrorMonad::SetInitializedError() {
    uint64_t previousPayload = RefCountPayloadFetchAnd(kInitializedErrorPayload);
    DAWN_ASSERT(previousPayload == kNotInitializedPayload);
}

void ErrorMonad::SetInitializedNoError() {
    uint64_t previousPayload = RefCountPayloadFetchAnd(kInitializedNoErrorPayload);
    DAWN_ASSERT(previousPayload == kNotInitializedPayload);
}

ObjectBase::ObjectBase(DeviceBase* device) : ErrorMonad(), mDevice(device) {}

ObjectBase::ObjectBase(DeviceBase* device, ErrorTag) : ErrorMonad(kError), mDevice(device) {}

ObjectBase::ObjectBase(DeviceBase* device, DelayedInitializationTag)
    : ErrorMonad(kDelayedInitialization), mDevice(device) {}

InstanceBase* ObjectBase::GetInstance() const {
    return mDevice->GetInstance();
}

DeviceBase* ObjectBase::GetDevice() const {
    return mDevice.Get();
}

void ApiObjectList::Track(ApiObjectBase* object) {
    if (mMarkedDestroyed.load(std::memory_order_acquire)) {
        object->DestroyImpl(DestroyReason::EarlyDestroy);
        return;
    }
    mObjects.Use([&object](auto lockedObjects) {
        lockedObjects->Prepend(object);
        object->mAliveState.store(ApiObjectBase::AliveState::Alive, std::memory_order_release);
    });
}

bool ApiObjectList::Untrack(ApiObjectBase* object) {
    return mObjects.Use([&object](auto lockedObjects) { return object->RemoveFromList(); });
}

void ApiObjectList::Destroy(DestroyReason reason) {
    std::vector<ApiObjectBase*> objectsToDestroy;
    mObjects.Use([&objectsToDestroy, this](auto lockedObjects) {
        mMarkedDestroyed.store(true, std::memory_order_release);

        // Try to acquire a reference to all objects to ensure they aren't deleted on another thread
        // before DestroyImpl() runs below. If the object already has zero refs then another thread
        // is about to delete it, the object is left in the list so it gets deleted+destroyed on the
        // other thread.
        for (auto* node = lockedObjects->head(); node != lockedObjects->end();) {
            auto* next = node->next();

            if (ApiObjectBase* object = node->value(); object->TryAddRef()) {
                objectsToDestroy.push_back(object);
                bool removed = node->RemoveFromList();
                DAWN_ASSERT(removed);
            }

            node = next;
        }
    });

    for (ApiObjectBase* object : objectsToDestroy) {
        object->DestroyInternal(reason);
        // `object` may be deleted here if last real reference was dropped on another thread.
        object->Release();
    }
}

ApiObjectBase::ApiObjectBase(DeviceBase* device, StringView label) : ObjectBase(device) {
    mLabel = std::string(label);
}

ApiObjectBase::ApiObjectBase(DeviceBase* device, ErrorTag tag, StringView label)
    : ObjectBase(device, tag) {
    mLabel = std::string(label);
}

ApiObjectBase::ApiObjectBase(DeviceBase* device, DelayedInitializationTag tag, StringView label)
    : ObjectBase(device, tag) {
    mLabel = std::string(label);
}

ApiObjectBase::ApiObjectBase(DeviceBase* device, LabelNotImplementedTag tag) : ObjectBase(device) {}

ApiObjectBase::~ApiObjectBase() {
    DAWN_ASSERT(!IsAlive());
}

void ApiObjectBase::APISetLabel(StringView label) {
    SetLabel(std::string(utils::NormalizeMessageString(label)));
}

void ApiObjectBase::SetLabel(std::string label) {
    mLabel = std::move(label);
    SetLabelImpl();
}

const std::string& ApiObjectBase::GetLabel() const {
    return mLabel;
}

void ApiObjectBase::FormatLabel(absl::FormatSink* s) const {
    s->Append(ObjectTypeAsString(GetType()));
    if (!mLabel.empty()) {
        s->Append(absl::StrFormat(" \"%s\"", mLabel));
    } else {
        s->Append(" (unlabeled)");
    }
}

void ApiObjectBase::SetLabelImpl() {}

bool ApiObjectBase::IsAlive() const {
    return mAliveState.load(std::memory_order_acquire) != AliveState::Destroyed;
}

MaybeError ApiObjectBase::BeginUseInSubmit() {
    AliveState expected = AliveState::Alive;
    while (!mAliveState.compare_exchange_weak(expected, AliveState::AliveInSubmit,
                                              std::memory_order_acq_rel,
                                              std::memory_order_acquire)) {
        // If the CAS fails, the previous state could be:
        // - Alive. This is spurious fail, it's OK to continue.
        // - AliveInSubmit. This means BeginUseInSubmit() was called twice without EndUseInSubmit()
        // in-between. This is Dawn's internal logic error. Throw assertion error.
        // - Destroyed/Destroying: the object is being destroyed on another thread. Throw validation
        // error.
        DAWN_ASSERT(expected != AliveState::AliveInSubmit);

        if (expected != AliveState::Alive) {
            DAWN_ASSERT(expected == AliveState::Destroyed || expected == AliveState::Destroying);
            return DAWN_VALIDATION_ERROR(
                "Cannot use %s in submit. It is used concurrently by other methods.", this);
        }
    }
    return {};
}

void ApiObjectBase::EndUseInSubmit() {
    AliveState previous = mAliveState.exchange(AliveState::Alive, std::memory_order_acq_rel);
    DAWN_ASSERT(previous == AliveState::AliveInSubmit);
}

void ApiObjectBase::DeleteThis() {
    Destroy(DestroyReason::CppDestructor);
    RefCounted::DeleteThis();
}

void ApiObjectBase::LockAndDeleteThis() {
    auto deviceGuard = GetDevice()->GetGuardForDelete();
    DeleteThis();
}

ApiObjectList* ApiObjectBase::GetObjectTrackingList() {
    DAWN_ASSERT(GetDevice() != nullptr);
    return GetDevice()->GetObjectTrackingList(GetType());
}

void ApiObjectBase::Destroy(DestroyReason reason) {
    // Try to transition to Destroying state
    AliveState expected = AliveState::Alive;
    while (!mAliveState.compare_exchange_weak(
        expected, AliveState::Destroying, std::memory_order_acq_rel, std::memory_order_acquire)) {
        if (expected == AliveState::AliveInSubmit) {
            std::ignore = GetDevice()->ConsumedError(
                DAWN_VALIDATION_ERROR("Cannot destroy %s while it is used in a "
                                      "queue submission.",
                                      this));
            return;
        }
        // If expected is Destroyed or Destroying, we're already being destroyed, so return
        if (expected == AliveState::Destroyed || expected == AliveState::Destroying) {
            return;
        }
    }

    // After successful CAS, expected should still be Alive
    DAWN_CHECK(expected == AliveState::Alive);

    ApiObjectList* list = GetObjectTrackingList();
    DAWN_ASSERT(list != nullptr);
    if (list->Untrack(this)) {
        DestroyInternal(reason);
    }
}

void ApiObjectBase::DestroyInternal(DestroyReason reason) {
    AliveState previous = mAliveState.exchange(AliveState::Destroyed, std::memory_order_acq_rel);
    DAWN_ASSERT(previous == AliveState::Alive || previous == AliveState::Destroying);
    DestroyImpl(reason);
}

}  // namespace dawn::native
