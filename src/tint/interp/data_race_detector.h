// Copyright 2025 The Dawn & Tint Authors
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

#ifndef SRC_TINT_INTERP_DATA_RACE_DETECTOR_H_
#define SRC_TINT_INTERP_DATA_RACE_DETECTOR_H_

#include <unordered_map>
#include <vector>

#include "src/tint/interp/uvec3.h"
#include "src/tint/lang/core/enums.h"
#include "src/tint/utils/diagnostic/source.h"
#include "src/tint/utils/math/hash.h"

// Forward Declarations
namespace tint::ast {
class CallExpression;
class Node;
}  // namespace tint::ast
namespace tint::interp {
class Invocation;
class MemoryView;
class ShaderExecutor;
class Workgroup;
}  // namespace tint::interp

namespace tint::interp {

/// The DataRaceDetector class implements a dynamic data race detector that works by tracking memory
/// accesses and checking for conflicts at synchronization points.
class DataRaceDetector {
  public:
    /// Construct a data race detector for a given shader executor.
    /// @param executor the shader executor to attach to
    explicit DataRaceDetector(ShaderExecutor& executor);

  private:
    ShaderExecutor& executor_;

    // Handlers for events that occur during shader execution.
    void Barrier(const Workgroup* workgroup, const ast::CallExpression* call);
    void MemoryLoad(const MemoryView* view);
    void MemoryStore(const MemoryView* view);
    void WorkgroupBegin(const Workgroup* workgroup);
    void WorkgroupComplete(const Workgroup* workgroup);

    /// MemoryAccessKind is used to distinguish between load and store operations.
    enum class MemoryAccessKind { kLoad, kStore };

    /// MemoryAccess represents a memory access to a single byte of memory.
    struct MemoryAccess {
        UVec3 workgroup;         // The workgroup's ID.
        UVec3 invocation;        // The invocation's local ID.
        const ast::Node* cause;  // The AST node that caused the access.
        MemoryAccessKind kind;   // The kind of access (load vs store).
        uint64_t byte_offset;    // The offset in bytes from the root of the memory allocation.
        uint64_t byte_size;      // The size in bytes of the access.

        // Flag to determine if this is a write to a single component of a vector.
        bool is_vector_component_write = false;
    };

    /// MemoryAccessKey is a key used to determine the uniqueness of a memory access location.
    struct MemoryAccessKey {
        const MemoryView* view;  // The root memory view.
        uint64_t offset;         // The offset in bytes from the start of the allocation.
        bool operator==(const MemoryAccessKey& rhs) const {
            return view == rhs.view && offset == rhs.offset;
        }
    };

    /// Helper struct to allow MemoryAccessKey to be used as the key in an unordered_map.
    struct MemoryAccessHasher {
        size_t operator()(const MemoryAccessKey& key) const {
            return tint::Hash(key.view, key.offset);
        }
    };
    using MemoryAccessMap = std::unordered_map<MemoryAccessKey, MemoryAccess, MemoryAccessHasher>;

    /// Register a memory access for an invocation.
    /// @param view the memory view being accessed
    /// @param kind the kind of access being performed
    void RegisterAccess(const MemoryView* view, MemoryAccessKind kind);

    /// Merge a memory access into an access map, optionally check for data races.
    /// @param access_key the memory access key
    /// @param access the memory access to merge
    /// @param access_map the memory access map
    /// @param check_for_races `true` to check for memory races when merging
    void MergeAccess(MemoryAccessKey access_key,
                     MemoryAccess access,
                     MemoryAccessMap& access_map,
                     bool check_for_races);

    /// Synchronize memory accesses within a workgroup.
    /// @param workgroup the workgroup to synchronize accesses for
    /// @param addrspace the address space to synchronize
    void SyncWorkgroup(const Workgroup* workgroup, core::AddressSpace addrspace);

    /// InvocationAccesses contains access maps for workgroup and storage buffer memory.
    struct InvocationAccesses {
        MemoryAccessMap workgroup_accesses_;
        MemoryAccessMap storage_accesses_;
    };

    // Access maps that record memory accesses at various scopes.
    std::unordered_map<UVec3, std::vector<InvocationAccesses>> per_invocation_accesses_;
    std::unordered_map<UVec3, MemoryAccessMap> per_group_storage_buffer_accesses_;
    MemoryAccessMap inter_group_storage_buffer_accesses_;

    /// Race represents a data race, and tracks whether or not it has been emitted yet.
    struct Race {
        const MemoryView* root_view;  // The root memory view.
        MemoryAccess a;               // The first access.
        MemoryAccess b;               // The second access.
        bool emitted = false;         // Tracks whether or not the race has been emitted.
    };

    /// RaceKey is a key used to determine the uniqueness of data race.
    struct RaceKey {
        Source::Location decl;
        const ast::Node* cause_a;
        const ast::Node* cause_b;
        bool operator==(const RaceKey& rhs) const {
            return decl == rhs.decl && cause_a == rhs.cause_a && cause_b == rhs.cause_b;
        }
    };

    /// Helper struct to allow RaceKey to be used as the key in an unordered_map.
    struct RaceHasher {
        size_t operator()(const RaceKey& key) const {
            return tint::Hash(key.decl.line, key.decl.column, key.cause_a, key.cause_b);
        }
    };

    /// Record a race to be emitted later.
    /// @param root_view the memory view that represents the variable being accessed
    /// @param a the first memory access
    /// @param b the second memory access
    void RecordRace(const MemoryView* root_view, MemoryAccess a, MemoryAccess b);

    /// Emit any races that have been recorded.
    void EmitRaces();

    /// The data races that have been recorded by this DataRaceDetector.
    std::unordered_map<RaceKey, Race, RaceHasher> races_;
};

}  // namespace tint::interp

#endif  // SRC_TINT_INTERP_DATA_RACE_DETECTOR_H_
