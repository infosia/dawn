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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "dawn/native/dawn_platform.h"

namespace dawn::native {
namespace {

// Test that default construction or assignment to wgpu::String produces the nil string.
TEST(CppAPITests, WGPUStringDefault) {
    {
        wgpu::String s;
        EXPECT_EQ(s.data, nullptr);
        EXPECT_EQ(s.length, SIZE_MAX);
    }
    {
        wgpu::String s{};
        EXPECT_EQ(s.data, nullptr);
        EXPECT_EQ(s.length, SIZE_MAX);
    }
    {
        wgpu::String s = {};
        EXPECT_EQ(s.data, nullptr);
        EXPECT_EQ(s.length, SIZE_MAX);
    }
    {
        wgpu::String s = wgpu::String();
        EXPECT_EQ(s.data, nullptr);
        EXPECT_EQ(s.length, SIZE_MAX);
    }

    // Test that resetting the string, clears both data and length.
    std::string_view sv("hello world!");
    {
        wgpu::String s(sv);
        s = {};
        EXPECT_EQ(s.data, nullptr);
        EXPECT_EQ(s.length, SIZE_MAX);
    }
    {
        wgpu::String s(sv);
        s = wgpu::String();
        EXPECT_EQ(s.data, nullptr);
        EXPECT_EQ(s.length, SIZE_MAX);
    }
}

// Test that construction or assignment to wgpu::String from const char*.
TEST(CppAPITests, WGPUStringFromCstr) {
    {
        wgpu::String s("hello world!");
        EXPECT_STREQ(s.data, "hello world!");
        EXPECT_EQ(s.length, SIZE_MAX);
    }
    {
        wgpu::String s{"hello world!"};
        EXPECT_STREQ(s.data, "hello world!");
        EXPECT_EQ(s.length, SIZE_MAX);
    }
    {
        wgpu::String s = {"hello world!"};
        EXPECT_STREQ(s.data, "hello world!");
        EXPECT_EQ(s.length, SIZE_MAX);
    }
    {
        wgpu::String s = wgpu::String("hello world!");
        EXPECT_STREQ(s.data, "hello world!");
        EXPECT_EQ(s.length, SIZE_MAX);
    }

    // Test that setting to a cstr clears the length.
    std::string_view sv("hello world!");
    {
        wgpu::String s(sv);
        s = "other str";
        EXPECT_STREQ(s.data, "other str");
        EXPECT_EQ(s.length, SIZE_MAX);
    }
}

// Test that construction or assignment to wgpu::String from std::string_view
TEST(CppAPITests, WGPUStringFromStdStringView) {
    std::string_view sv("hello\x00world!");
    {
        wgpu::String s(sv);
        EXPECT_EQ(s.data, sv.data());
        EXPECT_EQ(s.length, sv.length());
    }
    {
        wgpu::String s{sv};
        EXPECT_EQ(s.data, sv.data());
        EXPECT_EQ(s.length, sv.length());
    }
    {
        wgpu::String s = {sv};
        EXPECT_EQ(s.data, sv.data());
        EXPECT_EQ(s.length, sv.length());
    }
    {
        wgpu::String s = wgpu::String(sv);
        EXPECT_EQ(s.data, sv.data());
        EXPECT_EQ(s.length, sv.length());
    }
}

// Test that construction or assignment to wgpu::String from pointer and length
TEST(CppAPITests, WGPUStringFromPtrAndLength) {
    std::string_view sv("hello\x00world!");
    {
        wgpu::String s(sv.data(), sv.length());
        EXPECT_EQ(s.data, sv.data());
        EXPECT_EQ(s.length, sv.length());
    }
    {
        wgpu::String s{sv.data(), sv.length()};
        EXPECT_EQ(s.data, sv.data());
        EXPECT_EQ(s.length, sv.length());
    }
    {
        wgpu::String s = {sv.data(), sv.length()};
        EXPECT_EQ(s.data, sv.data());
        EXPECT_EQ(s.length, sv.length());
    }
    {
        wgpu::String s = wgpu::String(sv.data(), sv.length());
        EXPECT_EQ(s.data, sv.data());
        EXPECT_EQ(s.length, sv.length());
    }
}

// Test that construction or assignment to wgpu::String from nullptr
TEST(CppAPITests, WGPUStringFromNullptr) {
    {
        wgpu::String s(nullptr);
        EXPECT_EQ(s.data, nullptr);
        EXPECT_EQ(s.length, SIZE_MAX);
    }
    {
        wgpu::String s{nullptr};
        EXPECT_EQ(s.data, nullptr);
        EXPECT_EQ(s.length, SIZE_MAX);
    }
    {
        wgpu::String s = {nullptr};
        EXPECT_EQ(s.data, nullptr);
        EXPECT_EQ(s.length, SIZE_MAX);
    }
    {
        wgpu::String s = wgpu::String(nullptr);
        EXPECT_EQ(s.data, nullptr);
        EXPECT_EQ(s.length, SIZE_MAX);
    }

    // Test that setting to nullptr, clears both data and length.
    std::string_view sv("hello world!");
    {
        wgpu::String s(sv);
        s = nullptr;
        EXPECT_EQ(s.data, nullptr);
        EXPECT_EQ(s.length, SIZE_MAX);
    }
}

// Test that construction or assignment to wgpu::String from std::nullopt
TEST(CppAPITests, WGPUStringFromNullopt) {
    {
        wgpu::String s(std::nullopt);
        EXPECT_EQ(s.data, nullptr);
        EXPECT_EQ(s.length, SIZE_MAX);
    }
    {
        wgpu::String s{std::nullopt};
        EXPECT_EQ(s.data, nullptr);
        EXPECT_EQ(s.length, SIZE_MAX);
    }
    {
        wgpu::String s = {std::nullopt};
        EXPECT_EQ(s.data, nullptr);
        EXPECT_EQ(s.length, SIZE_MAX);
    }
    {
        wgpu::String s = wgpu::String(std::nullopt);
        EXPECT_EQ(s.data, nullptr);
        EXPECT_EQ(s.length, SIZE_MAX);
    }

    // Test that setting to std::nullopt, clears both data and length.
    std::string_view sv("hello world!");
    {
        wgpu::String s(sv);
        s = std::nullopt;
        EXPECT_EQ(s.data, nullptr);
        EXPECT_EQ(s.length, SIZE_MAX);
    }
}

}  // anonymous namespace
}  // namespace dawn::native
