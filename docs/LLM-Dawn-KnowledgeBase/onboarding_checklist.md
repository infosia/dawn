# Dawn Project Onboarding Checklist (Architecture, Design, Implementation)

Use this checklist to ramp up on Dawn deeply and calibrate review style. Each phase has concrete deliverables we can track in PRs or notes.

## Phase 0 — Environment and signals
- [x] Sync deps and set up build (workspace tasks: `sync`, `setup build`).
  - Synced and rebased onto `origin/main`; GN available (2234). Generated GN debug config to `out/active`.
- [x] Build once (task: `build`); record build config and toolchain.
  - Built with Ninja (Windows, Python 3.12). Final artifact example: `tint_unittests.exe` linked.
- [x] Run a tiny smoke test (one end2end + one unit test); capture PASS/FAIL.
  - Ran subset of Tint unit tests: 567 passed in 56 ms.
  - Ran subset of Dawn unit tests: 54 passed, 1 skipped.

Deliverables:
- [x] Build notes (configs, toolchain, pitfalls). See `docs/dawn/build_notes.md`.
- [x] Minimal test log with PASS/FAIL summary. See `docs/dawn/test_log.md`.

## Phase 1 — Top-level docs pass (what/why)
- [x] Read `docs/dawn/overview.md` (overall architecture).
- [x] Read `docs/dawn/features/core_features_and_limits.md` (API surface/limits).
- [x] Read `docs/dawn/features/dawn_native.md` (native layer specifics).
- [x] Read `docs/dawn/testing.md` and `docs/dawn/debugging.md` (quality bars).
- [x] Read `docs/dawn/errors.md` (error model).

Deliverables:
- [x] One-page “Dawn in 60s” summary (terms, layers, data flow). See `docs/dawn/dawn_in_60s.md`.

## Phase 2 — Core code map (who/where)
- [x] Map key subsystems in `src/dawn/native`: Instance, Adapter, Device, Queue.
- [x] Map pipeline objects: ShaderModule, PipelineLayout, Render/ComputePipeline.
- [x] Map resource objects: Buffer, Texture, BindGroup, Sampler, Views.
- [x] Map command path: CommandEncoder → CommandBuffer → submission.
- [x] Identify validation and error propagation points.
- [x] Note major utilities: `src/dawn/common`, caching, threading primitives.

Deliverables:
- [x] Directory → responsibility map (1–2 lines per dir/class). See `docs/dawn/code_map.md`.
- [x] Class interaction sketch (API → Native → Backend). See `docs/dawn/interaction_map.md`.

## Phase 3 — Backends deep dive (how)
- [x] Vulkan: device/pipeline/layout creation path; queue submit; memory model.
- [x] D3D12: descriptor heaps, root signatures, barriers; immediate data plumbing.
- [x] Metal: pipeline state creation; resource/state translation.
- [x] Note backend-agnostic abstractions vs. per-backend overrides.

Deliverables:
- [x] Per-backend flow notes (init, resource, pipeline, submit). See `docs/dawn/backend_flows.md` (first pass).
- [x] Backend delta table (where behavior diverges and why). See `docs/dawn/backend_deltas.md` (first pass).

## Phase 4 — Wire and multi-process
- [x] Read Wire layer code and how calls serialize/deserialize.
- [x] Map what runs client vs. server; how mock layers fit in tests.

Deliverables:
- [x] Wire path diagram (client calls → wire → server → native). See `docs/dawn/wire_path.md`.

## Phase 5 — Tint and shader pipeline
- [x] Read `docs/tint/arch.md` and `docs/tint/layering.md`.
- [x] Trace: WGSL → Tint IR → backend codegen (SPIR-V / MSL / HLSL).
- [x] Understand where Dawn invokes Tint and how modules flow to backends.

Deliverables:
- [x] Shader pipeline summary (inputs, transforms, outputs, failure modes). See `docs/dawn/shader_pipeline.md`.

## Phase 6 — Validation, errors, and diagnostics
- [x] Catalog validation helpers (e.g., descriptor validation hotspots).
- [x] Error scopes, reporting, and ASSERT vs. runtime error policy.
- [x] Identify places to add `Validate…` helpers per cwallez patterns.

Deliverables:
- [x] Validation playbook (what to check where; helper candidates). See `docs/dawn/validation_playbook.md`.

## Phase 7 — Testing & WebGPU CTS
- [x] Read `docs/rolling_cts.md`; understand CTS integration.
- [x] Run a small CTS shard locally; note flakes/timeouts.
- [x] Phase 7: Run a tiny WebGPU CTS slice
  - Note: Used node runner with query `webgpu:api,operation,command_buffer,copyBufferToBuffer:single:*` (2/2 passed).
- [x] Survey end2end and unit test patterns (`DawnTest` utilities).

Deliverables:
- [x] “How to test” notes + a tiny golden test to replicate quickly. See `docs/dawn/how_to_test.md`.
- [x] Testing entry points and `DawnTest` patterns documented. See `docs/dawn/testing.md`.

## Phase 8 — Performance and memory
- [x] Identify caches (pipeline/resource) and invalidation. (anchors: `Device::GetOrCreatePipelineCache`, `BlobCache`, `CacheKey`, `CachedObject`) — see `docs/dawn/perf_memory.md`.
- [x] Resource lifetime rules, ref-counting, and sync boundaries. (anchors: `RefCounted`, `RefCountedWithExternalCount`, Vulkan `RefCountedVkHandle`, D3D12 `QueueD3D12` destruction) — see `docs/dawn/perf_memory.md`.
- [x] Known perf hot paths and mitigations (brief scan of perf docs/issues) — see `docs/dawn/perf_memory.md`.

Deliverables:
- [x] Perf checklist (do’s/don’ts when reviewing CLs). See `docs/dawn/perf_checklist.md`.
- [x] Perf & memory notes with code anchors. See `docs/dawn/perf_memory.md`.

## Phase 9 — Contribution workflow & style calibration
- [x] Read `CONTRIBUTING.md`, `PRESUBMIT.py`, OWNERS patterns.
- [x] Sample 3–5 representative CLs by `cwallez@chromium.org`:
  - [x] Extract themes beyond nits (API design, validation factoring, test hygiene). (seeded from `gerrit_comments_report.md`)
  - [x] Contrast with current style guide; update where wrong/incomplete. (updated with CLs 2180, 3540, 4860, 4900, 4940, 5280)

Deliverables:
- [x] Reviewer style captured with actionable checklist. See `docs/reviewer_style_cwallez.md` (now with concrete sections 6–10 referencing specific CLs).
- [x] Add concrete code examples from sampled CLs. See `docs/reviewer_style_cwallez.md`.
- [x] “Before vs After” delta log for style calibration. See `docs/dawn/review_style_delta_log.md`.

## Phase 10 — Hands-on confirmation
- [x] Implement a very small, low-risk change (validation wording tweak + new test).
  - Tweaked B2B alignment error messages in `CommandEncoder.cpp` for clarity.
  - Added unit test `CopyCommandTest_B2B.ZeroSizeStillRequiresAlignedOffsets`.
- [x] Run presubmit and a CTS shard; fix issues.
  - Built `dawn_unittests` and ran the new test + nearby tests (all PASS locally).
  - CTS tiny shard previously PASS; no regressions expected (buffer copy path untouched functionally).
- [x] Do a self “cwallez-style” review on the change.
  - Focused on precise diagnostics, no behavior change, minimal surface area, test coverage added.

Deliverables:
- [x] Small CL + tests; self-review annotated against the style guide.

## Phase 11 — Reference artifacts for future reviews
- [x] Architecture map (PNG/SVG) checked into `docs/dawn/` (`docs/dawn/arch_map.svg`).
- [x] Glossary (Dawn/Tint/WebGPU terms). See `docs/dawn/glossary.md`.
- [x] Quick reviewer checklist (1‑pager) for common pitfalls per backend). See `docs/dawn/review_checklist.md`.

Deliverables:
- [x] `docs/dawn/arch_map.(svg|png)`, `docs/dawn/glossary.md`, `docs/dawn/review_checklist.md`.
