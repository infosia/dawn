// Copyright 2026 The Dawn & Tint Authors
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

#include "src/tint/cmd/fuzz/wgsl/cst/mutator.h"

#if TINT_BUILD_WGSL_READER

#include <random>
#include <vector>

namespace tint::fuzz::wgsl::cst {
namespace {

const std::vector<std::string>& GetReplacementTokens() {
    static const std::vector<std::string> tokens = {
        // Keywords / Types
        "fn", "var", "let", "const", "override", "alias", "struct", "if", "else", "switch", "case",
        "default", "for", "while", "loop", "break", "continue", "continuing", "return", "discard",
        "const_assert", "enable", "requires", "diagnostic", "u32", "i32", "f32", "f16", "bool",
        "vec2", "vec3", "vec4", "mat2x2", "mat3x3", "mat4x4", "ptr", "array", "atomic", "sampler",
        "sampler_comparison", "texture_1d", "texture_2d", "texture_3d", "texture_cube",
        "texture_storage_2d", "texture_depth_2d",

        // Built-in Functions / Decorators
        "abs", "acos", "asin", "atan", "atan2", "ceil", "clamp", "cos", "cosh", "cross", "degrees",
        "distance", "dot", "exp", "exp2", "floor", "fma", "fract", "ldexp", "length", "log", "log2",
        "max", "min", "mix", "normalize", "pow", "radians", "reflect", "refract", "round",
        "saturate", "sign", "sin", "sinh", "smoothstep", "sqrt", "step", "tan", "tanh", "transpose",
        "trunc", "select", "all", "any", "bitcast", "workgroupBarrier", "storageBarrier",
        "textureSample", "textureLoad", "textureStore",

        // Operators & Punctuations
        "=", "==", "!=", "<", ">", "<=", ">=", "+", "-", "*", "/", "%", "&&", "||", "!", "&", "|",
        "^", "~", "<<", ">>", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=", "++",
        "--", ".", ",", ":", ";", "->", "@"};
    return tokens;
}

void CollectLeafNodes(Node* node, std::vector<Node*>& list) {
    if (!node) {
        return;
    }
    if (node->is_leaf) {
        list.push_back(node);
    }
    for (auto* child : node->children) {
        CollectLeafNodes(child, list);
    }
}

/// ReplaceTokenMutator is a CST-level mutation that modifies specific leaf tokens
/// (such as keywords, operators, or literals) using interesting WGSL vocabularies.
///
/// It works by:
/// 1. Scans the CST and gathers all leaf nodes containing textual content.
/// 2. Randomly selects one leaf node (e.g. an operator like `+`, a literal like `5`,
///    or a type name like `f32`).
/// 3. Replaces the target token with a random keyword, builtin, type, or operator from
///    the WGSL dictionary to test compiler diagnostics, lexical recovery, and error tolerance.
class ReplaceTokenMutator : public Mutator {
  public:
    std::string Name() const override { return "ReplaceTokenMutator"; }

    bool Mutate(Node* root, unsigned int seed) override {
        std::vector<Node*> candidates;
        CollectLeafNodes(root, candidates);

        if (candidates.empty()) {
            return false;
        }

        std::mt19937 rng(seed);
        std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
        auto* leaf = candidates[dist(rng)];

        const auto& replacements = GetReplacementTokens();
        std::uniform_int_distribution<size_t> repl_dist(0, replacements.size() - 1);
        const std::string& replacement = replacements[repl_dist(rng)];

        leaf->token.type = tint::wgsl::reader::Token::Type::kIdentifier;
        leaf->token.value = replacement;
        return true;
    }
};

}  // namespace
}  // namespace tint::fuzz::wgsl::cst

TINT_CST_MUTATOR(tint::fuzz::wgsl::cst::ReplaceTokenMutator);

#endif  // TINT_BUILD_WGSL_READER
