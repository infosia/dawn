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

#include "src/tint/lang/spirv/reader/parser/namer.h"

#include "gtest/gtest.h"
#include "src/tint/lang/core/builtin_fn.h"
#include "src/tint/utils/text/string_stream.h"

namespace tint::spirv::reader {
namespace {

using SpirvReaderNamerTest = testing::Test;

TEST_F(SpirvReaderNamerTest, SanitizeEmpty) {
    EXPECT_EQ(Namer::Sanitize(""), "empty");
}

TEST_F(SpirvReaderNamerTest, SanitizeLeadingUnderscore) {
    EXPECT_EQ(Namer::Sanitize("_"), "x_");
}

TEST_F(SpirvReaderNamerTest, SanitizeLeadingDigit) {
    EXPECT_EQ(Namer::Sanitize("7zip"), "x7zip");
}

TEST_F(SpirvReaderNamerTest, SanitizeOkChars) {
    EXPECT_EQ(Namer::Sanitize("_abcdef12345"), "x_abcdef12345");
}

TEST_F(SpirvReaderNamerTest, SanitizeNonIdentifierChars) {
    EXPECT_EQ(Namer::Sanitize("a:1.2'f\n"), "a_1_2_f_");
}

TEST_F(SpirvReaderNamerTest, Register_Once) {
    Namer namer;

    const uint32_t id = 9;
    EXPECT_FALSE(namer.HasName(id));

    namer.RegisterSanitizedName(id, "abbey road");
    EXPECT_TRUE(namer.HasName(id));
    EXPECT_EQ(namer.GetName(id), "abbey road");
}

TEST_F(SpirvReaderNamerTest, Register_TwoIds) {
    Namer namer;

    EXPECT_FALSE(namer.HasName(8));
    EXPECT_FALSE(namer.HasName(9));

    namer.RegisterSanitizedName(8, "abbey road");
    namer.RegisterSanitizedName(9, "rubber soul");

    EXPECT_TRUE(namer.HasName(8));
    EXPECT_TRUE(namer.HasName(9));

    EXPECT_EQ(namer.GetName(9), "rubber soul");
    EXPECT_EQ(namer.GetName(8), "abbey road");
}

TEST_F(SpirvReaderNamerTest, RegisterSanitizedName_TakeSuggestionWhenNoConflict) {
    Namer namer;

    namer.RegisterSanitizedName(1, "father");
    EXPECT_EQ(namer.GetName(1), "father");
}

TEST_F(SpirvReaderNamerTest, RegisterSanitizedName_SanitizeSuggestion) {
    Namer namer;

    namer.RegisterSanitizedName(9, "m:kenzie");
    EXPECT_EQ(namer.GetName(9), "m_kenzie");
}

TEST_F(SpirvReaderNamerTest, RegisterSanitizedName_GenerateNewNameWhenConflictOnDifferentId) {
    Namer namer;

    namer.RegisterSanitizedName(7, "rice");
    namer.RegisterSanitizedName(9, "rice");
    EXPECT_EQ(namer.GetName(9), "rice_1");
}

TEST_F(SpirvReaderNamerTest, GetMemberName_EmptyStringForUnvisitedStruct) {
    Namer namer;
    EXPECT_EQ(namer.GetMemberName(1, 2), "");
}

TEST_F(SpirvReaderNamerTest, GetMemberName_EmptyStringForUnvisitedMember) {
    Namer namer;
    namer.RegisterSanitizedMemberName(1, 2, "mother");
    EXPECT_EQ(namer.GetMemberName(1, 0), "");
}

TEST_F(SpirvReaderNamerTest, RegisterSanitizedMemberName_TakeSuggestionWhenNoConflict) {
    Namer namer;
    namer.RegisterSanitizedMemberName(1, 2, "mother");
    EXPECT_EQ(namer.GetMemberName(1, 2), "mother");
}

TEST_F(SpirvReaderNamerTest, RegisterSanitizedMemberName_TakeSanitizedSuggestion) {
    Namer namer;
    namer.RegisterSanitizedMemberName(1, 2, "m:t%er");
    EXPECT_EQ(namer.GetMemberName(1, 2), "m_t_er");
}

TEST_F(
    SpirvReaderNamerTest,
    RegisterSanitizedMemberName_TakeSuggestionWhenNoConflictAfterSuggestionForLowerMember) {  // NOLINT
    Namer namer;
    namer.RegisterSanitizedMemberName(1, 7, "mother");
    EXPECT_EQ(namer.GetMemberName(1, 2), "");

    namer.RegisterSanitizedMemberName(1, 2, "mary");
    EXPECT_EQ(namer.GetMemberName(1, 2), "mary");
}

TEST_F(SpirvReaderNamerTest, Name_GeneratesNameIfNoneRegistered) {
    Namer namer;
    EXPECT_EQ(namer.GetName(14), "x_14");
}

TEST_F(SpirvReaderNamerTest, Name_GeneratesNameWithoutConflict) {
    Namer namer;
    namer.RegisterSanitizedName(42, "x_14");
    EXPECT_EQ(namer.GetName(14), "x_14_1");
}

TEST_F(SpirvReaderNamerTest, Name_ReturnsRegisteredName) {
    Namer namer;
    namer.RegisterSanitizedName(14, "hello");
    EXPECT_EQ(namer.GetName(14), "hello");
}

using SpirvParserNamerReservedWordTest = ::testing::TestWithParam<std::string>;

TEST_P(SpirvParserNamerReservedWordTest, ReservedWordsAreUsed) {
    Namer namer;

    const std::string reserved = GetParam();
    namer.RegisterSanitizedName(1, reserved);
    // Since it's reserved, it's marked as used, and we can't register an ID
    EXPECT_EQ(namer.GetName(1), reserved + "_1");
}

INSTANTIATE_TEST_SUITE_P(SpirvASTParserTest_ReservedWords,
                         SpirvParserNamerReservedWordTest,
                         ::testing::ValuesIn(std::vector<std::string>{
                             // Please keep this list sorted.
                             "array",      "as",          "asm",
                             "bf16",       "binding",     "block",
                             "bool",       "break",       "builtin",
                             "case",       "cast",        "compute",
                             "const",      "continue",    "default",
                             "discard",    "do",          "else",
                             "elseif",     "entry_point", "enum",
                             "f16",        "f32",         "fallthrough",
                             "false",      "fn",          "for",
                             "fragment",   "i16",         "i32",
                             "i64",        "i8",          "if",
                             "image",      "import",      "in",
                             "let",        "location",    "loop",
                             "mat2x2",     "mat2x3",      "mat2x4",
                             "mat3x2",     "mat3x3",      "mat3x4",
                             "mat4x2",     "mat4x3",      "mat4x4",
                             "offset",     "out",         "override",
                             "premerge",   "private",     "ptr",
                             "regardless", "return",      "set",
                             "storage",    "struct",      "switch",
                             "true",       "type",        "typedef",
                             "u16",        "u32",         "u64",
                             "u8",         "uniform",     "uniform_constant",
                             "unless",     "using",       "var",
                             "vec2",       "vec3",        "vec4",
                             "vertex",     "void",        "while",
                             "workgroup",
                         }));

using SpirvParserNamerBuiltinFunctionTest = ::testing::TestWithParam<const char*>;

TEST_P(SpirvParserNamerBuiltinFunctionTest, BuiltinFunctionsAreUsed) {
    Namer namer;

    const std::string builtin_fn = GetParam();
    namer.RegisterSanitizedName(1, builtin_fn);
    // Since it's a builtin function, it's marked as used, and we can't register an ID.
    EXPECT_EQ(namer.GetName(1), builtin_fn + "_1");
}

INSTANTIATE_TEST_SUITE_P(SpirvASTParserTest_BuiltinFunctions,
                         SpirvParserNamerBuiltinFunctionTest,
                         ::testing::ValuesIn(core::kBuiltinFnStrings));

}  // namespace
}  // namespace tint::spirv::reader
