# Tint Fuzzer Experimentation & Performance Harness

The fuzzer experiment framework provides a structured, automated, and reproducible way to run benchmark and performance trials on Tint's fuzzers (`tint_wgsl_fuzzer` and `tint_ir_fuzzer`). Its primary goal is to measure fuzzer effectiveness by tracking code coverage over time or execution run counts.

To ensure transparency, prevent "blackbox" magic, and provide absolute reproducibility, the framework splits fuzzer experimentation into three distinct, explicit phases: **Manual Setup**, **Execution**, and **Analysis**.

---

## High-Level Workflow

```
┌────────────────────────────────┐
│      Phase 1: Manual Setup     │
│  - Create folder <root>        │
│  - Create experiment.json      │
│  - Copy corpora to <root>/corp │
└───────────────┬────────────────┘
                │
                ▼
┌────────────────────────────────┐
│       Phase 2: Execution       │
│  - `fuzz -experiment <root>`    │
│  - Auto-compile fuzzer binaries│
│  - Run microbenchmarks         │
│  - Run fuzzers in worker pool  │
│  - Save raw results & logs     │
└───────────────┬────────────────┘
                │
                ▼
┌────────────────────────────────┐
│       Phase 3: Analysis        │
│  - `fuzz -report <root>`       │
│  - Auto-execute coverage.py    │
│  - Parse & segment LCOV data   │
│  - Normalize multi-machine data│
│  - Emit experiment_report.md   │
└────────────────────────────────┘
```

---

## Phase 1: Manual Setup & Configuration

To run an experiment, you must create a dedicated directory (the `<root>` directory, e.g. `experiments/my_mutator_trial/`). This directory will act as a self-contained archive holding your configuration, corpora, target binaries, raw logs, and analysis outputs.

### 1. The `experiment.json` Configuration File

Create a file named **exactly** `experiment.json` in your `<root>` directory.

#### JSON Schema Specification
*   `name` (string, required): A unique, descriptive name for the experiment.
*   `hash` (string, required): The exact Git commit hash of the Dawn repository that should be used to build the fuzzer binaries.
*   `fuzzers` (array of strings, required): List of fuzzer binary names to build and run. Only 4 binaries are supported:
    *   `"tint_wgsl_fuzzer"`
    *   `"tint_wgsl_mesa_fuzzer"`
    *   `"tint_ir_fuzzer"`
    *   `"tint_ir_mesa_fuzzer"`
    *(Any other fuzzer binary name, or an empty list, is rejected).*
*   `wgsl_benchmark_corpus` (string, required if any `tint_wgsl_*` fuzzers are specified): The name of a static, unchanging "gold standard" benchmark corpus directory located under `<root>/corpora/`. Used for pure hardware performance calibration.
*   `ir_benchmark_corpus` (string, required if any `tint_ir_*` fuzzers are specified): The name of a static, unchanging "gold standard" benchmark corpus directory located under `<root>/corpora/`. Used for pure hardware performance calibration.
*   `wgsl_corpora` (array of objects, required if any `tint_wgsl_*` fuzzers are specified): WGSL seed corpora definitions.
*   `ir_corpora` (array of objects, required if any `tint_ir_*` fuzzers are specified): IR seed corpora definitions.
    *   `name` (string): Unique identifier name for the corpus.
    *   `path` (string): Directory path relative to `<root>/corpora/`.
*   `default_iterations` (integer, required): The global fallback number of times to repeat each fuzzer run to gather statically significant averages and standard deviations.
*   `durations` (array of objects, required): The target execution limits. Each duration can be constrained either by time (`seconds`) or count (`runs`).
    *   `seconds` (integer, optional): Limit fuzzer execution time in seconds (uses `-max_total_time`).
    *   `runs` (integer, optional): Limit fuzzer execution count (uses `-runs`).
    *   `iterations` (integer, optional): Override the global `default_iterations` specifically for this duration (useful for capping very long-running runs to 1 or 2 iterations).

#### Configuration Example (`experiment.json`)
```json
{
  "name": "fuzzer_baseline_vs_new_mutators",
  "hash": "4a2b9c8d1e3f...",
  "fuzzers": ["tint_wgsl_fuzzer", "tint_ir_fuzzer"],
  "wgsl_benchmark_corpus": "gold_standard_wgsl",
  "ir_benchmark_corpus": "gold_standard_ir",
  "wgsl_corpora": [
    { "name": "tint_tests_wgsl", "path": "wgsl_seed_corpus" }
  ],
  "ir_corpora": [
    { "name": "tint_tests_ir", "path": "ir_seed_corpus" }
  ],
  "default_iterations": 5,
  "durations": [
    {"seconds": 3600},
    {"seconds": 7200},
    {"seconds": 14400, "iterations": 3},
    {"runs": 100000},
    {"runs": 500000, "iterations": 2}
  ]
}
```

### 2. Preparing Seed Corpora
Create a directory named `<root>/corpora/`. Copy all directories containing seed files into this folder. For example:
*   `experiments/my_mutator_trial/corpora/wgsl_seed_corpus/` (contains `.wgsl` seed files)
*   `experiments/my_mutator_trial/corpora/ir_seed_corpus/` (contains `.tirb` seed files)

---

## Phase 2: Execution

Run the experiment via the command-line interface:
```bash
./tools/run fuzz -experiment <root> -build <build_dir> [-machine <name>] [-j <jobs>] [-verbose]
```

### Workflow Internals & Verification
1.  **Validation**: The tool validates that `<root>/experiment.json` exists and parses correctly, and that all referenced corpora exist within `<root>/corpora/`.
2.  **GN Build Args Check**:
    *   Instead of parsing `args.gn` directly, the tool runs `gn args <build_dir> --list --json` to inspect the exact values and defaults of the GN build flags.
    *   **Common Flags (Required by all fuzzers)**:
        *   `use_libfuzzer == true`
        *   `tint_build_wgsl_reader == true`
        *   `use_clang_coverage == true`
        *   `optimize_for_fuzzing == false`
    *   **Fuzzer-specific Flags**:
        *   `tint_ir_fuzzer` additionally requires: `tint_build_ir_binary == true` and `tint_has_protobuf == true`.
        *   `tint_wgsl_mesa_fuzzer` additionally requires: `tint_build_mesa == true` and `tint_build_fuzzer_vulkan_support == true`.
        *   `tint_ir_mesa_fuzzer` additionally requires: `tint_build_ir_binary == true`, `tint_has_protobuf == true`, `tint_build_mesa == true`, and `tint_build_fuzzer_vulkan_support == true`.
    *   If any checked flag is missing or has an incorrect value, the tool exits early with a detailed report of the mismatched configuration.
3.  **Checkout & Build**:
    *   The tool checks if `<root>/bin/` exists and contains copies of the fuzzer binaries listed in the JSON config.
    *   If missing, it prompts the user for confirmation: "It looks like we are running a new experiment and need to build the fuzzers. Do you want to proceed?"
    *   If confirmed, the tool automatically saves the current Git HEAD state, checks out the specified `hash`, executes `gclient sync`, builds the target binaries via `autoninja`, copies them to `<root>/bin/`, and finally restores the repository to its original branch/commit.
4.  **Performance Normalization (Microbenchmark)**:
    *   Because experiments may run on different machines (or the same machine under varying CPU load), fuzzer speeds (`runs_per_second`) will differ.
    *   To normalize results, before running the trials, the framework executes a deterministic **Microbenchmark** for *each* fuzzer binary using the *static, gold-standard benchmark corpus* specified in the JSON (`wgsl_benchmark_corpus` and `ir_benchmark_corpus`). Running the calibration on a shared, static workload prevents normalization bias between different test corpora (such as comparing a seeded run against an empty run), ensuring that the normalized CPU hours accurately reflect pure machine compute power.
    *   The benchmark runs the fuzzer for up to 10,000 runs or 15 seconds. The resulting performance score (`runs_per_second`) is recorded in `<root>/results/<machine>/perf_scores.json`. If a score is already recorded, the benchmark is skipped.
5.  **Task Orchestration**:
    *   The runner expands the JSON configuration into a matrix of tasks: `Fuzzer × Corpus × Duration × Iteration`.
    *   Tasks are placed into a parallel worker pool sized via the CLI `-j` flag. Specifically for experiments, `-j` defaults to `1` to prevent CPU overload and thermal throttling from skewing microbenchmark timings (while other standard fuzzing modes default to the machine's CPU core count). You can increase this explicitly by passing `-j <count>` (e.g., `-j 4`) if your system has sufficient capacity and cooling.
6.  **State Management & Resumability**:
    *   All raw execution results are stored under `<root>/results/<machine>/` (where `<machine>` is specified via `-machine`, defaulting to `"default"`).
    *   Each task has a dedicated path: `<root>/results/<machine>/<fuzzer>/<corpus_name>/<limit_type>_<value>/iter_<N>/`
    *   Within this directory, a `state.json` file is maintained tracking `"pending"`, `"running"`, or `"completed"` status.
    *   **Interruptibility**: If the trial is interrupted (e.g. power failure or user abort), running the same command again will scan the results directory, automatically skip all `"completed"` iterations, clean up any stale `"running"` tasks, and resume from where it left off.
7.  **Task Isolation**:
    *   To prevent raw LLVM profiling data (`.profraw`) from bleeding between concurrent runs or filling up the disk, when an iteration starts, the fuzzer binary is copied directly from `<root>/bin/` to the iteration's directory.
    *   The fuzzer is run with the iteration's directory as its working directory. On completion, the raw fuzzer stdout/stderr is written to `fuzzer.log`, actual execution duration and runs are recorded in `state.json`, and the copied fuzzer binary is deleted.

---

## Phase 3: Analysis & Reporting

Generate the performance and coverage report using the `-report` flag:
```bash
./tools/run fuzz -report <root> [-machine <name>] -build <build_dir>
```

The analysis phase is explicitly separated from data generation to allow running different analysis passes post-run without polluting raw results.

### 1. Analysis Isolation
All coverage reports, intermediate HTML structures, and parsed metrics are stored in a separate `<root>/coverage/` directory structure, keeping the raw data in `<root>/results/` pristine.

### 2. Auto-running the Coverage Pipeline
For each completed iteration, if `<root>/coverage/<machine>/.../coverage.json` is missing:
1.  The tool invokes Chromium's coverage script: `tools/code_coverage/coverage.py` using the iteration's generated corpus directory (`results/.../corpus/`) as the test case input.
2.  The script outputs an LCOV tracefile under `<root>/coverage/.../`.
3.  The framework parses the `.lcov` file programmatically.

### 3. Coverage Segmentation
Fuzzers can cover various parts of the codebase. To provide actionable insights, the framework parses the LCOV file and segments line coverage metrics separately into three key categories based on target source paths:
*   **Tint Core (Primary)**: Combined coverage for `src/tint/` and `src/utils/`.
*   **Mesa**: If using a Mesa-based Vulkan fuzzer variant, coverage statistics for `third_party/mesa/`.
*   **DirectX**: If applicable, coverage statistics for `third_party/dx*` or `third_party/directx*`.

The segmented metrics are cached locally in `coverage.json`.

### 4. Machine Merging & Normalization
If the `-machine` flag is provided to `-report`, it produces a report exclusively for that machine. If omitted, the tool merges data from all machine subdirectories found under `<root>/results/`.

Because different machines have different CPU speeds, merging raw wall-clock time (seconds) directly is incorrect.
*   **Runs-based limits**: No normalization is needed; coverage is plotted directly against runs.
*   **Seconds-based limits**: The framework reads the fuzzer's `system_perf_score` (from `perf_scores.json`) and calculates **Normalized CPU Hours**:
    $$\text{Normalized CPU Hours} = \frac{\text{ActualSeconds} \times \text{PerfScore}}{\text{BaselinePerfScore} \times 3600.0}$$
    *(where $\text{BaselinePerfScore} = 1000.0$ runs/sec)*.

This converts machine-specific wall time into an objective, standardized "work completed" unit, enabling seamless merging of trials sharded across heterogeneous systems.

### 5. Report Output (`experiment_report.md`)
The tool calculates averages and standard deviations (SD) across all matching iterations and writes a clean Markdown report to `<root>/experiment_report.md`. This includes:
*   Summary tables of fuzzer, corpus, and codebase segment groupings showing runs, seconds, normalized hours, and coverage averages/SD.
*   Detailed trajectory graphs (represented in markdown tables) tracking coverage growth along the limit values.

---

## Example Workflow

Let's walk through a complete end-to-end example of running a benchmarking experiment to compare fuzzer coverage over time.

### Step 1: Manual Setup
Create our experiment directory and corpora directories:
```bash
# Create standard test corpus and the required gold-standard benchmark corpus
mkdir -p experiments/my_mutators/corpora/wgsl_tests
mkdir -p experiments/my_mutators/corpora/gold_standard
```

Copy our starting seed files and gold-standard benchmarks into their respective folders:
```bash
cp test/tint/benchmark/*.wgsl experiments/my_mutators/corpora/wgsl_tests/
cp test/tint/benchmark/*.wgsl experiments/my_mutators/corpora/gold_standard/
```

Create `experiments/my_mutators/experiment.json` with the following configuration:
```json
{
  "name": "mutator_performance_trial",
  "hash": "8f3d10ab2c4e...",
  "fuzzers": ["tint_wgsl_fuzzer"],
  "wgsl_benchmark_corpus": "gold_standard",
  "wgsl_corpora": [
    { "name": "wgsl_seed", "path": "wgsl_tests" }
  ],
  "default_iterations": 3,
  "durations": [
    {"seconds": 60},
    {"seconds": 120},
    {"runs": 50000}
  ]
}
```

### Step 2: Running the Experiment
Run the trial on the local machine (identifying it as `workstation_1`) using 4 parallel slots:
```bash
./tools/run fuzz -experiment experiments/my_mutators -build out/active -machine workstation_1 -j 4
```

The tool will:
1.  Verify `wgsl_tests` corpus exists under `experiments/my_mutators/corpora/`.
2.  Validate that `out/active/args.gn` has coverage enabled.
3.  Prompt: "It looks like we are running a new experiment and need to build the fuzzers. Do you want to proceed? [y/N]"
4.  If confirmed, checkout commit `8f3d10ab2c4e...`, compile `tint_wgsl_fuzzer`, copy it to `experiments/my_mutators/bin/`, and restore the git branch.
5.  Run a 15-second microbenchmark on `tint_wgsl_fuzzer` using `wgsl_tests` to establish a performance score (e.g. 1540 runs/sec).
6.  Start executing the 9 task iterations concurrently, isolating each binary and generating raw logs.

If you abort the run halfway through, simply execute the same command again to resume exactly where it was cut off.

### Step 3: Analysis
Once completed, generate the final report:
```bash
./tools/run fuzz -report experiments/my_mutators -build out/active
```

The analysis tool will execute `coverage.py` for each iteration, parse the generated LCOV tracefiles, calculate Tint Core coverage statistics, normalize workstation_1's seconds-based execution using the 1540 runs/sec score, and output a detailed markdown report at `experiments/my_mutators/experiment_report.md` showing the coverage trajectory.
