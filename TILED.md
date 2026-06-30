# Tint TBDR (Tiled / subpass-input) Backend Coverage — Port Plan

This document is the design/port plan for bringing the shader-compiler half of
the wgpu `feature/tiled` (tile-based deferred rendering, TBDR) work to Dawn's
**Tint** WGSL compiler, on branch `feature/tiled`.

The reference implementation is a separate wgpu fork (branch `feature/tiled`),
where the shader half lives in `naga`. Tint is Dawn's analog of naga, so
"introduce TBDR to Tint" is about the shader-compiler surface: subpass inputs,
framebuffer fetch, and per-backend lowering.

---

## Context

Investigation showed **Tint already has most of the core concepts**:

- `chromium_internal_input_attachments`: `input_attachment<T>` +
  `inputAttachmentLoad()` + `@input_attachment_index(N)` — works on **SPIR-V**.
- `chromium_experimental_framebuffer_fetch`: `@color(N)` attribute — works on
  **SPIR-V** and **MSL**.

The real gap is **backend coverage**, not new language concepts. Scope is
limited to the **MSL (Metal)** and **SPIR-V (Vulkan)** backends. **GLSL and HLSL
are out of scope.** Intended outcome: a Metal TBDR path that consumes input
attachments, plus MSAA subpass-input support on Vulkan.

### Scope decisions (confirmed)
- **In scope:** MSL `input_attachment` support; SPIR-V MSAA `input_attachment`.
- **Out of scope (documented deferrals):** GLSL/HLSL backends; depth/stencil
  aspect subpass inputs; MSL MSAA subpass input (Metal delivers the current
  sample via `[[sample_id]]`, not an explicit index — separate future item).

---

## Guiding principle: minimize & isolate upstream divergence

This fork (branch `feature/tiled`) must stay **easy to rebase on upstream Dawn**.
Same intent as wgpu's `TILED.md`, adapted to a key difference:

> **wgpu's TBDR features were brand-new**, so nearly everything lived in
> fork-only files. **In Dawn, `input_attachment` and `@color` already exist
> upstream**, so extending them is inherently *in-place edits to upstream-shared
> Tint files*. The mitigation is therefore NOT wgpu-style wrapping types — it is
> (a) push all real logic into new fork-only files, (b) keep every shared-file
> edit minimal and purely additive, and (c) fence every shared-file edit with
> grep-able markers.

Conventions (apply to all phases):

1. **Marker comments** around every edit to an upstream-shared file:
   ```cpp
   // tint-tbdr: begin <short-tag>
   ... fork code ...
   // tint-tbdr: end <short-tag>
   ```
   `git grep "tint-tbdr:"` then enumerates every divergence point in seconds.
2. **Bulk logic in fork-only new files** — the MSL transform
   (`raise/input_attachment.{h,cc}`) holds the substance; shared files get only
   a `#include` + one wiring line.
3. **Additive Options fields, not wrapping.** Adding a default-initialized field
   to a Tint `Options` struct is non-breaking in C++ and matches upstream's own
   precedent (`pixel_local_attachments`, `multisampled_framebuffer_fetch`). Do
   **not** introduce wrapper option structs (the wgpu `Limits`→`TiledCapabilities`
   pattern solves a Rust struct-literal problem that does not exist here).
4. **Generated code in separate mechanical commits.** `core.def` edits regenerate
   large intrinsic tables; keep that diff isolated from hand-written logic.
5. **Sequence by separability.** Land the most-isolated work first (MSL, almost
   entirely new files) before the more-divergent work (SPIR-V MSAA, which needs a
   `core.def` overload + in-place `builtin_polyfill.cc` edits).

---

## Current-state grounding (verified file references)

- MSL rejects input attachments: `src/tint/lang/msl/writer/writer.cc:65-67`
  (`Failure("input_attachment not supported by the MSL backend")`).
- MSL already emits `[[color(N)]]` from `IOAttributes::color`:
  `src/tint/lang/msl/writer/printer/printer.cc:1754-1755`.
- SPIR-V `@color` builds a **multisampled** SubpassData image + sample-indexed
  `OpImageRead`: `src/tint/lang/spirv/writer/raise/shader_io.cc:204-209,296-331`
  gated by `Options::multisampled_framebuffer_fetch`
  (`spirv/.../common/options.h:257`).
- SPIR-V `input_attachment` lowering is **single-sampled only**:
  type at `src/tint/lang/spirv/writer/raise/builtin_polyfill.cc:134-137`,
  `InputAttachmentLoad()` at `builtin_polyfill.cc:1267-1293` (reads coord (0,0),
  no sample operand).
- Reusable binding plumbing: `tint::Bindings::input_attachment`
  (`src/tint/api/common/bindings.h:90`, a `BindingMap`), present in both MSL and
  SPIR-V options. Precedent map: MSL `Options::pixel_local_attachments`
  (`src/tint/lang/msl/writer/common/options.h:218`). naga analog:
  `metal::Options::subpass_color_slots`.
- `input_attachment<T>` type: `src/tint/lang/core/type/input_attachment.h`
  (color-only, `T ∈ {f32,i32,u32}`). Builtin/type defined in
  `src/tint/lang/core/core.def` (`type input_attachment<T>` ~352;
  `inputAttachmentLoad` ~1596).

---

## Design overview

Metal has **no texture type for a subpass input**. Both naga and Tint's existing
`@color` path surface it as a `[[color(N)]]` fragment argument carrying the
already-resolved `vec4<T>`. So the MSL job is to **lower the handle-based
`input_attachment` mechanism into the IO-based `@color` representation**, then
reuse the unchanged `[[color(N)]]` emission. The SPIR-V job is to **extend the
existing `input_attachment` lowering with the MSAA path already proven for
`@color`**.

> **Sequencing rationale (per the separability principle):** MSL leads because
> its substance lives in new fork-only files and its shared-file footprint is
> tiny. SPIR-V MSAA comes last because it requires a `core.def` overload (large
> generated-table diff) plus in-place edits to the existing `builtin_polyfill.cc`
> lowering — the largest divergence surface in this work.

---

## Phase 1 — MSL option + rejection-removal scaffolding (no behavior yet)

1. **New option** in `src/tint/lang/msl/writer/common/options.h`, mirroring
   `pixel_local_attachments` (fence with `// tint-tbdr:` markers):
   ```cpp
   /// Map from WGSL binding point of an input_attachment to its Metal [[color(N)]] slot.
   std::unordered_map<BindingPoint, uint32_t> input_attachment_to_color_index{};
   ```
   - Add to `TINT_REFLECT(Options, …)` (~options.h:244-264) — required for
     hashing/serialization/equality.
   - Mirror into the fuzzer options: `src/tint/lang/msl/writer/writer_fuzz.cc`.
2. **Remove rejection** `src/tint/lang/msl/writer/writer.cc:65-67` (marker-fenced).
   Add a guarded `CanGenerate` check that every referenced `input_attachment` var
   has a slot in the new map (clear Failure otherwise, like vertex-pulling checks
   at `writer.cc:176-188`).
3. Transform invocation is a no-op stub at this point (keeps the PR reviewable).

## Phase 2 — MSL lowering transform (the substance; mostly fork-only files)

New files, modeled on `src/tint/lang/msl/writer/raise/shader_io.{h,cc}`:
`src/tint/lang/msl/writer/raise/input_attachment.{h,cc,_test.cc}`.

Config: `struct InputAttachmentConfig { const std::unordered_map<BindingPoint,uint32_t>& binding_to_color; };`

Transform (per fragment entry point + transitively referenced module vars — reuse
`core::ir::ReferencedModuleVars`, already used at `writer.cc:141`):
1. Find module-scope `var`s whose store type is `core::type::InputAttachment`.
2. Resolve the Metal color slot via `var->BindingPoint()` in `binding_to_color`
   (ICE/Failure if absent).
3. Replace each `inputAttachmentLoad(x)` (`core::BuiltinFn::kInputAttachmentLoad`)
   tracing to that var with the `vec4<T>` color value — identity substitution
   since both are `vec4<T>`.
4. Surface the value as a fragment entry input with `IOAttributes{ .color = slot }`
   so the unchanged `printer.cc:1754` emits `[[color(N)]]`.
5. Remove the now-dead module var.

**Threading approach (the one real design choice — flag in review):**
`inputAttachmentLoad` may sit inside a helper that receives the attachment as a
param. **Use Option A:** run this transform **before `raise::ModuleScopeVars`**
(`raise.cc:277`) **and before `raise::ShaderIO`** (`raise.cc:228`); rewrite each
load to a plain `Load` of a `vec4<T>` color-tagged module var and let the
existing `ModuleScopeVars` thread it through helper params to the entry point,
where `ShaderIO` attaches `@color`. This maximizes reuse of existing infra
(the explicit goal). Wire into `src/tint/lang/msl/writer/raise/raise.cc` with the
`#include` + invocation passing `options.input_attachment_to_color_index` — a
single marker-fenced line, the only shared-file edit in this phase.

**Tests:** `input_attachment_test.cc` — single attachment → `[[color(N)]]` +
identity load; multiple → distinct slots; **load inside a helper function**
(validates Option A threading); f32/i32/u32; missing slot → Failure. Extend
`src/tint/lang/msl/writer/raise/shader_io_test.cc` with the derived color input.

**Alternative (Option B), if ModuleScopeVars coupling proves fragile:** do the
entire hoist+thread inside the self-contained transform by injecting a `vec4<T>`
param into every function on the call path from entry to the load site. More
code, no ordering dependence — but re-implements threading. Prefer Option A.

## Phase 3 — SPIR-V MSAA `input_attachment` (most divergent; land last)

Reuses the exact pattern already working for `@color`, but touches the largest
shared surface — so it lands after MSL and keeps its generated diff isolated.

1. **`core.def`** (~line 1596): add a 2-arg overload (marker-fenced)
   ```
   @stage("fragment") implicit(T: fiu32)
     fn inputAttachmentLoad(input_attachment: input_attachment<T>, sample_index: u32) -> vec4<T>
   ```
   Regenerating the intrinsic tables is a **separate mechanical commit**.
2. **New SPIR-V option** `bool multisampled_input_attachment = false;` in
   `src/tint/lang/spirv/writer/common/options.h` (next to
   `multisampled_framebuffer_fetch:257`), added to its `TINT_REFLECT` (~:291),
   threaded into the builtin-polyfill/ShaderIO config via
   `src/tint/lang/spirv/writer/raise/raise.cc`. All marker-fenced.
3. **Type lowering** `builtin_polyfill.cc:134-137`: set
   `ms = type::Multisampled::kMultisampled` when MSAA indicated (marker-fenced).
4. **Builtin lowering** `InputAttachmentLoad()` `builtin_polyfill.cc:1267-1293`:
   when MSAA, push `SpvImageOperandsSampleMask` + sample-index arg, mirroring
   `shader_io.cc:318-323` (marker-fenced).
5. **Tests:** extend `src/tint/lang/spirv/writer/raise/builtin_polyfill_test.cc`
   (multisampled overload → `OpImageRead` + Sample operand + multisampled
   SubpassData image).
6. Regenerate: `./tools/run gen` (see Tooling).

**Decision:** keep multisampled-ness off the *type* — gate on the 2-arg overload
+ option. Putting it on the type ripples through IR binary roundtrip and the
resolver (more divergence); the option approach mirrors
`multisampled_framebuffer_fetch` and is cheapest.

**Constraint (consequence of the option approach).** `multisampled_input_attachment`
is a single module-wide flag, so single-sampled and multisampled input
attachments cannot be mixed in one module — the same limitation as
`multisampled_framebuffer_fetch`. Because the 2-arg overload is resolvable
whenever the extension is enabled (independent of the option), the SPIR-V
writer's `CanGenerate` rejects an arity/option mismatch with a clean `Failure`
(2-arg call without the option, or the option set with a 1-arg call) so the
backend never ICEs on otherwise-valid WGSL.

## Phase 4 — e2e fixtures + docs

- Framebuffer-fetch fixtures: `test/tint/extensions/framebuffer_fetch/*/` (do NOT
  author HLSL/GLSL expectations — out of scope; incidental regen is fine).
- There are currently no e2e `input_attachment` `.wgsl` fixtures (it needs
  driver-supplied binding/color config). Add e2e fixtures only if the harness can
  inject `input_attachment_to_color_index` / the SPIR-V MSAA option; otherwise keep
  coverage at the unit/`writer`-test layer that sets `Options` directly. Mirror
  naga's `subpass-*` fixtures conceptually (single, multiple, MSAA).
- Regenerate expectations: `./tools/run tests` (`docs/tint/end-to-end-tests.md:21`).
- Document the color-only scope and the MSL-MSAA / depth-stencil deferrals.
- **Fork-conventions doc** (mirrors wgpu's `docs/tiled-fork-conventions.md`):
  record the `// tint-tbdr:` marker convention, the list of upstream-shared files
  touched, and each known divergence point, so a future rebase on upstream Dawn
  is a mechanical `git grep "tint-tbdr:"` sweep.

---

## Tooling / codegen

- `src/tint/lang/core/core.def` is consumed by the Go intrinsic generator. The new
  overload **requires** `./tools/run gen` (`docs/tint/gen.md:6`), which regenerates
  intrinsic tables (`src/tint/lang/core/intrinsic/data.cc`, etc.).
- New pass files + build targets: refresh `BUILD.gn`/`BUILD.cmake`/`BUILD.bazel`
  from `BUILD.cfg` via `./tools/run gen` (and `tools/run gen build` for fuzz wiring,
  `docs/tint/fuzzers.md:217`). Commit generated output as a **separate mechanical
  commit** so reviewers can diff hand-written vs generated.

## Architectural risks (flag in review)

1. **Color-slot data dependency (the naga parallel).**
   `input_attachment_to_color_index` must be populated by Dawn's Metal backend from
   the render-pass attachment layout at pipeline-compile time — exactly as naga's
   hal/metal populates `subpass_color_slots`. Tint cannot infer it. The Dawn-side
   change is a hard runtime dependency (outside this Tint task); without the map,
   MSL generation must fail cleanly via the Phase-1 `CanGenerate` check.
2. **ModuleScopeVars ordering coupling (Option A).** Making `ModuleScopeVars` treat
   a color-tagged module var as `[[color(N)]]` IO (not a Metal resource binding)
   is the integration seam; verify no collision with `ArgumentBuffers`
   (`raise.cc:240`) / `BindingRemapper`.
3. **Helper-function threading.** Verify with the explicit "load in helper" test.
4. **Type-vs-option for MSAA.** Resolved: option-based (avoids IR roundtrip churn).

## Verification

- Build Tint + run unit tests: the new `input_attachment_test.cc`,
  `builtin_polyfill_test.cc`, `shader_io_test.cc` additions
  (e.g. via the standard Tint test target / `./tools/run tests`).
- Manual smoke (MSL): compile a WGSL fragment shader using
  `enable chromium_internal_input_attachments; var ia: input_attachment<f32>; ...
  inputAttachmentLoad(ia)` through the MSL writer with
  `input_attachment_to_color_index` set, and confirm a `[[color(N)]]` argument +
  identity use appear in the emitted `.metal`.
- Manual smoke (SPIR-V MSAA): compile the 2-arg `inputAttachmentLoad(ia, sid)` with
  `multisampled_input_attachment = true` and confirm a multisampled SubpassData
  `OpTypeImage` + `OpImageRead` with a Sample operand in the SPIR-V disassembly.
- Run `./tools/run gen` and confirm a clean tree (no stale generated files); run
  the e2e harness `./tools/run tests` to regenerate/verify `.expected.*`.

## Critical files

- `src/tint/lang/msl/writer/common/options.h` — `input_attachment_to_color_index`.
- `src/tint/lang/msl/writer/writer.cc:65-67` — remove rejection, add validation.
- `src/tint/lang/msl/writer/raise/raise.cc` — wire new transform before
  ShaderIO/ModuleScopeVars.
- **New:** `src/tint/lang/msl/writer/raise/input_attachment.{h,cc,_test.cc}` —
  modeled on `raise/shader_io.cc` (holds the substance).
- `src/tint/lang/core/core.def` — new MSAA `inputAttachmentLoad` overload (regen).
- `src/tint/lang/spirv/writer/raise/builtin_polyfill.cc:134-137,1267-1293` — MSAA
  type + load.
- `src/tint/lang/spirv/writer/common/options.h` — `multisampled_input_attachment`.
