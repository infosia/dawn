#include "dawn/native/opengl/BufferGL.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "dawn/native/ChainUtils.h"
#include "dawn/native/CommandBuffer.h"
#include "dawn/native/opengl/DeviceGL.h"
#include "dawn/native/opengl/QueueGL.h"
#include "dawn/native/opengl/UtilsGL.h"

namespace dawn::native::opengl {

// Buffer

// static
ResultOrError<Ref<Buffer>> Buffer::CreateInternalBuffer(Device* device,
                                                        const BufferDescriptor* descriptor,
                                                        bool shouldLazyClear) {
    Ref<Buffer> buffer = AcquireRef(new Buffer(device, Unpack(descriptor)));
    DAWN_TRY(buffer->Initialize(shouldLazyClear, descriptor->mappedAtCreation));

    if (descriptor->mappedAtCreation) {
        [[maybe_unused]] bool usingStagingBuffer;
        DAWN_TRY_ASSIGN(usingStagingBuffer, buffer->MapAtCreationInternal());
    }

    return std::move(buffer);
}

// static
ResultOrError<Ref<Buffer>> Buffer::Create(Device* device,
                                          const UnpackedPtr<BufferDescriptor>& descriptor) {
    Ref<Buffer> buffer = AcquireRef(new Buffer(device, descriptor));
    DAWN_TRY(buffer->Initialize(/*shouldLazyClear=*/true, descriptor->mappedAtCreation));
    return buffer;
}

Buffer::Buffer(Device* device, const UnpackedPtr<BufferDescriptor>& descriptor)
    : BufferBase(device, descriptor), mHandle(AcquireRef(new FutureHandle())) {
    // Allocate at least 4 bytes so clamped accesses are always in bounds.
    // Align with 4 byte to avoid out-of-bounds access issue in compute emulation for 2 byte
    // element.
    uint64_t alignment = 4u;
    // Round uniform buffer sizes up to a multiple of 16 bytes since Tint will polyfill them as
    // array<vec4u, ...>.
    if (GetUsage() & wgpu::BufferUsage::Uniform) {
        alignment = 16u;
    }
    mAllocatedSize = Align(std::max(GetSize(), uint64_t(4u)), alignment);
}

MaybeError Buffer::Initialize(bool shouldLazyClear, bool isMappedAtCreation) {
    if (!shouldLazyClear) {
        SetInitialized(true);
    }

    if (isMappedAtCreation) {
        mShadowCopy.resize(mAllocatedSize);
        // GL buffer will be created after mapAtCreation pointer is unmapped.
        return {};
    }

    DAWN_TRY(InitializeGLBuffer({}));

    TrackUsage();

    return {};
}

MaybeError Buffer::InitializeGLBuffer(std::vector<uint8_t> initialData) {
    Device* device = ToBackend(GetDevice());
    Ref<Buffer> self = this;
    return device->EnqueueGL([self, device, initialData = std::move(initialData)](
                                 const OpenGLFunctions& gl) mutable -> MaybeError {
        GLuint glHandle = 0;
        DAWN_GL_TRY(gl, GenBuffers(1, &glHandle));
        DAWN_GL_TRY(gl, BindBuffer(GL_ARRAY_BUFFER, glHandle));
        self->mHandle->Set(glHandle);

        // The buffers with mappedAtCreation == true will be initialized in
        // BufferBase::MapAtCreation().
        if (device->IsToggleEnabled(Toggle::NonzeroClearResourcesOnCreationForTesting) &&
            initialData.empty()) {
            std::vector<uint8_t> clearValues(self->mAllocatedSize, 1u);
            DAWN_GL_TRY_ALWAYS_CHECK(gl, BufferData(GL_ARRAY_BUFFER, self->mAllocatedSize,
                                                    clearValues.data(), GL_STATIC_DRAW));
        } else {
            const auto data = initialData.empty() ? nullptr : initialData.data();
            DAWN_GL_TRY_ALWAYS_CHECK(
                gl, BufferData(GL_ARRAY_BUFFER, self->mAllocatedSize, data, GL_STATIC_DRAW));
        }

        return {};
    });
}

Buffer::~Buffer() = default;

const FutureHandle& Buffer::GetHandle() const {
    return *mHandle.Get();
}

MaybeError Buffer::EnsureDataInitialized(const OpenGLFunctions& gl,
                                         bool* outDidDataInitialization) {
    if (!NeedsInitialization()) {
        if (outDidDataInitialization) {
            *outDidDataInitialization = false;
        }
        return {};
    }

    DAWN_TRY(InitializeToZero(gl));
    if (outDidDataInitialization) {
        *outDidDataInitialization = true;
    }
    return {};
}

MaybeError Buffer::EnsureDataInitializedAsDestination(const OpenGLFunctions& gl,
                                                      uint64_t offset,
                                                      uint64_t size,
                                                      bool* outDidDataInitialization) {
    if (!NeedsInitialization()) {
        if (outDidDataInitialization) {
            *outDidDataInitialization = false;
        }
        return {};
    }

    if (IsFullBufferRange(offset, size)) {
        SetInitialized(true);
        if (outDidDataInitialization) {
            *outDidDataInitialization = false;
        }
        return {};
    }

    DAWN_TRY(InitializeToZero(gl));
    if (outDidDataInitialization) {
        *outDidDataInitialization = true;
    }
    return {};
}

MaybeError Buffer::EnsureDataInitializedAsDestination(const OpenGLFunctions& gl,
                                                      const CopyTextureToBufferCmd* copy,
                                                      bool* outDidDataInitialization) {
    if (!NeedsInitialization()) {
        if (outDidDataInitialization) {
            *outDidDataInitialization = false;
        }
        return {};
    }

    if (IsFullBufferOverwrittenInTextureToBufferCopy(copy)) {
        SetInitialized(true);
        if (outDidDataInitialization) {
            *outDidDataInitialization = false;
        }
        return {};
    }

    DAWN_TRY(InitializeToZero(gl));
    if (outDidDataInitialization) {
        *outDidDataInitialization = true;
    }
    return {};
}

MaybeError Buffer::InitializeToZero(const OpenGLFunctions& gl) {
    DAWN_ASSERT(NeedsInitialization());

    const uint64_t size = GetAllocatedSize();
    const std::vector<uint8_t> clearValues(size, 0u);

    DAWN_GL_TRY(gl, BindBuffer(GL_ARRAY_BUFFER, GetHandle()));
    DAWN_GL_TRY(gl, BufferSubData(GL_ARRAY_BUFFER, 0, size, clearValues.data()));

    ToBackend(GetDevice())->IncrementLazyClearCountForTesting();

    TrackUsage();
    SetInitialized(true);
    return {};
}

bool Buffer::IsCPUWritableAtCreation() const {
    // We use shadow copy for mapAtCreation, so CPU access is always true.
    return true;
}

MaybeError Buffer::MapAtCreationImpl() {
    // We will need shadow copy to implement mapAtCreation.
    DAWN_ASSERT(!mShadowCopy.empty());
    mMappedData = mShadowCopy.data();
    return {};
}

MaybeError Buffer::MapAsyncImpl(wgpu::MapMode mode, size_t offset, size_t size) {
    Device* device = ToBackend(GetDevice());
    Ref<Buffer> self = this;

    if (NeedsInitialization()) {
        DAWN_TRY(device->EnqueueGL(
            [self](const auto& gl) -> MaybeError { return self->EnsureDataInitialized(gl); }));
    }

    // Will be handled in FinalizeMapImpl()
    DAWN_ASSERT(mShadowCopy.empty());
    return {};
}

MaybeError Buffer::FinalizeMapImpl(BufferState newState) {
    if (newState == BufferState::MappedAtCreation) {
        // mapAtCreation uses shadow copy so nothing to do here.
        return {};
    }

    DAWN_ASSERT(newState == BufferState::Mapped);

    Device* device = ToBackend(GetDevice());
    auto deviceGuard = device->GetGuard();
    void* mappedData = nullptr;
    auto size = MapSize();
    auto offset = MapOffset();
    const auto mode = MapMode();

    // It is an error to map an empty range in OpenGL. We always have at least a 4-byte
    // buffer
    // so we extend the range to be 4 bytes.
    if (size == 0) {
        if (offset != 0) {
            offset -= 4;
        }
        size = 4;
    }

    // Since FinalizeMap() is called when EventManager detects that all GPU uses of this buffer have
    // completed, it's safe to use ExecuteGL() to trigger GL command immediately.
    // TODO(451928481): We assume this is called on a thread that can make GL context current, e.g.
    // playback thread in Skia.
    auto glOperation = [&](const OpenGLFunctions& gl) mutable -> MaybeError {
        // UnmapImpl enqueues an asynchronous unmap operation, which may not have executed when
        // this function is called. We need to ensure the buffer is unmapped before we can map
        // it again. Calling DoGLUnmapIfNeeded() directly handles this by unmapping only if it
        // is currently mapped.
        DoGLUnmapIfNeeded(gl);

        // This does GPU->CPU synchronization, we could require a high
        // version of OpenGL that would let us map the buffer unsynchronized.
        DAWN_GL_TRY(gl, BindBuffer(GL_ARRAY_BUFFER, GetHandle()));
        if (mode & wgpu::MapMode::Read) {
            mappedData = DAWN_GL_TRY_ALWAYS_CHECK(
                gl, MapBufferRange(GL_ARRAY_BUFFER, offset, size, GL_MAP_READ_BIT));
        } else {
            DAWN_ASSERT(mode & wgpu::MapMode::Write);
            mappedData = DAWN_GL_TRY_ALWAYS_CHECK(
                gl, MapBufferRange(GL_ARRAY_BUFFER, offset, size,
                                   GL_MAP_WRITE_BIT | GL_MAP_UNSYNCHRONIZED_BIT));
        }

        mGLBufferIsMapped = true;

        return {};
    };

    DAWN_TRY(device->ExecuteGL(ExecutionQueueBase::SubmitMode::Passive, glOperation));

    // The frontend asks that the pointer returned by GetMappedPointer is from the start of
    // the resource but OpenGL gives us the pointer at offset. Remove the offset.
    mMappedData = static_cast<uint8_t*>(mappedData) - offset;
    return {};
}

void* Buffer::GetMappedPointerImpl() {
    return mMappedData;
}

void Buffer::UnmapImpl(BufferState oldState) {
    Device* device = ToBackend(GetDevice());

    if (GetState() == BufferState::MappedAtCreation) {
        // Transfer shadow copy the real GL buffer.
        std::ignore = device->ConsumedError(InitializeGLBuffer(std::move(mShadowCopy)));
        return;
    }

    // Unmap() might be called in DestroyImpl(), we need to check whether it is the case or not.
    if (!this->TryAddRef()) {
        // TryAddRef() fails mean we are being destroyed, skip further steps.
        return;
    }

    // adopt ref
    Ref<Buffer> self = AcquireRef(this);

    IgnoreErrors(device->EnqueueGL(ExecutionQueueBase::SubmitMode::Passive,
                                   [self](const OpenGLFunctions& gl) -> MaybeError {
                                       self->DoGLUnmapIfNeeded(gl);
                                       return {};
                                   }));
}

void Buffer::DoGLUnmapIfNeeded(const OpenGLFunctions& gl) {
    if (!mGLBufferIsMapped) {
        return;
    }

    DAWN_GL_TRY_IGNORE_ERRORS(gl, BindBuffer(GL_ARRAY_BUFFER, GetHandle()));
    DAWN_GL_TRY_IGNORE_ERRORS(gl, UnmapBuffer(GL_ARRAY_BUFFER));

    mGLBufferIsMapped = false;
}

void Buffer::DestroyImpl() {
    BufferBase::DestroyImpl();

    Device* device = ToBackend(GetDevice());
    Ref<FutureHandle> handle = mHandle;

    IgnoreErrors(device->EnqueueGL(ExecutionQueueBase::SubmitMode::Passive,
                                   [handle](const OpenGLFunctions& gl) -> MaybeError {
                                       GLuint realHandle = handle->GetOrDefault();
                                       DAWN_GL_TRY_IGNORE_ERRORS(gl, DeleteBuffers(1, &realHandle));
                                       return {};
                                   }));
}

}  // namespace dawn::native::opengl
