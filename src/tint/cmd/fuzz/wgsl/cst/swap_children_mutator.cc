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

void CollectNonLeafNodes(Node* node, std::vector<Node*>& list) {
    if (!node) {
        return;
    }
    if (!node->is_leaf && node->children.size() >= 2) {
        list.push_back(node);
    }
    for (auto* child : node->children) {
        CollectNonLeafNodes(child, list);
    }
}

/// SwapChildrenMutator is a CST-level mutation that introduces structural and logical
/// reordering inside bracketed groups.
///
/// It works by:
/// 1. Walks the tree to find group nodes (non-leaf matched-bracket groups) that contain
///    two or more immediate child nodes.
/// 2. Randomly selects one such group (e.g. a curly-braced block containing multiple
///    statements, or a parenthesized function parameter list).
/// 3. Randomly selects two children within this group and swaps their order to test
///    control flow out-of-order execution, parsing behavior, and semantic validation.
class SwapChildrenMutator : public Mutator {
  public:
    std::string Name() const override { return "SwapChildrenMutator"; }

    bool Mutate(Node* root, unsigned int seed) override {
        std::vector<Node*> candidates;
        CollectNonLeafNodes(root, candidates);

        if (candidates.empty()) {
            return false;
        }

        std::mt19937 rng(seed);
        std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
        auto* parent = candidates[dist(rng)];

        std::uniform_int_distribution<size_t> child_dist(0, parent->children.size() - 1);
        size_t first = child_dist(rng);
        size_t second = child_dist(rng);
        while (first == second) {
            second = child_dist(rng);
        }

        std::swap(parent->children[first], parent->children[second]);
        return true;
    }
};

}  // namespace
}  // namespace tint::fuzz::wgsl::cst

TINT_CST_MUTATOR(tint::fuzz::wgsl::cst::SwapChildrenMutator);

#endif  // TINT_BUILD_WGSL_READER
