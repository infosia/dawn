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

#ifndef SRC_TINT_CMD_FUZZ_WGSL_CST_NODE_CACHE_H_
#define SRC_TINT_CMD_FUZZ_WGSL_CST_NODE_CACHE_H_

#include "src/tint/utils/macros/compiler.h"

#if TINT_BUILD_WGSL_READER

#include <deque>
#include <mutex>
#include <random>
#include <string>
#include <unordered_map>

#include "src/tint/lang/wgsl/reader/parser/token.h"

namespace tint::fuzz::wgsl::cst {

using BracketType = tint::wgsl::reader::Token::Type;

/// Thread-safe, rolling-window cache of serialized CST subtrees categorized by opening bracket type
struct NodeCache {
    /// Mutex for thread safety
    std::mutex mutex;

    /// Maps opening bracket type to a rolling window (queue) of serialized subtree strings
    std::unordered_map<BracketType, std::deque<std::string>> queues;

    /// Maximum capacity of each bracket type's queue
    static constexpr size_t kMaxQueueSize = 500;

    /// @returns the static singleton instance
    static NodeCache& Instance();

    /// Inserts a new serialized subtree into the rolling-window cache
    /// @param type the opening bracket type
    /// @param subtree_str the serialized subtree string
    void Insert(BracketType type, const std::string& subtree_str);

    /// Retrieves a random serialized subtree from the cache of the matching bracket type
    /// @param type the opening bracket type
    /// @param rng the random engine to select with
    /// @returns a serialized subtree string, or empty if none available
    std::string GetRandom(BracketType type, std::mt19937& rng);
};

}  // namespace tint::fuzz::wgsl::cst

#endif  // TINT_BUILD_WGSL_READER

#endif  // SRC_TINT_CMD_FUZZ_WGSL_CST_NODE_CACHE_H_
