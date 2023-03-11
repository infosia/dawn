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

#include "gtest/gtest.h"

#include "tint/interp/memory.h"

namespace tint::interp {
namespace {

using MemoryTest = testing::Test;

TEST_F(MemoryTest, Create) {
    Memory alloc(4);
    EXPECT_EQ(alloc.Size(), 4u);
}

TEST_F(MemoryTest, LoadStore) {
    Memory alloc(8);
    uint32_t store1 = 0x91B7C3DA;
    uint32_t store2 = 0xF2C05E18;
    alloc.Store(&store1, 0);
    alloc.Store(&store2, 4);
    uint32_t load1 = 0;
    uint32_t load2 = 0;
    alloc.Load(&load1, 0);
    alloc.Load(&load2, 4);
    EXPECT_EQ(load1, store1);
    EXPECT_EQ(load2, store2);
}

TEST_F(MemoryTest, Load_OOB) {
    Memory alloc(4);
    uint32_t store1 = 0x91B7C3DA;
    alloc.Store(&store1, 0);
    uint32_t load1 = 0xFFFFFFFF;
    uint32_t load2 = 0xFFFFFFFF;
    uint32_t load3 = 0xFFFFFFFF;
    alloc.Load(&load1, 0);
    alloc.Load(&load2, 4);
    alloc.Load(&load3, (1 << 20u));
    EXPECT_EQ(load1, store1);
    EXPECT_EQ(load2, 0u);
    EXPECT_EQ(load3, 0u);
}

TEST_F(MemoryTest, Store_OOB) {
    Memory alloc(4);
    uint32_t store1 = 0x91B7C3DA;
    uint32_t store2 = 0xF2C05E18;
    uint32_t store3 = 0xDEADBEEF;
    alloc.Store(&store1, 0);
    alloc.Store(&store2, 4);
    alloc.Store(&store3, (1 << 20u));
    uint32_t load1 = 0xFFFFFFFF;
    alloc.Load(&load1, 0);
    EXPECT_EQ(load1, store1);
}

}  // namespace
}  // namespace tint::interp
