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

#include <gtest/gtest.h>
#include <webgpu/webgpu.h>

#include <cstdio>

namespace {

class LinkTest : public testing::Test {};

// Compile-time test that all symbols in the C API can be statically linked.
// (It is disabled because we don't actually need to run it, only link it.)
TEST_F(LinkTest, DISABLED_CFunctionsMethods) {
    // Write each function's pointer using a volatile write. This prevents the
    // the compiler from optimizing away the write (and thus the linkage).
    const void* fnPtr = nullptr;
    const void* volatile* fnPtrPtr = &fnPtr;

    {% for function in by_category["function"] %}
        *fnPtrPtr = reinterpret_cast<const void*>(&{{as_cMethod(None, function.name)}});
    {% endfor %}
    {% for type in by_category["object"] if len(c_methods(type)) > 0 %}

        {% for method in c_methods(type) %}
            *fnPtrPtr = reinterpret_cast<const void*>(&{{as_cMethod(type.name, method.name)}});
        {% endfor %}
    {% endfor %}
}

}  // namespace
