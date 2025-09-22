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

#ifndef SRC_DAWN_WIRE_SERVER_SERVER_H_
#define SRC_DAWN_WIRE_SERVER_SERVER_H_

#include <functional>
#include <memory>
#include <optional>
#include <utility>

#include "dawn/common/MutexProtected.h"
#include "dawn/wire/ChunkedCommandSerializer.h"
#include "dawn/wire/server/ServerBase_autogen.h"
#include "partition_alloc/pointers/raw_ptr.h"

namespace dawn::wire::server {

class Server;
class MemoryTransferService;

template <auto F, typename _ = decltype(F)>
struct ForwardToServerHelper;

template <auto F, typename UserdataT, typename... Args>
struct ForwardToServerHelper<F, void (Server::*)(UserdataT*, Args...)> {
    using Userdata = UserdataT;

    static void Callback(Args... args, void* userdata, void* serverdata);
};

struct MapUserdata {
    ObjectHandle buffer;
    WGPUBuffer bufferObj;
    ObjectHandle eventManager;
    WGPUFuture future;
    uint64_t offset;
    uint64_t size;
    WGPUMapMode mode;
};

struct ErrorScopeUserdata {
    ObjectHandle device;
    ObjectHandle eventManager;
    WGPUFuture future;
};

struct ShaderModuleGetCompilationInfoUserdata {
    ObjectHandle eventManager;
    WGPUFuture future;
};

struct QueueWorkDoneUserdata {
    ObjectHandle queue;
    ObjectHandle eventManager;
    WGPUFuture future;
};

struct CreatePipelineAsyncUserData {
    ObjectHandle device;
    ObjectHandle eventManager;
    WGPUFuture future;
    ObjectId pipelineObjectID;
};

struct RequestAdapterUserdata {
    ObjectHandle eventManager;
    WGPUFuture future;
    ObjectId adapterObjectId;
};

struct RequestDeviceUserdata {
    ObjectHandle eventManager;
    WGPUFuture future;
    ObjectId deviceObjectId;
    WGPUFuture deviceLostFuture;
};

struct DeviceLostUserdata {
    ObjectHandle eventManager;
    WGPUFuture future;
};

class Server : public ServerBase {
  public:
    static std::shared_ptr<Server> Create(const WireServerDescriptor& descriptor);
    ~Server() override;

    // ChunkedCommandHandler implementation
    const volatile char* HandleCommandsImpl(const volatile char* commands, size_t size) override;

    WireResult InjectBuffer(WGPUBuffer buffer, const Handle& handle, const Handle& deviceHandle);
    WireResult InjectTexture(WGPUTexture texture, const Handle& handle, const Handle& deviceHandle);
    WireResult InjectSurface(WGPUSurface surface,
                             const Handle& handle,
                             const Handle& instanceHandle);
    WireResult InjectInstance(WGPUInstance instance, const Handle& handle);

    WGPUDevice GetDevice(uint32_t id, uint32_t generation);
    bool IsDeviceKnown(WGPUDevice device) const;

    // Flushes the command serialized from server->client if spontaneous callbacks are enabled.
    void Flush();

    template <typename CallbackInfo,
              auto F,
              WGPUCallbackMode DefaultMode = WGPUCallbackMode_AllowProcessEvents>
    CallbackInfo MakeCallbackInfo(ForwardToServerHelper<F>::Userdata* userdata) {
        return {nullptr, mUseSpontaneousCallbacks ? WGPUCallbackMode_AllowSpontaneous : DefaultMode,
                &ForwardToServerHelper<F>::Callback, userdata,
                std::make_unique<std::weak_ptr<Server>>(mSelf).release()};
    }

    void HandleCallback(std::function<void()>&& cb);

  private:
    Server(const WireServerDescriptor& descriptor);

    template <typename Cmd>
    void SerializeCommand(const Cmd& cmd) {
        mSerializer->SerializeCommand(cmd);
    }

    template <typename Cmd, typename... Extensions>
    void SerializeCommand(const Cmd& cmd, Extensions&&... es) {
        mSerializer->SerializeCommand(cmd, std::forward<Extensions>(es)...);
    }

    template <typename T>
    WireResult FillReservation(ObjectId id, T handle, Known<T>* known = nullptr) {
        auto result = Objects<T>().FillReservation(id, handle, known);
        if (result == WireResult::FatalError) {
            Release(mProcs, handle);
        }
        return result;
    }

    // Wrapper RAII helper for structs with FreeMember calls.
    template <typename Struct>
    class FreeMembers : public Struct {
      public:
        explicit FreeMembers(const DawnProcTable& procs) : Struct({}), mProcs(procs) {}
        ~FreeMembers() { (mProcs.*WGPUTraits<Struct>::FreeMembers)(*this); }

      private:
        const DawnProcTable& mProcs;
    };

    void SetForwardingDeviceCallbacks(Known<WGPUDevice> device);
    void ClearDeviceCallbacks(WGPUDevice device);

    // Error callbacks
    void OnUncapturedError(ObjectHandle device, WGPUErrorType type, WGPUStringView message);
    void OnLogging(ObjectHandle device, WGPULoggingType type, WGPUStringView message);

    // Async event callbacks
    void OnDeviceLost(DeviceLostUserdata* userdata,
                      WGPUDevice const* device,
                      WGPUDeviceLostReason reason,
                      WGPUStringView message);
    void OnDevicePopErrorScope(ErrorScopeUserdata* userdata,
                               WGPUPopErrorScopeStatus status,
                               WGPUErrorType type,
                               WGPUStringView message);
    void OnBufferMapAsyncCallback(MapUserdata* userdata,
                                  WGPUMapAsyncStatus status,
                                  WGPUStringView message);
    void OnQueueWorkDone(QueueWorkDoneUserdata* userdata,
                         WGPUQueueWorkDoneStatus status,
                         WGPUStringView message);
    void OnCreateComputePipelineAsyncCallback(CreatePipelineAsyncUserData* userdata,
                                              WGPUCreatePipelineAsyncStatus status,
                                              WGPUComputePipeline pipeline,
                                              WGPUStringView message);
    void OnCreateRenderPipelineAsyncCallback(CreatePipelineAsyncUserData* userdata,
                                             WGPUCreatePipelineAsyncStatus status,
                                             WGPURenderPipeline pipeline,
                                             WGPUStringView message);
    void OnShaderModuleGetCompilationInfo(ShaderModuleGetCompilationInfoUserdata* userdata,
                                          WGPUCompilationInfoRequestStatus status,
                                          const WGPUCompilationInfo* info);
    void OnRequestAdapterCallback(RequestAdapterUserdata* userdata,
                                  WGPURequestAdapterStatus status,
                                  WGPUAdapter adapter,
                                  WGPUStringView message);
    void OnRequestDeviceCallback(RequestDeviceUserdata* userdata,
                                 WGPURequestDeviceStatus status,
                                 WGPUDevice device,
                                 WGPUStringView message);

#include "dawn/wire/server/ServerPrototypes_autogen.inc"

    WireDeserializeAllocator mAllocator;
    MutexProtected<ChunkedCommandSerializer> mSerializer;
    DawnProcTable mProcs;
    std::unique_ptr<MemoryTransferService> mOwnedMemoryTransferService = nullptr;
    raw_ptr<MemoryTransferService> mMemoryTransferService = nullptr;

    std::optional<CallbackHandler> mCallbackHandler = std::nullopt;
    bool mUseSpontaneousCallbacks = false;

    // Weak pointer to self to facilitate creation of userdata.
    std::weak_ptr<Server> mSelf;
};

std::unique_ptr<MemoryTransferService> CreateInlineMemoryTransferService();

template <auto F, typename UserdataT, typename... Args>
void ForwardToServerHelper<F, void (Server::*)(UserdataT*, Args...)>::Callback(Args... args,
                                                                               void* userdata,
                                                                               void* serverdata) {
    // Acquire the server data.
    std::unique_ptr<std::weak_ptr<Server>> weakServer(
        static_cast<std::weak_ptr<Server>*>(serverdata));
    auto server = weakServer->lock();
    if (!server) {
        // Do nothing if the server has already been destroyed.
        return;
    }

    server.get()->HandleCallback([... args = std::forward<Args>(args), userdata, server]() {
        // Acquire the userdata, and cast it to UserdataT.
        std::unique_ptr<UserdataT> data(static_cast<UserdataT*>(userdata));
        // Forward the arguments and the typed userdata to the Server:: member function.
        (server.get()->*F)(data.get(), args...);
        server.get()->Flush();
    });
}

}  // namespace dawn::wire::server

#endif  // SRC_DAWN_WIRE_SERVER_SERVER_H_
