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

#ifndef SRC_DAWN_COMMON_STRINGS_H_
#define SRC_DAWN_COMMON_STRINGS_H_

#include <algorithm>
#include <array>
#include <string>

#include "dawn/common/Assert.h"

// Marks a raw string as intentionally unminified, to avoid a PRESUBMIT warning.
#define DAWN_NO_MINIFY(str) (str)

// Minifies a string with "C-like" contents such as WGSL/MSL code.
// It runs at consteval time, to reduce binary size.
// - Removes single line comments.
// - Removes unnecessary whitespace where easily recognizable (leading, trailing, strings of
//   multiple whitespace characters, whitespace next to certain symbols).
//
// Notes:
// - Does not minify variable names (obviously).
// - Rejects multi-line comments (harder to implement and has different syntax in WGSL vs MSL).
#define DAWN_ALWAYS_MINIFY(str) dawn::detail::MinifyWhitespace<dawn::detail::CharArray{str}>()

// Minify as above, but only in release builds.
#if defined(DAWN_ENABLE_ASSERTS)
#define DAWN_MINIFY(str) dawn::detail::CharArray(str)
#else  // defined(DAWN_ENABLE_ASSERTS)
#define DAWN_MINIFY(str) DAWN_ALWAYS_MINIFY(str)
#endif  // defined(DAWN_ENABLE_ASSERTS)

namespace dawn::detail {

// Alias for std::array<char, N>, that can be constructed from a string literal.
template <size_t N>
struct CharArray : std::array<char, N> {
    consteval CharArray() = default;
    consteval CharArray(const char (&str)[N]) {
        DAWN_CHECK(str[N - 1] == '\0');  // Incomprehensible error here means the assertion failed.
        std::copy_n(str, N, this->data());
    }

    constexpr const char* c_str() const { return this->data(); }
    /*implicit*/ constexpr operator const char*() const { return c_str(); }
};

// Deduction guide to make -Wctad-maybe-unsupported allow inferring class type in constructor call
template <size_t N>
CharArray(const char (&str)[N]) -> CharArray<N>;

// Interface for a thing that receives output chars. CharCounter counts them so we can
// allocate space for a result, and FixedStringBuilder actually populates the result.
struct CharConsumer {
    virtual consteval void Consume(char) = 0;
};
struct CharCounter : CharConsumer {
    size_t count = 0;
    consteval void Consume(char) override { count++; }
};
template <size_t N>
struct FixedStringBuilder : CharConsumer {
    CharArray<N>& buffer;
    size_t current_output_idx = 0;
    consteval FixedStringBuilder(CharArray<N>& buf) : buffer(buf) {}
    consteval void Consume(char c) override { buffer[current_output_idx++] = c; }
};

// Core minification logic. Runs twice, first to compute the size of the output string (to determine
// the statically-sized type of the static storage area), and again to actually build the string.
template <auto Str>
consteval void MinifyWhitespaceImpl(CharConsumer& consumer) {
    // Space characters that we want to remove as much as possible.
    auto IsWhitespace = [](char c) { return std::string(" \t\n").find(c) != std::string::npos; };

    // Characters that never need spaces around them (in valid code).
    auto IsDelimiter = [](char c) {
        return std::string(".,;:+-*/%@|&^~=?!{}()<>").find(c) != std::string::npos;
    };

    enum class State {
        // Characters should be passed through to the result.
        Normal,
        // Whitespace sequence should be dropped (we're after a symbol and don't need whitespace).
        InRemovableWhitespace,
        // Whitespace sequence should be replaced with one space.
        InWhitespace,
    };
    // Initial state: whitespace at the beginning of the string is always removable.
    State state = State::InRemovableWhitespace;
    // Track whether we're inside a comment. If so, we treat all characters like whitespace.
    bool inComment = false;
    for (size_t i = 0; i < Str.size() - 1; ++i) {  // excludes the null terminator
        char c = Str[i];

        if (inComment) {
            if (c == '\n') {
                inComment = false;
            } else {
                // Treat all characters inside comments as whitespace.
                c = ' ';
            }
        }

        if (c == '/') {
            char next = Str[i + 1];
            if (next == '/') {
                inComment = true;
                continue;
            }
            // Multiline comments are not supported
            DAWN_CHECK(next != '*');  // Incomprehensible error here means the assertion failed.
        }

        if (IsDelimiter(c)) {
            // [If we were previously in whitespace, it gets dropped here.]
            consumer.Consume(c);
            // We can remove any whitespace after this character.
            state = State::InRemovableWhitespace;
        } else if (IsWhitespace(c)) {
            if (state == State::Normal) {
                // Entering whitespace from a non-delimiter character.
                state = State::InWhitespace;
            }
        } else {
            // If exiting non-removable whitespace, insert a whitespace before continuing.
            if (state == State::InWhitespace) {
                consumer.Consume(' ');
            }
            consumer.Consume(c);
            state = State::Normal;
        }
    }
}

template <auto Str>
consteval size_t ComputeMinifiedSize() {
    CharCounter counter;
    MinifyWhitespaceImpl<Str>(counter);
    return counter.count + 1;  // +1 for null terminator
}

template <auto Str>
consteval auto MinifyWhitespace() {
    constexpr size_t kMinifiedSize = ComputeMinifiedSize<Str>();
    CharArray<kMinifiedSize> result{};
    FixedStringBuilder<kMinifiedSize> builder(result);
    MinifyWhitespaceImpl<Str>(builder);
    return result;
}

}  // namespace dawn::detail

#endif  // SRC_DAWN_COMMON_STRINGS_H_
