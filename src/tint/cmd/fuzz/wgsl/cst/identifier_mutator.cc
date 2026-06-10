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

void CollectIdentifiers(Node* node, std::vector<Node*>& list) {
    if (!node) {
        return;
    }
    if (node->is_leaf && node->token.IsIdentifier()) {
        list.push_back(node);
    }
    for (auto* child : node->children) {
        CollectIdentifiers(child, list);
    }
}

/// IdentifierMutator is a CST-level mutation that test variable scoping, declaration
/// conflicts, and identifier shadowing.
///
/// It works by:
/// 1. Locates all leaf nodes that represent identifiers (e.g. variable names, struct names,
///    function names) in the input WGSL CST.
/// 2. Randomly selects two distinct identifiers `A` and `B` from across the program.
/// 3. Replaces identifier `A`'s text string with `B`'s text string, effectively triggering
///    variable shadowing, identifier reuse, or reference re-routing.
class IdentifierMutator : public Mutator {
  public:
    std::string Name() const override { return "IdentifierMutator"; }

    bool Mutate(Node* root, unsigned int seed) override {
        std::vector<Node*> idents;
        CollectIdentifiers(root, idents);

        if (idents.size() < 2) {
            return false;
        }

        std::mt19937 rng(seed);
        std::uniform_int_distribution<size_t> dist(0, idents.size() - 1);
        size_t first = dist(rng);
        size_t second = dist(rng);
        while (first == second) {
            second = dist(rng);
        }

        auto* node_a = idents[first];
        auto* node_b = idents[second];

        node_a->token.value = node_b->token.value;
        return true;
    }
};

}  // namespace
}  // namespace tint::fuzz::wgsl::cst

TINT_CST_MUTATOR(tint::fuzz::wgsl::cst::IdentifierMutator);

#endif  // TINT_BUILD_WGSL_READER
