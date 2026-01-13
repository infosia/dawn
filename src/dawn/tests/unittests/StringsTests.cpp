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

#include <gtest/gtest.h>

#include "dawn/common/Strings.h"

#define EXPECT_MINIFY(exp, in) EXPECT_EQ(std::string_view{exp}, DAWN_ALWAYS_MINIFY(in).c_str())

TEST(Strings, Minify) {
    EXPECT_MINIFY("", "  ");
    EXPECT_MINIFY("foo", "foo");
    EXPECT_MINIFY("foo bar", "  foo   bar  ");
    EXPECT_MINIFY("foo(bar){baz;{bu,zz}}", "  foo  (  bar  ) { baz ; { bu , zz } } ");
    EXPECT_MINIFY("foo{bar}", "foo\n{\nbar}");
    EXPECT_MINIFY("foo\nbar", "foo  \n  bar");

    EXPECT_MINIFY("@fragment\nfn fs_empty_main(){2/3}", R"(
        // comment
        // comment
        @fragment  // comment
        // comment
        // comment
        fn fs_empty_main  ( )  {  2 / 3  }
        // comment
        // comment
    )");

    // Note we can't test that multiline comments are an error, because it fails compilation.
}
