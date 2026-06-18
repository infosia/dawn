// Copyright 2019 The Dawn & Tint Authors
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

#include "src/dawn/wire/client/Buffer.h"

#include <functional>
#include <limits>
#include <string>
#include <utility>

#include "dawn/wire/WireCmd_autogen.h"
#include "partition_alloc/pointers/raw_ptr.h"
#include "src/dawn/common/StringViewUtils.h"
#include "src/dawn/wire/BufferConsumer_impl.h"
#include "src/dawn/wire/client/Client.h"
#include "src/dawn/wire/client/Device.h"
#include "src/dawn/wire/client/EventManager.h"
#include "src/utils/compiler.h"

namespace dawn::wire::client {
namespace {

// Returns either an error buffer or null, depending on mappedAtCreation.
[[nodiscard]] WGPUBuffer ReturnOOMAtClient(Device* device, const WGPUBufferDescriptor* descriptor) {
    if (descriptor->mappedAtCreation) {
        return nullptr;
    }
    WGPUBufferDescriptor errorBufferDescriptor = *descriptor;
    WGPUDawnBufferDescriptorErrorInfoFromWireClient errorInfo = {};
    errorInfo.chain.sType = WGPUSType_DawnBufferDescriptorErrorInfoFromWireClient;
    errorInfo.outOfMemory = static_cast<WGPUBool>(true);
    errorBufferDescriptor.nextInChain = &errorInfo.chain;
    return device->APICreateErrorBuffer(&errorBufferDescriptor);
}

}  // anonymous namespace

class Buffer::MapAsyncEvent : public TrackedEvent {
  public:
    static constexpr EventType kType = EventType::MapAsync;

    MapAsyncEvent(const WGPUBufferMapCallbackInfo& callbackInfo, Ref<Buffer> buffer)
        : TrackedEvent(callbackInfo.mode),
          mCallback(callbackInfo.callback),
          mUserdata1(callbackInfo.userdata1),
          mUserdata2(callbackInfo.userdata2),
          mBuffer(buffer) {
        DAWN_ASSERT(mBuffer != nullptr);
    }

    EventType GetType() override { return kType; }

    WireResult ReadyHook(FutureID futureID,
                         WGPUMapAsyncStatus status,
                         WGPUStringView message,
                         size_t readDataUpdateInfoLength = 0,
                         const uint8_t* readDataUpdateInfo = nullptr) {
        auto FailRequest = [this](const char* message) -> WireResult {
            mStatus = static_cast<WGPUMapAsyncStatus>(0);
            mMessage = message;
            return WireResult::FatalError;
        };

        return mBuffer->mState.Use([&](auto state) {
            // If we got a failure status, we always override the last one. Note that this can lead
            // to some indeterministic results since the server and the user could race different
            // non-success results. That said, given that it's a non-success result, the race only
            // determines what the user sees as the error message and status.
            if (status != WGPUMapAsyncStatus_Success) {
                mStatus = status;
                mMessage = ToString(message);
                return WireResult::Success;
            }

            // If the request was already aborted via the client side, we don't need to do anything,
            // so just return success.
            if (!state->PendingRequestIs(futureID)) {
                return WireResult::Success;
            }

            const auto& pending = *state->pendingMapRequest;
            if (!pending.type) {
                return FailRequest("Invalid map call without a specified mapping type.");
            }
            switch (*pending.type) {
                case MapRequestType::Read: {
                    // Update user map data with server returned data
                    std::span<const uint8_t> readDataUpdateInfoSpan(readDataUpdateInfo,
                                                                    readDataUpdateInfoLength);
                    if (!state->readHandle->DeserializeDataUpdate(readDataUpdateInfoSpan,
                                                                  pending.offset)) {
                        return FailRequest("Failed to deserialize data returned from the server.");
                    }
                    state->mappedData = std::span(reinterpret_cast<std::byte*>(const_cast<void*>(
                                                      state->readHandle->GetData())),
                                                  mBuffer->mSize);
                    break;
                }
                case MapRequestType::Write: {
                    state->mappedData =
                        std::span(reinterpret_cast<std::byte*>(state->writeHandle->GetData()),
                                  mBuffer->mSize);
                    break;
                }
            }
            state->mappedOffset = pending.offset;
            state->mappedSize = pending.size;

            return WireResult::Success;
        });
    }

  private:
    void CompleteImpl(FutureID futureID, EventCompletionType completionType) override {
        auto Callback = [&]() {
            if (mCallback) {
                mCallback(mStatus, ToOutputStringView(mMessage), mUserdata1.ExtractAsDangling(),
                          mUserdata2.ExtractAsDangling());
            }
        };

        return mBuffer->mState.Use([&](auto state) {
            if (completionType == EventCompletionType::Shutdown) {
                mStatus = WGPUMapAsyncStatus_CallbackCancelled;
                mMessage = "A valid external Instance reference no longer exists.";
            }

            // The request has been cancelled before completion, return that result.
            if (!state->PendingRequestIs(futureID)) {
                DAWN_ASSERT(mStatus != WGPUMapAsyncStatus_Success);
                return Callback();
            }

            // Device destruction/loss implicitly makes the map requests aborted.
            if (!mBuffer->mDevice->IsAlive()) {
                mStatus = WGPUMapAsyncStatus_Aborted;
                mMessage = "The Device was lost before mapping was resolved.";
            }

            if (mStatus == WGPUMapAsyncStatus_Success) {
                DAWN_ASSERT(state->pendingMapRequest && state->pendingMapRequest->type);
                switch (*state->pendingMapRequest->type) {
                    case MapRequestType::Read:
                        state->mappedState = MapState::MappedForRead;
                        break;
                    case MapRequestType::Write:
                        state->mappedState = MapState::MappedForWrite;
                        break;
                }
            }
            state->pendingMapRequest = std::nullopt;
            return Callback();
        });
    }

    WGPUBufferMapCallback mCallback;
    raw_ptr<void> mUserdata1;
    raw_ptr<void> mUserdata2;

    // The response for the map async callback are implicitly protected by the mutex protecting the
    // map state in the Buffer.
    WGPUMapAsyncStatus mStatus = WGPUMapAsyncStatus_Success;
    std::string mMessage;

    // Strong reference to the buffer for synchronization purposes.
    Ref<Buffer> mBuffer;
};

// static
WGPUBuffer Buffer::Create(Device* device, const WGPUBufferDescriptor* descriptor) {
    Client* wireClient = device->GetClient();

    bool fakeOOMAtWireClientMap = false;
    for (const auto* chain = descriptor->nextInChain; chain != nullptr; chain = chain->next) {
        switch (chain->sType) {
            case WGPUSType_DawnFakeBufferOOMForTesting: {
                auto oomForTesting =
                    reinterpret_cast<const WGPUDawnFakeBufferOOMForTesting*>(chain);
                fakeOOMAtWireClientMap = (oomForTesting->fakeOOMAtWireClientMap != 0u);
            } break;
            default:
                break;
        }
    }

    bool mappable =
        (descriptor->usage & (WGPUBufferUsage_MapRead | WGPUBufferUsage_MapWrite)) != 0 ||
        wgpu::Bool(descriptor->mappedAtCreation);
    if (mappable &&
        (descriptor->size >= std::numeric_limits<size_t>::max() || fakeOOMAtWireClientMap)) {
        return ReturnOOMAtClient(device, descriptor);
    }

    std::unique_ptr<MemoryTransferService::ReadHandle> readHandle = nullptr;
    std::unique_ptr<MemoryTransferService::WriteHandle> writeHandle = nullptr;

    DeviceCreateBufferCmd cmd;
    cmd.deviceId = device->GetWireHandle(wireClient).id;
    cmd.descriptor = descriptor;
    // Set the pointer lengths, but the pointed-to data itself won't be serialized as usual (due
    // to skip_serialize). Instead, the custom CommandExtensions below fill that memory.
    cmd.readHandleCreateInfoLength = 0;
    cmd.readHandleCreateInfo = nullptr;  // Skipped by skip_serialize.
    cmd.writeHandleCreateInfoLength = 0;
    cmd.writeHandleCreateInfo = nullptr;  // Skipped by skip_serialize.

    size_t readHandleCreateInfoLength = 0;
    size_t writeHandleCreateInfoLength = 0;
    if (mappable) {
        if ((descriptor->usage & WGPUBufferUsage_MapRead) != 0) {
            // Create the read handle on buffer creation.
            readHandle.reset(
                wireClient->GetMemoryTransferService()->CreateReadHandle(descriptor->size));
            if (readHandle == nullptr) {
                return ReturnOOMAtClient(device, descriptor);
            }
            readHandleCreateInfoLength = readHandle->SerializeCreateSize();
            cmd.readHandleCreateInfoLength = readHandleCreateInfoLength;
        }

        if ((descriptor->usage & WGPUBufferUsage_MapWrite) != 0 || descriptor->mappedAtCreation) {
            // Create the write handle on buffer creation.
            writeHandle.reset(
                wireClient->GetMemoryTransferService()->CreateWriteHandle(descriptor->size));
            if (writeHandle == nullptr) {
                return ReturnOOMAtClient(device, descriptor);
            }
            writeHandleCreateInfoLength = writeHandle->SerializeCreateSize();
            cmd.writeHandleCreateInfoLength = writeHandleCreateInfoLength;
        }
    }

    // Create the buffer and send the creation command.
    // This must happen after any potential error buffer creation
    // as server expects allocating ids to be monotonically increasing
    Ref<Buffer> buffer =
        wireClient->Make<Buffer>(device->GetEventManagerHandle(), device, descriptor);

    buffer->mState.Use([&](auto state) {
        state->readHandle = std::move(readHandle);
        state->writeHandle = std::move(writeHandle);

        if (descriptor->mappedAtCreation) {
            // If the buffer is mapped at creation, a write handle is created and will be
            // destructed on unmap if the buffer doesn't have MapWrite usage
            // The buffer is mapped right now.
            state->mappedState = MapState::MappedAtCreation;
            state->mappedOffset = 0;
            state->mappedSize = buffer->mSize;
            DAWN_ASSERT(state->writeHandle != nullptr);
            state->mappedData = std::span(
                reinterpret_cast<std::byte*>(state->writeHandle->GetData()), buffer->mSize);
        }

        cmd.result = buffer->GetWireHandle(wireClient);

        // clang-format off
        // Turning off clang format here because for some reason it does not format the
        // CommandExtensions consistently, making it harder to read.
        wireClient->SerializeCommand(
            cmd,
            // Extensions to replace fields skipped by skip_serialize.
            CommandExtension{readHandleCreateInfoLength,
                             [&](char* readHandleBuffer) {
                                 if (state->readHandle != nullptr) {
                                     // Serialize the ReadHandle into the space after the command.
                                     state->readHandle->SerializeCreate(readHandleBuffer);
                                 }
                             }},
            CommandExtension{writeHandleCreateInfoLength,
                             [&](char* writeHandleBuffer) {
                                 if (state->writeHandle != nullptr) {
                                     // Serialize the WriteHandle into the space after the command.
                                     state->writeHandle->SerializeCreate(writeHandleBuffer);
                                 }
                             }}
        );
        // clang-format on
    });

    return ReturnToAPI(std::move(buffer));
}

// static
WGPUBuffer Buffer::CreateError(Device* device, const WGPUBufferDescriptor* descriptor) {
    if (descriptor->mappedAtCreation) {
        // This codepath isn't used (at the time of this writing). Just return nullptr
        // (pretend there was a mapping OOM), so we don't have to bother mapping the ErrorBuffer
        // (would have to return nullptr anyway if there was actually an OOM).
        std::string error = "mappedAtCreation is not implemented for CreateErrorBuffer";
        device->HandleLogging(WGPULoggingType_Error, WGPUStringView{error.data(), error.size()});
        return nullptr;
    }

    Client* client = device->GetClient();
    Ref<Buffer> buffer = client->Make<Buffer>(device->GetEventManagerHandle(), device, descriptor);

    DeviceCreateErrorBufferCmd cmd;
    cmd.self = ToAPI(device);
    cmd.descriptor = descriptor;
    cmd.result = buffer->GetWireHandle(client);
    client->SerializeCommand(cmd);

    return ReturnToAPI(std::move(buffer));
}

Buffer::Buffer(const ObjectBaseParams& params,
               const ObjectHandle& eventManagerHandle,
               Device* device,
               const WGPUBufferDescriptor* descriptor)
    : RefCountedWithExternalCount<ObjectWithEventsBase>(params, eventManagerHandle),
      mSize(descriptor->size),
      mUsage(static_cast<WGPUBufferUsage>(descriptor->usage)),
      // This flag is for the write handle created by mappedAtCreation
      // instead of MapWrite usage. We don't have such a case for read handle.
      mDestructWriteHandleOnUnmap(wgpu::Bool(descriptor->mappedAtCreation) &&
                                  ((descriptor->usage & WGPUBufferUsage_MapWrite) == 0)),
      mDevice(device) {}

void Buffer::DeleteThis() {
    mState.Use([&](auto state) { FreeMappedData(state); });
    ObjectWithEventsBase::DeleteThis();
}

void Buffer::WillDropLastExternalRef() {
    mState.Use([this](auto state) {
        SetFutureStatus(state, WGPUMapAsyncStatus_Aborted,
                        "Buffer was destroyed before mapping was resolved.");
    });
}

ObjectType Buffer::GetObjectType() const {
    return ObjectType::Buffer;
}

void Buffer::SetFutureStatus(GuardedState& state,
                             WGPUMapAsyncStatus status,
                             std::string_view message) {
    if (!state->pendingMapRequest) {
        return;
    }

    FutureID futureID = state->pendingMapRequest->futureID;
    state->pendingMapRequest = std::nullopt;

    auto wireStatus = GetEventManager().SetFutureReady<MapAsyncEvent>(futureID, status,
                                                                      ToOutputStringView(message));
    DAWN_CHECK(wireStatus == WireResult::Success);
}

WGPUFuture Buffer::APIMapAsync(WGPUMapMode mode,
                               size_t offset,
                               size_t size,
                               const WGPUBufferMapCallbackInfo& callbackInfo) {
    Client* client = GetClient();
    auto [futureIDInternal, tracked] =
        GetEventManager().TrackEvent(AcquireRef(new MapAsyncEvent(callbackInfo, this)));
    if (!tracked) {
        return {futureIDInternal};
    }

    bool success = mState.Use([&](auto state) {
        if (state->pendingMapRequest) {
            [[maybe_unused]] auto id = GetEventManager().SetFutureReady<MapAsyncEvent>(
                futureIDInternal, WGPUMapAsyncStatus_Error,
                ToOutputStringView("Buffer already has an outstanding map pending."));
            return false;
        }

        // Handle the defaulting of size required by WebGPU.
        if ((size == WGPU_WHOLE_MAP_SIZE) && (offset <= mSize)) {
            size = mSize - offset;
        }

        // Set up the request structure that will hold information while this mapping is in flight.
        std::optional<MapRequestType> mapMode;
        if (mode & WGPUMapMode_Read) {
            mapMode = MapRequestType::Read;
        } else if (mode & WGPUMapMode_Write) {
            mapMode = MapRequestType::Write;
        }
        state->pendingMapRequest = {futureIDInternal, offset, size, mapMode};
        return true;
    });
    if (!success) {
        return {futureIDInternal};
    }

    // Serialize the command to send to the server.
    BufferMapAsyncCmd cmd;
    cmd.bufferId = GetWireHandle(client).id;
    cmd.eventManagerHandle = GetEventManagerHandle();
    cmd.future = {futureIDInternal};
    cmd.mode = mode;
    cmd.offset = offset;
    cmd.size = size;

    client->SerializeCommand(cmd);
    return {futureIDInternal};
}

WireResult Client::DoBufferMapAsyncCallback(ObjectHandle eventManager,
                                            WGPUFuture future,
                                            WGPUMapAsyncStatus status,
                                            WGPUStringView message,
                                            size_t readDataUpdateInfoLength,
                                            const uint8_t* readDataUpdateInfo) {
    return SetFutureReady<Buffer::MapAsyncEvent>(eventManager, future.id, status, message,
                                                 readDataUpdateInfoLength, readDataUpdateInfo);
}

void* Buffer::APIGetMappedRange(size_t offset, size_t size) {
    return mState.Use([&](auto state) -> void* {
        if (!state->IsMappedForWriting()) {
            if (state->IsMappedForReading()) {
                std::string error =
                    "GetMappedRange: Mapping is read-only. Use GetConstMappedRange instead.";
                mDevice->HandleLogging(WGPULoggingType_Error,
                                       WGPUStringView{error.data(), error.size()});
            }
            return nullptr;
        }
        return DAWN_UNSAFE_BUFFERS(state->GetMappedRange(offset, size).data());
    });
}

const void* Buffer::APIGetConstMappedRange(size_t offset, size_t size) {
    return mState.Use([&](auto state) -> const void* {
        if (!(state->IsMappedForWriting() || state->IsMappedForReading())) {
            return nullptr;
        }
        return DAWN_UNSAFE_BUFFERS(state->GetMappedRange(offset, size).data());
    });
}

WGPUStatus Buffer::APIWriteMappedRange(size_t offset, void const* data, size_t size) {
    return mState.Use([&](auto state) {
        auto dst = state->GetMappedRange(offset, size);
        if (dst.empty()) {
            return WGPUStatus_Error;
        }
        auto src = std::span<const std::byte>(reinterpret_cast<const std::byte*>(data), size);
        std::ranges::copy(src, dst.begin());
        return WGPUStatus_Success;
    });
}

WGPUStatus Buffer::APIReadMappedRange(size_t offset, void* data, size_t size) {
    return mState.Use([&](auto state) {
        auto src = state->GetMappedRange(offset, size);
        if (src.empty()) {
            return WGPUStatus_Error;
        }
        auto dst = std::span<std::byte>(reinterpret_cast<std::byte*>(data), size);
        std::ranges::copy(src, dst.begin());
        return WGPUStatus_Success;
    });
}

void Buffer::APIUnmap() {
    // Invalidate the local pointer, and cancel all other in-flight requests that would
    // turn into errors anyway (you can't double map). This prevents race when the following
    // happens, where the application code would have unmapped a buffer but still receive a
    // callback:
    //   - Client -> Server: MapRequest1, Unmap, MapRequest2
    //   - Server -> Client: Result of MapRequest1
    //   - Unmap locally on the client
    //   - Server -> Client: Result of MapRequest2
    Client* client = GetClient();

    mState.Use([&](auto state) {
        if (state->IsMappedForWriting()) {
            // Writes need to be flushed before Unmap is sent. Unmap calls all associated
            // in-flight callbacks which may read the updated data.
            DAWN_ASSERT(state->writeHandle != nullptr);

            // Get the serialization size of data update writes.
            size_t writeDataUpdateInfoLength = state->writeHandle->SizeOfSerializeDataUpdate(
                state->mappedOffset, state->mappedSize);

            BufferUpdateMappedDataCmd cmd{};
            cmd.bufferId = GetWireHandle(client).id;
            // Set the pointer length, but the pointed-to data itself won't be serialized as usual
            // (due to skip_serialize). Instead, the custom CommandExtension below fills that
            // memory.
            cmd.writeDataUpdateInfoLength = writeDataUpdateInfoLength;
            cmd.writeDataUpdateInfo = nullptr;  // Skipped by skip_serialize.
            cmd.offset = state->mappedOffset;
            cmd.size = state->mappedSize;

            client->SerializeCommand(
                cmd,
                // Extensions to replace fields skipped by skip_serialize.
                CommandExtension{writeDataUpdateInfoLength, [&](char* writeHandleBuffer) {
                                     // Serialize flush metadata into the space after the command.
                                     // This closes the handle for writing.
                                     std::span<char> writeHandleBufferSpan(
                                         writeHandleBuffer, writeDataUpdateInfoLength);
                                     state->writeHandle->SerializeDataUpdate(writeHandleBufferSpan,
                                                                             cmd.offset);
                                 }});

            // If mDestructWriteHandleOnUnmap is true, that means the write handle is merely
            // for mappedAtCreation usage. It is destroyed on unmap after flush to server
            // instead of at buffer destruction.
            if (mDestructWriteHandleOnUnmap) {
                state->mappedData = {};
                state->writeHandle = nullptr;
                if (state->readHandle) {
                    // If it's both mappedAtCreation and MapRead we need to reset
                    // mData to readHandle's GetData(). This could be changed to
                    // merging read/write handle in future
                    state->mappedData = std::span(reinterpret_cast<std::byte*>(const_cast<void*>(
                                                      state->readHandle->GetData())),
                                                  mSize);
                }
            }
        }

        // Free map access tokens
        state->mappedState = MapState::Unmapped;
        state->mappedOffset = 0;
        state->mappedSize = 0;

        SetFutureStatus(state, WGPUMapAsyncStatus_Aborted,
                        "Buffer was unmapped before mapping was resolved.");
    });

    BufferUnmapCmd cmd{};
    cmd.self = ToAPI(this);
    client->SerializeCommand(cmd);
}

void Buffer::APIDestroy() {
    Client* client = GetClient();

    // Remove the current mapping and destroy Read/WriteHandles.
    mState.Use([&](auto state) {
        SetFutureStatus(state, WGPUMapAsyncStatus_Aborted,
                        "Buffer was destroyed before mapping was resolved.");
        FreeMappedData(state);
    });

    BufferDestroyCmd cmd{};
    cmd.self = ToAPI(this);
    client->SerializeCommand(cmd);
}

WGPUBufferUsage Buffer::APIGetUsage() const {
    return mUsage;
}

uint64_t Buffer::APIGetSize() const {
    return mSize;
}

WGPUBufferMapState Buffer::APIGetMapState() const {
    return mState.Use([](auto state) {
        switch (state->mappedState) {
            case MapState::MappedForRead:
            case MapState::MappedForWrite:
            case MapState::MappedAtCreation:
                return WGPUBufferMapState_Mapped;
            case MapState::Unmapped:
                if (state->pendingMapRequest) {
                    return WGPUBufferMapState_Pending;
                } else {
                    return WGPUBufferMapState_Unmapped;
                }
        }
        DAWN_UNREACHABLE();
    });
}

void Buffer::FreeMappedData(GuardedState& state) {
#if defined(DAWN_ENABLE_ASSERTS)
    // When in "debug" mode, 0xCA-out the mapped data when we free it so that in we can detect
    // use-after-free of the mapped data. This is particularly useful for WebGPU test about the
    // interaction of mapping and GC.
    if (!state->mappedData.empty()) {
        std::ranges::fill(state->mappedData, std::byte(0xCA));
    }
#endif  // defined(DAWN_ENABLE_ASSERTS)

    state->mappedOffset = 0;
    state->mappedSize = 0;
    state->mappedData = {};
    state->readHandle = nullptr;
    state->writeHandle = nullptr;
    state->mappedState = MapState::Unmapped;
}

bool Buffer::State::IsMappedForReading() const {
    return mappedState == MapState::MappedForRead;
}

bool Buffer::State::IsMappedForWriting() const {
    return mappedState == MapState::MappedForWrite || mappedState == MapState::MappedAtCreation;
}

std::span<std::byte> Buffer::State::GetMappedRange(size_t offset, size_t size) const {
    auto CanGetMappedRange = [&]() -> bool {
        size_t bufferSize = mappedData.size();
        if (offset % 8 != 0 || offset < mappedOffset || offset > bufferSize) {
            return false;
        }

        size_t rangeSize = size == WGPU_WHOLE_MAP_SIZE ? bufferSize - offset : size;

        if (rangeSize % 4 != 0 || rangeSize > mappedSize) {
            return false;
        }

        size_t offsetInMappedRange = offset - mappedOffset;
        return offsetInMappedRange <= mappedSize - rangeSize;
    };

    if (CanGetMappedRange()) {
        return mappedData.subspan(offset, size);
    }
    return {};
}

bool Buffer::State::PendingRequestIs(FutureID futureID) const {
    return pendingMapRequest && pendingMapRequest->futureID == futureID;
}

}  // namespace dawn::wire::client
