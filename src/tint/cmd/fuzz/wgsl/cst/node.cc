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

#include "src/tint/cmd/fuzz/wgsl/cst/node.h"

#include <algorithm>

namespace tint::fuzz::wgsl::cst {

Node::Node() = default;

Node::~Node() {
    for (auto* child : children) {
        delete child;
    }
}

Node::Node(const Node& other)
    : is_leaf(other.is_leaf),
      token(other.token),
      type_open(other.type_open),
      type_close(other.type_close),
      parent(nullptr) {
    children.reserve(other.children.size());
    for (const auto* child : other.children) {
        auto* child_copy = new Node(*child);
        child_copy->parent = this;
        children.push_back(child_copy);
    }
}

Node& Node::operator=(const Node& other) {
    if (this != &other) {
        is_leaf = other.is_leaf;
        token = other.token;
        type_open = other.type_open;
        type_close = other.type_close;
        parent = nullptr;
        for (auto* child : children) {
            delete child;
        }
        children.clear();
        children.reserve(other.children.size());
        for (const auto* child : other.children) {
            auto* child_copy = new Node(*child);
            child_copy->parent = this;
            children.push_back(child_copy);
        }
    }
    return *this;
}

}  // namespace tint::fuzz::wgsl::cst
