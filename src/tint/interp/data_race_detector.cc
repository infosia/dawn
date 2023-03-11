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

#include "tint/interp/data_race_detector.h"

#include <utility>

#include "tint/interp/invocation.h"
#include "tint/interp/memory.h"
#include "tint/interp/shader_executor.h"
#include "tint/interp/workgroup.h"
#include "tint/lang/wgsl/sem/builtin_fn.h"
#include "tint/lang/wgsl/sem/call.h"

namespace tint::interp {

DataRaceDetector::DataRaceDetector(ShaderExecutor& executor) : executor_(executor) {
    executor_.AddBarrierCallback([&](const Workgroup* workgroup, const ast::CallExpression* call) {
        Barrier(workgroup, call);
    });
    executor_.AddMemoryLoadCallback([&](const MemoryView* view) { MemoryLoad(view); });
    executor_.AddMemoryStoreCallback([&](const MemoryView* view) { MemoryStore(view); });
    executor_.AddWorkgroupBeginCallback(
        [&](const Workgroup* workgroup) { WorkgroupBegin(workgroup); });
    executor_.AddWorkgroupCompleteCallback(
        [&](const Workgroup* workgroup) { WorkgroupComplete(workgroup); });
}

void DataRaceDetector::Barrier(const Workgroup* workgroup, const ast::CallExpression* call) {
    auto* builtin = executor_.Sem().Get<sem::Call>(call)->Target()->As<sem::BuiltinFn>();
    TINT_ASSERT(builtin != nullptr);

    // Synchronize accesses within the group according to the memory semantics of the barrier.
    switch (builtin->Fn()) {
        case wgsl::BuiltinFn::kStorageBarrier:
            SyncWorkgroup(workgroup, core::AddressSpace::kStorage);
            break;
        case wgsl::BuiltinFn::kWorkgroupBarrier:
        case wgsl::BuiltinFn::kWorkgroupUniformLoad:
            SyncWorkgroup(workgroup, core::AddressSpace::kWorkgroup);
            break;
        default:
            TINT_ASSERT(false && "unhandled barrier builtin");
            break;
    }
}

void DataRaceDetector::MemoryLoad(const MemoryView* view) {
    RegisterAccess(view, MemoryAccessKind::kLoad);
}

void DataRaceDetector::MemoryStore(const MemoryView* view) {
    RegisterAccess(view, MemoryAccessKind::kStore);
}

void DataRaceDetector::WorkgroupBegin(const Workgroup* workgroup) {
    // Allocate space to start accesses for this workgroup's invocations.
    auto wgsize = workgroup->Size();
    per_invocation_accesses_[workgroup->GroupId()].resize(wgsize.x * wgsize.y * wgsize.z);
}

void DataRaceDetector::WorkgroupComplete(const Workgroup* workgroup) {
    // Synchronize accesses for both address spaces.
    SyncWorkgroup(workgroup, core::AddressSpace::kWorkgroup);
    SyncWorkgroup(workgroup, core::AddressSpace::kStorage);

    // Merge storage buffer accesses into the inter-group map.
    for (auto& access : per_group_storage_buffer_accesses_[workgroup->GroupId()]) {
        MergeAccess(access.first, access.second, inter_group_storage_buffer_accesses_, true);
    }

    // Emit any data races that have been recorded so far.
    EmitRaces();

    // Clear the access maps for this workgroup.
    per_invocation_accesses_.erase(workgroup->GroupId());
    per_group_storage_buffer_accesses_.erase(workgroup->GroupId());
}

void DataRaceDetector::RegisterAccess(const MemoryView* view, MemoryAccessKind kind) {
    auto addrspace = view->AddressSpace();
    if (addrspace != core::AddressSpace::kStorage && addrspace != core::AddressSpace::kWorkgroup) {
        // We only care about shared resources.
        // TODO(jrprice): Check the access mode and skip read-only storage buffers too.
        return;
    }

    auto* invocation = executor_.CurrentInvocation();
    auto* workgroup = executor_.CurrentWorkgroup();
    if (invocation == nullptr) {
        // Loads that happen outside of the execution of an invocation currently cannot race.
        // This happens for workgroupUniformLoad.
        return;
    }
    TINT_ASSERT(workgroup != nullptr);
    auto group_id = workgroup->GroupId();
    auto local_id = invocation->LocalInvocationId();
    auto local_index = invocation->LocalInvocationIndex();

    // Get the AST expression or statement that caused the access.
    const ast::Node* node = invocation->CurrentExpression();
    if (!node) {
        node = invocation->CurrentStatement();
    }
    TINT_ASSERT(node != nullptr);

    // Determine the root memory view.
    auto* root_view = view;
    while (root_view->Parent()) {
        root_view = root_view->Parent();
    }

    // Create a memory access record.
    auto* type = view->Type();
    MemoryAccess access{group_id, local_id, node, kind, view->Offset(), type->Size()};

    // If we are storing to a vector component, update the access record to capture the fact that
    // all components may be modified by this access.
    if (kind == MemoryAccessKind::kStore && type->Is<core::type::Scalar>()) {
        auto* parent = view->Parent();
        if (parent && parent->Type()->Is<core::type::Vector>()) {
            access.byte_offset = parent->Offset();
            access.byte_size = parent->Size();
            access.is_vector_component_write = true;
        }
    }

    // Merge the memory access into the access map for this invocation.
    // We duplicate the access for each byte that it covers, to allow us to check for races that
    // only occur for a subset of the access.
    auto& access_map = per_invocation_accesses_[group_id][local_index];
    for (uint32_t i = 0; i < access.byte_size; i++) {
        MemoryAccessMap* map;
        if (addrspace == core::AddressSpace::kStorage) {
            map = &access_map.storage_accesses_;
        } else {
            map = &access_map.workgroup_accesses_;
        }

        // Disable race checking since we are merging into a per-invocation map.
        MergeAccess({root_view, access.byte_offset + i}, access, *map, /* check_for_races */ false);
    }
}

void DataRaceDetector::MergeAccess(MemoryAccessKey access_key,
                                   MemoryAccess access,
                                   MemoryAccessMap& access_map,
                                   bool check_for_races) {
    // Check there is already an access to this memory location.
    auto itr = access_map.find(access_key);
    if (itr != access_map.end()) {
        // If we are checking for data races, record a race if the entities are different, and if at
        // least one of the accesses is a store.
        if (check_for_races &&
            (access.invocation != itr->second.invocation ||
             access.workgroup != itr->second.workgroup) &&
            (access.kind == MemoryAccessKind::kStore ||
             itr->second.kind == MemoryAccessKind::kStore)) {
            RecordRace(access_key.view, access, itr->second);
        }

        if (itr->second.kind == MemoryAccessKind::kStore) {
            // There's already a store, so we don't need to log this access.
            return;
        }
    }

    // Record the memory access.
    access_map.insert_or_assign(itr, access_key, access);
}

void DataRaceDetector::SyncWorkgroup(const Workgroup* workgroup, core::AddressSpace addrspace) {
    // Temporary maps used for checking for races within this workgroup.
    MemoryAccessMap workgroup_addrspace_accesses;
    MemoryAccessMap storage_addrspace_accesses;

    // Merge the accesses for the target address space into the temporary map.
    // Clear the per-invocation access maps as we go.
    for (auto& access_map : per_invocation_accesses_[workgroup->GroupId()]) {
        if (addrspace == core::AddressSpace::kWorkgroup) {
            for (auto& access : access_map.workgroup_accesses_) {
                MergeAccess(access.first, access.second, workgroup_addrspace_accesses,
                            /* check_for_races */ true);
            }
            access_map.workgroup_accesses_.clear();
        } else if (addrspace == core::AddressSpace::kStorage) {
            for (auto& access : access_map.storage_accesses_) {
                MergeAccess(access.first, access.second, storage_addrspace_accesses,
                            /* check_for_races */ true);
            }
            access_map.storage_accesses_.clear();
        }
    }

    if (addrspace == core::AddressSpace::kStorage) {
        // Merge storage buffer accesses into the per-workgroup map, without checking for races.
        for (auto& access : storage_addrspace_accesses) {
            MergeAccess(access.first, access.second,
                        per_group_storage_buffer_accesses_[workgroup->GroupId()],
                        /* check_for_races */ false);
        }
    }
}

void DataRaceDetector::RecordRace(const MemoryView* root_view, MemoryAccess a, MemoryAccess b) {
    Race race{root_view, a, b};

    // Always put the store first, and then order the entities by workgroup/invocation ID.
    // This helps us filter out duplicate races.
    if (a.kind != MemoryAccessKind::kStore) {
        std::swap(race.a, race.b);
    } else if (b.kind == MemoryAccessKind::kStore) {
        if (b.workgroup < a.workgroup) {
            std::swap(race.a, race.b);
        } else if (b.workgroup == a.workgroup && b.invocation < a.invocation) {
            std::swap(race.a, race.b);
        }
    }

    // Record the race in the map, if there is no existing race on the same variable from the same
    // load/store locations.
    auto key = RaceKey{root_view->Source().range.begin, race.a.cause, race.b.cause};
    races_.insert({key, race});
}

void DataRaceDetector::EmitRaces() {
    for (auto& itr : races_) {
        auto& race = itr.second;
        if (race.emitted) {
            // Skip this race if it has already been emitted.
            continue;
        }

        diag::List error;

        // Show the workgroup or storage buffer declaration that is being accessed.
        {
            error.AddWarning(race.root_view->Source())
                << "data race detected on accesses to "
                << (race.root_view->AddressSpace() == core::AddressSpace::kStorage
                        ? "storage buffer"
                        : "workgroup variable");
        }

        // Show the two accesses as diagnostic notes.
        auto AddNote = [&](auto& a) {
            error.AddNote(a.cause->source)
                << (a.kind == MemoryAccessKind::kLoad ? "loaded " : "stored ") << a.byte_size
                << " bytes at offset " << a.byte_offset << "\n"
                << "while running local_invocation_id" << a.invocation.Str() << " workgroup_id"
                << a.workgroup.Str();
        };
        AddNote(race.a);
        AddNote(race.b);

        // Add a special note about vector component writes if necessary.
        if (race.a.is_vector_component_write) {
            error.AddNote({})
                << "writing to a component of a vector may write to every component of that vector";
        }

        executor_.ReportError(std::move(error));
        race.emitted = true;
    }
}

}  // namespace tint::interp
