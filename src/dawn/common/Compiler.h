// Copyright 2017 The Dawn & Tint Authors
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

#ifndef SRC_DAWN_COMMON_COMPILER_H_
#define SRC_DAWN_COMMON_COMPILER_H_

#include "src/utils/compiler.h"

// Defines macros for compiler-specific functionality

// DAWN_UNUSED_FUNC
//
// Prevents unused variable/expression warnings on FUNC.
#define DAWN_UNUSED_FUNC(FUNC) static_assert(sizeof(&FUNC) == sizeof(void (*)()))

// DAWN_FORCE_INLINE
//
// Annotate a function indicating it should really never be inline, even in debug mode.
#if DAWN_COMPILER_IS(CLANG) && defined(NDEBUG) && DAWN_HAS_CPP_ATTRIBUTE(clang::always_inline)
#define DAWN_FORCE_INLINE [[clang::always_inline]] inline
#elif DAWN_COMPILER_IS(GCC) && defined(NDEBUG) && DAWN_HAS_ATTRIBUTE(always_inline)
#define DAWN_FORCE_INLINE inline __attribute__((always_inline))
#elif DAWN_COMPILER_IS(MSVC) && defined(NDEBUG)
#define DAWN_FORCE_INLINE __forceinline
#else
#define DAWN_FORCE_INLINE inline
#endif

// DAWN_TRIVIAL_ABI
//
// Marks a type as being eligible for the "trivial" ABI despite having a
// non-trivial destructor or copy/move constructor. Such types can be relocated
// after construction by simply copying their memory, which makes them eligible
// to be passed in registers. The canonical example is std::unique_ptr.
//
// Use with caution; this has some subtle effects on constructor/destructor
// ordering and will be very incorrect if the type relies on its address
// remaining constant. When used as a function argument (by value), the value
// may be constructed in the caller's stack frame, passed in a register, and
// then used and destructed in the callee's stack frame. A similar thing can
// occur when values are returned.
//
// TRIVIAL_ABI is not needed for types which have a trivial destructor and
// copy/move constructors, such as base::TimeTicks and other POD.
//
// It is also not likely to be effective on types too large to be passed in one
// or two registers on typical target ABIs.
//
// See also:
//   https://clang.llvm.org/docs/AttributeReference.html#trivial-abi
//   https://libcxx.llvm.org/docs/DesignDocs/UniquePtrTrivialAbi.html
#if DAWN_COMPILER_IS(CLANG) && DAWN_HAS_ATTRIBUTE(trivial_abi)
#define DAWN_TRIVIAL_ABI [[clang::trivial_abi]]
#else
#define DAWN_TRIVIAL_ABI
#endif

// DAWN_CONSTINIT
//
// Requires constant initialization. See constinit in C++20. Allows to rely on a variable being
// initialized before execution, and not requiring a global constructor.
#if DAWN_HAS_ATTRIBUTE(require_constant_initialization)
#define DAWN_CONSTINIT __attribute__((require_constant_initialization))
#else
#define DAWN_CONSTINIT
#endif

// DAWN_GSL_POINTER
//
// In [lifetime safety], If you write a custom class wrapping a pointer, the [[gsl::Owner]] and
// [[gsl::Pointer]] can help the compiler to know if it acquired ownership over the pointee, or not.
// The compiler is then able to emit useful warning.
//
// [lifetime safety]: https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1179r0.pdf
#if DAWN_COMPILER_IS(CLANG)
#define DAWN_GSL_POINTER [[gsl::Pointer]]
#else
#define DAWN_GSL_POINTER
#endif

// DAWN_CONSTEXPR_DTOR
//
// Constexpr destructors were introduced in C++20. Dawn's minimum supported C++ version is C++17.
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201907L
#define DAWN_CONSTEXPR_DTOR constexpr
#else
#define DAWN_CONSTEXPR_DTOR
#endif

// DAWN_ATTRIBUTE_RETURNS_NONNULL
//
// Tells the compiler that a function never returns a null pointer. Sourced from Abseil's
// `attributes.h`.
#if DAWN_HAS_ATTRIBUTE(returns_nonnull)
#define DAWN_ATTRIBUTE_RETURNS_NONNULL __attribute__((returns_nonnull))
#else
#define DAWN_ATTRIBUTE_RETURNS_NONNULL
#endif

// DAWN_ENABLE_STRUCT_PADDING_WARNINGS
//
// Tells the compiler to emit a warning if the structure has any padding.
// This is helpful to avoid uninitialized bits in cache keys or other similar structures.
#if DAWN_COMPILER_IS(CLANG)
#define DAWN_ENABLE_STRUCT_PADDING_WARNINGS \
    _Pragma("clang diagnostic push") _Pragma("clang diagnostic error \"-Wpadded\"")
#define DAWN_DISABLE_STRUCT_PADDING_WARNINGS _Pragma("clang diagnostic pop")
#elif DAWN_COMPILER_IS(GCC)
#define DAWN_ENABLE_STRUCT_PADDING_WARNINGS \
    _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic error \"-Wpadded\"")
#define DAWN_DISABLE_STRUCT_PADDING_WARNINGS _Pragma("GCC diagnostic pop")
#elif DAWN_COMPILER_IS(MSVC)
#define DAWN_ENABLE_STRUCT_PADDING_WARNINGS __pragma(warning(push)) __pragma(warning(error : 4820))
#define DAWN_DISABLE_STRUCT_PADDING_WARNINGS __pragma(warning(pop))
#else
#define DAWN_ENABLE_STRUCT_PADDING_WARNINGS
#define DAWN_DISABLE_STRUCT_PADDING_WARNINGS
#endif

// DAWN_MUTEX_CAPABILITY
//
// Used when a class provides a mutex capability for thread-safety analysis.
//
// https://clang.llvm.org/docs/ThreadSafetyAnalysis.html#capability-string
#if DAWN_HAS_ATTRIBUTE(capability)
#define DAWN_MUTEX_CAPABILITY __attribute__((capability("mutex")))
#else
#define DAWN_MUTEX_CAPABILITY
#endif

// DAWN_SCOPED_LOCKABLE
//
// Used when a class implements RAII-style locking.
//
// https://clang.llvm.org/docs/ThreadSafetyAnalysis.html#scoped-capability
#if DAWN_HAS_ATTRIBUTE(scoped_lockable)
#define DAWN_SCOPED_LOCKABLE __attribute__((scoped_lockable))
#else
#define DAWN_SCOPED_LOCKABLE
#endif

// DAWN_EXCLUSIVE_LOCK_FUNCTION
//
// Used when a function acquires a lock. The lock must not be in the acquired state at the beginning
// of the function and must be in the acquired state at the end of the function.
//
// https://clang.llvm.org/docs/ThreadSafetyAnalysis.html#acquire-acquire-shared-release-release-shared-release-generic
#if DAWN_HAS_ATTRIBUTE(exclusive_lock_function)
#define DAWN_EXCLUSIVE_LOCK_FUNCTION __attribute__((exclusive_lock_function()))
#else
#define DAWN_EXCLUSIVE_LOCK_FUNCTION
#endif

// DAWN_UNLOCK_FUNCTION
//
// Used when a function releases a lock. The lock must be in the acquired state at the beginning of
// the function and must be in the released state at the end of the function.
//
// https://clang.llvm.org/docs/ThreadSafetyAnalysis.html#acquire-acquire-shared-release-release-shared-release-generic
#if DAWN_HAS_ATTRIBUTE(unlock_function)
#define DAWN_UNLOCK_FUNCTION __attribute__((unlock_function()))
#else
#define DAWN_UNLOCK_FUNCTION
#endif

// DAWN_NO_THREAD_SAFETY_ANALYSIS
//
// Used when a function is too complicated for the thread safety static
// analysis, and the compiler generates spurious warnings. Use this sparingly.
// For example, Clang does not know how to analyze conditionally-held locks, and
// may generate spurious warnings.
//
// Place this macro just before the opening brace of the function *definition*.
// Do not put it at the declaration.
//
// https://clang.llvm.org/docs/ThreadSafetyAnalysis.html#no-thread-safety-analysis
#if DAWN_HAS_ATTRIBUTE(no_thread_safety_analysis)
#define DAWN_NO_THREAD_SAFETY_ANALYSIS __attribute__((no_thread_safety_analysis))
#else
#define DAWN_NO_THREAD_SAFETY_ANALYSIS
#endif

// Annotates a function or class data member indicating it can lead to
// out-of-bounds accesses (OOB) if given incorrect inputs.
//
// For functions, this commonly includes functions which take pointers, sizes,
// iterators, sentinels, etc. and cannot fully check their preconditions (e.g.
// that the provided pointer actually points to an allocation of at least the
// provided size). Useful to diagnose potential misuse via
// `-Wunsafe-buffer-usage`, as well as to mark functions potentially in need of
// safer alternatives.
//
// For fields, this would be used to annotate both pointer and size fields that
// have not yet been converted to a span.
//
// All functions or fields annotated with this macro should come with a
// `// PRECONDITIONS: ` comment that explains what the caller must guarantee
// to ensure safe operation. Callers can then write `// SAFETY: ` comments
// explaining why the specific preconditions have been met.
//
// Ideally, unsafe functions should also be paired with a safer version, e.g.
// one that replaces pointer parameters with `span`s; otherwise, document safer
// replacement coding patterns callers can migrate to.
//
// Annotating a function `DAWN_UNSAFE_BUFFER_USAGE` means all call sites (that do not
// disable the warning) must wrap calls in `DAWN_UNSAFE_BUFFERS()`; see documentation
// there. Annotating a field `DAWN_UNSAFE_BUFFER_USAGE` means that `DAWN_UNSAFE_BUFFERS()`
// must wrap expressions that mutate of the field.
//
// See also:
//   https://chromium.googlesource.com/chromium/src/+/main/docs/unsafe_buffers.md
//   https://clang.llvm.org/docs/SafeBuffers.html
//   https://clang.llvm.org/docs/DiagnosticsReference.html#wunsafe-buffer-usage
//
// Usage:
// ```
//   // Calls to this function must be wrapped in `DAWN_UNSAFE_BUFFERS()`.
//   DAWN_UNSAFE_BUFFER_USAGE void Func(T* input, T* end);
//
//   struct S {
//     // Changing this pointer requires `DAWN_UNSAFE_BUFFERS()`.
//     DAWN_UNSAFE_BUFFER_USAGE int* p;
//   };
// ```
#if DAWN_HAS_CPP_ATTRIBUTE(clang::unsafe_buffer_usage)
#define DAWN_UNSAFE_BUFFER_USAGE [[clang::unsafe_buffer_usage]]
#else
#define DAWN_UNSAFE_BUFFER_USAGE
#endif

// Annotates code indicating that it should be permanently exempted from
// `-Wunsafe-buffer-usage`. For temporary cases such as migrating callers to
// safer patterns, use `DAWN_UNSAFE_TODO()` instead; see documentation there.
//
// All calls to functions annotated with `DAWN_UNSAFE_BUFFER_USAGE` must be marked
// with one of these two macros; they can also be used around pointer
// arithmetic, pointer subscripting, and the like.
//
// ** USE OF THIS MACRO SHOULD BE VERY RARE.** Using this macro indicates that
// the compiler cannot verify that the code avoids OOB, and manual review is
// required. Even with manual review, it's easy for assumptions to change and
// security bugs to creep in over time. Prefer safer patterns instead.
//
// Usage should wrap the minimum necessary code, and *must* include a
// `// SAFETY: ...` comment that explains how the code guarantees safety or
// meets the requirements of called `DAWN_UNSAFE_BUFFER_USAGE` functions. Guarantees
// must be manually verifiable by the Chrome security team using only local
// invariants; contact security@chromium.org to schedule such a review. Valid
// invariants include:
// - Runtime conditions or `CHECK()`s nearby
// - Invariants guaranteed by types in the surrounding code
// - Invariants guaranteed by function calls in the surrounding code
// - Caller requirements, if the containing function is itself annotated with
//   `DAWN_UNSAFE_BUFFER_USAGE`; this is less safe and should be a last resort
//
// See also:
//   https://chromium.googlesource.com/chromium/src/+/main/docs/unsafe_buffers.md
//   https://clang.llvm.org/docs/SafeBuffers.html
//   https://clang.llvm.org/docs/DiagnosticsReference.html#wunsafe-buffer-usage
//
// Usage:
// ```
//   // The following call will not trigger a compiler warning even if `Func()`
//   // is annotated `DAWN_UNSAFE_BUFFER_USAGE`.
//   return DAWN_UNSAFE_BUFFERS(Func(input, end));
// ```
//
// Test for `__clang__` directly, as there's no `__has_pragma` or similar (see
// https://github.com/llvm/llvm-project/issues/51887).
#if defined(__clang__)
// Disabling `clang-format` allows each `_Pragma` to be on its own line, as
// recommended by https://gcc.gnu.org/onlinedocs/cpp/Pragmas.html.
// clang-format off
#define DAWN_UNSAFE_BUFFERS(...)                  \
  _Pragma("clang unsafe_buffer_usage begin") \
  __VA_ARGS__                                \
  _Pragma("clang unsafe_buffer_usage end")
// clang-format on
#else
#define DAWN_UNSAFE_BUFFERS(...) __VA_ARGS__
#endif

// Annotates code indicating that it should be temporarily exempted from
// `-Wunsafe-buffer-usage`. While this is functionally the same as
// `DAWN_UNSAFE_BUFFERS()`, semantically it indicates that this is for migration
// purposes, and should be cleaned up as soon as possible.
//
// Usage:
// ```
//   // The following call will not trigger a compiler warning even if `Func()`
//   // is annotated `DAWN_UNSAFE_BUFFER_USAGE`.
//   return DAWN_UNSAFE_TODO(Func(input, end));
// ```
#define DAWN_UNSAFE_TODO(...) DAWN_UNSAFE_BUFFERS(__VA_ARGS__)

#endif  // SRC_DAWN_COMMON_COMPILER_H_
