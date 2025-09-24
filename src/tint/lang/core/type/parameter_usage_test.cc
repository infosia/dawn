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

#include "src/tint/lang/core/type/parameter_usage.h"

#include "gtest/gtest.h"

namespace tint::core::type {
namespace {

using ParamUsageTest = testing::Test;

using ParamUsageOnlyTextureTest = testing::TestWithParam<core::BuiltinFn>;

TEST_P(ParamUsageOnlyTextureTest, TextureDimension) {
    auto fn = GetParam();

    auto usage = LookupParameterIndices(fn, core::type::TextureDimension::k2d, false, 3);
    EXPECT_TRUE(usage.texture.has_value());
    EXPECT_EQ(0u, usage.texture.value());

    EXPECT_FALSE(usage.sampler.has_value());
    EXPECT_FALSE(usage.component.has_value());
    EXPECT_FALSE(usage.offset.has_value());
}
INSTANTIATE_TEST_SUITE_P(CoreTest,
                         ParamUsageOnlyTextureTest,
                         testing::Values(core::BuiltinFn::kTextureDimensions,
                                         core::BuiltinFn::kTextureNumLayers,
                                         core::BuiltinFn::kTextureNumLevels,
                                         core::BuiltinFn::kTextureNumSamples,
                                         core::BuiltinFn::kTextureLoad,
                                         core::BuiltinFn::kTextureStore));

TEST_F(ParamUsageTest, TextureSampleClampToEdge) {
    auto usage = LookupParameterIndices(core::BuiltinFn::kTextureSampleBaseClampToEdge,
                                        core::type::TextureDimension::k2d, false, 3);
    EXPECT_TRUE(usage.texture.has_value());
    EXPECT_EQ(0u, usage.texture.value());
    EXPECT_TRUE(usage.sampler.has_value());
    EXPECT_EQ(1u, usage.sampler.value());

    EXPECT_FALSE(usage.component.has_value());
    EXPECT_FALSE(usage.offset.has_value());
}

}  // namespace
}  // namespace tint::core::type
