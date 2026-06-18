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
	"encoding/json"
	"fmt"
	"math"
	"os"
	"path/filepath"
	"sort"
	"strconv"
	"strings"

	"dawn.googlesource.com/dawn/tools/src/fileutils"
)

// CoverageStats holds the line coverage metrics for a specific code segment.
type CoverageStats struct {
	LinesFound int     `json:"lines_found"`
	LinesHit   int     `json:"lines_hit"`
	Percentage float64 `json:"percentage"`
}

// IterCoverage aggregates coverage statistics across different project components.
type IterCoverage struct {
	TintCore CoverageStats `json:"tint_core"`
	Mesa     CoverageStats `json:"mesa"`
	DirectX  CoverageStats `json:"directx"`
}

// AggregatedResult stores the combined metrics for multiple iterations of a specific
// fuzzer, corpus, and limit configuration.
type AggregatedResult struct {
	FuzzerName      string
	CorpusName      string
	LimitType       string
	LimitValue      int
	Segment         string
	ActualRuns      []float64
	ActualSeconds   []float64
	NormalizedHours []float64
	CoveragePercent []float64
}

// runReport generates a Markdown performance and coverage report by analyzing
// the results of completed fuzzing iterations stored in the report path.
func runReport(t *taskConfig) error {
	config, err := loadExperimentConfig(t, t.reportPath)
	if err != nil {
		return err
	}

	resultsDir := filepath.Join(t.reportPath, "results")
	if !fileutils.IsDir(resultsDir, t.osWrapper) {
		return fmt.Errorf("results directory '%s' not found", resultsDir)
	}

	// Find machine folders
	var machines []string
	if t.machineName != "" {
		mPath := filepath.Join(resultsDir, t.machineName)
		if fileutils.IsDir(mPath, t.osWrapper) {
			machines = append(machines, t.machineName)
		} else {
			return fmt.Errorf("specified machine results directory '%s' not found", mPath)
		}
	} else {
		// Scan results Dir
		files, err := t.osWrapper.ReadDir(resultsDir)
		if err != nil {
			return fmt.Errorf("failed to read results directory: %w", err)
		}
		for _, f := range files {
			if f.IsDir() {
				machines = append(machines, f.Name())
			}
		}
	}

	if len(machines) == 0 {
		return fmt.Errorf("no machine results found under '%s'", resultsDir)
	}

	fmt.Printf("Analyzing results from machines: %s\n", strings.Join(machines, ", "))

	// Build tasks list from configuration
	var completedIters []struct {
		Machine    string
		Fuzzer     string
		Corpus     string
		LimitType  string
		LimitValue int
		Iter       int
		IterDir    string
		State      IterState
		Coverage   IterCoverage
	}

	for _, machine := range machines {
		machineDir := filepath.Join(resultsDir, machine)
		perfScoresPath := filepath.Join(machineDir, "perf_scores.json")
		perfScores := make(map[string]float64)
		if fileutils.IsFile(perfScoresPath, t.osWrapper) {
			psBytes, err := t.osWrapper.ReadFile(perfScoresPath)
			if err == nil {
				_ = json.Unmarshal(psBytes, &perfScores)
			}
		}

		for _, fuzzer := range config.Fuzzers {
			score := perfScores[fuzzer]
			if score <= 0 {
				score = 1.0
			}

			var corpora []CorpusDef
			switch allowedFuzzers[fuzzer] {
			case FuzzModeWgsl:
				corpora = config.WgslCorpora
			case FuzzModeIr:
				corpora = config.IrCorpora
			default:
				return fmt.Errorf("unknown fuzzer: %s", fuzzer)
			}

			for _, corpus := range corpora {
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
						iterDir := filepath.Join(machineDir, fuzzer, corpus.Name, limitStr, fmt.Sprintf("iter_%d", i))
						statePath := filepath.Join(iterDir, "state.json")
						if !fileutils.IsFile(statePath, t.osWrapper) {
							return err
						}

						stateBytes, err := t.osWrapper.ReadFile(statePath)
						if err != nil {
							return err
						}

						var state IterState
						if err := json.Unmarshal(stateBytes, &state); err != nil || state.Status != "completed" {
							return err
						}

						// Force inject score from perfScores if missing in state
						if state.PerfScore <= 0 {
							return fmt.Errorf("invalid perf score for fuzzer: %s", fuzzer)
						}

						// Run or load coverage
						cov, err := getOrRunCoverage(t, t.reportPath, machine, fuzzer, corpus.Name, limitStr, i, iterDir)
						if err != nil {
							return fmt.Errorf("Warning: failed to get/run coverage for %s/%s iter %d: %v\n", fuzzer, corpus.Name, i, err)
						}

						completedIters = append(completedIters, struct {
							Machine    string
							Fuzzer     string
							Corpus     string
							LimitType  string
							LimitValue int
							Iter       int
							IterDir    string
							State      IterState
							Coverage   IterCoverage
						}{
							Machine:    machine,
							Fuzzer:     fuzzer,
							Corpus:     corpus.Name,
							LimitType:  limitType,
							LimitValue: limitValue,
							Iter:       i,
							IterDir:    iterDir,
							State:      state,
							Coverage:   cov,
						})
					}
				}
			}
		}
	}

	if len(completedIters) == 0 {
		return fmt.Errorf("no completed iterations found to analyze")
	}

	fmt.Printf("Successfully loaded coverage data for %d task iterations.\n", len(completedIters))

	// Group and aggregate
	// Key: Fuzzer + "|" + Corpus + "|" + LimitType + "|" + LimitValue + "|" + Segment
	aggregations := make(map[string]*AggregatedResult)

	for _, ci := range completedIters {
		segments := []struct {
			Name  string
			Stats CoverageStats
		}{
			{"Tint Core", ci.Coverage.TintCore},
			{"Mesa", ci.Coverage.Mesa},
			{"DirectX", ci.Coverage.DirectX},
		}

		for _, seg := range segments {
			if seg.Stats.LinesFound == 0 {
				continue // skip reporting empty segments (e.g. Mesa if not a Mesa fuzzer)
			}

			key := fmt.Sprintf("%s|%s|%s|%d|%s", ci.Fuzzer, ci.Corpus, ci.LimitType, ci.LimitValue, seg.Name)
			agg, ok := aggregations[key]
			if !ok {
				agg = &AggregatedResult{
					FuzzerName: ci.Fuzzer,
					CorpusName: ci.Corpus,
					LimitType:  ci.LimitType,
					LimitValue: ci.LimitValue,
					Segment:    seg.Name,
				}
				aggregations[key] = agg
			}

			agg.ActualRuns = append(agg.ActualRuns, float64(ci.State.ActualRuns))
			agg.ActualSeconds = append(agg.ActualSeconds, ci.State.ActualSeconds)

			// Normalization: CPU Hours = (ActualSeconds * PerfScore) / (1000 * 3600)
			normalizedHours := (ci.State.ActualSeconds * ci.State.PerfScore) / (1000.0 * 3600.0)
			agg.NormalizedHours = append(agg.NormalizedHours, normalizedHours)
			agg.CoveragePercent = append(agg.CoveragePercent, seg.Stats.Percentage)
		}
	}

	// Generate Report Output
	var reportBuilder strings.Builder
	reportBuilder.WriteString(fmt.Sprintf("# Experiment Performance and Coverage Report: %s\n\n", config.Name))
	reportBuilder.WriteString(fmt.Sprintf("- **Git Hash**: `%s`\n", config.Hash))
	if t.machineName != "" {
		reportBuilder.WriteString(fmt.Sprintf("- **Target Machine**: `%s`\n", t.machineName))
	} else {
		reportBuilder.WriteString("- **Machines Merged**:\n")
		for _, m := range machines {
			reportBuilder.WriteString(fmt.Sprintf("  - `%s`\n", m))
		}
	}
	reportBuilder.WriteString("\n## Summary Table\n\n")
	reportBuilder.WriteString("| Fuzzer | Corpus | Segment | Limit | Runs (Avg/SD) | Seconds (Avg/SD) | Norm CPU Hours (Avg/SD) | Coverage (Avg/SD) |\n")
	reportBuilder.WriteString("| --- | --- | --- | --- | --- | --- | --- | --- |\n")

	// Sort keys for deterministic output
	var keys []string
	for k := range aggregations {
		keys = append(keys, k)
	}
	sort.Slice(keys, func(i, j int) bool {
		partsI := strings.Split(keys[i], "|")
		partsJ := strings.Split(keys[j], "|")
		// Sort by Fuzzer, Corpus, Segment, LimitType, LimitValue
		for idx := 0; idx < len(partsI); idx++ {
			if partsI[idx] != partsJ[idx] {
				if idx == 3 { // limit value conversion
					valI, _ := strconv.Atoi(partsI[idx])
					valJ, _ := strconv.Atoi(partsJ[idx])
					return valI < valJ
				}
				return partsI[idx] < partsJ[idx]
			}
		}
		return false
	})

	for _, k := range keys {
		agg := aggregations[k]
		runAvg, runSd := stats(agg.ActualRuns)
		secAvg, secSd := stats(agg.ActualSeconds)
		normAvg, normSd := stats(agg.NormalizedHours)
		covAvg, covSd := stats(agg.CoveragePercent)

		limitStr := fmt.Sprintf("%d %s", agg.LimitValue, agg.LimitType)

		reportBuilder.WriteString(fmt.Sprintf("| %s | %s | %s | %s | %.1f / %.1f | %.1f / %.1f | %.4f / %.4f | %.2f%% / %.2f%% |\n",
			agg.FuzzerName,
			agg.CorpusName,
			agg.Segment,
			limitStr,
			runAvg, runSd,
			secAvg, secSd,
			normAvg, normSd,
			covAvg, covSd,
		))
	}

	reportBuilder.WriteString("\n## Detailed Coverage Trajectories\n\n")

	// Group trajectory data by Fuzzer, Corpus, and Segment
	type TrajectoryPoint struct {
		LimitType  string
		LimitValue int
		CovAvg     float64
		CovSd      float64
		NormAvg    float64
	}

	trajectories := make(map[string][]TrajectoryPoint)
	for _, k := range keys {
		agg := aggregations[k]
		covAvg, covSd := stats(agg.CoveragePercent)
		normAvg, _ := stats(agg.NormalizedHours)

		tKey := fmt.Sprintf("%s - %s (%s)", agg.FuzzerName, agg.CorpusName, agg.Segment)
		trajectories[tKey] = append(trajectories[tKey], TrajectoryPoint{
			LimitType:  agg.LimitType,
			LimitValue: agg.LimitValue,
			CovAvg:     covAvg,
			CovSd:      covSd,
			NormAvg:    normAvg,
		})
	}

	// Sort trajectories keys
	var tKeys []string
	for k := range trajectories {
		tKeys = append(tKeys, k)
	}
	sort.Strings(tKeys)

	for _, tk := range tKeys {
		points := trajectories[tk]
		// Sort points by limit type and value
		sort.Slice(points, func(i, j int) bool {
			if points[i].LimitType != points[j].LimitType {
				return points[i].LimitType < points[j].LimitType
			}
			return points[i].LimitValue < points[j].LimitValue
		})

		reportBuilder.WriteString(fmt.Sprintf("### %s\n\n", tk))
		reportBuilder.WriteString("| Target Limit | Normalized CPU Hours | Coverage (Avg) | Coverage (StdDev) |\n")
		reportBuilder.WriteString("| --- | --- | --- | --- |\n")
		for _, pt := range points {
			reportBuilder.WriteString(fmt.Sprintf("| %d %s | %.4f | %.2f%% | %.2f%% |\n",
				pt.LimitValue, pt.LimitType, pt.NormAvg, pt.CovAvg, pt.CovSd))
		}
		reportBuilder.WriteString("\n")
	}

	// Save Report File
	reportFile := filepath.Join(t.reportPath, "experiment_report.md")
	if err := t.osWrapper.WriteFile(reportFile, []byte(reportBuilder.String()), 0644); err != nil {
		return fmt.Errorf("failed to write report file: %w", err)
	}

	fmt.Printf("\nReport generated successfully at: %s\n", reportFile)
	fmt.Println("\n--- SUMMARY REPORT ---")
	fmt.Println(reportBuilder.String()[:min(1500, len(reportBuilder.String()))] + "...\n[Report truncated in output]")
	fmt.Println("----------------------")

	return nil
}

// stats calculates the arithmetic mean and sample standard deviation for a slice of float64.
func stats(vals []float64) (avg float64, sd float64) {
	if len(vals) == 0 {
		return 0, 0
	}
	sum := 0.0
	for _, v := range vals {
		sum += v
	}
	avg = sum / float64(len(vals))

	if len(vals) < 2 {
		return avg, 0
	}

	sqSum := 0.0
	for _, v := range vals {
		diff := v - avg
		sqSum += diff * diff
	}
	variance := sqSum / float64(len(vals)-1)
	sd = math.Sqrt(variance)
	return avg, sd
}

func min(a, b int) int {
	if a < b {
		return a
	}
	return b
}

// getOrRunCoverage retrieves cached coverage data for an iteration or executes
// the coverage collection script if the data is missing.
func getOrRunCoverage(t *taskConfig, root string, machine string, fuzzer string, corpus string, limitStr string, iter int, iterDir string) (IterCoverage, error) {
	var cov IterCoverage

	taskCovDir := filepath.Join(root, "coverage", machine, fuzzer, corpus, limitStr, fmt.Sprintf("iter_%d", iter))
	covJsonPath := filepath.Join(taskCovDir, "coverage.json")

	// If coverage already exists, load and return
	if fileutils.IsFile(covJsonPath, t.osWrapper) {
		covBytes, err := t.osWrapper.ReadFile(covJsonPath)
		if err == nil {
			if err := json.Unmarshal(covBytes, &cov); err == nil {
				return cov, nil
			}
		}
	}

	// Ensure coverage dir exists
	if err := t.osWrapper.MkdirAll(taskCovDir, 0755); err != nil {
		return cov, fmt.Errorf("failed to create coverage dir: %w", err)
	}

	// Corpus check
	corpusPath := filepath.Join(iterDir, "corpus")
	isEmpty, err := fileutils.IsEmptyDir(corpusPath, t.osWrapper)
	if err != nil || isEmpty {
		// Empty corpus -> 0 coverage
		covBytes, _ := json.MarshalIndent(cov, "", "  ")
		_ = t.osWrapper.WriteFile(covJsonPath, covBytes, 0644)
		return cov, nil
	}

	// Run coverage.py to generate LCOV file
	scriptPath := filepath.Join(fileutils.DawnRoot(t.osWrapper), "tools", "code_coverage", "coverage.py")
	binPath := filepath.Join(root, "bin", fuzzer+fileutils.ExeExt)

	// Since we need to run coverage.py, we invoke it via vpython3
	cmdArgs := []string{
		scriptPath,
		fuzzer,
		"--no-component-view",
		"-b", t.build,
		"-o", taskCovDir,
		"--format", "lcov",
		"-c", fmt.Sprintf("%s %s", binPath, corpusPath),
	}

	fmt.Printf("Generating coverage for %s/%s iter %d...\n", fuzzer, corpus, iter)
	if _, err := t.runCmd("vpython3", cmdArgs...); err != nil {
		return cov, fmt.Errorf("failed to execute coverage.py: %w", err)
	}

	// Parse generated LCOV file
	lcovFile, err := findLcovFile(t, taskCovDir)
	if err != nil {
		return cov, fmt.Errorf("failed to find generated LCOV file: %w", err)
	}

	lcovBytes, err := t.osWrapper.ReadFile(lcovFile)
	if err != nil {
		return cov, fmt.Errorf("failed to read LCOV file: %w", err)
	}

	cov = parseLcov(string(lcovBytes))

	// Cache result
	covBytes, _ := json.MarshalIndent(cov, "", "  ")
	_ = t.osWrapper.WriteFile(covJsonPath, covBytes, 0644)

	return cov, nil
}

// findLcovFile searches for the first .lcov file within the specified directory.
func findLcovFile(t *taskConfig, dir string) (string, error) {
	// Look recursively for .lcov
	var foundPath string
	err := filepath.Walk(dir, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}
		if !info.IsDir() && filepath.Ext(path) == ".lcov" {
			foundPath = path
			return filepath.SkipAll // Stop searching
		}
		return nil
	})
	if err != nil && err != filepath.SkipAll {
		return "", err
	}
	if foundPath == "" {
		return "", fmt.Errorf("no LCOV file found in %s", dir)
	}
	return foundPath, nil
}

// parseLcov parses the content of an LCOV file and categorizes coverage metrics
// into Tint Core, Mesa, and DirectX segments based on file paths.
func parseLcov(content string) IterCoverage {
	lines := strings.Split(content, "\n")
	var currentFile string
	var inTintCore, inMesa, inDx bool

	metrics := map[string]*CoverageStats{
		"tint_core": {},
		"mesa":      {},
		"directx":   {},
	}

	for _, line := range lines {
		line = strings.TrimSpace(line)
		if strings.HasPrefix(line, "SF:") {
			currentFile = strings.TrimPrefix(line, "SF:")
			currentFile = filepath.ToSlash(currentFile)

			inTintCore = strings.HasPrefix(currentFile, "src/tint/") || strings.HasPrefix(currentFile, "src/utils/")
			inMesa = strings.Contains(currentFile, "third_party/mesa/")
			inDx = strings.Contains(currentFile, "third_party/dx") || strings.Contains(currentFile, "third_party/directx")
		} else if strings.HasPrefix(line, "LF:") {
			val, _ := strconv.Atoi(strings.TrimPrefix(line, "LF:"))
			if inTintCore {
				metrics["tint_core"].LinesFound += val
			}
			if inMesa {
				metrics["mesa"].LinesFound += val
			}
			if inDx {
				metrics["directx"].LinesFound += val
			}
		} else if strings.HasPrefix(line, "LH:") {
			val, _ := strconv.Atoi(strings.TrimPrefix(line, "LH:"))
			if inTintCore {
				metrics["tint_core"].LinesHit += val
			}
			if inMesa {
				metrics["mesa"].LinesHit += val
			}
			if inDx {
				metrics["directx"].LinesHit += val
			}
		}
	}

	for _, stats := range metrics {
		if stats.LinesFound > 0 {
			stats.Percentage = (float64(stats.LinesHit) / float64(stats.LinesFound)) * 100.0
		}
	}

	return IterCoverage{
		TintCore: *metrics["tint_core"],
		Mesa:     *metrics["mesa"],
		DirectX:  *metrics["directx"],
	}
}
