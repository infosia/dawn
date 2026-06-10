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

void CollectAllNodes(Node* node, std::vector<Node*>& list) {
    if (!node) {
        return;
    }
    list.push_back(node);
    for (auto* child : node->children) {
        CollectAllNodes(child, list);
    }
}

/// DeleteMutator is a CST-level mutation that prunes structural components
/// from the WGSL Concrete Syntax Tree.
///
/// It works by:
/// 1. Gathers all non-root nodes across the parsed CST tree.
/// 2. Selects a random eligible target node (which can either be a single leaf token
///    or an entire matched bracket group like a block or parameter list).
/// 3. Safely removes the target node from its parent's children vector and deletes
///    the subtree to reduce syntax verbosity and test parser/validator robustness.
class DeleteMutator : public Mutator {
  public:
    std::string Name() const override { return "DeleteMutator"; }

    bool Mutate(Node* root, unsigned int seed) override {
        std::vector<Node*> all_nodes;
        CollectAllNodes(root, all_nodes);

        std::vector<Node*> candidates;
        for (auto* node : all_nodes) {
            if (node != root && node->parent != nullptr) {
                candidates.push_back(node);
            }
        }

        if (candidates.empty()) {
            return false;
        }

        std::mt19937 rng(seed);
        std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
        auto* to_delete = candidates[dist(rng)];

        auto* parent = to_delete->parent;
        auto& sibs = parent->children;
        auto it = std::find(sibs.begin(), sibs.end(), to_delete);
        if (it != sibs.end()) {
            sibs.erase(it);
            delete to_delete;
            return true;
        }

        return false;
    }
};

}  // namespace
}  // namespace tint::fuzz::wgsl::cst

TINT_CST_MUTATOR(tint::fuzz::wgsl::cst::DeleteMutator);

#endif  // TINT_BUILD_WGSL_READER
