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

Result<pb::Module> ProtoFromText(const std::string& text_proto) {
    pb::Module pb;
    if (!google::protobuf::TextFormat::ParseFromString(text_proto, &pb)) {
        return Failure{"Failed to parse text proto"};
    }
    return pb;
}

Result<std::vector<uint8_t>> BinaryFromProto(pb::Module& pb) {
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

std::string BinaryToHexString(std::vector<uint8_t> vec) {
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

Result<Module> TextProtoToModule(const std::string& pb_text,
                                 const std::vector<uint8_t>& expected_bin) {
    pb::Module pb;
    {
        auto res = ProtoFromText(pb_text);
        if (res != Success) {
            return res.Failure();
        }
        pb = res.Move();
    }
    {
        auto res = BinaryFromProto(pb);
        if (res != Success) {
            return res.Failure();
        }
        auto pb_bin = res.Move();
        if (pb_bin != expected_bin) {
            return Failure{
                "protobuf did not produce expected binary, potentially due to backwards "
                "incompatible changes to ir.proto\n"
                "Expected:\n" +
                BinaryToHexString(expected_bin) + "\nActual:\n" + BinaryToHexString(pb_bin) + "\n"};
        }
    }
    return Decode(pb);
}

template <typename T>
bool CheckType(const type::Type* ty) {
    return ty->Is<T>();
}

using CheckTypeFn = decltype(&CheckType<type::I32>);
}  // namespace

TEST_F(DecodeTest, EmptyModule) {
    std::string text_proto = R"pb()pb";
    std::vector<uint8_t> expected_bin = {};
    auto res = TextProtoToModule(text_proto, expected_bin);
    ASSERT_EQ(res, Success) << res.Failure().reason;

    auto mod = res.Move();
    EXPECT_NE(mod.root_block, nullptr);
    EXPECT_TRUE(mod.functions.IsEmpty());
    EXPECT_EQ(mod.Types().begin(), mod.Types().end());
}

using BasicType = DecodeTestWithParam<std::tuple<
    /* ty_text */ std::string,
    /* expected_bin */ std::vector<uint8_t>,
    /* check_fn */ CheckTypeFn>>;

TEST_P(BasicType, Test) {
    std::string ty_text = std::get<0>(GetParam());
    std::vector<uint8_t> expected_bin = std::get<1>(GetParam());
    CheckTypeFn check_fn = std::get<2>(GetParam());

    std::string text_proto = R"pb(
                                 types { basic:)pb" +
                             ty_text + R"pb(}
    )pb";
    auto res = TextProtoToModule(text_proto, expected_bin);
    ASSERT_EQ(res, Success) << res.Failure().reason;
    auto mod = res.Move();

    size_t ty_count = 0;
    for (const type::Type* ty : mod.Types()) {
        ty_count++;
        EXPECT_TRUE(check_fn(ty));
    }
    EXPECT_EQ(ty_count, 1u);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(DecodeTest,
                         BasicType,
                         testing::Values(
                             std::make_tuple("void",
                                             std::vector<uint8_t>({0x0a, 0x02, 0x08, 0x00}),
                                             CheckType<type::Void>),
                             std::make_tuple("bool",
                                             std::vector<uint8_t>({0x0a, 0x02, 0x08, 0x01}),
                                             CheckType<type::Bool>),
                             std::make_tuple("i32",
                                             std::vector<uint8_t>({0x0a, 0x02, 0x08, 0x02}),
                                             CheckType<type::I32>),
                             std::make_tuple("u32",
                                             std::vector<uint8_t>({0x0a, 0x02, 0x08, 0x03}),
                                             CheckType<type::U32>),
                             std::make_tuple("f32",
                                             std::vector<uint8_t>({0x0a, 0x02, 0x08, 0x04}),
                                             CheckType<type::F32>),
                             std::make_tuple("f16",
                                             std::vector<uint8_t>({0x0a, 0x02, 0x08, 0x05}),
                                             CheckType<type::F16>),
                             std::make_tuple("i8",
                                             std::vector<uint8_t>({0x0a, 0x02, 0x08, 0x06}),
                                             CheckType<type::I8>),
                             std::make_tuple("u8",
                                             std::vector<uint8_t>({0x0a, 0x02, 0x08, 0x07}),
                                             CheckType<type::U8>)
                             ));
// clang-format on

}  // namespace tint::core::ir::binary
