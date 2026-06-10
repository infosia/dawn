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

#include "src/tint/cmd/fuzz/wgsl/cst/node_cache.h"

#if TINT_BUILD_WGSL_READER

namespace tint::fuzz::wgsl::cst {

NodeCache& NodeCache::Instance() {
    static NodeCache instance;
    return instance;
}

void NodeCache::Insert(BracketType type, const std::string& subtree_str) {
    std::lock_guard<std::mutex> lock(mutex);
    auto& q = queues[type];
    q.push_back(subtree_str);
    if (q.size() > kMaxQueueSize) {
        q.pop_front();
    }
}

std::string NodeCache::GetRandom(BracketType type, std::mt19937& rng) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = queues.find(type);
    if (it == queues.end() || it->second.empty()) {
        return "";
    }
    const auto& q = it->second;
    std::uniform_int_distribution<size_t> dist(0, q.size() - 1);
    return q[dist(rng)];
}

}  // namespace tint::fuzz::wgsl::cst

#endif  // TINT_BUILD_WGSL_READER
