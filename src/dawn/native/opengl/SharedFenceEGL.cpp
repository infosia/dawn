// Copyright 2024 The Dawn & Tint Authors
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

#include "dawn/native/opengl/SharedFenceEGL.h"

#include "dawn/native/ChainUtils.h"
#include "dawn/native/SystemHandle.h"
#include "dawn/native/opengl/DeviceGL.h"
#include "dawn/native/opengl/EGLFunctions.h"

namespace dawn::native::opengl {
ResultOrError<Ref<SharedFence>> SharedFenceEGL::Create(
    Device* device,
    StringView label,
    const SharedFenceSyncFDDescriptor* descriptor) {
#if DAWN_PLATFORM_IS(POSIX)
    DAWN_INVALID_IF(descriptor->handle < 0, "File descriptor (%d) was invalid.",
                    descriptor->handle);

    SystemHandle ownedHandle;
    DAWN_TRY_ASSIGN(ownedHandle, SystemHandle::Duplicate(descriptor->handle));

    const EGLAttrib attribs[] = {
        EGL_SYNC_NATIVE_FENCE_FD_ANDROID,
        ownedHandle.Get(),
        EGL_NONE,
    };

    Ref<WrappedEGLSync> sync;
    DAWN_TRY_ASSIGN(sync, WrappedEGLSync::Create(device, EGL_SYNC_NATIVE_FENCE_ANDROID, attribs));

    // If EGLSync creation succeeded, the sync now owns the handle.
    ownedHandle.Detach();

    auto fence = AcquireRef(new SharedFenceEGL(device, label, wgpu::SharedFenceType::SyncFD, sync));
    return fence;
#else
    DAWN_UNREACHABLE();
#endif
}

SharedFenceEGL::SharedFenceEGL(Device* device,
                               StringView label,
                               wgpu::SharedFenceType type,
                               Ref<WrappedEGLSync> sync)
    : SharedFence(device, label), mType(type), mSync(sync) {}

void SharedFenceEGL::ServerWait(uint64_t signaledValue) {
    // All GL sync objects are binary, this should be validated at SharedTextureMemory::BeginAccess.
    DAWN_ASSERT(signaledValue == 1);

    mSync->Wait();
}

MaybeError SharedFenceEGL::ExportInfoImpl(UnpackedPtr<SharedFenceExportInfo>& info) const {
    info->type = mType;

    switch (mType) {
#if DAWN_PLATFORM_IS(POSIX)
        case wgpu::SharedFenceType::SyncFD:
            DAWN_TRY(info.ValidateSubset<SharedFenceSyncFDExportInfo>());
            {
                SharedFenceSyncFDExportInfo* exportInfo = info.Get<SharedFenceSyncFDExportInfo>();
                if (exportInfo != nullptr) {
                    // Lazily duplicate the handle from the EGLSync for export. Unlike EGL, dawn
                    // maintains ownership of handles returned to the user.
                    if (!mHandle.IsValid()) {
                        Device* device = ToBackend(GetDevice());
                        const EGLFunctions& egl = device->GetEGL(false);

                        EGLint fd = egl.DupNativeFenceFD(device->GetEGLDisplay(), mSync.Get());
                        DAWN_INVALID_IF(fd == EGL_NO_NATIVE_FENCE_FD_ANDROID,
                                        "eglDupNativeFenceFDANDROID failed, 0x%X", egl.GetError());

                        mHandle.Acquire(fd);
                    }

                    exportInfo->handle = mHandle.Get();
                }
            }
            break;
#endif
        default:
            DAWN_UNREACHABLE();
    }

    return {};
}

}  // namespace dawn::native::opengl
