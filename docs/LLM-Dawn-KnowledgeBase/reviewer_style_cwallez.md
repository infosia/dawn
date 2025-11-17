# Reviewer style guide: cwallez@chromium.org

This guide distills recurring themes and patterns from `cwallez@chromium.org`’s review comments in `gerrit_comments_report.md` so we can mirror the review style when pre-reviewing CLs.

## TL;DR
- Align naming and structure with the WebGPU spec and Dawn conventions.
- Centralize validation in helper functions; assert invariants; keep API types precise.
- Prefer clear defaults (combo descriptors), minimal duplication, and small nits fixed early (sort, whitespace, consistent names).
- Keep tests up to date with validation and current APIs; remove obsolete builder-era tests.

## Core themes and expectations

1) Spec alignment and terminology
- Use WebGPU/Dawn names consistently (attachments state/descriptor, attachment descriptor, etc.).
- Avoid legacy “builder” terminology; prefer modern descriptors and combo descriptors.
- Keep shader stage, module, and entry-point naming consistent with spec (vertex/fragment, `ShaderModule*`, `const char*`).

2) API surface and types
- Prefer precise pointer types: `Device*`, `ShaderModule*`, `const char*`; avoid unnecessary wrappers or `ToBackend(...)` when not required.
- Forward declare where possible; avoid including headers unnecessarily.
- Don’t `std::move` raw pointers; compare pointers against `nullptr` directly.
- Set `nextInChain = nullptr` in all C-struct descriptors by default.

3) Validation and helpers
- Extract validation into helpers (e.g., `ValidateStageDescriptor`, `ValidateAttachmentsState`). Use `DAWN_TRY` when appropriate.
- Check renderability explicitly: `IsColorRenderableTextureFormat` / `IsDepthStencilRenderableTextureFormat` (and call `ValidateTextureFormat` first).
- Assume non-optional fields are present (avoid redundant branches) until error-handling lands; assert invariants (`ASSERT(...)`).
- Enforce sampleCount expectations (e.g., 1 for many cases) and consistent formats.

4) Code structure, defaults, and duplication
- Use `utils::ComboRenderPipelineDescriptor` to set sane defaults and reduce boilerplate.
- Group related configuration blocks (attachments, formats); keep initialization concise.
- Factor repeated logic into lambdas/helpers to avoid duplication.

5) Cleanup and lifetime
- Release temporary builder-like objects promptly after use (e.g., blend/input/depth-stencil builders in samples/tests).

6) Style nits (fix in-place)
- Sort includes/fields; remove extra spaces and empty lines; keep symmetrical formatting.
- Prefer clear names (`pipelineDescriptor`, `textureDescriptor`, `baseDescriptor`, etc.).
- Keep nit comments minimal by adhering to local style before upload.

7) Tests
- Update tests alongside code to reflect current validation (success and failure cases).
- Remove or rework obsolete builder-era tests; ensure success paths exist where failure is tested.
- In wire tests, set proper expectations and return mock objects; don’t disable tests in final patches.

## Emulation checklist (pre-submit)

Use this checklist to self-review CLs in the voice/style of `cwallez@chromium.org`:
- Spec naming: Do identifiers and structure names match WebGPU/Dawn terms? Any legacy/builder wording left?
- API types: Are `Device*`, `ShaderModule*`, `const char*` used appropriately? Forward declarations added? Any unnecessary `ToBackend(...)` or `std::move` of pointers?
- Defaults: Are combo descriptors used where helpful? Are defaults set in constructors to reduce boilerplate?
- Validation: Are there helpers like `ValidateStageDescriptor` / `ValidateAttachmentsState`? Are renderable format checks present? Redundant null checks removed? Assertions for invariants added?
- Structure/duplication: Is repeated logic factored (e.g., lambda reused for VS/FS)? Are related blocks grouped logically?
- Style: Includes/fields sorted; no stray whitespace/blank lines; consistent descriptor naming.
- Lifetime: Temporary builders/resources freed promptly in samples/tests.
- Tests: Updated for new validation; obsolete tests removed or rewritten; success cases covered; wire tests set expectations/mocks; nothing disabled in final patch.
- Commit scope: If some broad test updates are needed, consider a follow-up CL to avoid blocking landing.

## Example phrasings to emulate (tone and cadence)
- “nit: sort.” / “nit: no empty line.”
- “Can we move this into a ValidateX helper and `DAWN_TRY` here?”
- “This shouldn’t be needed; see comment at [1].”
- “This becomes just X once the constructor takes a Device*.”
- “Use IsColorRenderableTextureFormat / IsDepthStencilRenderableTextureFormat (validate first).”
- “Let’s remove this test; ‘missing property’ is a builder concept.”
- “Rename to pipelineDescriptor (and the other to textureDescriptor).”
- “We can assume this isn’t null.” / “Please compare against nullptr.”

## Notes
- Source: Aggregated from the `## cwallez@chromium.org` section in `gerrit_comments_report.md` (LGTM-only messages removed).
- Intent: Keep this living; refine as we see new patterns.

## Concrete examples (sampled)

1) Spec naming cleanup (CL 2180, dawn.json)
- Before: “attachment descriptor”, “attachments state descriptor”, inconsistent pluralization
- After: “attachment descriptor” → “attachment descriptor” (WebGPU term); “attachments state descriptor” → “attachments state”; use “num things” convention.
	- Ref: `gerrit_comments_report.md` → `## cwallez@chromium.org` comments on dawn.json lines 710, 908, 925, 877.

2) Descriptor defaults & Combo descriptors (CL 2180, examples)
- Before (verbose init):
	- SetDefaults is called manually; spread fields; extra local arrays; missing `nextInChain`.
- After (concise defaults):
	- Use `utils::ComboRenderPipelineDescriptor` with defaults, set entryPoints, one color attachment, depth-stencil format; set `nextInChain = nullptr` in all C structs.
	- Ref: comments on `examples/Animometer.cpp`, `CHelloTriangle.cpp`, `CppHelloTriangle.cpp` (group related blocks, remove extra spaces, release temp builders).

3) Validation factoring into helpers
- Before: Inline validation in multiple call sites; mixed assertions and errors.
- After: Extract `Validate*` helpers and call with `DAWN_TRY(...)` at the top of entry points.
	- Apply to stage descriptors, attachments/state, vertex formats, etc. (see `RenderPipeline.cpp` patterns with `DAWN_INVALID_IF` + helper calls).

4) Tests: remove builder-era patterns
- Before: Tests rely on “missing property” builder behavior or legacy naming.
- After: Descriptor-based tests with explicit success and failure cases; wire expectations where relevant; release temporaries promptly.

5) Pointers and types
- Before: Ambiguous pointer usage; unnecessary wrappers; inconsistent names.
- After: Prefer precise pointer types (`Device*`, `ShaderModule*`, `const char*`), forward-declare when possible; avoid `std::move` of raw pointers; compare against `nullptr`.

6) Extract backend utilities, de-duplicate across backends (CL 3540)
- Before: Duplicated conversion helpers (e.g., compare func) in multiple backend files.
- After: Introduce `UtilsD3D12.{h,cpp}` with clearer names (e.g., `ToD3D12ComparisonFunc`), remove duplicates in `RenderPipelineD3D12.cpp` and mirror for Metal/Vulkan.
	- Ref: CL 3540 comments on `src/dawn_native/d3d12/SamplerD3D12.cpp` and `UtilsD3D12.h` (naming, move impl to .cpp, factor all uses).

7) Name helpers precisely and document constraints (CL 4900)
- Before: Generic helper name for buffer copy alignment.
- After: Use a precise name (e.g., `ValidateB2BCopyAlignment`) and validate size/offsets are 4-byte aligned; add tests for unaligned size/offset.
	- Ref: CL 4900 notes on `CommandEncoder.cpp` (B2B in the name) and related tests in `CopyCommandsValidationTests.cpp`.

8) Comment quality and early-exit control flow (CL 4940)
- Before: Complex copy logic without guiding comments; nested else blocks.
- After: Add high-signal comments describing validator/workaround behavior; prefer “good case first + break” patterns; avoid redundant `else`.
	- Ref: CL 4940 comments on `metal/CommandBufferMTL.mm` (offset handling, last image copy, nit on comment formatting and `origin +=`).

9) Prune unsupported formats at the source (CL 5280)
- Before: Keep formats in `dawn.json` that aren’t universally supported.
- After: Remove them to avoid advertising unavailable capabilities; proceed as if group decided to remove.
	- Ref: CL 5280 comments on `dawn.json` (Metal/D3D12 unsupported formats and naming).

10) Small hygiene nits that keep patches clean (CL 4860)
- Before: Repeated computations and unnecessary calls inside loops.
- After: Hoist invariants out of loops; compute once; fix small arithmetic/field selection bugs.
	- Ref: CL 4860 comments across `CommandBufferMTL.mm` (offset vs dst.offset, update params once, handle “good case” first).
