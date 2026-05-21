# Test Expectations File for `dawn_end2end_tests`

## Background

`dawn_end2end_tests` uses two C++ macros to skip tests at runtime:

- `DAWN_SUPPRESS_TEST_IF(condition)` — skip a test when the runtime condition
  matches; overridable with `--run-suppressed-tests`.
- `DAWN_TEST_UNSUPPORTED_IF(condition)` — skip a test when a required feature or
  toggle is missing.

These work, but every suppression requires a code change, a rebuild, and a code
review. The three other test suites in the broader WebGPU ecosystem all already
use text-based expectation files:

| Suite | Format | Reference |
|---|---|---|
| `angle_end2end_tests` | ANGLE's `GPUTestExpectationsParser` | `third_party/angle/src/tests/angle_end2end_tests_expectations.txt` |
| WebGPU CTS | catapult `typ` (`TaggedTestListParser`) | `webgpu-cts/expectations.txt` |
| WebGL CTS | same `typ` parser | (lives in Chromium) |

This change brings `dawn_end2end_tests` in line with those suites by introducing
`src/dawn/tests/dawn_end2end_tests_expectations.txt` and a runtime loader.

## Benefits of an expectation file

1. **No rebuild to skip a failing test.** Edit the file, re-run.
2. **IHV bring-up without source.** Hand the binary and expectations file to a
   GPU vendor; they iterate locally without needing the full build environment.
3. **Downstream configurations.** Downstream projects (e.g. Edge bots) can
   maintain a separate expectations file without merge conflicts against
   upstream test source files.
4. **Detection of stale entries.** "Unused expectation" reporting flags lines
   that no longer match any test (e.g. after a test was renamed or deleted).
5. **Bug-ID provenance in one place.** Every entry carries a bug reference; the
   file becomes a searchable index of what's skipped and why.
6. **Convergence with neighboring suites.** People who already know
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

We chose **typ format** for three reasons:

1. **Reuse of Dawn's existing Go tooling** for sorting/validating/auto-updating
   the CTS expectations file.
2. **Tag header in the file itself** documents the legal vocabulary inline.
3. **UNION semantics across multiple matching lines** handles overlap cleanly.

### Results: `Skip` (initial implementation)

The initial implementation supports only `[ Skip ]`:

- **Skip** — the test does not run. `GTEST_SKIP()` in `DawnTestBase::SetUp()`.

The parser also accepts `Failure`, `RetryOnFailure`, and `Slow` tokens for
forward compatibility with the typ format, but only `Skip` has runtime behavior.

### Tag vocabulary

Declared at the top of `dawn_end2end_tests_expectations.txt`:

```
# OS:        win win11 linux mac android chromeos
# GPU:       amd apple arm intel nvidia qualcomm imgtec mesa-software
# Backend:   d3d11 d3d12 metal vulkan opengl opengles null
# WebGPU-on: webgpu-on-{d3d11,d3d12,metal,vulkan,opengl,opengles,webgpu}
# Adapter:   swiftshader warp
# Compiler:  angle dxc
# Sanitizer: asan tsan
# Intel-Gen: intel-gen9 intel-gen12 intel-gen12-or-later
# GPU-Arch:  nvidia-turing amd-rdna1
# Driver:    mesa-23.2
# Mode:      compat wire backend-validation full-backend-validation
#            implicit-device-sync capture-replay
```

Tags are populated in `DawnTestBase::SetUp()` from
`mParam.adapterProperties` (vendorID, deviceID, backendType, compatibilityMode,
driverDescription) plus the `DawnTestEnvironment` flags. Tags inside `[ ... ]`
on one line are AND-combined; the running test must carry all of them.

### Wildcards

A `*` matches any sequence (including empty, including `/`), anywhere in the
pattern, any number of times. `\*` is a literal asterisk.

Common pattern: `Fixture.TestName/*` matches every parameterization of one
test, since Dawn's parameterized test names take the shape
`TestSuite.TestName/D3D12_AMD_Radeon_RX_7900_XTX_...`.

### Conflict resolution

When multiple lines match the same test, the runner applies a two-tier
"longest-match-wins, UNION-among-ties" rule:

1. **Exact (non-glob) matches first.** If any exact line matches, only the
   exact lines contribute; globs are not consulted.
2. **Globs in descending pattern length.** When no exact line matches, the
   runner walks globs from longest pattern to shortest. As soon as one or
   more lines at a given length match, those lines' results are UNION-ed and
   the search stops.

### Auto-discovery of the expectations file

At startup, when `--expectations-file` is not passed, `DawnTestEnvironment`
probes candidate locations relative to the test executable:

1. `<exe_dir>/dawn_end2end_tests_expectations.txt` (copied via GN data deps).
2. For each ancestor directory (up to six levels), it tries:
   - `<ancestor>/src/dawn/tests/dawn_end2end_tests_expectations.txt`
   - `<ancestor>/third_party/dawn/src/dawn/tests/dawn_end2end_tests_expectations.txt`

The first existing file wins. If none exists, the binary runs without an
expectations file.

## Coexistence with `DAWN_SUPPRESS_TEST_IF`

Both mechanisms coexist. `DAWN_SUPPRESS_TEST_IF` remains the right choice for
conditions that aren't expressible as static tags (toggle state, test parameter
values, OS version inequalities, negated conditions). The expectations file is
best for platform/GPU/backend-level skips that benefit from no-rebuild
iteration.

## Files of interest

- `src/dawn/tests/TestExpectations.h` / `.cpp` — parser and lookup
- `src/dawn/tests/DawnTest.h` / `.cpp` — `TestConditions` population,
  `DawnTestEnvironment` integration
- `src/dawn/tests/dawn_end2end_tests_expectations.txt` — the file itself
- `src/dawn/tests/BUILD.gn` — adds the new sources and the expectations file
  as a `data` dep

## How to use

### Skip a test on a specific configuration

Add a line to the expectations file:

```
crbug.com/1234 [ vulkan nvidia win ] BufferTests.MapRead_NonZeroOffset/* [ Skip ]
```

Re-run. No rebuild.

### Skip an entire fixture on one config

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

This flag bypasses both `DAWN_SUPPRESS_TEST_IF` and expectations-file `[ Skip ]`
entries.
