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

/// SpliceMutator is a CST-level mutation that performs structural crossover
/// (splicing) on matched bracket groups of the same category.
///
/// It works by:
/// 1. Finds a random matched bracket group node `A` (such as a curly brace block or a
///    parenthesized group) in the current program's CST.
/// 2. Asks the node cache singleton (`NodeCache`) for a random, pre-serialized subtree
///    string that has the exact same opening bracket type as `A`.
/// 3. Parses this serialized string into a temporary CST subtree.
/// 4. Deep-copies the matching parsed group node and replaces `A` with it, allowing
///    the fuzzer to transplant syntax-valid blocks, loops, or declarations between different seed
///    files.
class SpliceMutator : public Mutator {
  public:
    std::string Name() const override { return "SpliceMutator"; }

    bool Mutate(Node* root, unsigned int seed) override {
        std::vector<Node*> groups;
        CollectBracketGroups(root, groups);

        if (groups.empty()) {
            return false;
        }

        std::mt19937 rng(seed);
        std::uniform_int_distribution<size_t> dist(0, groups.size() - 1);
        auto* target = groups[dist(rng)];

        auto* parent = target->parent;
        if (!parent) {
            return false;
        }

        std::string cached_str = NodeCache::Instance().GetRandom(target->type_open, rng);
        if (cached_str.empty()) {
            return false;
        }

        auto* parsed_root = Parse(cached_str);
        if (!parsed_root) {
            return false;
        }

        Node* source_group = nullptr;
        for (auto* child : parsed_root->children) {
            if (!child->is_leaf && child->type_open == target->type_open) {
                source_group = child;
                break;
            }
        }

        if (!source_group) {
            delete parsed_root;
            return false;
        }

        auto* replacement = new Node(*source_group);
        replacement->parent = parent;

        auto it = std::find(parent->children.begin(), parent->children.end(), target);
        if (it != parent->children.end()) {
            *it = replacement;
            delete target;
            delete parsed_root;
            return true;
        }

        delete replacement;
        delete parsed_root;
        return false;
    }
};

}  // namespace
}  // namespace tint::fuzz::wgsl::cst

TINT_CST_MUTATOR(tint::fuzz::wgsl::cst::SpliceMutator);

#endif  // TINT_BUILD_WGSL_READER
