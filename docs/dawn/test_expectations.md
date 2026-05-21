# Test Expectations File for `dawn_end2end_tests`

## Background

Before this change, `dawn_end2end_tests` had no out-of-source way to mark tests as
known-failing on specific hardware. The only mechanisms were two C++ macros:

- `DAWN_SUPPRESS_TEST_IF(condition)` — skip a test when the runtime condition
  matches; overridable with `--run-suppressed-tests`.
- `DAWN_TEST_UNSUPPORTED_IF(condition)` — skip a test when a required feature or
  toggle is missing.

These work, but every suppression requires a code change, a rebuild, and a code
review. As of the audit immediately preceding this work there were **845
`DAWN_SUPPRESS_TEST_IF` call sites across 89 test files**. The cost of adding,
removing, or triaging a single skip was disproportionately high.

The three other test suites in the broader WebGPU ecosystem all already use
text-based expectation files:

| Suite | Format | Reference |
|---|---|---|
| `angle_end2end_tests` | ANGLE's `GPUTestExpectationsParser` | `third_party/angle/src/tests/angle_end2end_tests_expectations.txt` |
| WebGPU CTS | catapult `typ` (`TaggedTestListParser`) | `webgpu-cts/expectations.txt` |
| WebGL CTS | same `typ` parser | (lives in Chromium) |

This change brings `dawn_end2end_tests` in line with those suites by introducing
`src/dawn/tests/dawn_end2end_tests_expectations.txt` and a runtime loader, then
migrates the bulk of existing in-code suppressions into the file.

## Benefits of an expectation file

1. **No rebuild to triage a flaky / failing test.** Edit the file, re-run.
2. **Mass triage at the bot / config level.** A single line can express
   "everything in `MultisampledRenderingTest` is broken on AMD + D3D12 + backend
   validation on Win 11," collapsing what used to be dozens of inline macros.
3. **Out-of-band tracking of regressions.** Tests marked `[ Failure ]` keep
   running; they get re-classified as "expected failure" or "unexpected pass"
   instead of disappearing from the run. This is what catches a fix landing
   silently for a previously-broken test.
4. **Detection of stale entries.** "Unused expectation" reporting flags lines
   that no longer match any test (e.g. after a test was renamed or deleted) so
   they can be pruned.
5. **Bug-ID provenance in one place.** Every entry carries a bug reference; the
   file becomes a searchable index of what's broken, where, and why.
6. **Easier code review for the suppression itself.** The diff for a
   one-line `.txt` change is unambiguous compared to wading through 89 `.cpp`
   files.
7. **Convergence with neighboring suites.** People who already know
   `angle_end2end_tests_expectations.txt` or `webgpu-cts/expectations.txt`
   immediately know how to read and edit ours.

## Design choices

### Format: typ-style (matching WebGPU/WebGL CTS), not ANGLE-style

Two reasonable options existed:

| Aspect | ANGLE format | typ format (chosen) |
|---|---|---|
| Example line | `crbug.com/1 D3D12 INTEL : Foo.Bar/* = SKIP` | `crbug.com/1 [ d3d12 intel ] Foo.Bar/* [ Skip ]` |
| Tag vocabulary | Hard-coded enum in `GPUTestConfig.h` | Declared per-file in `# tags: [ ... ]` blocks |
| Tie-breaking | First match in file order wins | Multiple matches UNION; globs by descending pattern length |
| Wildcards | Glob-anywhere | Glob-anywhere |
| Existing tooling in this repo | none | yes — `tools/src/cmd/cts/{format,validate,update-expectations,roll}` |
| Authoring ergonomics | Terse, fewer brackets | Slightly more verbose |

We chose **typ format** for three reasons:

1. **Reuse of Dawn's existing Go tooling** for sorting/validating/auto-updating
   the CTS expectations file. The tool source under `tools/src/cts/expectations/`
   is already format-aware and could be extended to manage this file too.
2. **Tag header in the file itself** documents the legal vocabulary inline.
   Anyone editing the file can see exactly which tags are valid without grepping
   the parser source.
3. **UNION semantics across multiple matching lines** handles overlap cleanly:
   one line can `[ Skip ]` a test on Vulkan + NVIDIA, another can `[ Slow ]` it
   on D3D12 + AMD, without either line needing to know about the other.

### Results: `Skip | Failure | RetryOnFailure | Slow`

These match typ. Mapping to runtime behavior:

- **Skip** — the test does not run. `GTEST_SKIP()` in `DawnTestBase::SetUp()`.
- **Failure** — the test runs. If it fails it's logged as
  `[ ExpectedFailure ]` and suppressed from the exit code. If it passes it's
  logged as `[ UnexpectedPass ]` and counted as a real failure. This is exactly
  the dEQP / WebGPU CTS semantic — running the test is what catches a fix you
  didn't notice landed.
- **RetryOnFailure** — the test runs. A failure is logged as
  `[ FlakyFailure ]` and suppressed; a pass is fine. (For now this is a
  one-shot tolerance, not an actual retry; see Limitations.)
- **Slow** — accepted by the parser but not yet acted on (parity with the typ
  vocabulary; reserved for future timeout extension).

A custom exit code is computed in `End2EndTestsMain.cpp` via
`DawnTestEnvironment::GetSuppressedExitCode()`: when an expectations file is in
effect, the process exits 0 unless there are **unexpected failures** or
**unexpected passes**. Expected failures and flaky failures do not affect the
exit code.

### Tag vocabulary

Declared at the top of `dawn_end2end_tests_expectations.txt`:

```
# OS:        win win11 linux mac android chromeos
# GPU:       amd apple arm intel nvidia qualcomm imgtec mesa-software
# Backend:   d3d11 d3d12 metal vulkan opengl opengles null
#            webgpu-on-{d3d11,d3d12,metal,vulkan,opengl,opengles,webgpu}
# Adapter:   swiftshader warp angle dxc
# Sanitizer: asan tsan
# Intel-Gen: intel-gen9 intel-gen12 intel-gen12-or-later
# Mode:      compat wire backend-validation full-backend-validation
#            implicit-device-sync capture-replay
```

These cover the predicates seen in the migrated `DAWN_SUPPRESS_TEST_IF` calls.
Tags are populated in `DawnTestBase::SetUp()` from
`mParam.adapterProperties` (vendorID, deviceID, backendType, compatibilityMode,
driverDescription) plus the `DawnTestEnvironment` flags. Tags inside `[ ... ]`
on one line are AND-combined; the running test must carry all of them.

### Wildcards

A `*` matches any sequence (including empty, including `/`), anywhere in the
pattern, any number of times. `\*` is a literal asterisk. Implementation
mirrors ANGLE's `NamesMatchWithWildcard`.

Common pattern: `Fixture.TestName/*` matches every parameterization (every
`AdapterTestParam`) of one test, since Dawn's parameterized test names take the
shape `TestSuite.TestName/D3D12_AMD_Radeon_RX_7900_XTX_...`.

### Conflict resolution

When multiple lines match the same test:

- All `[ Skip ]` / `[ Failure ]` / `[ RetryOnFailure ]` / `[ Slow ]` results
  are **UNION-ed**. So one line marking a test `[ Skip ]` and another marking
  it `[ Slow ]` simultaneously means both apply.
- Exact (non-glob) patterns are evaluated first; globs are evaluated in
  descending pattern-length order so the most specific glob wins. This is the
  same precedence rule as typ.

### Auto-discovery of the expectations file

The user no longer needs to pass `--expectations-file=` for the normal case.
At startup, when the flag is empty, `DawnTestEnvironment` probes:

1. `<exe_dir>/dawn_end2end_tests_expectations.txt`
2. `<exe_dir>/../../src/dawn/tests/dawn_end2end_tests_expectations.txt` — the
   standalone Dawn build layout, e.g. `out/release_x64/` up to
   `src/dawn/tests/`.
3. `<exe_dir>/../../third_party/dawn/src/dawn/tests/dawn_end2end_tests_expectations.txt`
   — the Chromium build layout.

The first existing file wins. Passing `--expectations-file=...` explicitly
overrides the search. If none of the candidates exists, the binary runs
without an expectations file (same as before this change).

### Reporting

At the end of a run with an expectations file in effect,
`DawnTestEnvironment::TearDown()` prints a summary:

```
Expectations file summary: N expected failures, M unexpected passes,
K flaky failures, U unexpected failures.
Found X unused expectation(s) in <path>:
  <path>:<line>: expectation '<pattern>' was unused
  ...
```

Unused-expectation lines surface stale entries (renamed/removed tests, or
suppressions that no longer apply on this configuration).

## Migration of existing `DAWN_SUPPRESS_TEST_IF` calls

Pre-migration: 845 call sites across 89 files.

A migration script translated each call by:

1. Parsing the boolean expression into a disjunctive-normal-form list of
   conjunctions, where each atomic predicate maps to a tag (e.g. `IsD3D12()`
   → `d3d12`, `IsImgTec()` → `imgtec`, `IsBackendValidationEnabled()` →
   `backend-validation`). Each conjunction becomes one expectation line.
2. Finding the enclosing `TEST_P` / `TEST_F` (or the enclosing fixture class
   when the call sits in a fixture's `SetUp()`).
3. Emitting `<bug> [ <tags> ] Fixture.TestName/* [ Skip ]`.
4. Removing the macro call from the source file.
5. A second pass walked the `git diff` for each modified file and extracted
   bug IDs from comments immediately preceding each deleted macro
   (`crbug.com/N`, `crbug.com/dawn/N`, `b/N`, `anglebug.com/N`, etc.),
   backfilling them into the matching expectation lines.

Post-migration:

| | Count |
|---|---:|
| Calls migrated to expectations file | 776 |
| Unique expectation lines emitted | 837 |
| Test source files modified | 84 |
| `DAWN_SUPPRESS_TEST_IF` calls remaining in code | 69 |
| Expectation lines with a real bug ID | 712 (85%) |
| Expectation lines still tagged `crbug.com/0` | 125 |

### Classification of migrated entries

Not all migrated entries are `[ Skip ]`. A classification pass examined the
comments immediately preceding each deleted `DAWN_SUPPRESS_TEST_IF` to assign
the most appropriate result:

| Result | Count | Criteria |
|---|---:|---|
| `[ Failure ]` | 775 | Known bug, test runs to detect silent fixes |
| `[ RetryOnFailure ]` | 47 | Comment mentions "flaky" / "intermittent" |
| `[ Skip ]` | 15 | Comment mentions "crash" / "hang" / "timeout" |

The default is `[ Failure ]` — the test runs and its failure is suppressed.
This is the most valuable result because it catches accidental fixes (which
appear as `[ UnexpectedPass ]`). `[ Skip ]` is reserved for tests that would
crash or hang the process, making them unsafe to run. `[ RetryOnFailure ]`
suppresses failures for tests known to be flaky without hiding a regression.

The remaining 69 `DAWN_SUPPRESS_TEST_IF` calls are intentionally kept in code
— they evaluate state that isn't expressible as a static tag (see
Limitations).

## Chromium bot compatibility

### How it works today

The expectations file is included in swarming isolates via the GN `data` dep
in BUILD.gn:

```gn
dawn_test("dawn_end2end_tests") {
  data = [ "dawn_end2end_tests_expectations.txt" ]
  ...
}
```

On Chromium bots, the file lands at
`third_party/dawn/src/dawn/tests/dawn_end2end_tests_expectations.txt` relative
to the output directory — matching auto-discovery candidate #3 in
`DawnTestEnvironment`.

### `[ Skip ]` works in both standalone and Chromium builds

When `DawnTestBase::SetUp()` calls `GTEST_SKIP()`, GTest records the test as
`SKIPPED` in the XML output. The Chromium test launcher
(`LaunchUnitTestsWithOptions` in `base/test/launcher/`) treats `TEST_SKIPPED`
as acceptable. No changes to bot configuration are needed.

### `[ Failure ]` and `[ RetryOnFailure ]` require awareness

Chromium's `gpu/dawn_end2end_tests_main.cc` uses `LaunchUnitTestsWithOptions`,
which spawns the test binary in child processes and determines pass/fail from
GTest XML output, not the process exit code. This means:

- A `[ Failure ]` test that fails will appear as `FAILED` in GTest XML. The
  Chromium test launcher will treat it as a real failure.
- The `GetSuppressedExitCode()` custom exit code in `End2EndTestsMain.cpp`
  only takes effect in the standalone Dawn build.

For the Chromium build path, the `ExpectationsListener::OnTestEnd` logs the
`[ ExpectedFailure ]` / `[ UnexpectedPass ]` tags to stdout, and CI result
processing (ResultDB / result_adapter) can be configured to recognize these.
Alternatively, the Chromium main could be updated to call
`GetSuppressedExitCode()` or use `RecordProperty` to surface the outcome in
GTest XML.

### No starlark / infra config changes needed

The bot configurations in `infra/config/targets/tests.star`, `mixins.star`,
and `binaries.star` do not need changes. The `data` dep handles file delivery,
and the existing `dawn_end2end_gpu_test` mixin args are compatible.

## Limitations

### What the expectation file cannot express

The 69 macros left in source code share these properties:

1. **Toggle state.** `HasToggleEnabled("use_blit_for_b2t")` is decided
   per-device based on adapter heuristics. Without exposing toggle state as a
   tag, the file can't represent it. ~7 cases.
2. **Test parameter values.** `GetParam().mTextureFormat == BGRA8Unorm`.
   Parameterizations vary per `TEST_P` and are not standardized as runner
   tags. ~25 cases.
3. **Driver-version comparisons.** `IsMesa("23.2")`, `majorVersion < 15`,
   `IsWindowsVersionAtLeast(26200u, 8246u)`. These require the runner to expose
   driver/OS version strings as queryable values. ~5 cases.
4. **GPU architecture detectors not yet tagged.** `gpu_info::IsNvidiaTuring(...)`,
   `IsAMDRDNA1(...)`. A future change could add `nvidia-turing`, `amd-rdna1`
   tags as needed. ~6 cases.
5. **WebGPU-on-X with mode negation.** `IsWebGPUOn(Metal) && !IsBackendValidationEnabled()`
   needs negative tags. typ does not support `! tag`; we did not extend the
   format here. ~4 cases.
6. **In-test adapter enumeration / `DAWN_PLATFORM_IS` macros / `DAWN_COMPILER_IS`.**
   Runtime checks that don't correspond to a per-test runner config. ~5 cases.

These all stay as `DAWN_SUPPRESS_TEST_IF` — the macro is not deprecated, and
both mechanisms coexist.

### What the implementation is currently missing

1. **`RetryOnFailure` is one-shot, not retry-on-failure.** A failed
   `[ RetryOnFailure ]` test is logged as `[ FlakyFailure ]` and the failure
   is suppressed, but the test is not actually re-run. True retry would
   require integration with the test launcher.
2. **`Slow` has no effect at runtime.** It parses correctly but doesn't
   extend any per-test timeout. ANGLE's parser uses `Slow` to scale the
   `kSlowTestTimeoutScale` timeout multiplier — we could add the same here.
3. **No load-time conflict detection.** ANGLE flags overlapping condition
   sets at parse time. We rely on UNION semantics instead, which means
   accidentally-overlapping lines silently merge results.
4. **`#` inside a test name is not supported.** Comments are stripped at the
   first `#`. Dawn test names never contain `#`, so this is a non-issue today.
5. **Bug-ID is optional but only minimally validated.** We accept the
   common prefixes; anything else is rejected as not-a-bug and falls through
   into the tag/test parsing path, which may produce a confusing error.
6. **GTest cannot truly suppress an already-recorded `Failure`.** A test
   marked `[ Failure ]` that fails will still appear as a failure in
   GTest's own stderr summary. The custom exit code via
   `GetSuppressedExitCode()` correctly ignores it, and the `[ ExpectedFailure ]`
   log tag makes the outcome unambiguous, but the GTest console output
   itself still says "FAILED". CI tooling that scrapes GTest output rather
   than reading the exit code will see noise unless it learns to look for
   the `[ ExpectedFailure ]` tag.
7. **`crbug.com/0` placeholders.** 125 migrated lines lack a real bug
   reference because the original macro had no nearby comment. These need
   manual triage.
8. **Fixture-scope `Fixture.*` wildcards from `SetUp()`-level suppressions
   may over-suppress.** Some migrated entries came from
   `DAWN_SUPPRESS_TEST_IF` calls in a fixture's `SetUp()`, which the script
   transcribed as `Fixture.*`. That correctly preserves the original
   intent (skip every `TEST_P` in the fixture) but may hide pass cases
   that, if run, would surface a fix-by-accident.
9. **No equivalent Go tooling yet.** Unlike `webgpu-cts/expectations.txt`,
   there is no `tools/cts format`-style tool to canonicalize line order, no
   `update-expectations` to rebaseline from ResultDB results, and no
   autoroller. The format is compatible enough that adapting those tools
   should be straightforward, but it's deferred.
10. **Inside-Chromium build path is unchanged.** Chromium's
    `//gpu/dawn_end2end_tests_main.cc` is used instead of
    `End2EndTestsMain.cpp` when building inside Chromium; the suppressed
    exit code path is in the standalone main only. The expectations file
    still loads and skips work in either main; only the exit-code custom
    treatment is standalone-only.

## Files of interest

- `src/dawn/tests/TestExpectations.h` / `.cpp` — parser and lookup
- `src/dawn/tests/DawnTest.h` / `.cpp` — `TestConditions` population,
  `ExpectationsListener`, `DawnTestEnvironment` integration, custom exit code
- `src/dawn/tests/End2EndTestsMain.cpp` — wires `GetSuppressedExitCode` into
  the process exit code
- `src/dawn/tests/dawn_end2end_tests_expectations.txt` — the file itself,
  with tag header
- `src/dawn/tests/BUILD.gn` — adds the new sources and the expectations file
  as a `data` dep

## How to use

### Skip a test on a specific configuration

Add a line under the appropriate section:

```
crbug.com/1234 [ vulkan nvidia win ] BufferTests.MapRead_NonZeroOffset/* [ Skip ]
```

Re-run. No rebuild.

### Mark a test as expected-failure

```
crbug.com/5678 [ d3d12 amd ] BindGroupTests.SomeTest/* [ Failure ]
```

The test still runs. A failure is suppressed and tagged
`[ ExpectedFailure ]`; an unexpected pass is reported and the run fails.

### Suppress an entire fixture on one config

```
crbug.com/9999 [ d3d11 qualcomm win ] CopyTests_T2B.* [ Skip ]
```

### Run a custom file

```
dawn_end2end_tests.exe --expectations-file=/path/to/my-file.txt --gtest_filter=...
```

### Override expectations to actually run skipped tests

```
dawn_end2end_tests.exe --run-suppressed-tests ...
```

This existing flag continues to bypass `DAWN_SUPPRESS_TEST_IF`. It does not
currently bypass the expectations file `[ Skip ]` entries; that's a small
follow-up if useful.

### Dump per-test results to a file

```
dawn_end2end_tests.exe --results-file=/path/to/results.json
```

Writes a JSON array with one object per test, similar to the `run-cts` tool's
`--output` format (`tools/src/cmd/run-cts/`):

```json
[
  {"TestCase": "BufferTests.MapRead/D3D12_NVIDIA_...", "Tags": "d3d12,nvidia,win", "Status": "Pass", "Duration": "1.5s"},
  {"TestCase": "BindGroupTests.SomeTest/Vulkan_AMD_...", "Tags": "amd,vulkan,win", "Status": "Failure", "Duration": "250ms"},
  {"TestCase": "CopyTests.T2B/D3D11_WARP_...", "Tags": "d3d11,warp,win", "Status": "Skip", "Duration": "0ms"}
]
```

Fields:

- **TestCase** — full GTest test name including parameterization
- **Tags** — sorted, comma-separated tag set (same tags as the expectations file)
- **Status** — `Pass`, `Failure`, `Skip`, or `RetryOnFailure`
- **Duration** — Go-style duration string (`1.5s` or `250ms`)

