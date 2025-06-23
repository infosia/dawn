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

#include "dawn/native/webgpu/RecordCommandSerializer.h"

#include <iostream>
#include <string>

// #if defined(__clang__)
// #define DAWN_DISABLE_WARNING_EXTRA_SEMICOLON \
//     _Pragma("clang diagnostic ignored \"-Wextra-semi-stmt\"")
// #endif

// Temp suppress protobuf compile error
#if defined(__clang__)
_Pragma("clang diagnostic ignored \"-Wextra-semi-stmt\"")
    _Pragma("clang diagnostic ignored \"-Winconsistent-missing-destructor-override\"")
        _Pragma("clang diagnostic ignored \"-Wsuggest-destructor-override\"")
#endif

#include "dawn/native/webgpu/protos/cmd.pb.h"

            namespace dawn::native::webgpu {

    RecordCommandSerializer::RecordCommandSerializer() {
        std::cout << "[RecordCommandSerializer] Initialized." << std::endl;
    }
    RecordCommandSerializer::~RecordCommandSerializer() {
        std::cout << "[RecordCommandSerializer] Destroyed." << std::endl;
    }

    void RecordCommandSerializer::Encode() {
        // cmd::pb::Cmd
    }

    // void* RecordCommandSerializer::GetCmdSpace(size_t size) {
    //     mBuffer.resize(size);
    //     // std::cout << "[RecordCommandSerializer] GetCmdSpace(" << size << ")" << std::endl;
    //     return mBuffer.data();
    // }

    // bool RecordCommandSerializer::Flush() {
    //     if (!mBuffer.empty()) {
    //         // In a real file serializer, you'd write mBuffer.data() of mBuffer.size() to a file.
    //         // Here, we just indicate a flush.
    //         std::cout << "[RecordCommandSerializer] Flushing " << mBuffer.size() << " bytes." <<
    //         std::endl;
    //         // Example: Pretend to write command type if it's a known simple command.
    //         // This is highly simplified and not robust for general command logging.
    //         if (mBuffer.size() >= sizeof(uint32_t) * 2) { // CmdHeader + WireCmd
    //             dawn::wire::WireCmd cmdId = *reinterpret_cast<const
    //             dawn::wire::WireCmd*>(mBuffer.data() + sizeof(dawn::wire::CmdHeader));
    //             // This mapping would need to be more extensive for real logging.
    //             std::cout << "  Command ID (approx): " << static_cast<uint32_t>(cmdId) <<
    //             std::endl;
    //         }
    //     }
    //     mBuffer.clear();
    //     return true;
    // }

    // size_t RecordCommandSerializer::GetMaximumAllocationSize() const {
    //     return 1024 * 1024; // 1MB, arbitrary for example
    // }

    // void RecordCommandSerializer::OnSerializeError() {
    //     std::cerr << "[StdOutCommandSerializer] Serialization error occurred." << std::endl;
    // }

}  // namespace dawn::native::webgpu
