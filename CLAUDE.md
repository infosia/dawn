# CLAUDE.md

## Project

Dawn: Google's open-source WebGPU implementation (`webgpu.h`). C++20 codebase.
Two main components: **Dawn** (WebGPU runtime) and **Tint** (WGSL shader compiler).
GPU backends: D3D12, D3D11, Vulkan, Metal, OpenGL.
Build system: GN (primary) + CMake. Toolchain: `depot_tools`, `gclient`, Ninja.
Code review: Gerrit at https://dawn-review.googlesource.com

## Structure

- `src/dawn/native/` — core WebGPU implementation (backend-agnostic)
- `src/dawn/native/d3d12/`, `vulkan/`, `metal/`, `opengl/` — per-backend implementations
- `src/dawn/native/d3d/` — shared D3D code (used by both D3D11 and D3D12)
- `src/dawn/wire/` — client-server serialization layer
- `src/dawn/tests/end2end/` — GPU end-to-end tests
- `src/dawn/tests/unittests/` — unit tests (no GPU required)
- `src/tint/` — WGSL compiler (read `docs/tint/style_guide.md` before working on Tint code)
- `include/` — public C/C++ headers
- `generator/` — code generation from `dawn.json`

## Build and Test

```
gn gen out/Debug
ninja -C out/Debug
out/Debug/dawn_unittests
out/Debug/dawn_end2end_tests
git cl presubmit
```

## Coding Conventions

Follow Chromium C++ style guide. Formatting enforced by `clang-format`.
- Backend-specific files use suffix: `FooD3D12.cpp`, `FooVk.cpp`, `FooMtl.cpp`
- Error propagation: `MaybeError` / `ResultOrError<T>` with `DAWN_TRY` / `DAWN_TRY_ASSIGN` — never throw exceptions
- Assertions: `DAWN_ASSERT`, `DAWN_UNREACHABLE` — never use `assert()`
- Tint: `TINT_ICE`, `TINT_UNREACHABLE`, no RTTI (use `Castable::As<T>()`), use `kind` not `type` for non-WGSL distinctions
- Include guards: `#ifndef SRC_DAWN_NATIVE_D3D12_DEVICED3D12_H_` format
- Upload CLs: `git cl upload --bypass-hooks -f` (preferred over raw `git push`). If auth expires, ask the user to run `git credential-luci reauth` and retry.
- Push changes: `git push origin HEAD:refs/for/main` (alternative to `git cl upload`)

### Reference Implementations

When writing new code, match the style of these files:
- Error handling: `src/dawn/native/Device.cpp` (MaybeError + DAWN_TRY pattern)
- End-to-end test: `src/dawn/tests/end2end/BasicTests.cpp`
- Validation test: `src/dawn/tests/unittests/validation/BufferValidationTests.cpp`

## Workflow

- One commit per task. Follow-up iterations use `git commit --amend --no-edit`. `git cl format` runs automatically after each amend — do not run it manually.
- Processes that repeat 3 times should be consolidated into a Skill.

## Review Intelligence

`.claude/reviews/` contains distilled reviewer feedback (`preferred_designs.md`, `review_patterns.md`, `self_review_checklist.md`). Read them during planning and before submitting code.

## Common Errors

- Do not use `try/catch` for error handling. Use `MaybeError` + `DAWN_TRY`.
- Do not modify shared code without checking all downstream backends for callers (`native/` affects all backends; `native/d3d/` affects both D3D11 and D3D12).
- Do not modify generated files directly. Change `dawn.json` and regenerate.
- Do not add tests to `end2end/` that can be tested as `unittests/` — prefer unit tests when no GPU is needed.
- Do not write over-complicated tests with unnecessary resources. Keep test setups minimal — only what's needed to exercise the specific behavior.
- Do not add behavior changes without corresponding test coverage (both success and error paths).
- Do not use `std::pair<T, T>` when both fields have different meanings — use a named struct.
- Do not forget `git cl presubmit` before uploading CLs.
