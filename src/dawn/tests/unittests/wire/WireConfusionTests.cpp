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

#include "dawn/tests/unittests/wire/WireTest.h"

#include "dawn/utils/WGPUHelpers.h"
#include "dawn/wire/WireClient.h"

namespace dawn::wire {
namespace {

enum class TestState {
    Alive,
    DisconnectBefore,
    DisconnectAfter,
};
std::ostream& operator<<(std::ostream& stream, TestState flavor) {
    switch (flavor) {
        case TestState::Alive:
            stream << "Alive";
            break;
        case TestState::DisconnectBefore:
            stream << "DisconnectBefore";
            break;
        case TestState::DisconnectAfter:
            stream << "DisconnectAfter";
            break;
    }
    return stream;
}

// Two copies of WireTest to set up a second parallel wire.
class WireOne : public WireTest {};
class WireTwo : public WireTest {};

class WireConfusionDeathTest : public WireOne,
                               public WireTwo,
                               public ::testing::WithParamInterface<TestState> {
  protected:
    void SetUp() override {
        WireOne::SetUp();
        WireTwo::SetUp();

        if (GetParam() == TestState::DisconnectBefore) {
            WireTwo::GetWireClient()->Disconnect();
        }
    }

    void TearDown() override {
        WireTwo::TearDown();
        WireOne::TearDown();
    }

    void MaybeDisconnectAfter() {
        if (GetParam() == TestState::DisconnectAfter) {
            WireTwo::GetWireClient()->Disconnect();
        }
    }
};

TEST_P(WireConfusionDeathTest, BindGroupFromLayout) {
    wgpu::BindGroupLayout two_bgl = utils::MakeBindGroupLayout(WireTwo::device, {});

    MaybeDisconnectAfter();

    wgpu::BindGroupDescriptor bgDesc{.layout = two_bgl};
    EXPECT_DEATH({ WireOne::device.CreateBindGroup(&bgDesc); }, "");
}

INSTANTIATE_TEST_SUITE_P(,
                         WireConfusionDeathTest,
                         ::testing::Values(
                             // TestState::Alive,
                             TestState::DisconnectBefore,
                             TestState::DisconnectAfter),
                         testing::PrintToStringParamName());

}  // anonymous namespace
}  // namespace dawn::wire
