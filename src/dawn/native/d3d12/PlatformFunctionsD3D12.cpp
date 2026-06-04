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

#include "src/dawn/native/d3d12/PlatformFunctionsD3D12.h"

#include <array>
#include <sstream>
#include <string>
#include <utility>

#include "src/dawn/common/SystemUtils.h"
#include "src/utils/log.h"

namespace dawn::native::d3d12 {

PlatformFunctions::PlatformFunctions() = default;
PlatformFunctions::~PlatformFunctions() = default;

MaybeError PlatformFunctions::Initialize(std::span<const std::string> searchPaths,
                                         bool useAgilitySDK) {
    DAWN_TRY(Base::Initialize());

    DAWN_TRY(LoadD3D12());
    DAWN_TRY(LoadD3D11());
    LoadPIXRuntime(searchPaths);
    if (useAgilitySDK) {
        // Soft-fails to the inbox d3d12.dll exports below when D3D12GetInterface or
        // CreateDeviceFactory are not available on the host system.
        TryInitializeAgilitySDKDeviceFactory();
    }
    return {};
}

MaybeError PlatformFunctions::LoadD3D12() {
#if DAWN_PLATFORM_IS(WINUWP)
    d3d12CreateDevice = &D3D12CreateDevice;
    d3d12GetDebugInterface = &D3D12GetDebugInterface;
    d3d12SerializeRootSignature = &D3D12SerializeRootSignature;
    d3d12CreateRootSignatureDeserializer = &D3D12CreateRootSignatureDeserializer;
    d3d12SerializeVersionedRootSignature = &D3D12SerializeVersionedRootSignature;
    d3d12CreateVersionedRootSignatureDeserializer = &D3D12CreateVersionedRootSignatureDeserializer;
#else
    std::string error;
    if (!mD3D12Lib.OpenSystemLibrary(L"d3d12.dll", &error) ||
        !mD3D12Lib.GetProc(&d3d12CreateDevice, "D3D12CreateDevice", &error) ||
        !mD3D12Lib.GetProc(&d3d12GetDebugInterface, "D3D12GetDebugInterface", &error) ||
        !mD3D12Lib.GetProc(&d3d12SerializeRootSignature, "D3D12SerializeRootSignature", &error) ||
        !mD3D12Lib.GetProc(&d3d12CreateRootSignatureDeserializer,
                           "D3D12CreateRootSignatureDeserializer", &error) ||
        !mD3D12Lib.GetProc(&d3d12SerializeVersionedRootSignature,
                           "D3D12SerializeVersionedRootSignature", &error) ||
        !mD3D12Lib.GetProc(&d3d12CreateVersionedRootSignatureDeserializer,
                           "D3D12CreateVersionedRootSignatureDeserializer", &error)) {
        return DAWN_INTERNAL_ERROR(error.c_str());
    }
    // Optional: only present in Agility SDK / newer d3d12.dll. Absence is not an error and is
    // handled by TryInitializeAgilitySDKDeviceFactory() gracefully falling back to System32 D3D12.
    mD3D12Lib.GetProc(&d3d12GetInterface, "D3D12GetInterface", &error);
#endif

    return {};
}

MaybeError PlatformFunctions::LoadD3D11() {
#if DAWN_PLATFORM_IS(WINUWP)
    d3d11on12CreateDevice = &D3D11On12CreateDevice;
#else
    std::string error;
    if (!mD3D11Lib.OpenSystemLibrary(L"d3d11.dll", &error) ||
        !mD3D11Lib.GetProc(&d3d11on12CreateDevice, "D3D11On12CreateDevice", &error)) {
        return DAWN_INTERNAL_ERROR(error.c_str());
    }
#endif

    return {};
}

bool PlatformFunctions::IsPIXEventRuntimeLoaded() const {
    return mPIXEventRuntimeLib.Valid();
}

HRESULT PlatformFunctions::CreateDevice(IUnknown* adapter,
                                        D3D_FEATURE_LEVEL featureLevel,
                                        REFIID riid,
                                        void** ppDevice) const {
    if (IsAgilitySDKActive()) {
        return mDeviceFactory->CreateDevice(adapter, featureLevel, riid, ppDevice);
    }
    return d3d12CreateDevice(adapter, featureLevel, riid, ppDevice);
}

HRESULT PlatformFunctions::SerializeVersionedRootSignature(
    const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* pDesc,
    ID3DBlob** ppResult,
    ID3DBlob** ppError) const {
    if (IsAgilitySDKActive()) {
        return mDeviceConfiguration->SerializeVersionedRootSignature(pDesc, ppResult, ppError);
    }
    return d3d12SerializeVersionedRootSignature(pDesc, ppResult, ppError);
}

HRESULT PlatformFunctions::CreateVersionedRootSignatureDeserializer(const void* pBlob,
                                                                    SIZE_T size,
                                                                    REFIID riid,
                                                                    void** ppDeserializer) const {
    if (IsAgilitySDKActive()) {
        return mDeviceConfiguration->CreateVersionedRootSignatureDeserializer(pBlob, size, riid,
                                                                              ppDeserializer);
    }
    return d3d12CreateVersionedRootSignatureDeserializer(pBlob, size, riid, ppDeserializer);
}

// Best-effort initializer for the Agility SDK device factory. Sets mDeviceFactory and
// mDeviceConfiguration on success. On graceful failure emits an InfoLog breadcrumb and leaves the
// members null; expected on systems whose inbox d3d12.dll predates the Agility SDK. Other failures
// (SetFlags, ID3D12DeviceConfiguration QueryInterface) are unexpected once the factory has been
// successfully created and are fatal via DAWN_CHECK.
void PlatformFunctions::TryInitializeAgilitySDKDeviceFactory() {
    DAWN_ASSERT(!mDeviceFactory);

    // Older inbox Win10 D3D12.dlls don't export D3D12GetInterface. Gracefully fall back to the
    // inbox path and emit an info-level breadcrumb so the absence is discoverable.
    if (d3d12GetInterface == nullptr) {
        dawn::InfoLog() << "[AgilitySDK] D3D12GetInterface not exported by the loaded d3d12.dll; "
                           "falling back to inbox d3d12.dll exports.";
        return;
    }

    ComPtr<ID3D12SDKConfiguration1> sdkConfig1;
    HRESULT hr = d3d12GetInterface(CLSID_D3D12SDKConfiguration, IID_PPV_ARGS(&sdkConfig1));
    if (FAILED(hr)) {
        dawn::InfoLog() << "[AgilitySDK] D3D12GetInterface(CLSID_D3D12SDKConfiguration) failed "
                           "(HRESULT 0x"
                        << std::hex << static_cast<uint32_t>(hr)
                        << "); falling back to inbox d3d12.dll exports.";
        return;
    }

    ComPtr<ID3D12DeviceFactory> factory;
    hr = sdkConfig1->CreateDeviceFactory(D3D12_PREVIEW_SDK_VERSION, ".\\D3D12\\",
                                         IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        dawn::InfoLog() << "[AgilitySDK] CreateDeviceFactory(SDK version "
                        << D3D12_PREVIEW_SDK_VERSION << ") failed (HRESULT 0x" << std::hex
                        << static_cast<uint32_t>(hr)
                        << "); falling back to inbox d3d12.dll exports.";
        return;
    }

    // Once the device factory has been created, configuring it must succeed — failures here
    // indicate a broken Agility SDK runtime rather than an absent one. Fail fast so the bug is
    // surfaced rather than silently masked by the inbox fallback.

    // Allow the factory to return an existing compatible device rather than always creating a
    // new one. Without this flag, Dawn and the Chromium media engine would each load a separate
    // UMD instance, wasting memory.
    DAWN_CHECK(
        SUCCEEDED(factory->SetFlags(D3D12_DEVICE_FACTORY_FLAG_ALLOW_RETURNING_EXISTING_DEVICE)));

    ComPtr<ID3D12DeviceConfiguration> deviceConfiguration;
    DAWN_CHECK(SUCCEEDED(factory.As(&deviceConfiguration)));

    mDeviceFactory = std::move(factory);
    mDeviceConfiguration = std::move(deviceConfiguration);
    dawn::InfoLog() << "[AgilitySDK] active: SDK version = " << D3D12_PREVIEW_SDK_VERSION;
}

void PlatformFunctions::LoadPIXRuntime(std::span<const std::string> searchPaths) {
    // TODO(dawn:766):
    // In UWP PIX should be statically linked WinPixEventRuntime_UAP.lib
    // So maybe we should put WinPixEventRuntime as a third party package
    // Currently PIX is not going to be loaded in UWP since the following
    // mPIXEventRuntimeLib.Open will fail.

    if (!mPIXEventRuntimeLib.Open("WinPixEventRuntime.dll", searchPaths) ||
        !mPIXEventRuntimeLib.GetProc(&pixBeginEventOnCommandList, "PIXBeginEventOnCommandList") ||
        !mPIXEventRuntimeLib.GetProc(&pixEndEventOnCommandList, "PIXEndEventOnCommandList") ||
        !mPIXEventRuntimeLib.GetProc(&pixSetMarkerOnCommandList, "PIXSetMarkerOnCommandList")) {
        mPIXEventRuntimeLib.Close();
    }
}

#if DAWN_USE_BUILT_DXC
MaybeError PlatformFunctions::EnsureDXCLibraries(std::span<const std::string> searchPaths) {
    // TODO(dawn:766)
    // Statically linked with dxcompiler.lib in UWP
    // currently linked with dxcompiler.lib making CoreApp unable to activate
    // LoadDXIL and LoadDXCompiler will fail in UWP, but Initialize() can still be
    // successfully executed.

    if (mDXILLib.Valid()) {
        // The libraries are already loaded, no need to load them again.
        DAWN_CHECK(mDXCompilerLib.Valid());
        return {};
    }

    DynamicLib dxilLib;
    std::string error;
    // DXIL must be loaded before DXC, otherwise shader signing is unavailable
    if (!dxilLib.Open("dxil.dll", searchPaths, &error)) {
        return DAWN_INTERNAL_ERROR(std::move(error));
    }

    DynamicLib dxCompilerLib;
    if (!dxCompilerLib.Open("dxcompiler.dll", searchPaths, &error)) {
        return DAWN_INTERNAL_ERROR(std::move(error));
    }

    if (!dxCompilerLib.GetProc(&dxcCreateInstance, "DxcCreateInstance", &error)) {
        return DAWN_INTERNAL_ERROR(std::move(error));
    }

    mDXCompilerLib = std::move(dxCompilerLib);
    mDXILLib = std::move(dxilLib);
    return {};
}
#endif  // DAWN_USE_BUILT_DXC
}  // namespace dawn::native::d3d12
