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

#ifndef SRC_TINT_CMD_FUZZ_WGSL_CST_MUTATOR_H_
#define SRC_TINT_CMD_FUZZ_WGSL_CST_MUTATOR_H_

#include "src/tint/utils/macros/compiler.h"

#if TINT_BUILD_WGSL_READER

#include <memory>
#include <random>
#include <string>
#include <vector>

#include "src/tint/cmd/fuzz/wgsl/cst/node.h"
#include "src/tint/cmd/fuzz/wgsl/cst/node_cache.h"
#include "src/tint/utils/macros/static_init.h"

namespace tint::fuzz::wgsl::cst {

/// Base class for modular CST-level mutators
class Mutator {
  public:
    virtual ~Mutator() = default;

    /// @returns the name of this mutator
    virtual std::string Name() const = 0;

    /// Mutates the CST subtree starting from the root
    /// @param root the root node of the CST
    /// @param seed random seed for the mutation
    /// @returns true if mutation was successfully applied, false otherwise
    virtual bool Mutate(Node* root, unsigned int seed) = 0;
};

/// Registry managing the registered Mutators
class MutatorRegistry {
  public:
    /// @returns the static singleton instance
    static MutatorRegistry& Instance();

    /// Registers a new mutator
    /// @param mutator the mutator to register
    void Register(std::unique_ptr<Mutator> mutator);

    /// @returns the list of registered mutators
    const std::vector<std::unique_ptr<Mutator>>& Mutators() const;

  private:
    MutatorRegistry();
    ~MutatorRegistry();

    std::vector<std::unique_ptr<Mutator>> mutators_;
};

/// Parses a WGSL source string into a resilient bracket-matching CST
/// @param source the input WGSL source code
/// @returns the root Node of the parsed tree, or nullptr on failure
Node* Parse(std::string_view source);

/// Recursively serializes the CST node tree back to a WGSL string, dynamically inserting spacing
/// @param root the root node of the tree
/// @returns the serialized WGSL source string
std::string Serialize(const Node* root);

/// Parses the input WGSL string, walks its CST, and inserts subtrees into the cache
/// @param wgsl the input WGSL source code
void PopulateCache(std::string_view wgsl);

/// Performs the 1-of-7 custom mutation logic on input data, falling back to LLVMFuzzerMutate
/// @param data the data buffer to mutate
/// @param size the size of the data
/// @param max_size the maximum allowed size of the mutated data
/// @param seed random seed for selection and mutations
/// @returns the size of the mutated data
size_t CustomMutate(uint8_t* data, size_t size, size_t max_size, unsigned int seed);

}  // namespace tint::fuzz::wgsl::cst

/// Macro to register a custom Mutator class dynamically at static initialization time
#define TINT_CST_MUTATOR(CLASS) \
    TINT_STATIC_INIT(           \
        ::tint::fuzz::wgsl::cst::MutatorRegistry::Instance().Register(std::make_unique<CLASS>()))

#endif  // TINT_BUILD_WGSL_READER

#endif  // SRC_TINT_CMD_FUZZ_WGSL_CST_MUTATOR_H_
