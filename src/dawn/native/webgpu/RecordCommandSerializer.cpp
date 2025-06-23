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

    RecordCommandSerializer::RecordCommandSerializer(WGPUDevice device) {
        std::cout << "[RecordCommandSerializer] Initialized." << std::endl;
    }
    RecordCommandSerializer::~RecordCommandSerializer() {
        std::cout << "[RecordCommandSerializer] Destroyed." << std::endl;
    }

    void RecordCommandSerializer::EncodeDeviceCreateBuffer(
        std::ostream * output, const UnpackedPtr<BufferDescriptor>& descriptor) {
        cmd::pb::Cmd cmd;

        cmd::pb::DeviceCreateBufferCmd* createBufferCmd = cmd.mutable_devicecreatebuffer();
        createBufferCmd->set_deviceid(0);

        // // Serialize the descriptor into a byte array that matches the wire format.
        // WGPUBufferDescriptor apiDesc = ToAPI(*descriptor);

        // size_t labelLength = apiDesc.label ? strlen(apiDesc.label) : 0;

        // // The serialized data is the transfer struct followed by the label.
        // size_t requiredSize = sizeof(wire::WGPUBufferDescriptorTransfer) + labelLength;

        // std::vector<char> data(requiredSize);

        // // wire::SerializeBuffer serializeBuffer(data.data(), data.size());

        // // wire::WGPUBufferDescriptorTransfer* transfer;
        // // serializeBuffer.Next(&transfer);
        // // wire::WGPUBufferDescriptorSerialize(apiDesc, transfer, &serializeBuffer);

        // createBufferCmd->set_descriptor(data.data(), data.size());

        // Serialize the whole command to the output stream.
        cmd.SerializeToOstream(output);
    }

}  // namespace dawn::native::webgpu
