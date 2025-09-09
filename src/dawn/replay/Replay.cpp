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

#include "dawn/replay/Replay.h"

#include <type_traits>

#include "dawn/replay/Serialization.h"

namespace dawn::replay {

namespace {

template <typename T>
bool Read(ReadHead& readHead,
          T* value,
          typename std::enable_if<
              std::is_same<T, uint32_t>::value ||
                  (std::is_enum<T>::value &&
                   std::is_same<typename std::underlying_type<T>::type, uint32_t>::value),
              void>::type* = nullptr) {
    readHead.read(reinterpret_cast<char*>(value), sizeof(T));
    return !readHead.isDone();
}

bool Read(ReadHead& readHead, uint64_t* value) {
    readHead.read(reinterpret_cast<char*>(value), sizeof(value));
    return !readHead.isDone();
}

bool Read(ReadHead& readHead, size_t* value) {
    readHead.read(reinterpret_cast<char*>(value), sizeof(value));
    return !readHead.isDone();
}

bool Read(ReadHead& readHead, std::string* label) {
    size_t size = 0;
    Read(readHead, &size);
    label->resize(size);
    readHead.read(label->data(), size);
    return !readHead.isDone();
}

bool ReadContentIntoBuffer(ReadHead& readHead,
                           wgpu::Device device,
                           wgpu::Buffer buffer,
                           uint64_t bufferOffset,
                           uint64_t size) {
    const uint8_t* data = readHead.GetData(size);
    device.GetQueue().WriteBuffer(buffer, bufferOffset, data, size);
    return !readHead.isDone();
}

wgpu::Buffer CreateBuffer(wgpu::Device device, ReadHead& readHead, const std::string& label) {
    wgpu::BufferDescriptor desc;
    Read(readHead, &desc.size);
    Read(readHead, reinterpret_cast<uint64_t*>(&desc.usage));
    desc.label = wgpu::StringView(label);
    if (!(desc.usage & wgpu::BufferUsage::MapRead)) {
        desc.usage |= wgpu::BufferUsage::CopySrc;
    }
    wgpu::Buffer buffer = device.CreateBuffer(&desc);
    return buffer;
}

}  // anonymous namespace

std::unique_ptr<Replay> Replay::Create(wgpu::Device device, const Capture* capture) {
    return std::unique_ptr<Replay>(new Replay(device, capture));
}

Replay::Replay(wgpu::Device device, const Capture* capture) : mDevice(device), mCapture(capture) {}

bool Replay::CreateResource(wgpu::Device device, ReadHead& readHead) {
    ObjectType objectType;
    ObjectId objectId;
    std::string label;
    Read(readHead, &objectType);
    Read(readHead, &objectId);
    Read(readHead, &label);  // every resource has a label

    switch (objectType) {
        case ObjectType::Buffer: {
            mResources.insert({objectId, {label, CreateBuffer(device, readHead, label)}});
            return true;
        }

        default:
            // UNIMPLEMENTED();
            break;
    }
    return true;
}

void Replay::Play() {
    auto readHead = mCapture->GetCommandReadHead();
    auto contentReadHead = mCapture->GetContentReadHead();
    RootCommand cmd;

    while (!readHead.isDone()) {
        Read(readHead, &cmd);
        switch (cmd) {
            case RootCommand::CreateResource: {
                CreateResource(mDevice, readHead);
                break;
            }
            case RootCommand::WriteBuffer: {
                ObjectId bufferId;
                uint64_t bufferOffset;
                size_t size;
                Read(readHead, &bufferId);
                Read(readHead, &bufferOffset);
                Read(readHead, &size);
                wgpu::Buffer buffer = GetObjectById<wgpu::Buffer>(bufferId);
                ReadContentIntoBuffer(contentReadHead, mDevice, buffer, bufferOffset, size);
                break;
            }
            default: {
                // UNIMPLEMENTED();
                break;
            }
        }
    }
}

}  // namespace dawn::replay
