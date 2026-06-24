// Copyright 2026 The Dawn & Tint Authors
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//  3. Neither the name of the copyright holder nor the names of its
//     contributors may be used to endorse or promote products derived from
//     this software without specific prior written permission.
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

package main

import (
	"bufio"
	"context"
	"encoding/json"
	"fmt"
	"maps"
	"os"
	"path/filepath"
	"regexp"
	"strconv"
	"strings"
	"sync"
	"time"

	"dawn.googlesource.com/dawn/tools/src/fileutils"
)

// CorpusDef defines the name and relative path of a fuzzer corpus.
type CorpusDef struct {
	Name string `json:"name"`
	Path string `json:"path"`
}

// DurationDef defines the stopping criteria for a fuzzer iteration,
// either by time (seconds), total executions (runs), or a specific number of iterations.
type DurationDef struct {
	Seconds    *int `json:"seconds,omitempty"`
	Runs       *int `json:"runs,omitempty"`
	Iterations *int `json:"iterations,omitempty"`
}

// ExperimentConfig represents the structure of the experiment.json configuration file.
type ExperimentConfig struct {
	Name    string   `json:"name"`
	Hash    string   `json:"hash"`
	Fuzzers []string `json:"fuzzers"`
	Timeout *int     `json:"timeout,omitempty"`
	// <foo>BenchmarkCorpus is the corpus used for <foo> fuzzer microbenchmarking.
	WgslBenchmarkCorpus string        `json:"wgsl_benchmark_corpus"`
	IrBenchmarkCorpus   string        `json:"ir_benchmark_corpus"`
	WgslCorpora         []CorpusDef   `json:"wgsl_corpora"`
	IrCorpora           []CorpusDef   `json:"ir_corpora"`
	DefaultIterations   int           `json:"default_iterations"`
	Durations           []DurationDef `json:"durations"`
}

// IterState tracks the execution status and results of a single fuzzer iteration.
type IterState struct {
	Status        string  `json:"status"` // "pending", "running", "completed"
	StartTime     string  `json:"start_time"`
	EndTime       string  `json:"end_time"`
	PerfScore     float64 `json:"perf_score"`
	LimitType     string  `json:"limit_type"` // "seconds" or "runs"
	LimitValue    int     `json:"limit_value"`
	ActualRuns    int     `json:"actual_runs"`
	ActualSeconds float64 `json:"actual_seconds"`
}

// ExperimentTask represents a single unit of work to be executed by a worker.
type ExperimentTask struct {
	FuzzerName string
	CorpusName string
	CorpusPath string
	LimitType  string // "seconds" or "runs"
	LimitValue int
	Iteration  int
	TaskDir    string
}

var (
	allowedFuzzers = map[string]FuzzMode{
		"tint_wgsl_fuzzer":      FuzzModeWgsl,
		"tint_wgsl_mesa_fuzzer": FuzzModeWgsl,
		"tint_ir_fuzzer":        FuzzModeIr,
		"tint_ir_mesa_fuzzer":   FuzzModeIr,
	}

	baseRequiredFlags = map[string]string{
		"use_libfuzzer":          "true",
		"tint_build_wgsl_reader": "true",
		"use_clang_coverage":     "true",
		"optimize_for_fuzzing":   "false",
	}

	additionalRequiredFlags = map[string]map[string]string{
		"tint_wgsl_fuzzer": {},
		"tint_ir_fuzzer": {
			"tint_build_ir_binary": "true",
			"tint_has_protobuf":    "true",
		},
		"tint_wgsl_mesa_fuzzer": {
			"tint_build_mesa":                  "true",
			"tint_build_fuzzer_vulkan_support": "true",
		},
		"tint_ir_mesa_fuzzer": {
			"tint_build_ir_binary":             "true",
			"tint_has_protobuf":                "true",
			"tint_build_mesa":                  "true",
			"tint_build_fuzzer_vulkan_support": "true",
		},
	}
)

// validateCorpora checks that for a given fuzzer there is a benchmarking corpus and referenced experimental corpora present
func validateCorpora(mode FuzzMode, config *ExperimentConfig, corporaDir string, t *taskConfig) error {
	var benchmarkCorpus string
	var corpora []CorpusDef
	var typeName string

	switch mode {
	case FuzzModeWgsl:
		benchmarkCorpus = config.WgslBenchmarkCorpus
		corpora = config.WgslCorpora
		typeName = "WGSL"
	case FuzzModeIr:
		benchmarkCorpus = config.IrBenchmarkCorpus
		corpora = config.IrCorpora
		typeName = "IR"
	default:
		return fmt.Errorf("unknown fuzz mode %d", mode)
	}

	if benchmarkCorpus == "" {
		return fmt.Errorf("%s_benchmark_corpus is required in experiment.json because %s fuzzers are specified", strings.ToLower(typeName), typeName)
	}

	bcPath := filepath.Join(corporaDir, benchmarkCorpus)
	if !fileutils.IsDir(bcPath, t.osWrapper) {
		return fmt.Errorf("%s benchmark corpus directory '%s' not found under corpora root '%s'", typeName, benchmarkCorpus, corporaDir)
	}

	if len(corpora) == 0 {
		return fmt.Errorf("at least one %s_corpora definition is required in experiment.json because %s fuzzers are specified", strings.ToLower(typeName), typeName)
	}

	for _, cDef := range corpora {
		cPath := filepath.Join(corporaDir, cDef.Path)
		if !fileutils.IsDir(cPath, t.osWrapper) {
			return fmt.Errorf("%s corpus directory '%s' not found under corpora root '%s'", typeName, cDef.Path, corporaDir)
		}
	}
	return nil
}

// runExperiment is the main entry point for the 'experiment' subcommand.
// It validates the environment, builds binaries if needed, and runs the task queue.
func runExperiment(t *taskConfig) error {
	config, err := loadExperimentConfig(t, t.experimentPath)
	if err != nil {
		return err
	}

	// Check what fuzzers are needed/going to be run
	modes := map[FuzzMode]struct{}{}
	for _, fuzzer := range config.Fuzzers {
		if _, ok := allowedFuzzers[fuzzer]; !ok {
			return fmt.Errorf("unsupported fuzzer '%s'. Supported fuzzers are: tint_wgsl_fuzzer, tint_wgsl_mesa_fuzzer, tint_ir_fuzzer, tint_ir_mesa_fuzzer", fuzzer)
		}
		modes[allowedFuzzers[fuzzer]] = struct{}{}
	}

	// Verify expected corpora directories exist under t.experimentPath/corpora/
	corporaDir := filepath.Join(t.experimentPath, "corpora")
	if len(config.Fuzzers) == 0 {
		return fmt.Errorf("no fuzzers specified in 'fuzzers' list inside experiment.json")
	}

	for m := range modes {
		if err := validateCorpora(m, &config, corporaDir, t); err != nil {
			return err
		}
	}

	// 2. Build checking phase & Args validation
	binDir := filepath.Join(t.experimentPath, "bin")
	missingBinaries := make([]string, 0, len(config.Fuzzers))
	for _, fuzzer := range config.Fuzzers {
		fPath := filepath.Join(binDir, fuzzer+fileutils.ExeExt)
		if !fileutils.IsExe(fPath, t.osWrapper) {
			missingBinaries = append(missingBinaries, fuzzer)
		}
	}

	if len(missingBinaries) > 0 {
		// Verify GN args in provided -build contains necessary flags
		hasSanitizers, err := checkGnArgs(t, missingBinaries)
		if err != nil {
			return err
		}

		if hasSanitizers {
			timeoutOverridden := config.Timeout != nil && *config.Timeout > 0
			warnMsg := "\n================================ WARNING ================================\n" +
				"Your build directory has compiler sanitizers (ASan, UBSan, TSan, or MSan) enabled.\n" +
				"Fuzzing with sanitizers will significantly increase the runtime per test case,\n" +
				"which will tend to dominate the results and possibly lead to false-positive timeouts.\n"
			if !timeoutOverridden {
				warnMsg += "We highly recommend overriding the default 5s fuzzer timeout (by adding a \"timeout\"\n" +
					"field to your experiment.json) if you proceed with sanitizers.\n"
			}
			warnMsg += "=========================================================================\n\n"
			fmt.Print(warnMsg)
			confirm, err := promptConfirm("Are you sure you want to proceed with sanitizers enabled?")
			if err != nil {
				return fmt.Errorf("failed to read sanitizer confirmation: %w", err)
			}
			if !confirm {
				return fmt.Errorf("experiment execution aborted by user due to active sanitizers")
			}
		}

		fmt.Printf("It looks like we are running a new experiment and need to build the fuzzers.\n")
		fmt.Printf("Building will checkout commit '%s' and run gclient sync.\n", config.Hash)
		confirm, err := promptConfirm("Do you want to proceed with the build?")
		if err != nil {
			return fmt.Errorf("failed to read build confirmation: %w", err)
		}
		if !confirm {
			return fmt.Errorf("experiment execution aborted by user")
		}

		// Perform Checkout, Sync, and Build
		if err := prepareBinaries(t, &config, binDir, missingBinaries); err != nil {
			return err
		}
	}

	// Create results/<machine>/ directory structure
	machine := t.machineName
	if machine == "" {
		machine = "default"
	}
	machineResultsDir := filepath.Join(t.experimentPath, "results", machine)
	if err := t.osWrapper.MkdirAll(machineResultsDir, 0755); err != nil {
		return fmt.Errorf("failed to create machine results directory '%s': %w", machineResultsDir, err)
	}

	// 3. Performance Normalization (Microbenchmark)
	perfScoresPath := filepath.Join(machineResultsDir, "perf_scores.json")
	perfScores := make(map[string]float64)
	if fileutils.IsFile(perfScoresPath, t.osWrapper) {
		perfScoresBytes, err := t.osWrapper.ReadFile(perfScoresPath)
		if err == nil {
			_ = json.Unmarshal(perfScoresBytes, &perfScores)
		}
	}

	for _, fuzzer := range config.Fuzzers {
		if _, ok := perfScores[fuzzer]; !ok {
			fmt.Printf("Running microbenchmark for %s...\n", fuzzer)
			score, err := runMicrobenchmark(t, t.experimentPath, fuzzer, &config)
			if err != nil {
				return err
			}
			perfScores[fuzzer] = score
			// Save immediately
			scoresBytes, _ := json.MarshalIndent(perfScores, "", "  ")
			_ = t.osWrapper.WriteFile(perfScoresPath, scoresBytes, 0644)
		}
	}

	// 4. Generate & Orchestrate Tasks
	var tasks []ExperimentTask
	for _, fuzzer := range config.Fuzzers {
		var corpora []CorpusDef
		switch allowedFuzzers[fuzzer] {
		case FuzzModeWgsl:
			corpora = config.WgslCorpora
		case FuzzModeIr:
			corpora = config.IrCorpora
		default:
			return fmt.Errorf("unknown fuzz mode %d", allowedFuzzers[fuzzer])
		}
		for _, corpus := range corpora {
			tasks = append(tasks, generateTasksForFuzzer(fuzzer, corpus, corporaDir, machineResultsDir, &config)...)
		}
	}

	fmt.Printf("Found %d total tasks in the experiment plan.\n", len(tasks))

	// Queue up pending/interrupted tasks
	var pendingTasks []ExperimentTask
	for _, task := range tasks {
		statePath := filepath.Join(task.TaskDir, "state.json")
		if fileutils.IsFile(statePath, t.osWrapper) {
			stateBytes, err := t.osWrapper.ReadFile(statePath)
			if err == nil {
				var state IterState
				if err := json.Unmarshal(stateBytes, &state); err == nil && state.Status == "completed" {
					continue // already completed, skip
				}
			}
		}
		// Reset/Prepare task folder
		_ = t.osWrapper.MkdirAll(filepath.Join(task.TaskDir, "corpus"), 0755)
		pendingTasks = append(pendingTasks, task)
	}

	fmt.Printf("Executing %d pending/incomplete tasks using %d parallel jobs...\n", len(pendingTasks), t.numProcesses)

	// Execute via parallel worker pool
	taskChan := make(chan ExperimentTask, len(pendingTasks))
	for _, task := range pendingTasks {
		taskChan <- task
	}
	close(taskChan)

	var wg sync.WaitGroup
	errChan := make(chan error, t.numProcesses)
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	timeoutVal := 5
	if config.Timeout != nil && *config.Timeout > 0 {
		timeoutVal = *config.Timeout
	}

	for i := 0; i < t.numProcesses; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			for {
				select {
				case <-ctx.Done():
					return
				case task, ok := <-taskChan:
					if !ok {
						return
					}
					score := perfScores[task.FuzzerName]
					if err := executeTask(ctx, t, binDir, task, score, timeoutVal); err != nil {
						if err != context.Canceled && err != context.DeadlineExceeded {
							errChan <- err
							cancel()
						}
						return
					}
				}
			}
		}()
	}

	wg.Wait()
	close(errChan)

	if len(errChan) > 0 {
		return <-errChan // return first error encountered
	}

	fmt.Println("Experiment completed successfully!")
	return nil
}

// loadExperimentConfig reads and parses the experiment.json file from the
// specified experiment root directory.
func loadExperimentConfig(t *taskConfig, experimentRoot string) (ExperimentConfig, error) {
	if experimentRoot == "" {
		return ExperimentConfig{}, fmt.Errorf("experiment root path is required")
	}
	configPath := filepath.Join(experimentRoot, "experiment.json")
	if !fileutils.IsFile(configPath, t.osWrapper) {
		return ExperimentConfig{}, fmt.Errorf("experiment configuration file '%s' not found", configPath)
	}

	// Read and parse configuration
	configBytes, err := t.osWrapper.ReadFile(configPath)
	if err != nil {
		return ExperimentConfig{}, fmt.Errorf("failed to read experiment config: %w", err)
	}

	var config ExperimentConfig
	if err := json.Unmarshal(configBytes, &config); err != nil {
		return ExperimentConfig{}, fmt.Errorf("failed to parse experiment config JSON: %w", err)
	}
	return config, nil
}

// GnValue holds the string value of a GN argument.
type GnValue struct {
	Value string `json:"value"`
}

// GnArg represents a single GN argument as returned by 'gn args --json'.
type GnArg struct {
	Name    string   `json:"name"`
	Current *GnValue `json:"current,omitempty"`
	Default *GnValue `json:"default,omitempty"`
}

// GetValue returns the effective value of a GN argument, stripping quotes.
func (g *GnArg) GetValue() string {
	val := ""
	if g.Current != nil {
		val = g.Current.Value
	} else if g.Default != nil {
		val = g.Default.Value
	}
	if strings.HasPrefix(val, "\"") && strings.HasSuffix(val, "\"") {
		val = strings.Trim(val, "\"")
	}
	return val
}

// checkGnArgs verifies that the build directory has the required GN flags set for the fuzzers being tested.
// It also checks and returns true if any standard compiler sanitizers are enabled.
func checkGnArgs(t *taskConfig, fuzzers []string) (bool, error) {
	// Get current GN args for build and put in lookup table
	out, err := t.runCmd("gn", "args", t.build, "--list", "--json")
	if err != nil {
		return false, fmt.Errorf("failed to query GN arguments for build directory '%s': %w", t.build, err)
	}

	var args []GnArg
	if err := json.Unmarshal(out, &args); err != nil {
		return false, fmt.Errorf("failed to parse GN arguments JSON: %w", err)
	}

	flagValues := make(map[string]string)
	for _, arg := range args {
		flagValues[arg.Name] = arg.GetValue()
	}

	// Determine what flag values need to be set for the fuzzers being run
	requiredFlags := maps.Clone(baseRequiredFlags)
	for _, fuzzer := range fuzzers {
		maps.Copy(requiredFlags, additionalRequiredFlags[fuzzer])
	}

	// Check required flag state against lookup table
	var incorrect []string
	for flag, req := range requiredFlags {
		val := flagValues[flag]
		if val != req {
			incorrect = append(incorrect, fmt.Sprintf("%s (expected: %s, actual: %s)", flag, req, val))
		}
	}

	if len(incorrect) > 0 {
		return false, fmt.Errorf("the build directory '%s' has incorrect/missing GN configuration flags:\n  - %s\nPlease update your GN args and re-run", t.build, strings.Join(incorrect, "\n  - "))
	}

	hasSanitizers := flagValues["is_asan"] == "true" ||
		flagValues["is_ubsan"] == "true" ||
		flagValues["is_tsan"] == "true" ||
		flagValues["is_msan"] == "true"

	return hasSanitizers, nil
}

// promptConfirm displays a yes/no prompt to the user.
func promptConfirm(msg string) (bool, error) {
	fmt.Printf("%s [y/N]: ", msg)
	reader := bufio.NewReader(os.Stdin)
	text, err := reader.ReadString('\n')
	if err != nil {
		return false, err
	}
	text = strings.TrimSpace(strings.ToLower(text))
	return text == "y" || text == "yes", nil
}

// prepareBinaries checks out the specified git hash, syncs dependencies,
// builds the fuzzers, and copies them to the experiment's bin directory.
func prepareBinaries(t *taskConfig, config *ExperimentConfig, binDir string, fuzzers []string) error {
	return WithCheckout(t, config.Hash,
		func() error {
			if err := SyncAndBuildTargets(t, fuzzers); err != nil {
				return err
			}

			// Create bin folder and copy compiled executables
			if err := t.osWrapper.MkdirAll(binDir, 0755); err != nil {
				return fmt.Errorf("failed to create binary folder '%s': %w", binDir, err)
			}

			for _, fuzzer := range fuzzers {
				srcPath := filepath.Join(t.build, fuzzer+fileutils.ExeExt)
				dstPath := filepath.Join(binDir, fuzzer+fileutils.ExeExt)
				if err := fileutils.CopyFile(dstPath, srcPath, t.osWrapper); err != nil {
					return fmt.Errorf("failed to copy built binary %s to bin folder: %w", fuzzer, err)
				}
			}

			// Copy any companion shared libraries (*.so, *.dylib, *.dll) from build directory
			files, err := t.osWrapper.ReadDir(t.build)
			if err == nil {
				for _, f := range files {
					if !f.IsDir() {
						ext := filepath.Ext(f.Name())
						if ext == ".so" || ext == ".dylib" || ext == ".dll" {
							srcLib := filepath.Join(t.build, f.Name())
							dstLib := filepath.Join(binDir, f.Name())
							_ = fileutils.CopyFile(dstLib, srcLib, t.osWrapper)
						}
					}
				}
			}
			return nil
		})
}

// runMicrobenchmark executes a short fuzzer run to determine the execution
// speed (runs/sec) on the current hardware for normalization of results.
func runMicrobenchmark(t *taskConfig, root string, fuzzer string, config *ExperimentConfig) (float64, error) {
	// Find the first relevant corpus in the configuration
	var corpusPath string
	switch allowedFuzzers[fuzzer] {
	case FuzzModeWgsl:
		corpusPath = filepath.Join(root, "corpora", config.WgslBenchmarkCorpus)
	case FuzzModeIr:
		corpusPath = filepath.Join(root, "corpora", config.IrBenchmarkCorpus)
	default:
		return 1.0, fmt.Errorf("unknown fuzz mode %d", allowedFuzzers[fuzzer])
	}

	binPath := filepath.Join(root, "bin", fuzzer+fileutils.ExeExt)
	tmpOut, err := t.osWrapper.MkdirTemp("", "perf_out")
	if err != nil {
		return 1.0, err
	}
	defer t.osWrapper.RemoveAll(tmpOut)

	timeoutVal := 5
	if config.Timeout != nil && *config.Timeout > 0 {
		timeoutVal = *config.Timeout
	}

	args := []string{tmpOut, corpusPath, "-runs=10000", fmt.Sprintf("-timeout=%d", timeoutVal), "-max_total_time=15"}

	start := time.Now()
	out, err := t.runCmd(binPath, args...)
	duration := time.Since(start)

	if err != nil {
		// The fuzzer may exit with non-zero if interrupted by max_total_time or if it hit a crash report, which is
		// fine, unless it exited really fast, since that means it is finding issues quickly, which means that the whole
		// experimental apparatus is not going to work well
		if duration.Seconds() < 1.0 {
			return 1.0, fmt.Errorf("microbenchmark terminated too quickly: %w", err)
		}
	}

	// Parse actual runs performed from out
	actualRuns, err := parseActualRuns(out)
	if err != nil {
		return 1.0, err
	}

	score := float64(actualRuns) / duration.Seconds()
	if score <= 0 {
		score = 1.0
	}
	fmt.Printf("Microbenchmark result for %s: %d runs in %.2fs (%.2f runs/sec)\n", fuzzer, actualRuns, duration.Seconds(), score)
	return score, nil
}

// generateTasksForFuzzer creates the list of ExperimentTasks for a specific
// fuzzer and corpus combination based on the experiment durations.
func generateTasksForFuzzer(fuzzer string, corpus CorpusDef, corporaDir string, machineDir string, config *ExperimentConfig) []ExperimentTask {
	var tasks []ExperimentTask
	cPath := filepath.Join(corporaDir, corpus.Path)

	for _, dDef := range config.Durations {
		limitType := "seconds"
		limitValue := 0
		if dDef.Seconds != nil {
			limitType = "seconds"
			limitValue = *dDef.Seconds
		} else if dDef.Runs != nil {
			limitType = "runs"
			limitValue = *dDef.Runs
		}

		iterations := config.DefaultIterations
		if dDef.Iterations != nil {
			iterations = *dDef.Iterations
		}

		limitStr := fmt.Sprintf("%s_%d", limitType, limitValue)

		for i := 1; i <= iterations; i++ {
			taskDir := filepath.Join(machineDir, fuzzer, corpus.Name, limitStr, fmt.Sprintf("iter_%d", i))
			tasks = append(tasks, ExperimentTask{
				FuzzerName: fuzzer,
				CorpusName: corpus.Name,
				CorpusPath: cPath,
				LimitType:  limitType,
				LimitValue: limitValue,
				Iteration:  i,
				TaskDir:    taskDir,
			})
		}
	}
	return tasks
}

// executeTask runs a single fuzzer iteration, manages its state file,
// and captures the output logs and performance data.
func executeTask(ctx context.Context, t *taskConfig, binDir string, task ExperimentTask, score float64, timeoutVal int) error {
	statePath := filepath.Join(task.TaskDir, "state.json")

	// Update state to running
	state := IterState{
		Status:     "running",
		StartTime:  time.Now().Format(time.RFC3339),
		PerfScore:  score,
		LimitType:  task.LimitType,
		LimitValue: task.LimitValue,
	}
	stateBytes, _ := json.MarshalIndent(state, "", "  ")
	_ = t.osWrapper.WriteFile(statePath, stateBytes, 0644)

	corpusOut := filepath.Join(task.TaskDir, "corpus")
	_ = t.osWrapper.MkdirAll(corpusOut, 0755)

	absCorpusOut, err := filepath.Abs(corpusOut)
	if err != nil {
		return fmt.Errorf("failed to get absolute path for corpus out: %w", err)
	}

	absCorpusPath, err := filepath.Abs(task.CorpusPath)
	if err != nil {
		return fmt.Errorf("failed to get absolute path for corpus input: %w", err)
	}

	args := []string{absCorpusOut, absCorpusPath, fmt.Sprintf("-timeout=%d", timeoutVal)}
	if task.LimitType == "seconds" {
		args = append(args, fmt.Sprintf("-max_total_time=%d", task.LimitValue))
	} else {
		args = append(args, fmt.Sprintf("-runs=%d", task.LimitValue))
	}

	// Add dictionary if running WGSL fuzzer
	if allowedFuzzers[task.FuzzerName] == FuzzModeWgsl {
		dictPath := filepath.Join(filepath.Join(fileutils.DawnRoot(t.osWrapper), "src", "tint", "cmd", "fuzz", "wgsl"), "dictionary.txt")
		if fileutils.IsFile(dictPath, t.osWrapper) {
			absDictPath, err := filepath.Abs(dictPath)
			if err != nil {
				return fmt.Errorf("failed to get absolute path for dictionary: %w", err)
			}
			args = append(args, "-dict="+absDictPath)
		}
	}

	fmt.Printf("[%s] Starting: %s on %s (%s=%d, iter %d)\n",
		time.Now().Format("15:04:05"), task.FuzzerName, task.CorpusName, task.LimitType, task.LimitValue, task.Iteration)

	start := time.Now()
	// Run the binary directly from binDir, setting working directory to TaskDir so raw profraw files are dumped right there!
	binPath := filepath.Join(binDir, task.FuzzerName+fileutils.ExeExt)
	absTaskBin, err := filepath.Abs(binPath)
	if err != nil {
		return fmt.Errorf("failed to get absolute path of fuzzer binary: %w", err)
	}

	var out []byte
	for retry := 0; retry < 5; retry++ {
		cmd := t.execWrapper.CommandContext(ctx, absTaskBin, args...).WithDir(task.TaskDir)
		out, err = cmd.RunWithCombinedOutput()
		if err == nil {
			break
		}
		if strings.Contains(err.Error(), "text file busy") {
			time.Sleep(100 * time.Millisecond)
			continue
		}
		break
	}
	duration := time.Since(start)

	if err != nil {
		if ctx.Err() != nil {
			return ctx.Err()
		}
		if t.verbose {
			fmt.Printf("Fuzzer process returned error/exit status: %v\n", err)
		}
	}

	// Write logs
	logPath := filepath.Join(task.TaskDir, "fuzzer.log")
	_ = t.osWrapper.WriteFile(logPath, out, 0644)

	// Parse actual runs completed
	actualRuns, err := parseActualRuns(out)
	if err != nil {
		return err
	}

	state.Status = "completed"
	state.EndTime = time.Now().Format(time.RFC3339)
	state.ActualRuns = actualRuns
	state.ActualSeconds = duration.Seconds()

	fmt.Printf("[%s] Completed: %s on %s (iter %d) in %.2fs, %d runs\n",
		time.Now().Format("15:04:05"), task.FuzzerName, task.CorpusName, task.Iteration, duration.Seconds(), actualRuns)

	stateBytes, _ = json.MarshalIndent(state, "", "  ")
	_ = t.osWrapper.WriteFile(statePath, stateBytes, 0644)

	return nil
}

// parseActualRuns extracts the number of fuzzer executions from the fuzzer output.
func parseActualRuns(output []byte) (int, error) {
	// First, try to parse the 'stat::number_of_executed_units' line.
	re := regexp.MustCompile(`stat::number_of_executed_units:\s*(\d+)`)
	if match := re.FindStringSubmatch(string(output)); len(match) > 1 {
		if val, err := strconv.Atoi(match[1]); err == nil {
			return val, nil
		}
	}

	// Fallback to parsing pulse lines like: #1234	PULSE
	rePulse := regexp.MustCompile(`#(\d+)\s+`)
	matches := rePulse.FindAllStringSubmatch(string(output), -1)
	if len(matches) > 0 {
		lastMatch := matches[len(matches)-1]
		if val, err := strconv.Atoi(lastMatch[1]); err == nil {
			return val, nil
		}
	}

	return 0, fmt.Errorf("failed to parse actual runs from fuzzer output")
}
