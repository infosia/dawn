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

#ifndef SRC_DAWN_NATIVE_D3D12_AGILITYSDK_H_
#define SRC_DAWN_NATIVE_D3D12_AGILITYSDK_H_

#include <cstdint>

struct ID3D12Device;

namespace dawn::native::d3d12 {

// The D3D12 Agility SDK version that Dawn requests via
// ID3D12SDKConfiguration1::CreateDeviceFactory.  This must be kept in sync
// with the NuGet package version downloaded by tools/fetch_agility_sdk.py
// (declared in //DEPS) and with the directory name referenced by BUILD.gn.
//
// When bumping the SDK, update all four locations:
//   - //DEPS                                (NuGet package version)
//   - src/dawn/native/BUILD.gn              (versioned directory path)
//   - src/dawn/native/d3d12/AgilitySDK.h    (this constant)
inline constexpr uint32_t kAgilitySDKVersion = 720u;

// Queries the device's ID3D12DeviceConfiguration::GetDesc() to determine the
// SDK version actually in use and logs the result.  Must be called after the
// D3D12 device has been successfully created.
void LogAgilitySDKStatus(ID3D12Device* device);

}  // namespace dawn::native::d3d12

#endif  // SRC_DAWN_NATIVE_D3D12_AGILITYSDK_H_
