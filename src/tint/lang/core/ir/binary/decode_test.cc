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

#include "src/tint/lang/core/ir/binary/decode.h"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#include "gtest/gtest.h"

#include "src/tint/lang/core/ir/builder.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/utils/macros/compiler.h"

TINT_BEGIN_DISABLE_PROTOBUF_WARNINGS();
#include "google/protobuf/text_format.h"
#include "src/tint/utils/protos/ir/ir.pb.h"
TINT_END_DISABLE_PROTOBUF_WARNINGS();

namespace tint::core::ir::binary {
namespace {

template <typename BASE>
class DecodeTestBase : public BASE {
  public:
    DecodeTestBase() = default;
    ~DecodeTestBase() override = default;
};

using DecodeTest = DecodeTestBase<testing::Test>;
template <typename T>
using DecodeTestWithParam = DecodeTestBase<testing::TestWithParam<T>>;

Result<pb::Module> TextProtoToProtoModule(const std::string& text_proto) {
    pb::Module pb;
    if (!google::protobuf::TextFormat::ParseFromString(text_proto, &pb)) {
        return Failure{"Failed to parse text proto"};
    }
    return pb;
}

Result<std::vector<uint8_t>> ProtoModuleToBinaryProto(pb::Module& pb) {
    std::vector<uint8_t> buf;
    size_t len = pb.ByteSizeLong();
    buf.resize(len);
    if (len > 0) {
        if (!pb.SerializeToArray(&buf[0], static_cast<int>(len))) {
            return Failure{"Failed to serialize protobuf"};
        }
    }
    return buf;
}

std::string BinaryProtoToHexString(std::vector<uint8_t> vec) {
    std::stringstream ss;
    ss << "{ ";
    bool first_elem = true;
    for (uint8_t val : vec) {
        if (!first_elem) {
            ss << ", ";
        } else {
            first_elem = false;
        }
        ss << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(val);
    }
    ss << " }";
    return ss.str();
}

Result<Module> TextProtoToIRModule(const std::string& pb_text,
                                   const std::vector<uint8_t>& expected_bin) {
    pb::Module pb;
    {
        auto res = TextProtoToProtoModule(pb_text);
        if (res != Success) {
            return res.Failure();
        }
        pb = res.Move();
    }
    {
        auto res = ProtoModuleToBinaryProto(pb);
        if (res != Success) {
            return res.Failure();
        }
        auto pb_bin = res.Move();
        if (pb_bin != expected_bin) {
            return Failure{
                "protobuf did not produce expected binary, potentially due to backwards "
                "incompatible changes to ir.proto"
                "\nInput Text:\n" +
                pb_text + "\nExpected Binary:\n" + BinaryProtoToHexString(expected_bin) +
                "\nActual Binary:\n" + BinaryProtoToHexString(pb_bin)};
        }
    }
    return Decode(pb);
}

template <typename T>
bool CheckType(const type::Type* ty) {
    return ty->Is<T>();
}
using CheckTypeFn = decltype(&CheckType<type::I32>);

typedef struct {
    std::string ty_str;
    CheckTypeFn check_fn;
    std::vector<uint8_t> expected_bin;
} BasicTypeCheck;

TINT_BEGIN_DISABLE_WARNING(EXIT_TIME_DESTRUCTORS);
static BasicTypeCheck void_check{"void", CheckType<type::Void>, {0x0a, 0x02, 0x08, 0x00}};
static BasicTypeCheck bool_check{"bool", CheckType<type::Bool>, {0x0a, 0x02, 0x08, 0x01}};
static BasicTypeCheck i32_check{"i32", CheckType<type::I32>, {0x0a, 0x02, 0x08, 0x02}};
static BasicTypeCheck u32_check{"u32", CheckType<type::U32>, {0x0a, 0x02, 0x08, 0x03}};
static BasicTypeCheck f32_check{"f32", CheckType<type::F32>, {0x0a, 0x02, 0x08, 0x04}};
static BasicTypeCheck f16_check{"f16", CheckType<type::F16>, {0x0a, 0x02, 0x08, 0x05}};
static BasicTypeCheck i8_check{"i8", CheckType<type::I8>, {0x0a, 0x02, 0x08, 0x06}};
static BasicTypeCheck u8_check{"u8", CheckType<type::U8>, {0x0a, 0x02, 0x08, 0x07}};
TINT_END_DISABLE_WARNING(EXIT_TIME_DESTRUCTORS);

typedef struct {
    uint32_t width;
    std::vector<uint8_t> expected_bin;
} VectorTypeCheck;

TINT_BEGIN_DISABLE_WARNING(EXIT_TIME_DESTRUCTORS);
static VectorTypeCheck vec2_check{2u, {0x0a, 0x04, 0x12, 0x02, 0x08, 0x02}};
static VectorTypeCheck vec3_check{3u, {0x0a, 0x04, 0x12, 0x02, 0x08, 0x03}};
static VectorTypeCheck vec4_check{4u, {0x0a, 0x04, 0x12, 0x02, 0x08, 0x04}};
TINT_END_DISABLE_WARNING(EXIT_TIME_DESTRUCTORS);

}  // namespace

TEST_F(DecodeTest, EmptyModule) {
    std::string text_proto = R"pb()pb";
    std::vector<uint8_t> expected_bin = {};
    auto res = TextProtoToIRModule(text_proto, expected_bin);
    ASSERT_EQ(res, Success) << res.Failure().reason;

    auto mod = res.Move();
    EXPECT_NE(mod.root_block, nullptr);
    EXPECT_TRUE(mod.functions.IsEmpty());
    EXPECT_EQ(mod.Types().begin(), mod.Types().end());
}

using BasicType = DecodeTestWithParam<BasicTypeCheck>;

TEST_P(BasicType, Test) {
    auto checker = GetParam();
    std::string text_proto = R"pb(types { basic:)pb" + checker.ty_str + R"pb(})pb";
    auto res = TextProtoToIRModule(text_proto, checker.expected_bin);
    ASSERT_EQ(res, Success) << res.Failure().reason;
    auto mod = res.Move();

    size_t ty_count = 0;
    for (const type::Type* ty : mod.Types()) {
        ty_count++;
        EXPECT_TRUE(checker.check_fn(ty));
    }
    EXPECT_EQ(ty_count, 1u);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(DecodeTest,
                         BasicType,
                         testing::Values(void_check,
                                         bool_check,
                                         i32_check,
                                         u32_check,
                                         f32_check,
                                         f16_check,
                                         i8_check,
                                         u8_check));
// clang-format on

using VectorType = DecodeTestWithParam<std::tuple<
    /* elem_check */ BasicTypeCheck,
    /* vec_check */ VectorTypeCheck>>;

TEST_P(VectorType, Test) {
    BasicTypeCheck elem_check = std::get<0>(GetParam());
    VectorTypeCheck vec_check = std::get<1>(GetParam());

    // clang-format off
    std::string text_proto = \
R"pb(types { basic: )pb" + elem_check.ty_str  + R"pb( }
types {
    vector {
        element_type: 0
        width: )pb" + std::to_string(vec_check.width) + R"pb(
    }
})pb";
    // clang-format on

    auto expected_bin = elem_check.expected_bin;
    expected_bin.insert(expected_bin.end(), vec_check.expected_bin.begin(),
                        vec_check.expected_bin.end());

    auto res = TextProtoToIRModule(text_proto, expected_bin);
    ASSERT_EQ(res, Success) << res.Failure().reason;
    auto mod = res.Move();

    const type::Vector* vec_ty = nullptr;
    size_t ty_count = 0;
    for (const type::Type* ty : mod.Types()) {
        ty_count++;
        if (auto* v = ty->As<type::Vector>()) {
            EXPECT_EQ(vec_ty, nullptr) << "Found more than one vector type";
            vec_ty = v;
        }
    }
    EXPECT_EQ(ty_count, 2u) << "Expected one element type and one vector type";
    ASSERT_NE(vec_ty, nullptr) << "Vector type not found";

    EXPECT_TRUE(elem_check.check_fn(vec_ty->Type())) << "Element type mismatch";
    EXPECT_EQ(vec_ty->Width(), vec_check.width) << "Vector width mismatch";
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(DecodeTest,
                         VectorType,
                         testing::Combine(
                             testing::Values(void_check,
                                             bool_check,
                                             i32_check,
                                             u32_check,
                                             f32_check,
                                             f16_check,
                                             i8_check,
                                             u8_check),
                             testing::Values(vec2_check,
                                             vec3_check,
                                             vec4_check)));
// clang-format on

}  // namespace tint::core::ir::binary
