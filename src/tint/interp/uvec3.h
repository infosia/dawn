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

#ifndef SRC_TINT_INTERP_UVEC3_H_
#define SRC_TINT_INTERP_UVEC3_H_

#include <array>
#include <cstdint>
#include <string>

#include "src/tint/utils/math/hash.h"

namespace tint::interp {

/// UVec3 is a wrapper around std::array<uint32_t, 3>.
class UVec3 {
  public:
    /// Default constructor - zero all values.
    UVec3() {}

    /// Constructor - initialize with values.
    /// @param x_value the x component
    /// @param y_value the y component
    /// @param z_value the z component
    UVec3(uint32_t x_value, uint32_t y_value, uint32_t z_value)
        : x(x_value), y(y_value), z(z_value) {}

    /// @param rhs the other UVec3 to compare to
    /// @returns true if this UVec3 is equal to `rhs`
    bool operator==(const UVec3& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }

    /// @param rhs the other UVec3 to compare to
    /// @returns true if this UVec3 is not equal to `rhs`
    bool operator!=(const UVec3& rhs) const { return !(*this == rhs); }

    /// @param rhs the other UVec3 to compare to
    /// @returns true if this UVec3 has a lower value than `rhs`
    bool operator<(const UVec3& rhs) const {
        if (z < rhs.z) {
            return true;
        } else if (z == rhs.z) {
            if (y < rhs.y) {
                return true;
            } else if (y == rhs.y) {
                if (x < rhs.x) {
                    return true;
                }
            }
        }
        return false;
    }

    /// @returns a string representation of the values, as "(x, y, z)"
    std::string Str() const {
        return "(" + std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(z) + ")";
    }

    /// The X component.
    uint32_t x = 0;
    /// The Y component.
    uint32_t y = 0;
    /// The Z component.
    uint32_t z = 0;
};

}  // namespace tint::interp

namespace std {

/// Custom std::hash specialization for UVec3 so that it can be used as a key for maps and sets.
template <>
class hash<tint::interp::UVec3> {
  public:
    /// @param vec the UVec3 to create a hash for
    /// @return the hash value
    inline std::size_t operator()(const tint::interp::UVec3& vec) const {
        return tint::Hash(vec.x, vec.y, vec.z);
    }
};

}  // namespace std

#endif  // SRC_TINT_INTERP_UVEC3_H_
