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
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"regexp"
	"strconv"
	"strings"
	"sync"
	"time"

	"dawn.googlesource.com/dawn/tools/src/fileutils"
)

type CorpusDef struct {
	Name string `json:"name"`
	Path string `json:"path"`
}

type DurationDef struct {
	Seconds    *int `json:"seconds,omitempty"`
	Runs       *int `json:"runs,omitempty"`
	Iterations *int `json:"iterations,omitempty"`
}

type ExperimentConfig struct {
	Name                string        `json:"name"`
	Hash                string        `json:"hash"`
	Fuzzers             []string      `json:"fuzzers"`
	WgslBenchmarkCorpus string        `json:"wgsl_benchmark_corpus"`
	IrBenchmarkCorpus   string        `json:"ir_benchmark_corpus"`
	WgslCorpora         []CorpusDef   `json:"wgsl_corpora"`
	IrCorpora           []CorpusDef   `json:"ir_corpora"`
	DefaultIterations   int           `json:"default_iterations"`
	Durations           []DurationDef `json:"durations"`
}

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

type ExperimentTask struct {
	FuzzerName string
	CorpusName string
	CorpusPath string
	LimitType  string // "seconds" or "runs"
	LimitValue int
	Iteration  int
	TaskDir    string
}

func runExperiment(t *taskConfig) error {
	root := t.experimentPath
	if root == "" {
		return fmt.Errorf("experiment root path is required")
	}

	configPath := filepath.Join(root, "experiment.json")
	if !fileutils.IsFile(configPath, t.osWrapper) {
		return fmt.Errorf("experiment configuration file '%s' not found", configPath)
	}

	// Read and parse configuration
	configBytes, err := t.osWrapper.ReadFile(configPath)
	if err != nil {
		return fmt.Errorf("failed to read experiment config: %w", err)
	}

	var config ExperimentConfig
	if err := json.Unmarshal(configBytes, &config); err != nil {
		return fmt.Errorf("failed to parse experiment config JSON: %w", err)
	}

	// 1. Verify corpora directories exist under root/corpora/
	corporaDir := filepath.Join(root, "corpora")

	if len(config.Fuzzers) == 0 {
		return fmt.Errorf("no fuzzers specified in 'fuzzers' list inside experiment.json")
	}

	hasWgsl := false
	hasIr := false

	allowedFuzzers := map[string]bool{
		"tint_wgsl_fuzzer":      true,
		"tint_wgsl_mesa_fuzzer": true,
		"tint_ir_fuzzer":        true,
		"tint_ir_mesa_fuzzer":   true,
	}

	for _, fuzzer := range config.Fuzzers {
		if !allowedFuzzers[fuzzer] {
			return fmt.Errorf("unsupported fuzzer '%s'. Supported fuzzers are: tint_wgsl_fuzzer, tint_wgsl_mesa_fuzzer, tint_ir_fuzzer, tint_ir_mesa_fuzzer", fuzzer)
		}
		if strings.Contains(fuzzer, "wgsl") {
			hasWgsl = true
		} else {
			hasIr = true
		}
	}

	if hasWgsl {
		if config.WgslBenchmarkCorpus == "" {
			return fmt.Errorf("wgsl_benchmark_corpus is required in experiment.json because wgsl fuzzers are specified")
		}
		bcPath := filepath.Join(corporaDir, config.WgslBenchmarkCorpus)
		if !fileutils.IsDir(bcPath, t.osWrapper) {
			return fmt.Errorf("WGSL benchmark corpus directory '%s' not found under corpora root '%s'", config.WgslBenchmarkCorpus, corporaDir)
		}
		if len(config.WgslCorpora) == 0 {
			return fmt.Errorf("at least one wgsl_corpora definition is required in experiment.json because wgsl fuzzers are specified")
		}
		for _, cDef := range config.WgslCorpora {
			cPath := filepath.Join(corporaDir, cDef.Path)
			if !fileutils.IsDir(cPath, t.osWrapper) {
				return fmt.Errorf("WGSL corpus directory '%s' not found under corpora root '%s'", cDef.Path, corporaDir)
			}
		}
	}

	if hasIr {
		if config.IrBenchmarkCorpus == "" {
			return fmt.Errorf("ir_benchmark_corpus is required in experiment.json because ir fuzzers are specified")
		}
		bcPath := filepath.Join(corporaDir, config.IrBenchmarkCorpus)
		if !fileutils.IsDir(bcPath, t.osWrapper) {
			return fmt.Errorf("IR benchmark corpus directory '%s' not found under corpora root '%s'", config.IrBenchmarkCorpus, corporaDir)
		}
		if len(config.IrCorpora) == 0 {
			return fmt.Errorf("at least one ir_corpora definition is required in experiment.json because ir fuzzers are specified")
		}
		for _, cDef := range config.IrCorpora {
			cPath := filepath.Join(corporaDir, cDef.Path)
			if !fileutils.IsDir(cPath, t.osWrapper) {
				return fmt.Errorf("IR corpus directory '%s' not found under corpora root '%s'", cDef.Path, corporaDir)
			}
		}
	}

	// 2. Build checking phase & Args validation
	binDir := filepath.Join(root, "bin")
	allFuzzers := config.Fuzzers

	missingBinaries := false
	for _, fuzzer := range allFuzzers {
		fPath := filepath.Join(binDir, fuzzer+fileutils.ExeExt)
		if !fileutils.IsExe(fPath, t.osWrapper) {
			missingBinaries = true
			break
		}
	}

	if missingBinaries {
		// Verify GN args in provided -build contains necessary flags
		if err := checkGnArgs(t, allFuzzers); err != nil {
			return err
		}

		fmt.Printf("It looks like we are running a new experiment and need to build the fuzzers.\n")
		fmt.Printf("Building will checkout commit '%s' and run gclient sync.\n", config.Hash)
		confirm, err := promptConfirm("Do you want to proceed?")
		if err != nil {
			return fmt.Errorf("failed to read confirmation: %w", err)
		}
		if !confirm {
			return fmt.Errorf("experiment execution aborted by user")
		}

		// Perform Checkout, Sync, and Build
		if err := prepareBinaries(t, &config, binDir, allFuzzers); err != nil {
			return err
		}
	}

	// Create results/<machine>/ directory structure
	machine := t.machineName
	if machine == "" {
		machine = "default"
	}
	machineResultsDir := filepath.Join(root, "results", machine)
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

	for _, fuzzer := range allFuzzers {
		if _, ok := perfScores[fuzzer]; !ok {
			fmt.Printf("Running microbenchmark for %s...\n", fuzzer)
			score, err := runMicrobenchmark(t, root, fuzzer, &config)
			if err != nil {
				fmt.Printf("Warning: microbenchmark for %s failed: %v. Using default score 1.0\n", fuzzer, err)
				score = 1.0
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
		isWgslFuzzer := strings.Contains(fuzzer, "wgsl")
		var corpora []CorpusDef
		if isWgslFuzzer {
			corpora = config.WgslCorpora
		} else {
			corpora = config.IrCorpora
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

	for i := 0; i < t.numProcesses; i++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			for task := range taskChan {
				score := perfScores[task.FuzzerName]
				if err := executeTask(t, binDir, task, score); err != nil {
					errChan <- err
					return
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

type GnValue struct {
	Value string `json:"value"`
}

type GnArg struct {
	Name    string   `json:"name"`
	Current *GnValue `json:"current,omitempty"`
	Default *GnValue `json:"default,omitempty"`
}

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

func checkGnArgs(t *taskConfig, fuzzers []string) error {
	out, err := t.runCmd("gn", "args", t.build, "--list", "--json")
	if err != nil {
		return fmt.Errorf("failed to query GN arguments for build directory '%s': %w", t.build, err)
	}

	var args []GnArg
	if err := json.Unmarshal(out, &args); err != nil {
		return fmt.Errorf("failed to parse GN arguments JSON: %w", err)
	}

	flagValues := make(map[string]string)
	for _, arg := range args {
		flagValues[arg.Name] = arg.GetValue()
	}

	requiredFlags := map[string]string{
		"use_libfuzzer":          "true",
		"tint_build_wgsl_reader": "true",
		"use_clang_coverage":     "true",
		"optimize_for_fuzzing":   "false",
	}

	hasIr := false
	hasWgslMesa := false
	hasIrMesa := false

	for _, f := range fuzzers {
		if f == "tint_ir_fuzzer" {
			hasIr = true
		} else if f == "tint_wgsl_mesa_fuzzer" {
			hasWgslMesa = true
		} else if f == "tint_ir_mesa_fuzzer" {
			hasIrMesa = true
		}
	}

	if hasIr {
		requiredFlags["tint_build_ir_binary"] = "true"
		requiredFlags["tint_has_protobuf"] = "true"
	}
	if hasWgslMesa {
		requiredFlags["tint_build_mesa"] = "true"
		requiredFlags["tint_build_fuzzer_vulkan_support"] = "true"
	}
	if hasIrMesa {
		requiredFlags["tint_build_ir_binary"] = "true"
		requiredFlags["tint_has_protobuf"] = "true"
		requiredFlags["tint_build_mesa"] = "true"
		requiredFlags["tint_build_fuzzer_vulkan_support"] = "true"
	}

	var missing []string
	for flag, req := range requiredFlags {
		val := flagValues[flag]
		if val != req {
			missing = append(missing, fmt.Sprintf("%s (expected: %s, actual: %s)", flag, req, val))
		}
	}

	if len(missing) > 0 {
		return fmt.Errorf("the build directory '%s' has incorrect/missing GN configuration flags:\n  - %s\nPlease update your GN args and re-run", t.build, strings.Join(missing, "\n  - "))
	}

	return nil
}

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

func prepareBinaries(t *taskConfig, config *ExperimentConfig, binDir string, fuzzers []string) error {
	origRefBytes, err := t.runCmd("git", "rev-parse", "--abbrev-ref", "HEAD")
	if err != nil {
		return fmt.Errorf("failed to get original HEAD reference: %w", err)
	}
	origRef := strings.TrimSpace(string(origRefBytes))
	if origRef == "HEAD" {
		origHeadBytes, err := t.runCmd("git", "rev-parse", "HEAD")
		if err != nil {
			return fmt.Errorf("failed to get original HEAD commit: %w", err)
		}
		origRef = strings.TrimSpace(string(origHeadBytes))
	}

	fmt.Printf("Syncing repository to hash %s...\n", config.Hash)
	if _, err := t.runCmd("git", "checkout", config.Hash); err != nil {
		return fmt.Errorf("failed to checkout %s: %w", config.Hash, err)
	}

	restoreRepo := func() {
		fmt.Printf("Restoring repository to original state (%s)...\n", origRef)
		_, _ = t.runCmd("git", "checkout", origRef)
		_, _ = t.runCmd("gclient", "sync")
	}

	// Guarantee repository state restore upon completion/error
	defer restoreRepo()

	if _, err := t.runCmd("gclient", "sync"); err != nil {
		return fmt.Errorf("gclient sync failed: %w", err)
	}

	// Build each required fuzzer binary
	for _, fuzzer := range fuzzers {
		fmt.Printf("Building %s...\n", fuzzer)
		if _, err := t.runCmd("autoninja", "-C", t.build, fuzzer); err != nil {
			return fmt.Errorf("build failed for %s: %w", fuzzer, err)
		}
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

	return nil
}

func runMicrobenchmark(t *taskConfig, root string, fuzzer string, config *ExperimentConfig) (float64, error) {
	// Find the first relevant corpus in the configuration
	var corpusPath string
	isWgsl := strings.Contains(fuzzer, "wgsl")

	if isWgsl {
		corpusPath = filepath.Join(root, "corpora", config.WgslBenchmarkCorpus)
	} else {
		corpusPath = filepath.Join(root, "corpora", config.IrBenchmarkCorpus)
	}

	binPath := filepath.Join(root, "bin", fuzzer+fileutils.ExeExt)
	tmpOut, err := t.osWrapper.MkdirTemp("", "perf_out")
	if err != nil {
		return 1.0, err
	}
	defer t.osWrapper.RemoveAll(tmpOut)

	args := []string{tmpOut, corpusPath, "-runs=10000", "-max_total_time=15"}

	start := time.Now()
	out, err := t.runCmd(binPath, args...)
	duration := time.Since(start)

	if err != nil {
		// libFuzzer may exit with non-zero if interrupted by max_total_time, which is fine
		if duration.Seconds() < 1.0 {
			return 1.0, fmt.Errorf("microbenchmark terminated too quickly: %w", err)
		}
	}

	// Parse actual runs from out
	actualRuns := 10000
	re := regexp.MustCompile(`stat::number_of_executed_units:\s*(\d+)`)
	if match := re.FindStringSubmatch(string(out)); len(match) > 1 {
		if val, err := strconv.Atoi(match[1]); err == nil {
			actualRuns = val
		}
	} else {
		// Fallback parse pulse lines like: #1234	PULSE
		rePulse := regexp.MustCompile(`#(\d+)\s+`)
		matches := rePulse.FindAllStringSubmatch(string(out), -1)
		if len(matches) > 0 {
			lastMatch := matches[len(matches)-1]
			if val, err := strconv.Atoi(lastMatch[1]); err == nil {
				actualRuns = val
			}
		}
	}

	score := float64(actualRuns) / duration.Seconds()
	if score <= 0 {
		score = 1.0
	}
	fmt.Printf("Microbenchmark result for %s: %d runs in %.2fs (%.2f runs/sec)\n", fuzzer, actualRuns, duration.Seconds(), score)
	return score, nil
}

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

func executeTask(t *taskConfig, binDir string, task ExperimentTask, score float64) error {
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

	// Copy binary to TaskDir to isolate coverage reporting
	srcBin := filepath.Join(binDir, task.FuzzerName+fileutils.ExeExt)
	taskBin := filepath.Join(task.TaskDir, task.FuzzerName+fileutils.ExeExt)
	if err := fileutils.CopyFile(taskBin, srcBin, t.osWrapper); err != nil {
		return fmt.Errorf("failed to copy binary to task directory: %w", err)
	}

	// Delete binary on complete
	defer func() {
		_ = t.osWrapper.Remove(taskBin)
	}()

	corpusOut := filepath.Join(task.TaskDir, "corpus")
	_ = t.osWrapper.MkdirAll(corpusOut, 0755)

	args := []string{corpusOut, task.CorpusPath}
	if task.LimitType == "seconds" {
		args = append(args, fmt.Sprintf("-max_total_time=%d", task.LimitValue))
	} else {
		args = append(args, fmt.Sprintf("-runs=%d", task.LimitValue))
	}

	// Dictionary check (if wgsl write fuzzer)
	if strings.Contains(task.FuzzerName, "wgsl") {
		dictPath := filepath.Join(filepath.Join(fileutils.DawnRoot(t.osWrapper), "src", "tint", "cmd", "fuzz", "wgsl"), "dictionary.txt")
		if fileutils.IsFile(dictPath, t.osWrapper) {
			args = append(args, "-dict="+dictPath)
		}
	}

	fmt.Printf("[%s] Starting: %s on %s (%s=%d, iter %d)\n",
		time.Now().Format("15:04:05"), task.FuzzerName, task.CorpusName, task.LimitType, task.LimitValue, task.Iteration)

	start := time.Now()
	// Run the copied binary, setting working directory to TaskDir so raw profraw files are dumped right there!
	cmd := t.execWrapper.Command(taskBin, args...).WithDir(task.TaskDir)
	out, err := cmd.RunWithCombinedOutput()
	duration := time.Since(start)

	if err != nil && t.verbose {
		fmt.Printf("Fuzzer process returned error/exit status: %v\n", err)
	}

	// Write logs
	logPath := filepath.Join(task.TaskDir, "fuzzer.log")
	_ = t.osWrapper.WriteFile(logPath, out, 0644)

	// Parse actual runs completed
	actualRuns := 0
	re := regexp.MustCompile(`stat::number_of_executed_units:\s*(\d+)`)
	if match := re.FindStringSubmatch(string(out)); len(match) > 1 {
		if val, err := strconv.Atoi(match[1]); err == nil {
			actualRuns = val
		}
	} else {
		rePulse := regexp.MustCompile(`#(\d+)\s+`)
		matches := rePulse.FindAllStringSubmatch(string(out), -1)
		if len(matches) > 0 {
			lastMatch := matches[len(matches)-1]
			if val, err := strconv.Atoi(lastMatch[1]); err == nil {
				actualRuns = val
			}
		}
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
