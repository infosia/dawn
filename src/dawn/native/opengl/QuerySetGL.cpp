// Copyright 2020 The Dawn & Tint Authors
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

#include "dawn/native/opengl/QuerySetGL.h"

#include <utility>
#include <vector>

#include "dawn/native/opengl/DeviceGL.h"
#include "dawn/native/opengl/FutureHandle.h"
#include "dawn/native/opengl/QueueGL.h"
#include "dawn/native/opengl/UtilsGL.h"

namespace dawn::native::opengl {

// static
ResultOrError<Ref<QuerySet>> QuerySet::Create(Device* device,
                                              const QuerySetDescriptor* descriptor) {
    Ref<QuerySet> querySet = AcquireRef(new QuerySet(device, descriptor));
    DAWN_TRY(querySet->Initialize());
    return querySet;
}

QuerySet::QuerySet(Device* device, const QuerySetDescriptor* descriptor)
    : QuerySetBase(device, descriptor) {
    mHandles.resize(descriptor->count);
}

MaybeError QuerySet::Initialize() {
    Device* device = ToBackend(GetDevice());
    Queue* queue = ToBackend(device->GetQueue());
    uint32_t count = mHandles.size();

    Ref<QuerySet> self = this;
    DAWN_TRY(queue->AddGLTask([self, device, count]() mutable -> MaybeError {
        const OpenGLFunctions& gl = device->GetGL();
        std::vector<GLuint> glHandles(count);
        if (count > 0) {
            DAWN_GL_TRY(gl, GenQueries(count, glHandles.data()));
        }
        for (uint32_t i = 0; i < count; ++i) {
            self->mHandles[i]->Set(glHandles[i]);
        }
        return {};
    }));

    return {};
}

QuerySet::~QuerySet() = default;

void QuerySet::DestroyImpl() {
    Device* device = ToBackend(GetDevice());
    Queue* queue = ToBackend(device->GetQueue());
    std::vector<Ref<FutureHandle>> handles = std::move(mHandles);

    IgnoreErrors(queue->AddGLTask([device, handles = std::move(handles)]() -> MaybeError {
        std::vector<GLuint> glHandles;
        glHandles.reserve(handles.size());
        for (const auto& handle : handles) {
            glHandles.push_back(handle->GetOrDefault());
        }

        const OpenGLFunctions& gl = device->GetGL();
        if (!glHandles.empty()) {
            DAWN_GL_TRY_IGNORE_ERRORS(gl, DeleteQueries(glHandles.size(), glHandles.data()));
        }
        return {};
    }));

    QuerySetBase::DestroyImpl();
}

const FutureHandle& QuerySet::Get(uint32_t index) const {
    DAWN_ASSERT(index < mHandles.size());
    return *mHandles[index].Get();
}

}  // namespace dawn::native::opengl
