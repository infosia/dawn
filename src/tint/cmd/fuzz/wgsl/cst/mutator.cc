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

#include <algorithm>
#include <new>
#include <random>
#include <utility>

#include "src/tint/lang/wgsl/reader/parser/lexer.h"
#include "src/tint/utils/diagnostic/source.h"

extern "C" size_t LLVMFuzzerMutate(uint8_t* data, size_t size, size_t max_size);

namespace tint::fuzz::wgsl::cst {

MutatorRegistry::MutatorRegistry() = default;
MutatorRegistry::~MutatorRegistry() = default;

MutatorRegistry& MutatorRegistry::Instance() {
    static MutatorRegistry instance;
    return instance;
}

void MutatorRegistry::Register(std::unique_ptr<Mutator> mutator) {
    mutators_.push_back(std::move(mutator));
}

const std::vector<std::unique_ptr<Mutator>>& MutatorRegistry::Mutators() const {
    return mutators_;
}

namespace {

bool IsOpenBracket(tint::wgsl::reader::Token::Type t) {
    return t == tint::wgsl::reader::Token::Type::kBraceLeft ||
           t == tint::wgsl::reader::Token::Type::kParenLeft ||
           t == tint::wgsl::reader::Token::Type::kBracketLeft ||
           t == tint::wgsl::reader::Token::Type::kTemplateArgsLeft;
}

bool IsCloseBracket(tint::wgsl::reader::Token::Type t) {
    return t == tint::wgsl::reader::Token::Type::kBraceRight ||
           t == tint::wgsl::reader::Token::Type::kParenRight ||
           t == tint::wgsl::reader::Token::Type::kBracketRight ||
           t == tint::wgsl::reader::Token::Type::kTemplateArgsRight;
}

bool BracketsMatch(tint::wgsl::reader::Token::Type open, tint::wgsl::reader::Token::Type close) {
    if (open == tint::wgsl::reader::Token::Type::kBraceLeft) {
        return close == tint::wgsl::reader::Token::Type::kBraceRight;
    }
    if (open == tint::wgsl::reader::Token::Type::kParenLeft) {
        return close == tint::wgsl::reader::Token::Type::kParenRight;
    }
    if (open == tint::wgsl::reader::Token::Type::kBracketLeft) {
        return close == tint::wgsl::reader::Token::Type::kBracketRight;
    }
    if (open == tint::wgsl::reader::Token::Type::kTemplateArgsLeft) {
        return close == tint::wgsl::reader::Token::Type::kTemplateArgsRight;
    }
    return false;
}

cst::Token ToCSTToken(const tint::wgsl::reader::Token& token) {
    cst::Token out;
    out.type = token.type();
    out.source = token.source();

    // Resolve literal text representation immediately during conversion
    if (token.IsIdentifier() || token.IsLiteral() || token.IsError()) {
        out.value = token.to_str();
    } else {
        auto type = token.type();
        if (type == tint::wgsl::reader::Token::Type::kTemplateArgsLeft) {
            out.value = "<";
        } else if (type == tint::wgsl::reader::Token::Type::kTemplateArgsRight) {
            out.value = ">";
        } else {
            out.value = std::string(tint::wgsl::reader::Token::TypeToName(type));
        }
    }
    return out;
}

bool RequiresSpaceBetween(tint::wgsl::reader::Token::Type a, tint::wgsl::reader::Token::Type b) {
    auto is_word_or_num = [](tint::wgsl::reader::Token::Type t) {
        return t == tint::wgsl::reader::Token::Type::kIdentifier ||
               t == tint::wgsl::reader::Token::Type::kFloatLiteral ||
               t == tint::wgsl::reader::Token::Type::kFloatLiteral_F ||
               t == tint::wgsl::reader::Token::Type::kFloatLiteral_H ||
               t == tint::wgsl::reader::Token::Type::kIntLiteral ||
               t == tint::wgsl::reader::Token::Type::kIntLiteral_I ||
               t == tint::wgsl::reader::Token::Type::kIntLiteral_U ||
               (t >= tint::wgsl::reader::Token::Type::kAlias &&
                t <= tint::wgsl::reader::Token::Type::kWhile);
    };
    return is_word_or_num(a) && is_word_or_num(b);
}

void SerializeNode(const Node* node, std::string& out, tint::wgsl::reader::Token::Type& last_type) {
    if (node->is_leaf) {
        tint::wgsl::reader::Token::Type cur_type = node->token.type;
        if (RequiresSpaceBetween(last_type, cur_type)) {
            out += " ";
        }
        out += node->token.value;
        last_type = cur_type;
        return;
    }

    if (node->type_open != tint::wgsl::reader::Token::Type::kUninitialized) {
        if (RequiresSpaceBetween(last_type, node->type_open)) {
            out += " ";
        }
        out += ToCSTToken(tint::wgsl::reader::Token(node->type_open, {})).value;
        last_type = node->type_open;
    }

    for (const auto* child : node->children) {
        SerializeNode(child, out, last_type);
    }

    if (node->type_close != tint::wgsl::reader::Token::Type::kUninitialized) {
        if (RequiresSpaceBetween(last_type, node->type_close)) {
            out += " ";
        }
        out += ToCSTToken(tint::wgsl::reader::Token(node->type_close, {})).value;
        last_type = node->type_close;
    }
}

void WalkAndCache(const Node* node) {
    if (!node->is_leaf && node->type_open != tint::wgsl::reader::Token::Type::kUninitialized) {
        std::string subtree_str = Serialize(node);
        if (!subtree_str.empty()) {
            NodeCache::Instance().Insert(node->type_open, subtree_str);
        }
    }
    for (const auto* child : node->children) {
        WalkAndCache(child);
    }
}

}  // namespace

Node* Parse(std::string_view source) {
    tint::Source::File file("test.wgsl", std::string(source));
    tint::wgsl::reader::Lexer lexer(&file);
    auto tokens = lexer.Lex();

    auto* root = new Node();
    Node* current = root;

    for (const auto& token : tokens) {
        auto type = token.type();
        if (type == tint::wgsl::reader::Token::Type::kEOF ||
            type == tint::wgsl::reader::Token::Type::kUninitialized) {
            continue;
        }

        if (IsOpenBracket(type)) {
            auto* group = new Node();
            group->type_open = type;
            group->parent = current;
            current->children.push_back(group);
            current = group;
        } else if (IsCloseBracket(type)) {
            if (current->parent && BracketsMatch(current->type_open, type)) {
                current->type_close = type;
                current = current->parent;
            } else {
                // Mismatched bracket, parse as a leaf to stay completely robust
                auto* leaf = new Node();
                leaf->is_leaf = true;
                leaf->token = ToCSTToken(token);
                leaf->parent = current;
                current->children.push_back(leaf);
            }
        } else {
            auto* leaf = new Node();
            leaf->is_leaf = true;
            leaf->token = ToCSTToken(token);
            leaf->parent = current;
            current->children.push_back(leaf);
        }
    }

    return root;
}

std::string Serialize(const Node* root) {
    std::string out;
    tint::wgsl::reader::Token::Type last_type = tint::wgsl::reader::Token::Type::kUninitialized;
    SerializeNode(root, out, last_type);
    return out;
}

void PopulateCache(std::string_view wgsl) {
    Node* root = Parse(wgsl);
    if (root) {
        WalkAndCache(root);
        delete root;
    }
}

size_t CustomMutate(uint8_t* data, size_t size, size_t max_size, unsigned int seed) {
    const auto& mutators = MutatorRegistry::Instance().Mutators();
    size_t num_custom = mutators.size();

    // Choose 1 of (num_custom + 1) mutators. (The last one represents the default mutator)
    std::mt19937 rng(seed);
    std::uniform_int_distribution<size_t> dist(0, num_custom);
    size_t chosen_idx = dist(rng);

    if (chosen_idx < num_custom) {
        // Attempt custom mutation
        auto* root = Parse(std::string_view(reinterpret_cast<const char*>(data), size));
        if (root) {
            bool success = mutators[chosen_idx]->Mutate(root, rng());
            if (success) {
                std::string mutated = Serialize(root);
                delete root;
                if (mutated.size() <= max_size) {
                    std::copy(mutated.begin(), mutated.end(), data);
                    return mutated.size();
                }
            } else {
                delete root;
            }
        }
    }

    // Fallback/Default Mutator (either chosen_idx == num_custom, or custom mutation failed)
    return LLVMFuzzerMutate(data, size, max_size);
}

}  // namespace tint::fuzz::wgsl::cst

#endif  // TINT_BUILD_WGSL_READER
