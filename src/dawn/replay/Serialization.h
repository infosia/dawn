// Copyright 2025 The Dawn & Tint Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS-IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SRC_DAWN_REPLAY_SERIALIZATION_H_
#define SRC_DAWN_REPLAY_SERIALIZATION_H_

#include <cstdint>
#include <string>
#include <type_traits>

#include "dawn/replay/Error.h"

namespace dawn::native::webgpu {

class CaptureContext;

void WriteBytes(CaptureContext& context, const void* data, size_t size);
void Serialize(CaptureContext& context, int32_t v);
void Serialize(CaptureContext& context, uint32_t v);
void Serialize(CaptureContext& context, uint64_t v);
void Serialize(CaptureContext& context, size_t v);
void Serialize(CaptureContext& context, const std::string& v);

}  // namespace dawn::native::webgpu

namespace dawn::replay {

using MaybeError = dawn::native::MaybeError;
using CaptureContext = dawn::native::webgpu::CaptureContext;
class ReadHead;

// ----- Serialization -----

// Serialize for enum types with uint32_t or uint64_t underlying type.
template <typename T>
void Serialize(CaptureContext& s,
               T value,
               typename std::enable_if<
                   std::is_enum<T>::value &&
                       (std::is_same<typename std::underlying_type<T>::type, uint32_t>::value ||
                        std::is_same<typename std::underlying_type<T>::type, uint64_t>::value),
                   void>::type* = nullptr) {
    return WriteBytes(s, &value, sizeof(T));
}

// Helper to call Serialize on a parameter pack.
template <typename T, typename... Ts>
constexpr auto Serialize(CaptureContext& s, const T& v, const Ts&... vs)
    -> std::enable_if_t<(sizeof...(Ts) > 0) || !std::is_enum_v<T>, void> {
    Serialize(s, v);
    Serialize(s, vs...);
}

// Helper to call Serialize on an empty parameter pack.
// Do nothing.
inline void Serialize(CaptureContext& context) {}

// ----- Deserialization -----

MaybeError ReadBytes(ReadHead& s, void* data, size_t size);
MaybeError Deserialize(ReadHead& s, int32_t* v);
MaybeError Deserialize(ReadHead& s, uint32_t* v);
MaybeError Deserialize(ReadHead& s, uint64_t* v);
MaybeError Deserialize(ReadHead& s, size_t* v);
MaybeError Deserialize(ReadHead& s, std::string* v);

// Deserialize for enum types with uint32_t or uint64_t underlying type.
template <typename T>
MaybeError Deserialize(
    ReadHead& s,
    T* value,
    typename std::enable_if<
        std::is_enum<T>::value &&
            (std::is_same<typename std::underlying_type<T>::type, uint32_t>::value ||
             std::is_same<typename std::underlying_type<T>::type, uint64_t>::value),
        void>::type* = nullptr) {
    return ReadBytes(s, reinterpret_cast<char*>(value), sizeof(T));
}

// Helper to call Deserialize on a parameter pack.
template <typename T, typename... Ts>
auto Deserialize(ReadHead& s, T* v, Ts*... vs)
    -> std::enable_if_t<(sizeof...(Ts) > 0) || !std::is_enum_v<T>, MaybeError> {
    DAWN_TRY(Deserialize(s, v));
    return Deserialize(s, vs...);
}

// Helper to call Deserialize on an empty parameter pack.
// Do nothing and return success.
inline MaybeError Deserialize(ReadHead& s) {
    return {};
}

// ----- Serializable -----

template <typename Derived>
class Serializable {
  public:
    friend void Serialize(CaptureContext& context, const Derived& in) {
        in.VisitAll([&](const auto&... members) { Serialize(context, members...); });
    }

    friend MaybeError Deserialize(ReadHead& readHead, Derived* out) {
        return out->VisitAll([&](auto&... members) { return Deserialize(readHead, &members...); });
    }
};

// TODO(crbug.com/413053623): Switch to protobufs or json. This is just to get
// stuff working. We'll switch to something else once we're sure of the data we
// want to capture.

using ObjectId = uint64_t;

enum class ObjectType : uint32_t {
    Invalid = 0,  // 0 is invalid at it's more likely to catch bugs.
    BindGroup,
    BindGroupLayout,
    Buffer,
    CommandBuffer,
    ComputePipeline,
    Device,
    PipelineLayout,
    QuerySet,
    RenderPipeline,
    Sampler,
    ShaderModule,
    Texture,
    TextureView,
};

enum class RootCommand : uint32_t {
    Invalid = 0,  // 0 is invalid at it's more likely to catch bugs.
    CreateResource,
    QueueSubmit,
    WriteBuffer,
    WriteTexture,
    Unmap,

    End,
};

// Helper for X macro to declare a visitable member.
#define DAWN_REPLAY_INTERNAL_VISITABLE_MEMBER_DECL(type, name, ...) \
    type name{__VA_OPT__(__VA_ARGS__)};

// Helper for X macro for visiting a visitable member.
#define DAWN_REPLAY_INTERNAL_VISITABLE_MEMBER_ARG(type, name, ...) , name

namespace detail {
constexpr int kInternalVisitableUnusedForComma = 0;

// Helper X macro to declare members of a class or struct, along with VisitAll
// methods to call a functor on all members.
// Example usage:
//   #define MEMBERS(X)    \
//       X(int, a)         \
//       X(float, b, 42.0) \
//       X(Foo, foo, kFoo) \
//       X(Bar, bar)
//   struct MyStruct {
//    DAWN_REPLAY_VISITABLE_MEMBERS(MEMBERS)
//   };
//   #undef MEMBERS
#define DAWN_REPLAY_VISITABLE_MEMBERS(MEMBERS)                                                  \
    MEMBERS(DAWN_REPLAY_INTERNAL_VISITABLE_MEMBER_DECL)                                         \
                                                                                                \
    template <typename V>                                                                       \
    constexpr auto VisitAll(V&& visit) const {                                                  \
        return [&](int, const auto&... ms) {                                                    \
            return visit(ms...);                                                                \
        }(kInternalVisitableUnusedForComma MEMBERS(DAWN_REPLAY_INTERNAL_VISITABLE_MEMBER_ARG)); \
    }                                                                                           \
                                                                                                \
    template <typename V>                                                                       \
    constexpr auto VisitAll(V&& visit) {                                                        \
        return [&](int, auto&... ms) {                                                          \
            return visit(ms...);                                                                \
        }(kInternalVisitableUnusedForComma MEMBERS(DAWN_REPLAY_INTERNAL_VISITABLE_MEMBER_ARG)); \
    }

// Helper macro to define a struct or class along with VisitAll methods to call
// a functor on all members.
// Example usage:
//   #define MEMBERS(X) \
//       X(int, a)              \
//       X(float, b)            \
//       X(Foo, foo)            \
//       X(Bar, bar)
//   DAWN_REPLAY_SERIALIZABLE(struct, MyStruct, MEMBERS) {
//      void SomeAdditionalMethod();
//   };
//   #undef MEMBERS
#define DAWN_REPLAY_SERIALIZABLE(qualifier, Name, MEMBERS) \
    struct Name##__Contents {                              \
        DAWN_REPLAY_VISITABLE_MEMBERS(MEMBERS)             \
    };                                                     \
    qualifier Name : Name##__Contents, public ::dawn::replay::Serializable<Name>

// Makes both a CmdData and a Cmd struct for a given root command name.
#define DAWN_REPLAY_MAKE_ROOT_CMD_AND_CMD_DATA(CmdName, CMD_MEMBERS)                           \
    DAWN_REPLAY_SERIALIZABLE(struct, CmdName##CmdData, CMD_MEMBERS){};                         \
                                                                                               \
    struct CmdName##Cmd##__Contents {                                                          \
        DAWN_REPLAY_INTERNAL_VISITABLE_MEMBER_DECL(RootCommand, command, RootCommand::CmdName) \
        DAWN_REPLAY_INTERNAL_VISITABLE_MEMBER_DECL(CmdName##CmdData, data)                     \
                                                                                               \
        template <typename V>                                                                  \
        constexpr auto VisitAll(V&& visit) const {                                             \
            return [&](int, const auto&... ms) { return visit(ms...); }(                       \
                       kInternalVisitableUnusedForComma, command, data);                       \
        }                                                                                      \
                                                                                               \
        template <typename V>                                                                  \
        constexpr auto VisitAll(V&& visit) {                                                   \
            return [&](int, auto&... ms) { return visit(ms...); }(                             \
                       kInternalVisitableUnusedForComma, command, data);                       \
        }                                                                                      \
    };                                                                                         \
    struct CmdName##Cmd : CmdName##Cmd##__Contents,                                            \
                          public ::dawn::replay::Serializable<CmdName##Cmd>

#define BUFFER_CREATION_MEMBER(X) \
    X(uint64_t, size)             \
    X(wgpu::BufferUsage, usage)

DAWN_REPLAY_SERIALIZABLE(struct, Buffer, BUFFER_CREATION_MEMBER){};

#define LABELED_RESOURCE_MEMBER(X) \
    X(ObjectType, type)            \
    X(ObjectId, id)                \
    X(std::string, label)

DAWN_REPLAY_SERIALIZABLE(struct, LabeledResource, LABELED_RESOURCE_MEMBER){};

#define CREATE_RESOURCE_CMD_DATA_MEMBER(X) X(LabeledResource, resource)

DAWN_REPLAY_MAKE_ROOT_CMD_AND_CMD_DATA(CreateResource, CREATE_RESOURCE_CMD_DATA_MEMBER){};

#define WRITE_BUFFER_CMD_DATA_MEMBER(X) \
    X(ObjectId, bufferId)               \
    X(uint64_t, bufferOffset)           \
    X(size_t, size)

DAWN_REPLAY_MAKE_ROOT_CMD_AND_CMD_DATA(WriteBuffer, WRITE_BUFFER_CMD_DATA_MEMBER){};

}  // namespace detail

}  // namespace dawn::replay

#endif  // SRC_DAWN_REPLAY_SERIALIZATION_H_
