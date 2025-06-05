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

    static Result<pb::Module> ProtoFromText(const std::string& text_proto) {
        pb::Module pb;
        if (!google::protobuf::TextFormat::ParseFromString(text_proto, &pb)) {
            return Failure{"Failed to parse text proto"};
        }
        return pb;
    }

    static Result<std::vector<uint8_t>> BinaryFromProto(pb::Module& pb) {
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

    static std::string BinaryToHexString(std::vector<uint8_t> vec) {
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

    static Result<Module> TextProtoToModule(const std::string& pb_text,
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
            auto bin_proto = res.Move();
            fprintf(stderr, "RHARRSION: expected_bin: %s\n",
                    BinaryToHexString(expected_bin).c_str());
            fprintf(stderr, "RHARRSION: bin_proto: %s\n", BinaryToHexString(bin_proto).c_str());
            fflush(stderr);
            if (res != Success) {
                return Failure{
                    "text protobuf did not produce expected binary, potentially due to .proto "
                    "incompatible change"};
            }
        }
        return Decode(pb);
    }
};

using DecodeTest = DecodeTestBase<testing::Test>;

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

TEST_F(DecodeTest, BasicTypes_i32) {
    std::string text_proto = R"pb(
        types { basic: i32 }
        root_block: 0
        blocks {}  # Block 0
    )pb";
    std::vector<uint8_t> expected_bin = {0x0a, 0x02, 0x08, 0x02, 0x2a, 0x00};
    auto res = TextProtoToModule(text_proto, expected_bin);
    ASSERT_EQ(res, Success) << res.Failure().reason;
    auto mod = res.Move();

    size_t ty_count = 0;
    for (const type::Type* ty : mod.Types()) {
        ty_count++;
        EXPECT_TRUE(ty->Is<type::I32>());
    }
    EXPECT_EQ(ty_count, 1u);
}

}  // namespace
}  // namespace tint::core::ir::binary
