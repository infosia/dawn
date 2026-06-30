# CLAUDE.md

## Active context: Tint TBDR fork (`feature/tiled`)

This checkout is a **fork of Dawn/Tint** (C++), branch `feature/tiled`. The work
extends Tint's **backend coverage** for tile-based deferred rendering (TBDR)
shader features — subpass inputs (`input_attachment` / `inputAttachmentLoad`) and
framebuffer fetch (`@color`) — on the **MSL** and **SPIR-V** backends. The core
WGSL concepts already exist upstream; this fork is closing backend gaps, not
adding new language surface.

- **Design source of truth:** `TILED.md` (repo root). Read it before coding.
- **Active task handoff:** `HANDOFF.md` (repo root), when present. It is
  git-ignored — a scratch contract between the orchestrator and the coding
  agent, not a committed artifact.

## Roles

Claude **plans and orchestrates**: authors `TILED.md` and task handoffs, emits
handoffs to a **separate coding agent** (claude or codex), reviews the agent's
diff against `TILED.md`, drives builds/tests, runs the Phase Review, and manages
git (`add`/`commit`). The coding agent implements, builds, tests, and reports
back; it does not change scope or commit. Claude may make small edits directly
but delegates substantial implementation.

## Scope

- **In scope:** MSL (Metal) and SPIR-V (Vulkan) backend support for
  `input_attachment` / framebuffer-fetch / MSAA subpass inputs.
- **Out of scope:** the GLSL and HLSL backends; depth/stencil-aspect subpass
  inputs; MSL MSAA subpass input (Metal exposes the current sample via
  `[[sample_id]]`, not an explicit index). Document deferrals; do not silently
  widen scope.

## Fork discipline (keep upstream rebases mechanical)

The branch must stay **easy to rebase on upstream Dawn**. Because
`input_attachment` and `@color` already exist upstream, extending them means
in-place edits to upstream-shared files — so the mitigation is isolation and
grep-ability, not wrapper types.

1. **Marker-fence every edit to an upstream-shared file:**
   `// tint-tbdr: begin <tag>` … `// tint-tbdr: end <tag>`.
   `git grep "tint-tbdr:"` must enumerate every divergence point.
2. **Bulk logic lives in fork-only new files** (e.g.
   `src/tint/lang/msl/writer/raise/input_attachment.{h,cc}`); shared files get
   only a `#include` + one wiring line.
3. **Additive `Options` fields only** — no wrapper option structs. Default-
   initialized C++ fields are non-breaking and match upstream precedent
   (`pixel_local_attachments`, `multisampled_framebuffer_fetch`).
4. **Generated code in separate mechanical commits.** Intrinsic tables
   (`core.def` → `./tools/run gen`) and `BUILD.{gn,cmake,bazel}` are generated;
   never hand-edit them, and keep their diffs out of hand-written commits.
5. **Sequence by separability:** land the most-isolated work first (MSL, almost
   entirely new files) before the more-divergent work (SPIR-V MSAA, which needs
   a `core.def` overload + in-place `builtin_polyfill.cc` edits).

## Engineering rules

- **No panics / no new ICEs on normal paths.** Unsupported shader shapes return
  `Failure(...)`, not `TINT_ICE` / `TINT_UNREACHABLE` / `unreachable`. Genuine-
  invariant `TINT_IR_ASSERT` checks are fine. A backend that previously rejected
  a construct with a clean `Failure` must not regress to an ICE.
- **Documentation, comments, identifiers: English. Chat with the operator:
  Japanese.**
- **Phase Review (mandatory).** End each phase with a fresh, no-context subagent
  reviewing the cumulative diff against `TILED.md` and the fork rules, emitting
  `CRITICAL` / `MAJOR` / `MINOR` findings. Fix in severity order; a phase cannot
  be COMPLETE with an open CRITICAL or MAJOR. Prefer one hand-written commit per
  phase plus a separate mechanical commit for generated output.

## Build & test (this repo)

C++ via **CMake** (`out/Release`, Unix Makefiles generator). Tint unit tests are
a CMake target; the test binary is `out/Release/tint_unittests`.

```
# Build the unittests (discover the exact target name if unsure):
cmake --build out/Release --target help
cmake --build out/Release --target tint_cmd_test_test_cmd -j 1

# Run, filtering by test name:
./out/Release/tint_unittests --gtest_filter='*InputAttachment*'
./out/Release/tint_unittests --gtest_filter='*Msl*'
```

- **CRITICAL — build with `-j 1` only.** Compiling Dawn/Tint with high
  parallelism exhausts system memory and **hangs the OS** on this machine.
  Always pass `-j 1` to every `cmake --build` / `make` / `ninja` invocation.
  Never use bare `-j`, `-j$(nproc)`, or any value > 1.
- **Regenerating BUILD / intrinsic tables.** `./tools/run gen` regenerates the
  `BUILD.*` file lists (from a directory scan) and intrinsic tables (from
  `core.def`). Run it after adding/renaming source files or editing `core.def`.
  It needs Go + network; the Go build cache lives outside the sandbox, which is
  the one accepted reason to disable the Bash sandbox for that command.
- **Metal device tests.** GPU-backed MSL writer tests fail with
  `MTLCreateSystemDefaultDevice returned null` under the sandbox (no Metal
  device in spawned children). Those failures are environmental, not
  regressions; the IR-transform and writer-IR tests run device-free.

## Git / repo hygiene

- Commit only when the operator asks. Keep hand-written and generated
  (`BUILD.*`, gen output) changes in separate commits.
- `HANDOFF.md` / `REPORT.md` are git-ignored scratch files.

**Pre-commit hygiene check (mandatory).** Before every commit, scan the staged
changes — including code, comments, docs, and the commit message — and **abort
the commit** if any of the following appear. Fix or remove them first.

- **Credentials / signing material / device-specific secrets:** API keys,
  tokens, passwords, private keys, Apple Team IDs or signing identities, device
  UDIDs, provisioning-profile UUIDs, keystore aliases.
- **Local path strings:** absolute paths under a user home directory and
  **sibling `../` references to other local clones** (e.g. a `../../<sibling>`
  path to a neighbouring project checkout). Refer to other projects conceptually
  by name, never by a filesystem path. In-repo, repo-relative source paths
  (`src/tint/lang/...`) are fine.
- **Personal information:** real names or personal email addresses in source or
  docs (the git author config is fine).

Run a quick grep for user-home absolute paths and `../..` sibling references
over the staged set as a backstop before committing, using judgement for
intentional rule-text examples.

## Shader frontend — Tint

Tint is Dawn's WGSL compiler. Pipeline: WGSL reader → resolver (validation) →
core IR → per-backend raise transforms (`src/tint/lang/<backend>/writer/raise/`)
→ backend printers. TBDR lowering lives in the raise stage: new fork-only
transforms plus minimal, marker-fenced wiring in the shared `raise.cc`,
`writer.cc`, and `common/options.h` of each backend. Unlike the parent project's
historical naga frontend, **here we edit Tint source directly** — there is no
"fix it in a shim only" rule.
