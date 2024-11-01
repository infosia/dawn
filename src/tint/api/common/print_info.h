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

#ifndef SRC_TINT_API_COMMON_PRINT_INFO_H_
#define SRC_TINT_API_COMMON_PRINT_INFO_H_

#include <cstdint>
#include <unordered_map>

#include "src/tint/utils/diagnostic/source.h"

namespace tint {

/// The maximum number of print entries in the print buffer.
constexpr uint32_t kMaxPrintEntries = 100;
/// The size in bytes of each entry in the print buffer.
constexpr uint32_t kPrintEntrySize = 32;
/// The offset in bytes from the start of the print buffer to the print entries.
constexpr uint32_t kPrintEntryOffset = 16;

/// An enum of different value types used for `print()` calls.
enum PrintValueType : uint8_t {
    kBool,
    kF16,
    kF32,
    kI32,
    kU32,
    kVec2Bool,
    kVec2F16,
    kVec2F32,
    kVec2I32,
    kVec2U32,
    kVec3Bool,
    kVec3F16,
    kVec3F32,
    kVec3I32,
    kVec3U32,
    kVec4Bool,
    kVec4F16,
    kVec4F32,
    kVec4I32,
    kVec4U32,
};

/// A struct that describes the WGSL source location and value type for a print() call.
struct PrintInfo {
    tint::Source::Location location;
    PrintValueType value_type;
};

/// A map from print call ID to information about its value type and WGSL source location.
using PrintIdToInfo = std::unordered_map<uint32_t, PrintInfo>;

}  // namespace tint

#endif  // SRC_TINT_API_COMMON_PRINT_INFO_H_
