// Copyright 2022 The Dawn & Tint Authors
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

#ifndef SRC_TINT_UTILS_CONTAINERS_VECTOR_H_
#define SRC_TINT_UTILS_CONTAINERS_VECTOR_H_

#include <stddef.h>
#include <stdint.h>

#include <algorithm>
#include <atomic>
#include <functional>
#include <iterator>
#include <new>
#include <span>
#include <utility>
#include <vector>

#include "src/tint/utils/containers/empty.h"
#include "src/tint/utils/containers/reinterpret.h"
#include "src/tint/utils/ice/ice.h"
#include "src/tint/utils/macros/compiler.h"
#include "src/tint/utils/math/hash.h"
#include "src/tint/utils/memory/aligned_storage.h"
#include "src/tint/utils/memory/bitcast.h"

// This file implements a custom STL style container & iterator in a performant manner, using
// C-style data access. It is not unexpected that -Wunsafe-buffer-usage triggers in this code, since
// the type of dynamic access being used cannot be guaranteed to be safe via static analysis.
// Attempting to change this code in simple ways to quiet these errors either a) negatively affects
// the performance by introducing unneeded copes, or b) uses typing shenanigans to work around the
// warning that other linters/analyses are unhappy with.
TINT_BEGIN_DISABLE_WARNING(UNSAFE_BUFFER_USAGE);

#ifndef TINT_VECTOR_MUTATION_CHECKS_ENABLED
#ifdef NDEBUG
#define TINT_VECTOR_MUTATION_CHECKS_ENABLED 0
#else
#define TINT_VECTOR_MUTATION_CHECKS_ENABLED 1
#endif
#endif

#if TINT_VECTOR_MUTATION_CHECKS_ENABLED
#define TINT_VECTOR_MUTATION_CHECK_ASSERT(x) TINT_ASSERT(x)
#else
#define TINT_VECTOR_MUTATION_CHECK_ASSERT(x)
#endif

/// Forward declarations
namespace tint {
template <typename>
class VectorRef;
}  // namespace tint

namespace tint {

/// VectorIterator is a forward iterator of Vector elements.
template <typename T, bool FORWARD = true>
class VectorIterator {
  public:
    /// The iterator trait
    using iterator_category = std::random_access_iterator_tag;
    /// The type of an element that this iterator points to
    using value_type = T;
    /// The type of the difference of two iterators
    using difference_type = std::ptrdiff_t;
    /// A pointer of the element type
    using pointer = T*;
    /// A reference of the element type
    using reference = T&;

    /// Constructor
    VectorIterator() = default;

    /// Destructor
    ~VectorIterator() {
#if TINT_VECTOR_MUTATION_CHECKS_ENABLED
        if (iterator_count_) {
            TINT_ASSERT(*iterator_count_ > 0);
            (*iterator_count_)--;
        }
#endif
    }

#if TINT_VECTOR_MUTATION_CHECKS_ENABLED
    /// Constructor
    /// @param p the pointer to the vector element
    /// @param it_cnt a pointer to an iterator count
    VectorIterator(T* p, std::atomic<uint32_t>* it_cnt)  // NOLINT(runtime/explicit)
        : ptr_(p), iterator_count_(it_cnt) {
        (*iterator_count_)++;
    }

    /// Constructor
    /// @param p the pointer to the vector element
    VectorIterator(T* p) : ptr_(p) {}  // NOLINT(runtime/explicit)

    /// Copy constructor
    /// @param other the VectorIterator to copy
    VectorIterator(const VectorIterator& other)
        : ptr_(other.ptr_), iterator_count_(other.iterator_count_) {
        if (iterator_count_) {
            (*iterator_count_)++;
        }
    }

    /// Move constructor
    /// @param other the VectorIterator to move
    VectorIterator(VectorIterator&& other)
        : ptr_(other.ptr_), iterator_count_(other.iterator_count_) {
        other.ptr_ = nullptr;
        other.iterator_count_ = nullptr;
    }

    /// Conversion constructor
    /// @param other the VectorIterator to copy
    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    VectorIterator(const VectorIterator<U, FORWARD>& other)  // NOLINT(runtime/explicit)
        : ptr_(other.ptr_), iterator_count_(other.iterator_count_) {
        if (iterator_count_) {
            (*iterator_count_)++;
        }
    }
#else
    /// Constructor
    /// @param p the pointer to the vector element
    VectorIterator(T* p) : ptr_(p) {}  // NOLINT(runtime/explicit)

    /// Copy constructor
    /// @param other the VectorIterator to copy
    VectorIterator(const VectorIterator& other) : ptr_(other.ptr_) {}

    /// Move constructor
    /// @param other the VectorIterator to move
    VectorIterator(VectorIterator&& other) : ptr_(other.ptr_) { other.ptr_ = nullptr; }

    /// Conversion constructor
    /// @param other the VectorIterator to copy
    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    VectorIterator(const VectorIterator<U, FORWARD>& other)  // NOLINT(runtime/explicit)
        : ptr_(other.ptr_) {}
#endif

    /// Assignment operator
    /// @param other the VectorIterator to copy
    /// @return this VectorIterator
    VectorIterator& operator=(const VectorIterator& other) {
        ptr_ = other.ptr_;
#if TINT_VECTOR_MUTATION_CHECKS_ENABLED
        if (iterator_count_ != other.iterator_count_) {
            if (iterator_count_) {
                (*iterator_count_)--;
            }
            iterator_count_ = other.iterator_count_;
            if (iterator_count_) {
                (*iterator_count_)++;
            }
        }
#endif
        return *this;
    }

    /// Move-assignment operator
    /// @param other the VectorIterator to move
    /// @return this VectorIterator
    VectorIterator& operator=(VectorIterator&& other) {
        ptr_ = other.ptr_;
        other.ptr_ = nullptr;
#if TINT_VECTOR_MUTATION_CHECKS_ENABLED
        if (iterator_count_) {
            (*iterator_count_)--;
        }
        iterator_count_ = other.iterator_count_;
        other.iterator_count_ = nullptr;
#endif
        return *this;
    }

    /// @return the element this iterator currently points at
    operator T*() const { return ptr_; }

    /// @return the element this iterator currently points at
    T& operator*() const { return *ptr_; }

    /// @return the element this iterator currently points at
    T* operator->() const { return ptr_; }

    /// Equality operator
    /// @param other the other VectorIterator
    /// @return true if this iterator is equal to @p other
    bool operator==(const VectorIterator& other) const { return ptr_ == other.ptr_; }

    /// Inequality operator
    /// @param other the other VectorIterator
    /// @return true if this iterator is not equal to @p other
    bool operator!=(const VectorIterator& other) const { return ptr_ != other.ptr_; }

    /// Less-than operator
    /// @param other the other iterator
    /// @returns true if this iterator comes before @p other
    bool operator<(const VectorIterator& other) const { return other - *this > 0; }

    /// Greater-than operator
    /// @param other the other iterator
    /// @returns true if this iterator comes after @p other
    bool operator>(const VectorIterator& other) const { return *this - other > 0; }

    /// Index operator
    /// @param i the number of elements from the element this iterator points to
    /// @return the element
    T& operator[](std::ptrdiff_t i) const { return *(*this + i); }

    /// Increments the iterator (prefix)
    /// @returns this VectorIterator
    VectorIterator& operator++() {
        this->ptr_ = FORWARD ? this->ptr_ + 1 : this->ptr_ - 1;
        return *this;
    }

    /// Decrements the iterator (prefix)
    /// @returns this VectorIterator
    VectorIterator& operator--() {
        this->ptr_ = FORWARD ? this->ptr_ - 1 : this->ptr_ + 1;
        return *this;
    }

    /// Increments the iterator (postfix)
    /// @returns a VectorIterator that points to the element before the increment
    VectorIterator operator++(int) {
        VectorIterator res = *this;
        this->ptr_ = FORWARD ? this->ptr_ + 1 : this->ptr_ - 1;
        return res;
    }

    /// Decrements the iterator (postfix)
    /// @returns a VectorIterator that points to the element before the decrement
    VectorIterator operator--(int) {
        VectorIterator res = *this;
        this->ptr_ = FORWARD ? this->ptr_ - 1 : this->ptr_ + 1;
        return res;
    }

    /// Moves the iterator forward by @p n elements
    /// @param n the number of elements
    /// @returns this VectorIterator
    VectorIterator operator+=(std::ptrdiff_t n) {
        this->ptr_ = FORWARD ? this->ptr_ + n : this->ptr_ - n;
        return *this;
    }

    /// Moves the iterator backwards by @p n elements
    /// @param n the number of elements
    /// @returns this VectorIterator
    VectorIterator operator-=(std::ptrdiff_t n) {
        this->ptr_ = FORWARD ? this->ptr_ - n : this->ptr_ + n;
        return *this;
    }

    /// @param n the number of elements
    /// @returns a new VectorIterator progressed by @p n elements
    VectorIterator operator+(std::ptrdiff_t n) const {
#if TINT_VECTOR_MUTATION_CHECKS_ENABLED
        return VectorIterator{FORWARD ? ptr_ + n : ptr_ - n, iterator_count_};
#else
        return VectorIterator{FORWARD ? ptr_ + n : ptr_ - n};
#endif
    }

    /// @param n the number of elements
    /// @returns a new VectorIterator regressed by @p n elements
    VectorIterator operator-(std::ptrdiff_t n) const {
#if TINT_VECTOR_MUTATION_CHECKS_ENABLED
        return VectorIterator{FORWARD ? ptr_ - n : ptr_ + n, iterator_count_};
#else
        return VectorIterator{FORWARD ? ptr_ - n : ptr_ + n};
#endif
    }

    /// @param other the other iterator
    /// @returns the number of elements between this iterator and @p other
    std::ptrdiff_t operator-(const VectorIterator& other) const {
        return FORWARD ? ptr_ - other.ptr_ : other.ptr_ - ptr_;
    }

  private:
    /// Friend class
    template <typename, bool>
    friend class VectorIterator;

    T* ptr_ = nullptr;
#if TINT_VECTOR_MUTATION_CHECKS_ENABLED
    std::atomic<uint32_t>* iterator_count_ = nullptr;
#endif
};

/// @param out the stream to write to
/// @param it the VectorIterator
/// @returns @p out so calls can be chained
template <typename STREAM, typename T, bool FORWARD>
    requires(traits::IsOStream<STREAM>)
auto& operator<<(STREAM& out, const VectorIterator<T, FORWARD>& it) {
    return out << *it;
}

/// Internal implementation of Vector.
struct VectorImplBase {
    /// Pointer to the first element in the vector.
    void* data = nullptr;
    /// The number of elements in the vector.
    size_t len = 0;
    /// The total capacity of the vector.
    size_t cap = 0;
};

namespace detail {

/// IsVectorLike<T>::value is true if T is a Vector or VectorRef.
template <typename T>
struct IsVectorLike {
    /// Non-specialized form of IsVectorLike defaults to false
    static constexpr bool value = false;
};

/// IsVectorLike specialization for Vector
template <typename T, size_t N>
struct IsVectorLike<Vector<T, N>> {
    /// True for the IsVectorLike specialization of Vector
    static constexpr bool value = true;
};

/// IsVectorLike specialization for VectorRef
template <typename T>
struct IsVectorLike<VectorRef<T>> {
    /// True for the IsVectorLike specialization of VectorRef
    static constexpr bool value = true;
};
}  // namespace detail

/// True if T is a Vector<T, N> or VectorRef<T>
template <typename T>
static constexpr bool IsVectorLike = tint::detail::IsVectorLike<T>::value;

/// Vector is a small-object-optimized, dynamically-sized vector of contigious elements of type T.
///
/// Vector will fit `N` elements internally before spilling to heap allocations. If `N` is greater
/// than zero, the internal elements are stored in a 'small array' held internally by the Vector.
///
/// Vectors can be copied or moved.
///
/// Copying a vector will either copy to the 'small array' if the number of elements is equal to or
/// less than N, otherwise elements will be copied into a new heap allocation.
///
/// Moving a vector will reassign ownership of the heap-allocation memory, if the source vector
/// holds its elements in a heap allocation, otherwise a copy will be made as described above.
///
/// Vector is optimized for CPU performance over memory efficiency. For example:
/// * Moving a vector that stores its elements in a heap allocation to another vector will simply
///   assign the heap allocation, even if the target vector can hold the elements in its 'small
///   array'. This reduces memory copying, but may incur additional memory usage.
/// * Resizing, or popping elements from a vector that has spilled to a heap allocation does not
///   revert back to using the 'small array'. Again, this is to reduce memory copying.
template <typename T, size_t N>
class Vector {
  public:
    /// Alias to the non-const forward iterator
    using iterator = VectorIterator<T, /* forward */ true>;
    /// Alias to the const forward iterator
    using const_iterator = VectorIterator<const T, /* forward */ true>;
    /// Alias to the non-const reverse  iterator
    using reverse_iterator = VectorIterator<T, /* forward */ false>;
    /// Alias to the const reverse iterator
    using const_reverse_iterator = VectorIterator<const T, /* forward */ false>;
    /// Alias to `T`.
    using value_type = T;
    /// Value of `N`
    static constexpr size_t static_length = N;

    /// @returns the internal data of the vector as a std::span
    std::span<T> AsSpan() { return {Bitcast<T*>(impl_.data), impl_.len}; }

    /// @returns the internal data of the vector as a std::span
    std::span<const T> AsSpan() const { return {Bitcast<const T*>(impl_.data), impl_.len}; }

    /// Copies all the elements from `other` to this vector, replacing the content of this vector.
    /// @param other the span to copy
    template <typename U>
    void Copy(const std::span<U>& other) {
        if (impl_.cap < other.size()) {
            ClearAndFree();
            impl_.Allocate(other.size());
        } else {
            Clear();
        }

        impl_.len = other.size();
        T* data = Bitcast<T*>(impl_.data);
        for (size_t i = 0; i < impl_.len; i++) {
            new (&data[i]) T(other[i]);
        }
    }

    /// Copies all the elements from `other` to this vector, replacing the content of this vector.
    /// @param other the vector-like to copy
    template <typename VEC, ReinterpretMode MODE = ReinterpretMode::kSafe>
        requires(IsVectorLike<std::decay_t<VEC>>)
    void Copy(VEC&& other) {
        this->Copy(tint::Reinterpret<std::add_const_t<T>, MODE>(other.AsSpan()));
    }

    /// Moves all the elements from `other` to this vector, replacing the content of this vector.
    /// @param other the vector to move
    template <typename U, size_t N2, ReinterpretMode MODE = ReinterpretMode::kSafe>
    void Move(Vector<U, N2>&& other) {
        if constexpr (CanReinterpret<MODE, T, U>) {
            if (other.impl_.CanMove()) {
                // Just steal the data.
                ClearAndFree();
                impl_.data = other.impl_.data;
                impl_.len = other.impl_.len;
                impl_.cap = other.impl_.cap;
                other.impl_.OnMoveToHeap();
                return;
            }
        }

        // Can't steal the data, so we have to move the elements instead.

        // Ensure we have capacity for all the elements
        if (impl_.cap < other.impl_.len) {
            ClearAndFree();
            impl_.Allocate(other.impl_.len);
        } else {
            Clear();
        }

        // Move each of the elements.
        impl_.len = other.impl_.len;
        T* data = Bitcast<T*>(impl_.data);
        U* other_data = Bitcast<U*>(other.impl_.data);
        for (size_t i = 0; i < impl_.len; i++) {
            new (&data[i]) T(std::move(other_data[i]));
        }

        // Clear other
        other.Clear();
    }

    /// Constructor
    Vector() = default;

    /// Constructor
    Vector(EmptyType) {}  // NOLINT(runtime/explicit)

    /// Constructor
    /// @param elements the elements to place into the vector
    Vector(std::initializer_list<T> elements) {
        Reserve(elements.size());
        for (auto& el : elements) {
            new (&Bitcast<T*>(impl_.data)[impl_.len++]) T(el);
        }
    }

    /// Copy constructor
    /// @param other the vector to copy
    Vector(const Vector& other) { Copy(other.AsSpan()); }

    /// Move constructor
    /// @param other the vector to move
    Vector(Vector&& other) { Move(std::move(other)); }

    /// Copy constructor (differing N length)
    /// @param other the vector to copy
    template <size_t N2>
    Vector(const Vector<T, N2>& other) {
        Copy(other.AsSpan());
    }

    /// Move constructor (differing N length)
    /// @param other the vector to move
    template <size_t N2>
    Vector(Vector<T, N2>&& other) {
        Move(std::move(other));
    }

    /// Copy constructor with covariance / const conversion
    /// @param other the vector to copy
    /// @see CanReinterpret for rules about conversion
    template <typename U,
              size_t N2,
              ReinterpretMode MODE = ReinterpretMode::kSafe,
              typename = std::enable_if_t<CanReinterpret<MODE, T, U>>>
    Vector(const Vector<U, N2>& other) {  // NOLINT(runtime/explicit)
        this->Copy(tint::Reinterpret<std::add_const_t<T>, MODE>(other.AsSpan()));
    }

    /// Move constructor with covariance / const conversion
    /// @param other the vector to move
    /// @see CanReinterpret for rules about conversion
    template <typename U,
              size_t N2,
              ReinterpretMode MODE = ReinterpretMode::kSafe,
              typename = std::enable_if_t<CanReinterpret<MODE, T, U>>>
    Vector(Vector<U, N2>&& other) {  // NOLINT(runtime/explicit)
        this->template Move<U, N2, MODE>(std::move(other));
    }

    /// Move constructor from a mutable vector reference
    /// @param other the vector reference to move
    Vector(VectorRef<T>&& other) { MoveOrCopy(std::move(other)); }  // NOLINT(runtime/explicit)

    /// Copy constructor from an immutable vector reference
    /// @param other the vector reference to copy
    Vector(const VectorRef<T>& other) { this->Copy(other.AsSpan()); }  // NOLINT(runtime/explicit)

    /// Copy constructor from a span
    /// @param other the span to copy
    template <typename U>
    Vector(const std::span<U>& other) {  // NOLINT(runtime/explicit)
        this->Copy(other);
    }

    /// Destructor
    ~Vector() { ClearAndFree(); }

    /// Assignment operator
    /// @param other the vector to copy
    /// @returns this vector so calls can be chained
    Vector& operator=(const Vector& other) {
        if (&other != this) {
            this->Copy(other.AsSpan());
        }
        return *this;
    }

    /// Move operator
    /// @param other the vector to move
    /// @returns this vector so calls can be chained
    Vector& operator=(Vector&& other) {
        if (&other != this) {
            this->Move(std::move(other));
        }
        return *this;
    }

    /// Assignment operator (differing N length)
    /// @param other the vector to copy
    /// @returns this vector so calls can be chained
    template <size_t N2>
    Vector& operator=(const Vector<T, N2>& other) {
        this->Copy(other.AsSpan());
        return *this;
    }

    /// Move operator (differing N length)
    /// @param other the vector to copy
    /// @returns this vector so calls can be chained
    template <size_t N2, ReinterpretMode MODE = ReinterpretMode::kSafe>
    Vector& operator=(Vector<T, N2>&& other) {
        this->template Move<T, N2, MODE>(std::move(other));
        return *this;
    }

    /// Assignment operator (differing N length)
    /// @param other the vector reference to copy
    /// @returns this vector so calls can be chained
    Vector& operator=(const VectorRef<T>& other) {
        if (&other.Data() != &impl_.data) {
            this->Copy(other.AsSpan());
        }
        return *this;
    }

    /// Move operator (differing N length)
    /// @param other the vector reference to copy
    /// @returns this vector so calls can be chained
    Vector& operator=(VectorRef<T>&& other) {
        if (&other.Data() != &impl_.data) {
            MoveOrCopy(std::move(other));
        }
        return *this;
    }

    /// Index operator
    /// @param i the element index. Must be less than `len`.
    /// @returns a reference to the i'th element.
    T& operator[](size_t i) {
        TINT_ASSERT(i < impl_.len);
        return Bitcast<T*>(impl_.data)[i];
    }

    /// Index operator
    /// @param i the element index. Must be less than `len`.
    /// @returns a reference to the i'th element.
    const T& operator[](size_t i) const {
        TINT_ASSERT(i < impl_.len);
        return Bitcast<const T*>(impl_.data)[i];
    }

    /// @returns the number of elements in the vector
    size_t Length() const { return impl_.len; }

    /// @returns true if the vector is empty.
    bool IsEmpty() const { return impl_.len == 0; }

    /// @returns the total number of elements that the vector can hold before a heap allocation is
    /// required.
    size_t Capacity() const { return impl_.cap; }

    /// Clears the vector, then re-allocates the backing store to the given capacity.
    /// @param new_cap the new capacity
    void Allocate(size_t new_cap) {
        ClearAndFree();
        impl_.Allocate(new_cap);
    }

    /// Reserves capacity for at least `new_cap` elements.
    /// @param new_cap the new capacity
    void Reserve(size_t new_cap) {
        if (new_cap > impl_.cap) {
            T* old_data = Bitcast<T*>(impl_.data);
            size_t old_len = impl_.len;
            impl_.Allocate(new_cap);
            T* new_data = Bitcast<T*>(impl_.data);
            for (size_t i = 0; i < old_len; i++) {
                new (&new_data[i]) T(std::move(old_data[i]));
                old_data[i].~T();
            }
            impl_.len = old_len;
            impl_.Free(old_data);
        }
    }

    /// Shrinks the vector to the given length.
    /// @param new_len the new length. Must be less than or equal to `Length()`.
    void Shrink(size_t new_len) {
        TINT_ASSERT(new_len <= impl_.len);
        TINT_VECTOR_MUTATION_CHECK_ASSERT(iterator_count_ == 0);
        T* data = Bitcast<T*>(impl_.data);
        while (impl_.len > new_len) {
            data[--impl_.len].~T();
        }
    }

    /// Resizes the vector to the given length.
    /// @param new_len the new length.
    /// @param arg argument to pass to the element constructor if the vector is expanded.
    template <typename ARG>
    void Resize(size_t new_len, ARG&& arg) {
        TINT_VECTOR_MUTATION_CHECK_ASSERT(iterator_count_ == 0);
        if (new_len <= impl_.len) {
            Shrink(new_len);
        } else {
            Reserve(new_len);
            T* data = Bitcast<T*>(impl_.data);
            while (impl_.len < new_len) {
                new (&data[impl_.len++]) T(static_cast<T>(std::forward<ARG>(arg)));
            }
        }
    }

    /// Resizes the vector to the given length.
    /// @param new_len the new length.
    /// @param args arguments to pass to the element constructor if the vector is expanded.
    template <typename... ARGS>
    void Resize(size_t new_len, ARGS&&... args) {
        TINT_VECTOR_MUTATION_CHECK_ASSERT(iterator_count_ == 0);
        if (new_len <= impl_.len) {
            Shrink(new_len);
        } else {
            Reserve(new_len);
            T* data = Bitcast<T*>(impl_.data);
            while (impl_.len < new_len) {
                new (&data[impl_.len++]) T(std::forward<ARGS>(args)...);
            }
        }
    }

    /// Pushes a new element to the end of the vector.
    /// @param el the element to push
    void Push(const T& el) {
        TINT_VECTOR_MUTATION_CHECK_ASSERT(iterator_count_ == 0);
        if (impl_.len == impl_.cap) {
            Grow();
        }
        new (&Bitcast<T*>(impl_.data)[impl_.len++]) T(el);
    }

    /// Pushes a new element to the end of the vector.
    /// @param el the element to push
    void Push(T&& el) {
        TINT_VECTOR_MUTATION_CHECK_ASSERT(iterator_count_ == 0);
        if (impl_.len == impl_.cap) {
            Grow();
        }
        new (&Bitcast<T*>(impl_.data)[impl_.len++]) T(std::move(el));
    }

    /// Pushes a new element to the end of the vector.
    /// @param el the element to push
    template <typename U>
    void Push(U&& el) {
        TINT_VECTOR_MUTATION_CHECK_ASSERT(iterator_count_ == 0);
        if (impl_.len == impl_.cap) {
            Grow();
        }
        new (&Bitcast<T*>(impl_.data)[impl_.len++]) T(static_cast<T>(std::forward<U>(el)));
    }

    /// Pushes a new element to the end of the vector.
    /// @param args the arguments to pass to the element constructor
    /// @returns a reference to the new element
    template <typename... ARGS>
    T& Emplace(ARGS&&... args) {
        TINT_VECTOR_MUTATION_CHECK_ASSERT(iterator_count_ == 0);
        if (impl_.len == impl_.cap) {
            Grow();
        }
        T* el = new (&Bitcast<T*>(impl_.data)[impl_.len++]) T{std::forward<ARGS>(args)...};
        return *el;
    }

    /// Pushes a number of elements to the end of the vector.
    /// @param elements the elements to push
    template <typename... ARGS>
    void PushAll(ARGS&&... elements) {
        TINT_VECTOR_MUTATION_CHECK_ASSERT(iterator_count_ == 0);
        AppendVariadic(std::forward<ARGS>(elements)...);
    }

    /// Pushes each of the elements of `other` to the end of this vector.
    /// @param other the other vector to copy elements from
    template <typename U, size_t N2>
    void PushAll(const Vector<U, N2>& other) {
        TINT_VECTOR_MUTATION_CHECK_ASSERT(iterator_count_ == 0);
        auto other_span = other.AsSpan();
        Reserve(impl_.len + other_span.size());
        for (auto& el : other_span) {
            new (&Bitcast<T*>(impl_.data)[impl_.len++]) T(el);
        }
    }

    /// Removes the last element from the vector.
    /// @returns the popped element
    T Pop() {
        TINT_ASSERT(impl_.len > 0);
        TINT_VECTOR_MUTATION_CHECK_ASSERT(iterator_count_ == 0);
        T el = std::move(Bitcast<T*>(impl_.data)[--impl_.len]);
        Bitcast<T*>(impl_.data)[impl_.len].~T();
        return el;
    }

    /// Removes and returns the first element from the vector.
    /// @returns the first element
    T PopFront() {
        TINT_ASSERT(impl_.len > 0);
        TINT_VECTOR_MUTATION_CHECK_ASSERT(iterator_count_ == 0);
        T* data = Bitcast<T*>(impl_.data);
        T el = std::move(data[0]);
        for (size_t i = 1; i < impl_.len; i++) {
            data[i - 1] = std::move(data[i]);
        }
        data[--impl_.len].~T();
        return el;
    }

    /// Clears the vector.
    void Clear() {
        TINT_VECTOR_MUTATION_CHECK_ASSERT(iterator_count_ == 0);
        T* data = Bitcast<T*>(impl_.data);
        while (impl_.len > 0) {
            data[--impl_.len].~T();
        }
    }

    /// @returns a pointer to the first element in the vector
    T* begin() { return Bitcast<T*>(impl_.data); }

    /// @returns a pointer to the first element in the vector
    const T* begin() const { return Bitcast<const T*>(impl_.data); }

    /// @returns a pointer to one past the last element in the vector
    T* end() { return Bitcast<T*>(impl_.data) + impl_.len; }

    /// @returns a pointer to one past the last element in the vector
    const T* end() const { return Bitcast<const T*>(impl_.data) + impl_.len; }

    /// @returns a reverse iterator starting with the last element in the vector
    reverse_iterator rbegin() { return reverse_iterator(end() - 1); }

    /// @returns a reverse iterator starting with the last element in the vector
    const_reverse_iterator rbegin() const { return const_reverse_iterator(end() - 1); }

    /// @returns the end for a reverse iterator
    reverse_iterator rend() { return reverse_iterator(begin() - 1); }

    /// @returns the end for a reverse iterator
    const_reverse_iterator rend() const { return const_reverse_iterator(begin() - 1); }

    /// @returns a reference to the first element in the vector
    T& Front() {
        TINT_ASSERT(impl_.len > 0);
        return Bitcast<T*>(impl_.data)[0];
    }

    /// @returns a reference to the first element in the vector
    const T& Front() const {
        TINT_ASSERT(impl_.len > 0);
        return Bitcast<const T*>(impl_.data)[0];
    }

    /// @returns a reference to the last element in the vector
    T& Back() {
        TINT_ASSERT(impl_.len > 0);
        return Bitcast<T*>(impl_.data)[impl_.len - 1];
    }

    /// @returns a reference to the last element in the vector
    const T& Back() const {
        TINT_ASSERT(impl_.len > 0);
        return Bitcast<const T*>(impl_.data)[impl_.len - 1];
    }

    /// Equality operator.
    /// @param other the other vector
    /// @returns true if all elements are equal.
    template <typename T2, size_t N2>
    bool operator==(const Vector<T2, N2>& other) const {
        if (Length() != other.Length()) {
            return false;
        }
        for (size_t i = 0; i < Length(); i++) {
            if ((*this)[i] != other[i]) {
                return false;
            }
        }
        return true;
    }

    /// Inequality operator.
    /// @param other the other vector
    /// @returns true if this vector is not the same length as `other`, or all elements are not
    ///          equal.
    template <typename T2, size_t N2>
    bool operator!=(const Vector<T2, N2>& other) const {
        return !(*this == other);
    }

    /// Inserts an element at the given index.
    /// @param index the index to insert at
    /// @param el the element to insert
    template <typename U>
    void Insert(size_t index, U&& el) {
        TINT_ASSERT(index <= impl_.len);
        TINT_VECTOR_MUTATION_CHECK_ASSERT(iterator_count_ == 0);
        if (impl_.len == impl_.cap) {
            Grow();
        }
        T* data = Bitcast<T*>(impl_.data);
        for (size_t i = impl_.len; i > index; i--) {
            new (&data[i]) T(std::move(data[i - 1]));
            data[i - 1].~T();
        }
        new (&data[index]) T(std::forward<U>(el));
        impl_.len++;
    }

    /// Removes elements from the vector.
    /// @param start the index of the first element to remove
    /// @param count the number of elements to remove
    void Erase(size_t start, size_t count = 1) {
        TINT_ASSERT(start + count <= impl_.len);
        TINT_VECTOR_MUTATION_CHECK_ASSERT(iterator_count_ == 0);
        T* data = Bitcast<T*>(impl_.data);
        for (size_t i = start + count; i < impl_.len; i++) {
            data[i - count] = std::move(data[i]);
        }
        Shrink(impl_.len - count);
    }

    /// @returns true if the predicate function returns true for any of the elements of the vector
    /// @param pred a function-like with the signature `bool(T)`
    template <typename PREDICATE>
    bool Any(PREDICATE&& pred) const {
        return std::any_of(begin(), end(), std::forward<PREDICATE>(pred));
    }

    /// @returns true if the predicate function returns true for all the elements of the vector
    /// @param pred a function-like with the signature `bool(T)`
    template <typename PREDICATE>
    bool All(PREDICATE&& pred) const {
        return std::all_of(begin(), end(), std::forward<PREDICATE>(pred));
    }

    /// Removes all elements that match the given predicate function.
    /// @param pred a function-like with the signature `bool(T)`
    template <typename PREDICATE>
    void EraseIf(PREDICATE&& pred) {
        TINT_VECTOR_MUTATION_CHECK_ASSERT(iterator_count_ == 0);
        T* d = begin();
        T* el = std::remove_if(d, end(), std::forward<PREDICATE>(pred));
        Shrink(static_cast<size_t>(el - d));
    }

    /// Sorts the elements of the vector.
    /// @param compare a function-like with the signature `bool(T, T)`
    /// @returns this vector
    template <typename COMPARE = std::less<T>>
    Vector& Sort(COMPARE&& compare = COMPARE{}) {
        TINT_VECTOR_MUTATION_CHECK_ASSERT(iterator_count_ == 0);
        std::sort(begin(), end(), std::forward<COMPARE>(compare));
        return *this;
    }

    /// Reverses the elements of the vector.
    void Reverse() {
        TINT_VECTOR_MUTATION_CHECK_ASSERT(iterator_count_ == 0);
        std::reverse(begin(), end());
    }

    /// @returns a hash code of the Vector
    tint::HashCode HashCode() const {
        auto hash = Hash(Length());
        for (auto& el : *this) {
            hash = HashCombine(hash, el);
        }
        return hash;
    }

  private:
    /// Friend class (differing specializations of this class)
    template <typename, size_t>
    friend class Vector;

    /// Friend class
    template <typename>
    friend class VectorRef;

    /// Friend class
    template <typename>
    friend class VectorRef;

    template <typename... Ts>
    void AppendVariadic(Ts&&... args) {
        ((new (&Bitcast<T*>(impl_.data)[impl_.len++]) T{std::forward<Ts>(args)}), ...);
    }

    /// Expands the capacity of the vector
    void Grow() { Reserve(std::max(impl_.cap, static_cast<size_t>(1)) * 2); }

    /// Moves 'other' to this vector, if possible, otherwise performs a copy.
    void MoveOrCopy(VectorRef<T>&& other) {
        if (other.can_move_) {
            // Just steal the data.
            ClearAndFree();
            impl_.data = *other.data_;
            impl_.len = *other.len_;
            impl_.cap = *other.cap_;
            *other.data_ = nullptr;
            *other.len_ = 0;
            *other.cap_ = 0;
            other.can_move_ = false;
        } else {
            Copy(other.AsSpan());
        }
    }

    /// Clears the vector, then frees the data.
    void ClearAndFree() {
        Clear();
        impl_.Free(Bitcast<T*>(impl_.data));
    }

    /// True if this vector uses a small array for small object optimization.
    constexpr static bool HasSmallArray = N > 0;

    /// A structure that has the same size and alignment as T.
    using TStorage = AlignedStorage<T>;

    /// The internal structure for the vector with a small array.
    struct ImplWithSmallArray : VectorImplBase {
        TStorage small_arr[N];

        ImplWithSmallArray() {
            data = &small_arr[0].Get();
            cap = N;
        }

        /// Allocates a new vector of `T` either from #small_arr, or from the heap, then assigns the
        /// pointer it to #data, and updates #cap.
        void Allocate(size_t new_cap) {
            if (new_cap <= N) {
                data = &small_arr[0].Get();
                cap = N;
            } else {
                data = new TStorage[new_cap];
                cap = new_cap;
            }
        }

        /// Frees `d`, if isn't a pointer to #small_arr
        void Free(T* d) const {
            if (d != Bitcast<T*>(&small_arr[0].Get())) {
                delete[] Bitcast<TStorage*>(d);
            }
        }

        /// Indicates whether the internal data can be std::move()d.
        /// @returns true if #data does not point to #small_arr
        bool CanMove() const { return data != &small_arr[0].Get(); }

        /// Called when the internal data has been moved.
        void OnMoveToHeap() {
            data = &small_arr[0].Get();
            len = 0;
            cap = N;
        }
    };

    /// The internal structure for the vector without a small array.
    struct ImplWithoutSmallArray : VectorImplBase {
        /// Allocates a new vector of `T` and assigns it to #data, and updates #cap.
        void Allocate(size_t new_cap) {
            data = new TStorage[new_cap];
            cap = new_cap;
        }

        /// Frees `d`.
        void Free(T* d) const { delete[] Bitcast<TStorage*>(d); }

        /// Indicates whether the internal data can be std::move()d.
        /// @returns true
        bool CanMove() const { return true; }

        /// Called when the internal data has been moved.
        void OnMoveToHeap() {
            data = nullptr;
            len = 0;
            cap = 0;
        }
    };

    /// Either a ImplWithSmallArray or ImplWithoutSmallArray based on N.
    std::conditional_t<HasSmallArray, ImplWithSmallArray, ImplWithoutSmallArray> impl_;

    /// The current number of iterators referring to this vector
    mutable std::atomic<uint32_t> iterator_count_ = 0;
};

namespace detail {

/// Helper for determining the Vector element type (`T`) from the vector's constuctor arguments
/// @tparam IS_CASTABLE true if the types of `Ts` derive from CastableBase
/// @tparam Ts the vector constructor argument types to infer the vector element type from.
template <bool IS_CASTABLE, typename... Ts>
struct VectorCommonType;

/// VectorCommonType specialization for non-castable types.
template <typename... Ts>
struct VectorCommonType</*IS_CASTABLE*/ false, Ts...> {
    /// The common T type to use for the vector
    using type = std::common_type_t<Ts...>;
};

/// VectorCommonType specialization for castable types.
template <typename... Ts>
struct VectorCommonType</*IS_CASTABLE*/ true, Ts...> {
    /// The common Castable type (excluding pointer)
    using common_ty = CastableCommonBase<std::remove_pointer_t<Ts>...>;
    /// The common T type to use for the vector
    using type = std::conditional_t<(std::is_const_v<std::remove_pointer_t<Ts>> || ...),
                                    const common_ty*,
                                    common_ty*>;
};

}  // namespace detail

/// Helper for determining the Vector element type (`T`) from the vector's constuctor arguments
template <typename... Ts>
using VectorCommonType =
    typename tint::detail::VectorCommonType<IsCastable<std::remove_pointer_t<Ts>...>, Ts...>::type;

/// Deduction guide for Vector
template <typename... Ts>
Vector(Ts...) -> Vector<VectorCommonType<Ts...>, sizeof...(Ts)>;

/// VectorRef is a weak reference to a Vector, used to pass vectors as parameters, avoiding copies
/// between the caller and the callee, or as an non-static sized accessor on a vector. VectorRef can
/// accept a Vector of any 'N' value, decoupling the caller's vector internal size from the callee's
/// vector size. A VectorRef tracks the usage of moves either side of the call. If at the call site,
/// a Vector argument is moved to a VectorRef parameter, and within the callee, the VectorRef
/// parameter is moved to a Vector, then the Vector heap allocation will be moved.
template <typename T>
class VectorRef {
  public:
    /// Type of `T`.
    using value_type = T;

    /// Constructor - empty reference
    VectorRef() = default;

    /// Constructor
    VectorRef(EmptyType) {}  // NOLINT(runtime/explicit)

    /// Constructor from a Vector
    /// @param vector the vector to create a reference of
    template <size_t N>
    VectorRef(Vector<T, N>& vector)  // NOLINT(runtime/explicit)
        : data_(&vector.impl_.data), len_(&vector.impl_.len), cap_(&vector.impl_.cap) {}

    /// Constructor from a const Vector
    /// @param vector the vector to create a reference of
    template <size_t N>
    VectorRef(const Vector<T, N>& vector)  // NOLINT(runtime/explicit)
        : data_(const_cast<void**>(&vector.impl_.data)),
          len_(const_cast<size_t*>(&vector.impl_.len)),
          cap_(const_cast<size_t*>(&vector.impl_.cap)) {}

    /// Constructor from a moved Vector
    /// @param vector the vector being moved
    template <size_t N>
    VectorRef(Vector<T, N>&& vector)  // NOLINT(runtime/explicit)
        : data_(&vector.impl_.data),
          len_(&vector.impl_.len),
          cap_(&vector.impl_.cap),
          can_move_(vector.impl_.CanMove()) {}

    /// Copy constructor
    /// @param other the vector reference
    VectorRef(const VectorRef& other)
        : data_(other.data_), len_(other.len_), cap_(other.cap_), can_move_(false) {}

    /// Move constructor
    /// @param other the vector reference
    VectorRef(VectorRef&& other)
        : data_(other.data_), len_(other.len_), cap_(other.cap_), can_move_(other.can_move_) {
        other.can_move_ = false;
    }

    /// Copy constructor with covariance / const conversion
    /// @param other the other vector reference
    template <typename U, typename = std::enable_if_t<CanReinterpret<ReinterpretMode::kSafe, T, U>>>
    VectorRef(const VectorRef<U>& other)  // NOLINT(runtime/explicit)
        : data_(const_cast<void**>(reinterpret_cast<void* const*>(other.data_))),
          len_(const_cast<size_t*>(other.len_)),
          cap_(const_cast<size_t*>(other.cap_)),
          can_move_(false) {}

    /// Move constructor with covariance / const conversion
    /// @param other the vector reference
    template <typename U, typename = std::enable_if_t<CanReinterpret<ReinterpretMode::kSafe, T, U>>>
    VectorRef(VectorRef<U>&& other)  // NOLINT(runtime/explicit)
        : data_(const_cast<void**>(reinterpret_cast<void* const*>(other.data_))),
          len_(const_cast<size_t*>(other.len_)),
          cap_(const_cast<size_t*>(other.cap_)),
          can_move_(other.can_move_) {
        other.can_move_ = false;
    }

    /// Constructor from a Vector with covariance / const conversion
    /// @param vector the vector to create a reference of
    /// @see CanReinterpret for rules about conversion
    template <typename U,
              size_t N,
              typename = std::enable_if_t<CanReinterpret<ReinterpretMode::kSafe, T, U>>>
    VectorRef(const Vector<U, N>& vector)  // NOLINT(runtime/explicit)
        : data_(const_cast<void**>(reinterpret_cast<void* const*>(&vector.impl_.data))),
          len_(const_cast<size_t*>(&vector.impl_.len)),
          cap_(const_cast<size_t*>(&vector.impl_.cap)) {}

    /// Constructor from a moved Vector with covariance / const conversion
    /// @param vector the vector to create a reference of
    /// @see CanReinterpret for rules about conversion
    template <typename U,
              size_t N,
              typename = std::enable_if_t<CanReinterpret<ReinterpretMode::kSafe, T, U>>>
    VectorRef(Vector<U, N>&& vector)  // NOLINT(runtime/explicit)
        : data_(const_cast<void**>(reinterpret_cast<void* const*>(&vector.impl_.data))),
          len_(const_cast<size_t*>(&vector.impl_.len)),
          cap_(const_cast<size_t*>(&vector.impl_.cap)),
          can_move_(vector.impl_.CanMove()) {}

    /// Index operator
    /// @param i the element index. Must be less than `len`.
    /// @returns a reference to the i'th element.
    const T& operator[](size_t i) const {
        TINT_ASSERT(data_ && i < *len_);
        return Bitcast<T*>(*data_)[i];
    }

    /// @return the number of elements in the vector
    size_t Length() const { return len_ ? *len_ : 0; }

    /// @return the number of elements that the vector could hold before a heap allocation needs to
    /// be made
    size_t Capacity() const { return cap_ ? *cap_ : 0; }

    /// @return a reinterpretation of this VectorRef as elements of type U.
    /// @note this is doing a reinterpret_cast of elements. It is up to the caller to ensure that
    /// this is a safe operation.
    template <typename U>
    VectorRef<U> ReinterpretCast() const {
        VectorRef<U> out;
        out.data_ = const_cast<void**>(data_);
        out.len_ = const_cast<size_t*>(len_);
        out.cap_ = const_cast<size_t*>(cap_);
        out.can_move_ = false;
        return out;
    }

    /// @returns the internal data of the vector as a std::span
    std::span<T> AsSpan() { return {Bitcast<T*>(data_ ? *data_ : nullptr), Length()}; }

    /// @returns the internal data of the vector as a std::span
    std::span<const T> AsSpan() const {
        return {Bitcast<const T*>(data_ ? *data_ : nullptr), Length()};
    }

    /// @returns true if the vector is empty.
    bool IsEmpty() const { return Length() == 0; }

    /// @returns a reference to the first element in the vector
    const T& Front() const {
        TINT_ASSERT(!IsEmpty());
        return Bitcast<T*>(*data_)[0];
    }

    /// @returns a reference to the last element in the vector
    const T& Back() const {
        TINT_ASSERT(!IsEmpty());
        return Bitcast<T*>(*data_)[*len_ - 1];
    }

    /// @returns a pointer to the first element in the vector
    const T* begin() const { return Bitcast<T*>(data_ ? *data_ : nullptr); }

    /// @returns a pointer to one past the last element in the vector
    const T* end() const { return begin() + Length(); }

    /// @returns a reverse iterator starting with the last element in the vector
    auto rbegin() const { return std::reverse_iterator<const T*>(end()); }

    /// @returns the end for a reverse iterator
    auto rend() const { return std::reverse_iterator<const T*>(begin()); }

    /// @returns a hash code of the Vector
    tint::HashCode HashCode() const {
        auto hash = Hash(Length());
        for (auto& el : *this) {
            hash = HashCombine(hash, el);
        }
        return hash;
    }

    /// @returns true if the predicate function returns true for any of the elements of the vector
    /// @param pred a function-like with the signature `bool(T)`
    template <typename PREDICATE>
    bool Any(PREDICATE&& pred) const {
        return std::any_of(begin(), end(), std::forward<PREDICATE>(pred));
    }

    /// @returns true if the predicate function returns true for all the elements of the vector
    /// @param pred a function-like with the signature `bool(T)`
    template <typename PREDICATE>
    bool All(PREDICATE&& pred) const {
        return std::all_of(begin(), end(), std::forward<PREDICATE>(pred));
    }

  private:
    /// Friend class
    template <typename, size_t>
    friend class Vector;

    /// Friend class
    template <typename>
    friend class VectorRef;

    /// The data of the vector being referenced.
    void** data_ = nullptr;
    /// The length of the vector being referenced.
    size_t* len_ = nullptr;
    /// The capacity of the vector being referenced.
    size_t* cap_ = nullptr;
    /// Whether the data is passed by r-value reference, and can be moved.
    bool can_move_ = false;

    /// @returns the pointer to the first element in the vector
    void*& Data() { return *data_; }

    /// @returns the pointer to the first element in the vector
    void* const& Data() const { return *data_; }
};

/// Helper for converting a Vector to a std::vector.
/// @param vector the input vector
/// @return the converted vector
/// @note This helper exists to help code migration. Avoid if possible.
template <typename T, size_t N>
std::vector<T> ToStdVector(const Vector<T, N>& vector) {
    std::vector<T> out;
    out.reserve(vector.Length());
    for (auto& el : vector) {
        out.emplace_back(el);
    }
    return out;
}

/// Helper for constructing a Vector from a span.
template <size_t N, typename T>
auto ToVector(std::span<T> span) {
    using U = std::conditional_t<std::is_pointer_v<T> || std::is_reference_v<T>,
                                 std::remove_const_t<T>, T>;
    return Vector<U, N>{span};
}

/// Helper for converting a std::vector to a Vector.
/// @param vector the input vector
/// @return the converted vector
/// @note This helper exists to help code migration. Avoid if possible.
template <typename T, size_t N = 0>
Vector<T, N> ToVector(const std::vector<T>& vector) {
    Vector<T, N> out;
    out.Reserve(vector.size());
    for (auto& el : vector) {
        out.Push(el);
    }
    return out;
}

/// Prints the vector @p vec to @p o
/// @param o the stream to write to
/// @param vec the vector
/// @return the stream so calls can be chained
template <typename STREAM, typename T, size_t N>
    requires(traits::IsOStream<STREAM>)
auto& operator<<(STREAM& o, const Vector<T, N>& vec) {
    o << "[";
    bool first = true;
    for (auto& el : vec) {
        if (!first) {
            o << ", ";
        }
        first = false;
        o << el;
    }
    o << "]";
    return o;
}

/// Prints the vector @p vec to @p o
/// @param o the stream to write to
/// @param vec the vector reference
/// @return the stream so calls can be chained
template <typename STREAM, typename T>
    requires(traits::IsOStream<STREAM>)
auto& operator<<(STREAM& o, VectorRef<T> vec) {
    o << "[";
    bool first = true;
    for (auto& el : vec) {
        if (!first) {
            o << ", ";
        }
        first = false;
        o << el;
    }
    o << "]";
    return o;
}

}  // namespace tint

TINT_END_DISABLE_WARNING(UNSAFE_BUFFER_USAGE);

#endif  // SRC_TINT_UTILS_CONTAINERS_VECTOR_H_
