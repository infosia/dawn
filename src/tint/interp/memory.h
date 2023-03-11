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

#ifndef SRC_TINT_INTERP_MEMORY_H_
#define SRC_TINT_INTERP_MEMORY_H_

#include <cstdint>
#include <string>
#include <vector>

#include "src/tint/lang/core/enums.h"
#include "src/tint/utils/diagnostic/source.h"
#include "src/tint/utils/memory/block_allocator.h"

// Forward Declarations
namespace tint::core::constant {
class Value;
}  // namespace tint::core::constant
namespace tint::interp {
class Memory;
class ShaderExecutor;
}  // namespace tint::interp
namespace tint::core::type {
class Type;
}  // namespace tint::core::type

namespace tint::interp {

/// The type of an atomic read-modify-write operation.
enum class AtomicOp {
    kAdd,
    kSub,
    kMax,
    kMin,
    kAnd,
    kOr,
    kXor,
    kXchg,
};

/// A MemoryView object represents a view into a memory allocation from within the shader, and
/// provides methods to load and store from it.
class MemoryView {
  public:
    // Default constructor - produces an invalid memory view.
    MemoryView() = default;

    /// @returns the address space of this memory view
    core::AddressSpace AddressSpace() const { return address_space_; }

    /// @returns the type of this memory view
    const core::type::Type* Type() const { return type_; }

    /// @returns the offset in bytes of this memory view from the start of the allocation
    uint64_t Offset() const { return offset_; }

    /// @returns the size in bytes of this memory view
    uint64_t Size() const { return size_; }

    /// @returns the parent memory view, or nullptr if this is a root memory view
    const MemoryView* Parent() const { return parent_; }

    /// @returns the source location that this memory view corresponds to
    const tint::Source& Source() const { return source_; }

    /// Load the value from this memory view.
    /// @returns the loaded value as a core::constant::Value
    const core::constant::Value* Load();

    /// Store a value to this memory view.
    /// @param value the value to store
    void Store(const core::constant::Value* value);

    /// Create a sub-view into the view.
    /// @param type the type of the sub-view
    /// @param offset the offset into this view that the new sub-view will start from
    /// @param size the size in bytes of the new sub-view
    /// @param source the source location that this sub-view is being created at
    /// @returns the sub-view
    MemoryView* CreateSubview(const core::type::Type* type,
                              uint64_t offset,
                              uint64_t size,
                              tint::Source source);

    /// Perform an atomic load on this memory view.
    /// @returns the loaded value
    const core::constant::Value* AtomicLoad();

    /// Perform an atomic store on this memory view.
    /// @param value the value to store
    void AtomicStore(const core::constant::Value* value);

    /// Perform an atomic read-modify-write operation on this memory view.
    /// @param op the operation to apply
    /// @param value the value to add
    /// @returns the original value
    const core::constant::Value* AtomicRMW(AtomicOp op, const core::constant::Value* value);

    /// Perform an atomic compare-exchange on this memory view.
    /// @param cmp the comparator value
    /// @param value the value to store
    /// @param exchanged receives `true` if an exchange was performed, otherwise `false`
    /// @returns the original value
    const core::constant::Value* AtomicCompareExchange(const core::constant::Value* cmp,
                                                       const core::constant::Value* value,
                                                       bool& exchanged);

  private:
    ShaderExecutor* executor_ = nullptr;
    Memory* memory_ = nullptr;
    MemoryView* parent_ = nullptr;
    core::AddressSpace address_space_ = core::AddressSpace::kUndefined;
    const core::type::Type* type_ = nullptr;
    uint64_t offset_ = 0;
    uint64_t size_ = 0;
    tint::Source source_{};
    bool is_valid_ = false;

    /// Constructor
    MemoryView(ShaderExecutor* executor,
               Memory* memory,
               MemoryView* parent,
               core::AddressSpace addrspace,
               const core::type::Type* type,
               uint64_t offset,
               uint64_t size,
               tint::Source source,
               bool valid);

    /// Load a value from this memory view at an offset.
    /// Used internally to load aggregate elements recursively.
    /// @param type the type of the value to load
    /// @param offset the offset at which to load it from
    /// @returns the loaded value as a core::constant::Value
    const core::constant::Value* Load(const core::type::Type* type, uint64_t offset);

    /// Store a value to this memory view at an offset.
    /// Used internally to store aggregate elements recursively.
    /// @param value the value to store
    /// @param offset the offset at which to store it
    void Store(const core::constant::Value* value, uint64_t offset);

    /// Report an out-of-bounds access.
    /// @param msg the error message
    void ReportOutOfBounds(std::string msg);

    // Allow the block allocator to construct MemoryView objects.
    friend class BlockAllocator<MemoryView>;
};

/// A Memory object represents an allocation in the interpreter. This is used for buffers, workgroup
/// allocations, and for variables in the private and function address spaces.
class Memory {
  public:
    /// Constructor
    /// @param size the size of the allocation in bytes
    explicit Memory(uint64_t size);

    /// Destructor
    ~Memory();

    /// @returns the size of this memory object in bytes
    uint64_t Size() const { return static_cast<uint64_t>(data_.size()); }

    /// @returns a raw pointer to the underlying data
    uint8_t* Data() { return data_.data(); }

    /// Creates a new memory view into this memory allocation.
    /// Reports an error if the view is not in bounds.
    /// @param executor the shader executor in which this view is going to be used
    /// @param addrspace the address space of this memory view
    /// @param type the type of the memory view
    /// @param offset the optional offset at which the view should start
    /// @param size the optional size of the view in bytes
    /// @param source the source location that this view is being created at
    /// @returns the new memory view
    MemoryView* CreateView(ShaderExecutor* executor,
                           core::AddressSpace addrspace,
                           const core::type::Type* type,
                           uint64_t offset,
                           uint64_t size,
                           Source source);

    /// Creates a new memory view into this memory allocation.
    /// The returned memory view will encompass the whole allocation.
    /// Raises an error if the view is not in bounds.
    /// @param executor the shader executor in which this view is going to be used
    /// @param addrspace the address space of this memory view
    /// @param type the type of the memory view
    /// @param source the source location that this view is being created at
    /// @returns the new memory view
    MemoryView* CreateView(ShaderExecutor* executor,
                           core::AddressSpace addrspace,
                           const core::type::Type* type,
                           Source source);

    /// Load `size` bytes at `offset` from this memory allocation, and store the result in `value`.
    /// @param value receives the loaded data
    /// @param offset the offset in bytes from the start of the allocation
    /// @param size the size in bytes to load
    template <typename T>
    void Load(T* value, uint64_t offset, uint64_t size = sizeof(T)) {
        if (offset + size > Size()) {
            // Zero the result of an OOB load.
            memset(value, 0, size);
            return;
        }
        memcpy(value, data_.data() + offset, size);
    }

    /// Store `size` bytes at `offset` to this memory allocation, from the contents of `value`.
    /// @param value the data to store
    /// @param offset the offset in bytes from the start of the allocation
    /// @param size the size in bytes to store
    template <typename T>
    void Store(const T* value, uint64_t offset, uint64_t size = sizeof(T)) {
        if (offset + size > Size()) {
            // Skip OOB stores.
            return;
        }
        memcpy(data_.data() + offset, value, size);
    }

    /// Copy data into this memory allocation from another one.
    /// @param dst_offset the offset in bytes from the start of this allocation
    /// @param src the allocation to copy from
    /// @param src_offset the offset in bytes from the start of the source allocation
    /// @param size the size in bytes to copy
    void CopyFrom(uint64_t dst_offset, Memory& src, uint64_t src_offset, uint64_t size);

    /// Perform an atomic load from this memory allocation.
    /// @param offset the offset in bytes from the start of the allocation
    /// @returns the original value
    template <typename T>
    T AtomicLoad(uint64_t offset);

    /// Perform an atomic store to this memory allocation.
    /// @param offset the offset in bytes from the start of the allocation
    /// @param value the value to store
    template <typename T>
    void AtomicStore(uint64_t offset, T value);

    /// Perform an atomic read-modify-write operation on this memory allocation.
    /// @param offset the offset in bytes from the start of the allocation
    /// @param op the atomic operation to perform
    /// @param value the value operand
    /// @returns the original value
    template <typename T>
    T AtomicRMW(uint64_t offset, AtomicOp op, T value);

    /// Perform an atomic compare-exchange operation on this memory allocation.
    /// @param offset the offset in bytes from the start of the allocation
    /// @param cmp the comparator value
    /// @param value the value to store
    /// @param exchanged receives `true` if an exchange was performed
    /// @returns the original value
    template <typename T>
    T AtomicCompareExchange(uint64_t offset, T cmp, T value, bool& exchanged);

  private:
    std::vector<uint8_t> data_;
};

}  // namespace tint::interp

#endif  // SRC_TINT_INTERP_MEMORY_H_
