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

#ifndef SRC_DAWN_SERIALIZATION_SERIALIZER_H_
#define SRC_DAWN_SERIALIZATION_SERIALIZER_H_

#include <webgpu/webgpu_cpp.h>

#include <array>
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

namespace dawn::serialization {

// Forward declarations to ensure visibility for recursive templates.

template <typename Context>
void WriteBytes(Context& context, const void* data, size_t size);

template <typename Context>
void Serialize(Context& context, int32_t v);
template <typename Context>
void Serialize(Context& context, uint8_t v);
template <typename Context>
void Serialize(Context& context, uint16_t v);
template <typename Context>
void Serialize(Context& context, uint32_t v);
template <typename Context>
void Serialize(Context& context, uint64_t v);
template <typename Context>
void Serialize(Context& context, float v);
template <typename Context>
void Serialize(Context& context, double v);
template <typename Context>
void Serialize(Context& context, bool v);
template <typename Context>
void Serialize(Context& context, const std::string& v);

template <typename Context, typename T>
void Serialize(Context& context, T v)
    requires(std::is_same_v<T, size_t> && !std::is_same_v<T, uint64_t> &&
             !std::is_same_v<T, uint32_t>);

template <typename Context, typename T>
void Serialize(Context& context, const std::vector<T>& v);

template <typename Context, typename T, size_t N>
void Serialize(Context& context, const std::array<T, N>& a);

template <typename Context, typename T>
void Serialize(Context& s,
               T value,
               typename std::enable_if<
                   std::is_enum<T>::value &&
                       (std::is_same<typename std::underlying_type<T>::type, uint32_t>::value ||
                        std::is_same<typename std::underlying_type<T>::type, uint64_t>::value),
                   void>::type* = nullptr);

template <typename Context>
void Serialize(Context& context);

template <typename Context, typename T, typename... Ts>
constexpr auto Serialize(Context& s, const T& v, const Ts&... vs)
    -> std::enable_if_t<(sizeof...(Ts) > 0), void>;

namespace detail {
struct AnyVisitor {
    template <typename... Args>
    void operator()(Args&&...) const;
};
}  // namespace detail

template <typename Context, typename T>
auto Serialize(Context& context, const T& value)
    -> decltype(value.VisitAll(detail::AnyVisitor{}), void());

// Implementations

template <typename Context>
void WriteBytes(Context& context, const void* data, size_t size) {
    context.WriteCommandBytes(data, size);
}

template <typename Context>
void Serialize(Context& context, int32_t v) {
    WriteBytes(context, &v, sizeof(v));
}

template <typename Context>
void Serialize(Context& context, uint8_t v) {
    WriteBytes(context, &v, sizeof(v));
}

template <typename Context>
void Serialize(Context& context, uint16_t v) {
    WriteBytes(context, &v, sizeof(v));
}

template <typename Context>
void Serialize(Context& context, uint32_t v) {
    WriteBytes(context, &v, sizeof(v));
}

template <typename Context>
void Serialize(Context& context, uint64_t v) {
    WriteBytes(context, &v, sizeof(v));
}

template <typename Context>
void Serialize(Context& context, float v) {
    WriteBytes(context, &v, sizeof(v));
}

template <typename Context>
void Serialize(Context& context, double v) {
    WriteBytes(context, &v, sizeof(v));
}

template <typename Context>
void Serialize(Context& context, bool v) {
    WriteBytes(context, &v, sizeof(v));
}

template <typename Context>
void Serialize(Context& context, const std::string& v) {
    Serialize(context, static_cast<size_t>(v.size()));
    WriteBytes(context, v.data(), v.size());
}

template <typename Context, typename T>
void Serialize(Context& context, T v)
    requires(std::is_same_v<T, size_t> && !std::is_same_v<T, uint64_t> &&
             !std::is_same_v<T, uint32_t>)
{
    WriteBytes(context, reinterpret_cast<const char*>(&v), sizeof(v));
}

template <typename Context, typename T>
void Serialize(Context& context, const std::vector<T>& v) {
    Serialize(context, static_cast<size_t>(v.size()));
    for (const auto& elem : v) {
        Serialize(context, elem);
    }
}

template <typename Context, typename T, size_t N>
void Serialize(Context& context, const std::array<T, N>& a) {
    for (const auto& elem : a) {
        Serialize(context, elem);
    }
}

// Serialize for enum types with uint32_t or uint64_t underlying type.
template <typename Context, typename T>
void Serialize(Context& s,
               T value,
               typename std::enable_if<
                   std::is_enum<T>::value &&
                       (std::is_same<typename std::underlying_type<T>::type, uint32_t>::value ||
                        std::is_same<typename std::underlying_type<T>::type, uint64_t>::value),
                   void>::type*) {
    return WriteBytes(s, &value, sizeof(T));
}

// Helper to call Serialize on an empty parameter pack.
// Do nothing.
template <typename Context>
inline void Serialize(Context& context) {}

// Generic Serialize for any type with VisitAll
template <typename Context, typename T>
auto Serialize(Context& context, const T& value)
    -> decltype(value.VisitAll(detail::AnyVisitor{}), void()) {
    value.VisitAll([&](const auto&... members) { Serialize(context, members...); });
}

// Helper to call Serialize on a parameter pack.
template <typename Context, typename T, typename... Ts>
constexpr auto Serialize(Context& s, const T& v, const Ts&... vs)
    -> std::enable_if_t<(sizeof...(Ts) > 0), void> {
    Serialize(s, v);
    Serialize(s, vs...);
}

}  // namespace dawn::serialization

#endif  // SRC_DAWN_SERIALIZATION_SERIALIZER_H_
