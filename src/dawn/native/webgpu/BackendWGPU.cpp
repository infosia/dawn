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

#include "dawn/native/webgpu/BackendWGPU.h"

#include <memory>
#include <string>
#include <utility>

#include "dawn/dawn_proc.h"
#include "dawn/dawn_proc_table.h"

#include "dawn/native/WebGPUBackend.h"

#include "dawn/common/SystemUtils.h"
#include "dawn/native/ChainUtils.h"
#include "dawn/native/Instance.h"
// #include "dawn/native/WebGPUBackend.h"
#include "dawn/native/DawnNative.h"         // For dawn::native::GetProcs
#include "dawn/native/webgpu/DeviceWGPU.h"  // For webgpu::PhysicalDevice

namespace dawn::native::webgpu {

namespace {

// constexpr std::string_view kDawnProcLibSelection = "libdawn_native";
// constexpr std::string_view kDawnProcLibSelection = "libdawn_proc";

#if DAWN_PLATFORM_IS(WINDOWS)
constexpr const char* kDawnProcLibName = "libdawn_proc.dll";
#elif DAWN_PLATFORM_IS(MACOS)
constexpr const char* kDawnProcLibName = "libdawn_proc.dylib";
#else
// constexpr const char* kDawnProcLibName = "libdawn_native.so";
constexpr const char* kDawnProcLibName = "libdawn_proc.so";
#endif
}  // anonymous namespace

Backend::Backend(InstanceBase* instance, wgpu::BackendType backendType)
    : BackendConnection(instance, backendType) {
    instance->ConsumedErrorAndWarnOnce(Initialize());
}

MaybeError Backend::Initialize() {
    if (mIsInitialized) {
        return {};
    }

    // dawnProcSetProcs(&dawn::native::GetProcs());
    if (!mDawnProcLib.Open(kDawnProcLibName)) {
        return DAWN_FORMAT_INTERNAL_ERROR("Couldn't load Dawn. Searched %s.", kDawnProcLibName);
    }

    SetDawnProcTableFromDynamicLib(mDawnProcLib, &mDawnProcs);

    mIsInitialized = true;
    return {};
}

std::vector<Ref<PhysicalDeviceBase>> Backend::DiscoverPhysicalDevices(
    const UnpackedPtr<RequestAdapterOptions>& options) {
    if (options->forceFallbackAdapter) {
        return {};
    }

    // Ensure libdawn_proc is loaded and initialized.
    if (!mIsInitialized) {
        GetInstance()->ConsumedErrorAndWarnOnce(DAWN_INTERNAL_ERROR(
            "WebGPU backend initialization failed (libdawn_proc not loaded or initialized)."));
        return {};
    }

    // The WebGPU backend is currently a placeholder/CPU implementation.
    if (mPhysicalDevice == nullptr) {
        mPhysicalDevice = PhysicalDevice::Create(&mDawnProcs);
    }
    return {mPhysicalDevice};
}

const DawnProcTable& Backend::GetFunctions() const {
    return mDawnProcs;
}

// MaybeError Backend::Initialize() {
//     if (mIsInitialized) {
//         return {};
//     }

//     const std::vector<std::string>& searchPaths = GetInstance()->GetRuntimeSearchPaths();

//     bool loaded = false;
//     for (const std::string& path : searchPaths) {
//         std::string resolvedPath = path + kDawnProcLibName;
//         if (mDawnProcLib.Open(resolvedPath)) {
//             loaded = true;
//             break;
//         }
//     }

//     if (!loaded) {
//         // Construct comma-separated list for error message
//         std::string searchedPathsStr;
//         bool first = true;
//         for (const std::string& path : searchPaths) {
//             if (!first) searchedPathsStr += ", ";
//             first = false;
//             searchedPathsStr += (path + kDawnProcLibName);
//         }
//         return DAWN_FORMAT_INTERNAL_ERROR("Couldn't load %s. Searched %s.",
//                                             kDawnProcLibName, searchedPathsStr);
//     }

//     mProcsSetProcs = reinterpret_cast<PFN_dawnProcSetProcs>(
//         mDawnProcLib.GetProc("dawnProcSetProcs"));

//     if (mProcsSetProcs == nullptr) {
//         return DAWN_INTERNAL_ERROR("Couldn't load dawnProcSetProcs from %s.", kDawnProcLibName);
//     }

//     // Set the procs using the native implementation's proc table.
//     mProcsSetProcs(&dawn::native::GetProcs());

//     mIsInitialized = true;
//     return {};
// }

// std::vector<Ref<PhysicalDeviceBase>> Backend::DiscoverPhysicalDevices(
//     const UnpackedPtr<RequestAdapterOptions>& options) {
//     if (options->forceFallbackAdapter) {
//         return {};
//     }

//     // Ensure libdawn_proc is loaded and initialized.
//     if (!mIsInitialized) {
//         GetInstance()->ConsumedErrorAndWarnOnce(
//             DAWN_INTERNAL_ERROR("WebGPU backend initialization failed (libdawn_proc not loaded or
//             initialized)."));
//         return {};
//     }

//     // The WebGPU backend is currently a placeholder/CPU implementation.
//     if (mPhysicalDevice == nullptr) {
//         mPhysicalDevice = PhysicalDevice::Create();
//     }
//     return {mPhysicalDevice};
// }

BackendConnection* Connect(InstanceBase* instance) {
    return new Backend(instance, wgpu::BackendType::WebGPU);
}

}  // namespace dawn::native::webgpu
