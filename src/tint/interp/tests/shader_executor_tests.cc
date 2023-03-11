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

#include "tint/interp/shader_executor.h"
#include "tint/tint.h"
#include "tint/utils/text/styled_text_printer.h"

namespace tint::interp {
namespace {

class ShaderExecutorTest : public testing::Test {
  protected:
    // Initialize the test with a WGSL source string.
    void Init(std::string source) {
        // Parse the source file to produce a Tint program object.
        file_ = std::make_unique<Source::File>("test.wgsl", source);
        program_ = std::make_unique<Program>(wgsl::reader::Parse(file_.get()));
        auto diag_printer = StyledTextPrinter::Create(stderr);
        diag::Formatter diag_formatter;
        ASSERT_TRUE(program_) << "Failed to run WGSL parser.";
        if (program_->Diagnostics().Count() > 0) {
            diag_printer->Print(diag_formatter.Format(program_->Diagnostics()));
        }
        ASSERT_TRUE(program_->IsValid()) << "Source WGSL was invalid.";
    }

    std::unique_ptr<Source::File> file_;
    std::unique_ptr<Program> program_;
};

TEST_F(ShaderExecutorTest, EntryPointNotFound) {
    Init(R"(
@compute @workgroup_size(1)
fn foo() {
})");
    auto result = ShaderExecutor::Create(*program_, "main", {});
    EXPECT_NE(result, Success);
    EXPECT_EQ(result.Failure(), R"(error: entry point 'main' not found in module)");
}

TEST_F(ShaderExecutorTest, EntryPointNotCompute) {
    Init(R"(
@fragment
fn main() {
})");
    auto result = ShaderExecutor::Create(*program_, "main", {});
    EXPECT_NE(result, Success);
    EXPECT_EQ(result.Failure(), R"(error: function 'main' is not a compute shader)");
}

TEST_F(ShaderExecutorTest, MissingNamedOverride) {
    Init(R"(
override x : i32;

@compute @workgroup_size(1)
fn main() {
    let y = x;
})");
    auto result = ShaderExecutor::Create(*program_, "main", {});
    EXPECT_NE(result, Success);
    EXPECT_EQ(result.Failure(), R"(test.wgsl:2:1 error: missing pipeline-override value for 'x'
override x : i32;
^^^^^^^^^^^^^^^^
)");
}

TEST_F(ShaderExecutorTest, MissingBufferBinding) {
    Init(R"(
@group(0) @binding(0) var<storage, read_write> x : i32;

@compute @workgroup_size(1)
fn main() {
    x++;
})");
    auto executor = ShaderExecutor::Create(*program_, "main", {});
    ASSERT_EQ(executor, Success) << executor.Failure();
    auto result = executor.Get()->Run({1, 1, 1}, {});
    EXPECT_EQ(result.Failure(), R"(error: missing buffer binding for @group(0) @binding(0))");
}

TEST_F(ShaderExecutorTest, InvalidBufferBinding) {
    Init(R"(
@group(0) @binding(0) var<storage, read_write> x : i32;

@compute @workgroup_size(1)
fn main() {
    x++;
})");
    auto executor = ShaderExecutor::Create(*program_, "main", {});
    ASSERT_EQ(executor, Success) << executor.Failure();

    Binding buffer;
    buffer.buffer = nullptr;
    BindingList bindings;
    bindings[{0, 0}] = buffer;
    auto result = executor.Get()->Run({1, 1, 1}, std::move(bindings));
    EXPECT_EQ(result.Failure(), R"(error: invalid binding resource for @group(0) @binding(0))");
}

}  // namespace
}  // namespace tint::interp
