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

void CollectBracketGroups(Node* node, std::vector<Node*>& list) {
    if (!node) {
        return;
    }
    if (!node->is_leaf && node->type_open != tint::wgsl::reader::Token::Type::kUninitialized) {
        list.push_back(node);
    }
    for (auto* child : node->children) {
        CollectBracketGroups(child, list);
    }
}

/// DeepenMutator is a CST-level mutation that increases syntactic complexity by
/// recursively nesting matched-bracket structures.
///
/// It works by:
/// 1. Finds a random matched bracket group node `A` (such as a block statement `{ ... }`)
///    in the program.
/// 2. Gathers all nested bracket group nodes inside `A` (including `A` itself).
/// 3. Selects a target nested group `B` within that set.
/// 4. Generates a deep clone of `A` and recursively inserts it into a random child position
///    under `B`, introducing deeper layers of matching-bracket structures to test stack limits and
///    recursive parser handling.
class DeepenMutator : public Mutator {
  public:
    std::string Name() const override { return "DeepenMutator"; }

    bool Mutate(Node* root, unsigned int seed) override {
        std::vector<Node*> groups;
        CollectBracketGroups(root, groups);

        if (groups.empty()) {
            return false;
        }

        std::mt19937 rng(seed);
        std::uniform_int_distribution<size_t> dist(0, groups.size() - 1);
        auto* node_a = groups[dist(rng)];

        // Find all matched bracket groups inside node_a (including node_a itself)
        std::vector<Node*> sub_groups;
        CollectBracketGroups(node_a, sub_groups);

        if (sub_groups.empty()) {
            return false;
        }

        std::uniform_int_distribution<size_t> sub_dist(0, sub_groups.size() - 1);
        auto* target_group = sub_groups[sub_dist(rng)];

        // Clone node_a (deep copy)
        auto* copy_a = new Node(*node_a);
        copy_a->parent = target_group;

        // Insert the cloned node_a inside the target_group's children list at a random position
        std::uniform_int_distribution<size_t> pos_dist(0, target_group->children.size());
        size_t insert_pos = pos_dist(rng);

        target_group->children.insert(target_group->children.begin() + insert_pos, copy_a);
        return true;
    }
};

}  // namespace
}  // namespace tint::fuzz::wgsl::cst

TINT_CST_MUTATOR(tint::fuzz::wgsl::cst::DeepenMutator);

#endif  // TINT_BUILD_WGSL_READER
