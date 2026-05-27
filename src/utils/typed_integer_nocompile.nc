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

// Strong TypedInteger type checks are only enabled in debug.
#if !defined(DAWN_ENABLE_ASSERTS)
// expected-no-diagnostics
#else

#include "dawn/util/typed_integer.h"

namespace dawn {

using MyI32 = TypedInteger<struct MyI32T, int32_t>;
using MyU32 = TypedInteger<struct MyU32T, uint32_t>;

static uint8_t v = 3;

void TestConstruction() {

    // Cannot be converted using operator=
    { [[maybe_unused]] MyI32 x = MyI32{v}; }
    { [[maybe_unused]] MyI32 x = int32_t{v}; }  // expected-error {{no viable conversion from}}

    // Cannot be constructed from a distinct but otherwise identical type
    using MyOtherI32 = TypedInteger<struct MyOtherI32T, int32_t>;
    (void) MyI32{MyI32{v}};
    (void) MyI32{MyOtherI32{v}};  // expected-error {{no matching constructor for initialization of}}
}

void TestNarrowingConstructionFromPrimitive() {
    // Cannot convert to narower types
    (void) MyI32{int32_t{v}};
    (void) MyI32{int64_t{v}};   // expected-error {{no matching constructor for initialization of}}
    (void) MyI32{uint32_t{v}};  // expected-error {{no matching constructor for initialization of}}
    (void) MyU32{uint64_t{v}};  // expected-error {{no matching constructor for initialization of}}
    (void) MyU32{int16_t{v}};   // expected-error {{no matching constructor for initialization of}}
    (void) MyU32{int32_t{v}};   // expected-error {{no matching constructor for initialization of}}
}

void TestNarrowingCastToPrimitive() {
    MyI32 i{3};
    MyU32 u{3u};

    // Cannot convert to narower types
    (void) int32_t{i};
    (void) int16_t{i};   // expected-error {{no viable conversion from}}
    (void) uint32_t{i};  // expected-error {{no viable conversion from}}
    (void) int32_t{u};   // expected-error {{no viable conversion from}}
}

}

#endif
