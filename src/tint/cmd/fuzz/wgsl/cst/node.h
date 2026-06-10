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

#ifndef SRC_TINT_CMD_FUZZ_WGSL_CST_NODE_H_
#define SRC_TINT_CMD_FUZZ_WGSL_CST_NODE_H_

#include "src/tint/utils/macros/compiler.h"

#if TINT_BUILD_WGSL_READER

#include <string>
#include <vector>

#include "src/tint/lang/wgsl/reader/parser/token.h"

namespace tint::fuzz::wgsl::cst {

/// Represents a fully copyable, lightweight Token type inside the CST
struct Token {
    /// The Tint token type
    tint::wgsl::reader::Token::Type type = tint::wgsl::reader::Token::Type::kUninitialized;
    /// Source position
    tint::Source source;
    /// Textual value representation of the token
    std::string value;

    /// Helper to check if the token is an identifier
    bool IsIdentifier() const { return type == tint::wgsl::reader::Token::Type::kIdentifier; }
};

/// Represents a node in our lightweight bracket-matched Concrete Syntax Tree (CST)
struct Node {
    /// True if the node is a single leaf token, false if it is a matched bracket group
    bool is_leaf = false;

    /// The token representing this node (valid if is_leaf is true)
    Token token;

    /// The opening bracket type if this is a group node (e.g. kBraceLeft)
    tint::wgsl::reader::Token::Type type_open = tint::wgsl::reader::Token::Type::kUninitialized;

    /// The closing bracket type if this is a group node (e.g. kBraceRight)
    tint::wgsl::reader::Token::Type type_close = tint::wgsl::reader::Token::Type::kUninitialized;

    /// List of child nodes under this group
    std::vector<Node*> children;

    /// Pointer to the parent node
    Node* parent = nullptr;

    /// Constructor
    Node();

    /// Destructor
    ~Node();

    /// Copy constructor (performs a deep copy of the CST subtree)
    Node(const Node& other);

    /// Assignment operator (deep copy)
    Node& operator=(const Node& other);
};

}  // namespace tint::fuzz::wgsl::cst

#endif  // TINT_BUILD_WGSL_READER

#endif  // SRC_TINT_CMD_FUZZ_WGSL_CST_NODE_H_
