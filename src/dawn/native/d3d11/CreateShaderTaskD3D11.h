// Copyright 2025 The Dawn & Tint Authors
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

#ifndef SRC_DAWN_NATIVE_D3D11_CREATESHADERTASKD3D11_H_
#define SRC_DAWN_NATIVE_D3D11_CREATESHADERTASKD3D11_H_

#include <future>
#include <string>

#include "dawn/common/LRUCache.h"
#include "dawn/common/Sha3.h"
#include "dawn/native/AsyncTask.h"
#include "dawn/native/PerStage.h"
#include "dawn/native/d3d/D3DError.h"
#include "dawn/native/d3d/ShaderUtils.h"
#include "dawn/native/d3d/d3d_platform.h"
#include "dawn/platform/tracing/TraceEvent.h"

namespace dawn::platform {
class Platform;
}

namespace dawn::native::d3d11 {

namespace detail {

struct Sha3CacheFuncs {
    size_t operator()(const Sha3_256::Output& key) const {
        return absl::HashOf(absl::MakeSpan(key.data(), 8), absl::MakeSpan(key.data() + 24, 8));
    }
    bool operator()(const Sha3_256::Output& a, const Sha3_256::Output& b) const {
        return std::memcmp(a.data(), b.data(), sizeof(Sha3_256::Output)) == 0;
    }
};

template <SingleShaderStage kShaderStage>
struct CreateShaderTaskTraits {
    using Type = void;
};

template <>
struct CreateShaderTaskTraits<SingleShaderStage::Vertex> {
    using Type = ID3D11VertexShader;
};

template <>
struct CreateShaderTaskTraits<SingleShaderStage::Fragment> {
    using Type = ID3D11PixelShader;
};

template <>
struct CreateShaderTaskTraits<SingleShaderStage::Compute> {
    using Type = ID3D11ComputeShader;
};

}  // namespace detail

template <typename T>
using ShaderObjectCache = LRUCache<Sha3_256::Output, ComPtr<T>, detail::Sha3CacheFuncs>;

template <SingleShaderStage kShaderStage>
class CreateShaderTask : public AsyncTaskBase {
  public:
    using ShaderComPtr = ComPtr<typename detail::CreateShaderTaskTraits<kShaderStage>::Type>;
    using Cache = ShaderObjectCache<typename detail::CreateShaderTaskTraits<kShaderStage>::Type>;

    CreateShaderTask(ComPtr<ID3D11Device> d3d11Device,
                     dawn::platform::Platform* platform,
                     Cache& cache,
                     d3d::CompiledShader&& compiledShader,
                     std::promise<ResultOrError<ShaderComPtr>>&& promise,
                     std::string&& label)
        : mD3D11Device(std::move(d3d11Device)),
          mPlatform(platform),
          mCache(cache),
          mCompiledShader(std::move(compiledShader)),
          mPromise(std::move(promise)),
          mLabel(std::move(label)) {}

    ~CreateShaderTask() override = default;

  private:
    void RunImpl() override {
        static const char* traceLabel = nullptr;
        static const char* histogramLabel = nullptr;
        if constexpr (kShaderStage == SingleShaderStage::Vertex) {
            traceLabel = "DeviceD3D11::CreateVertexShader";
            histogramLabel = "D3D11.CreateVertexShaderUs";
        } else if constexpr (kShaderStage == SingleShaderStage::Fragment) {
            traceLabel = "DeviceD3D11::CreatePixelShader";
            histogramLabel = "D3D11.CreatePixelShaderUs";
        } else if constexpr (kShaderStage == SingleShaderStage::Compute) {
            traceLabel = "DeviceD3D11::CreateComputeShader";
            histogramLabel = "D3D11.CreateComputeShaderUs";
        } else {
            DAWN_UNREACHABLE();
        }

        TRACE_EVENT1(mPlatform, General, traceLabel, "label", mLabel.c_str());
        SCOPED_DAWN_HISTOGRAM_TIMER_MICROS(mPlatform, histogramLabel);

        mPromise.set_value(mCache.GetOrCreate(
            mCompiledShader.sha3, [&](const Sha3_256::Output&) -> ResultOrError<ShaderComPtr> {
                return CreateShaderImpl();
            }));
    }

    ResultOrError<ShaderComPtr> CreateShaderImpl() {
        ShaderComPtr shader;
        if constexpr (kShaderStage == SingleShaderStage::Vertex) {
            DAWN_TRY(CheckHRESULT(mD3D11Device->CreateVertexShader(
                                      mCompiledShader.shaderBlob.Data(),
                                      mCompiledShader.shaderBlob.Size(), nullptr, &shader),
                                  "D3D11 create vertex shader"));
        } else if constexpr (kShaderStage == SingleShaderStage::Fragment) {
            DAWN_TRY(CheckHRESULT(mD3D11Device->CreatePixelShader(mCompiledShader.shaderBlob.Data(),
                                                                  mCompiledShader.shaderBlob.Size(),
                                                                  nullptr, &shader),
                                  "D3D11 create pixel shader"));
        } else if constexpr (kShaderStage == SingleShaderStage::Compute) {
            DAWN_TRY(CheckHRESULT(mD3D11Device->CreateComputeShader(
                                      mCompiledShader.shaderBlob.Data(),
                                      mCompiledShader.shaderBlob.Size(), nullptr, &shader),
                                  "D3D11 create compute shader"));
        } else {
            DAWN_UNREACHABLE();
        }
        return shader;
    }

    ComPtr<ID3D11Device> mD3D11Device;
    dawn::platform::Platform* mPlatform;
    Cache& mCache;
    d3d::CompiledShader mCompiledShader;
    std::promise<ResultOrError<ShaderComPtr>> mPromise;
    std::string mLabel;
};

}  // namespace dawn::native::d3d11

#endif  // SRC_DAWN_NATIVE_D3D11_CREATESHADERTASKD3D11_H_
