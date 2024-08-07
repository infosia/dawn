// Copyright 2022 The Dawn & Tint Authors
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

#include "src/tint/lang/core/type/subgroup_matrix_right.h"

#include "src/tint/lang/core/type/manager.h"
#include "src/tint/lang/core/type/vector.h"
#include "src/tint/utils/text/string_stream.h"

TINT_INSTANTIATE_TYPEINFO(tint::core::type::SubgroupMatrixRight);

namespace tint::core::type {

SubgroupMatrixRight::SubgroupMatrixRight(const Vector* column_type, uint32_t columns)
    : Base(column_type, columns) {}

SubgroupMatrixRight::~SubgroupMatrixRight() = default;

std::string SubgroupMatrixRight::FriendlyName() const {
    StringStream out;
    out << "subgroup_matrix_right<" << subtype_->FriendlyName() << ", " << columns_ << ", " << rows_
        << ">";
    return out.str();
}

SubgroupMatrixRight* SubgroupMatrixRight::Clone(CloneContext& ctx) const {
    auto* col_ty = column_type_->Clone(ctx);
    return ctx.dst.mgr->Get<SubgroupMatrixRight>(col_ty, columns_);
}

}  // namespace tint::core::type
