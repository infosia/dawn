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

#include "src/tint/lang/hlsl/ir/for_loop.h"

#include "src/tint/lang/core/ir/clone_context.h"
#include "src/tint/lang/core/ir/module.h"
#include "src/tint/lang/core/ir/multi_in_block.h"
#include "src/tint/utils/ice/ice.h"

TINT_INSTANTIATE_TYPEINFO(tint::hlsl::ir::ForLoop);

namespace tint::hlsl::ir {

ForLoop::ForLoop(Id id) : Base(id) {}

ForLoop::ForLoop(Id id,
                 tint::core::ir::Block* i,
                 tint::core::ir::MultiInBlock* cond,
                 tint::core::ir::MultiInBlock* b,
                 tint::core::ir::MultiInBlock* c)
    : Base(id, i, b, c), condblock_(cond) {
    if (condblock_) {
        condblock_->SetParent(this);
    }
}

ForLoop::~ForLoop() = default;

ForLoop* ForLoop::Clone(tint::core::ir::CloneContext& ctx) {
    auto* new_init = ctx.ir.blocks.Create<tint::core::ir::MultiInBlock>();
    auto* new_cond = ctx.ir.blocks.Create<tint::core::ir::MultiInBlock>();
    auto* new_body = ctx.ir.blocks.Create<tint::core::ir::MultiInBlock>();
    auto* new_continuing = ctx.ir.blocks.Create<tint::core::ir::MultiInBlock>();

    auto* new_loop =
        ctx.ir.CreateInstruction<ForLoop>(new_init, new_cond, new_body, new_continuing);
    ctx.Replace(this, new_loop);

    initializer_->CloneInto(ctx, new_init);
    condblock_->CloneInto(ctx, new_cond);
    body_->CloneInto(ctx, new_body);
    continuing_->CloneInto(ctx, new_continuing);

    new_loop->SetResults(ctx.Clone(results_));

    return new_loop;
}

bool ForLoop::HasCondBlock() const {
    return condblock_->Terminator() != nullptr;
}

void ForLoop::SetCondBlock(core::ir::MultiInBlock* block) {
    condblock_ = block;
}

void ForLoop::ForeachBlock(const std::function<void(tint::core::ir::Block*)>& cb) {
    Base::ForeachBlock(cb);
    cb(condblock_);
}

void ForLoop::ForeachBlock(const std::function<void(const tint::core::ir::Block*)>& cb) const {
    Base::ForeachBlock(cb);
    cb(condblock_);
}

}  // namespace tint::hlsl::ir
