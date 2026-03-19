// Copyright 2026 The Dawn & Tint Authors
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

// This file implements helpers for the D3D12 Agility SDK redistributable
// runtime.  When the UseAgilitySDK toggle is enabled (default true) and the
// Agility SDK DLLs are present at runtime, Dawn will request that newer
// runtime via ID3D12SDKConfiguration1::CreateDeviceFactory.
//
// See: https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/

#include "dawn/native/d3d12/AgilitySDK.h"

#include "dawn/common/Log.h"
#include "dawn/native/d3d12/d3d12_platform.h"

namespace dawn::native::d3d12 {

// Query the device for ID3D12DeviceConfiguration and call GetDesc() to read
// the actual SDK version in use.  This is more reliable than
// GetModuleHandleW("D3D12Core.dll"), which has undefined behaviour when
// multiple DLLs with the same name but different paths are loaded.
void LogAgilitySDKStatus(ID3D12Device* device) {
    if (!device) {
        return;
    }

    ComPtr<ID3D12DeviceConfiguration> deviceConfig;
    if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&deviceConfig)))) {
        D3D12_DEVICE_CONFIGURATION_DESC desc = deviceConfig->GetDesc();
        if (desc.SDKVersion >= kAgilitySDKVersion) {
            dawn::InfoLog() << "[AgilitySDK] OK: device SDK version = " << desc.SDKVersion
                            << " (requested " << kAgilitySDKVersion << ")";
        } else if (desc.SDKVersion > 0) {
            dawn::InfoLog() << "[AgilitySDK] WARNING: device SDK version = " << desc.SDKVersion
                            << " < requested " << kAgilitySDKVersion
                            << " - Agility SDK may not be active";
        } else {
            dawn::InfoLog() << "[AgilitySDK] device SDK version = 0"
                               " - using system inbox runtime";
        }
    } else {
        // ID3D12DeviceConfiguration is not available on this runtime.
        dawn::InfoLog() << "[AgilitySDK] ID3D12DeviceConfiguration unavailable"
                           " - cannot determine SDK version";
    }
}

}  // namespace dawn::native::d3d12
