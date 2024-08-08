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

#include "src/tint/lang/core/type/multi_indexed.h"

#include "src/tint/lang/core/type/manager.h"
#include "src/tint/lang/core/type/vector.h"
#include "src/tint/utils/ice/ice.h"

TINT_INSTANTIATE_TYPEINFO(tint::core::type::MultiIndexed);

namespace tint::core::type {

MultiIndexed::MultiIndexed(const Vector* column_type, uint32_t columns)
    : Base(Hash(tint::TypeCode::Of<Vector>().bits, columns, column_type),
           core::type::Flags{
               Flag::kConstructable,
               Flag::kCreationFixedFootprint,
               Flag::kFixedFootprint,
           }),
      subtype_(column_type->type()),
      column_type_(column_type),
      rows_(column_type->Width()),
      columns_(columns) {
    TINT_ASSERT(rows_ > 1);
    TINT_ASSERT(rows_ < 5);
    TINT_ASSERT(columns_ > 1);
    TINT_ASSERT(columns_ < 5);
}

MultiIndexed::~MultiIndexed() = default;

bool MultiIndexed::Equals(const UniqueNode& other) const {
    if (auto* v = other.As<MultiIndexed>()) {
        return v->rows_ == rows_ && v->columns_ == columns_ && v->column_type_ == column_type_;
    }
    return false;
}

uint32_t MultiIndexed::Size() const {
    return column_type_->Align() * columns();
}

uint32_t MultiIndexed::Align() const {
    return column_type_->Align();
}

uint32_t MultiIndexed::ColumnStride() const {
    return column_type_->Align();
}

TypeAndCount MultiIndexed::Elements([[maybe_unused]] const Type* type_if_invalid,
                                    [[maybe_unused]] uint32_t count_if_invalid) const {
    return {column_type_, columns_};
}

const Vector* MultiIndexed::Element(uint32_t index) const {
    return index < columns_ ? column_type_ : nullptr;
}

}  // namespace tint::core::type
