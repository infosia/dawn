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
	"image/color"
	"math"
	"os"
	"path/filepath"
	"sort"
	"strconv"
	"strings"
	"unicode/utf8"

	"dawn.googlesource.com/dawn/tools/src/fileutils"
	"gonum.org/v1/plot"
	"gonum.org/v1/plot/plotter"
	"gonum.org/v1/plot/vg"
	"gonum.org/v1/plot/vg/draw"
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
	NormalizedSecs  []float64
	CoveragePercent []float64
	CoveragePerSec  []float64
}

var (
	// Subset of the available shapes that are clear/easy to read
	plotShapes = []draw.GlyphDrawer{
		draw.CircleGlyph{},
		draw.BoxGlyph{},
		draw.PyramidGlyph{},
		draw.CrossGlyph{},
	}
)

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
						cov, err := getOrRunCoverage(t, t.reportPath, machine, fuzzer, corpus, limitStr, i, iterDir)
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

			// Normalization: CPU Seconds = (ActualSeconds * PerfScore) / 1000
			normalizedSecs := (ci.State.ActualSeconds * ci.State.PerfScore) / 1000.0
			agg.NormalizedSecs = append(agg.NormalizedSecs, normalizedSecs)
			agg.CoveragePercent = append(agg.CoveragePercent, seg.Stats.Percentage)

			covPerSec := 0.0
			if normalizedSecs > 0 {
				covPerSec = seg.Stats.Percentage / normalizedSecs
			}
			agg.CoveragePerSec = append(agg.CoveragePerSec, covPerSec)
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

	reportBuilder.WriteString("\n## Detailed Coverage Trajectories\n\n")

	// Group trajectory data by Fuzzer, Corpus, and Segment
	type TrajectoryPoint struct {
		LimitType    string
		LimitValue   int
		NormSecsAvg  float64
		NormSecsSem  float64
		CovAvg       float64
		CovSem       float64
		CovRate      float64
		CovRateError float64
		N            int
	}

	trajectories := make(map[string][]TrajectoryPoint)
	for _, k := range keys {
		agg := aggregations[k]
		covAvg, covSd := stats(agg.CoveragePercent)
		normSecsAvg, normSecsSd := stats(agg.NormalizedSecs)
		N := len(agg.CoveragePercent)

		covSem := 0.0
		if N > 0 {
			covSem = covSd / math.Sqrt(float64(N))
		}

		normSecsSem := 0.0
		if N > 0 {
			normSecsSem = normSecsSd / math.Sqrt(float64(N))
		}

		covRate := 0.0
		if normSecsAvg > 0 {
			covRate = covAvg / normSecsAvg
		}

		// Quadrature error propagation: SE_R = R * sqrt((SE_C / C)^2 + (SE_T / T)^2)
		covRateError := 0.0
		if covRate > 0 && covAvg > 0 && normSecsAvg > 0 {
			relCovErr := covSem / covAvg
			relTimeErr := normSecsSem / normSecsAvg
			covRateError = covRate * math.Sqrt(relCovErr*relCovErr+relTimeErr*relTimeErr)
		}

		tKey := fmt.Sprintf("%s - %s (%s)", agg.FuzzerName, agg.CorpusName, agg.Segment)
		trajectories[tKey] = append(trajectories[tKey], TrajectoryPoint{
			LimitType:    agg.LimitType,
			LimitValue:   agg.LimitValue,
			NormSecsAvg:  normSecsAvg,
			NormSecsSem:  normSecsSem,
			CovAvg:       covAvg,
			CovSem:       covSem,
			CovRate:      covRate,
			CovRateError: covRateError,
			N:            N,
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

		trajectoryHeaders := []string{"Samples (N)", "Target Limit", "Normalized CPU Seconds (Avg ± SEM)", "Coverage % (Avg ± SEM)", "Coverage Rate (%/sec) (Avg ± SEM)"}
		var trajectoryRows [][]string
		for _, pt := range points {
			limitStr := fmt.Sprintf("%d %s", pt.LimitValue, pt.LimitType)
			normSecsStr := fmt.Sprintf("%.2f ± %.2f", pt.NormSecsAvg, pt.NormSecsSem)
			covStr := fmt.Sprintf("%.2f%% ± %.2f%%", pt.CovAvg, pt.CovSem)
			covRateStr := fmt.Sprintf("%.6f ± %.6f", pt.CovRate, pt.CovRateError)
			nStr := fmt.Sprintf("%d", pt.N)

			trajectoryRows = append(trajectoryRows, []string{
				nStr,
				limitStr,
				normSecsStr,
				covStr,
				covRateStr,
			})
		}
		reportBuilder.WriteString(formatMarkdownTable(trajectoryHeaders, trajectoryRows))
		reportBuilder.WriteString("\n")
	}

	// Save Report File
	reportFile := filepath.Join(t.reportPath, "experiment_report.md")
	if err := t.osWrapper.WriteFile(reportFile, []byte(reportBuilder.String()), 0644); err != nil {
		return fmt.Errorf("failed to write report file: %w", err)
	}

	// Generate Raw Iteration CSV Output
	var csvBuilder strings.Builder
	csvBuilder.WriteString("Machine,Fuzzer,Corpus,LimitType,LimitValue,Iteration,PerfScore,ActualSeconds,ActualRuns,NormalizedCPUSeconds," +
		"TintCore_LinesFound,TintCore_LinesHit,TintCore_CoveragePercent," +
		"Mesa_LinesFound,Mesa_LinesHit,Mesa_CoveragePercent," +
		"DirectX_LinesFound,DirectX_LinesHit,DirectX_CoveragePercent\n")

	for _, ci := range completedIters {
		normalizedSecs := (ci.State.ActualSeconds * ci.State.PerfScore) / 1000.0

		csvBuilder.WriteString(fmt.Sprintf("%s,%s,%s,%s,%d,%d,%.4f,%.2f,%d,%.4f,%d,%d,%.2f,%d,%d,%.2f,%d,%d,%.2f\n",
			ci.Machine,
			ci.Fuzzer,
			ci.Corpus,
			ci.LimitType,
			ci.LimitValue,
			ci.Iter,
			ci.State.PerfScore,
			ci.State.ActualSeconds,
			ci.State.ActualRuns,
			normalizedSecs,
			ci.Coverage.TintCore.LinesFound,
			ci.Coverage.TintCore.LinesHit,
			ci.Coverage.TintCore.Percentage,
			ci.Coverage.Mesa.LinesFound,
			ci.Coverage.Mesa.LinesHit,
			ci.Coverage.Mesa.Percentage,
			ci.Coverage.DirectX.LinesFound,
			ci.Coverage.DirectX.LinesHit,
			ci.Coverage.DirectX.Percentage,
		))
	}

	csvFile := filepath.Join(t.reportPath, "raw_iteration_data.csv")
	if err := t.osWrapper.WriteFile(csvFile, []byte(csvBuilder.String()), 0644); err != nil {
		return fmt.Errorf("failed to write raw CSV file: %w", err)
	}

	// Generate trajectory plots inside `<root>/plots/`
	if err := generatePlots(t.reportPath, aggregations, keys); err != nil {
		return fmt.Errorf("failed to generate fuzzer comparison plots: %w", err)
	}

	fmt.Printf("\nReport generated successfully at: %s\n", reportFile)
	fmt.Printf("Raw iteration metrics exported successfully to: %s\n", csvFile)
	fmt.Printf("Visual trajectory graphs generated inside: %s/plots/\n", t.reportPath)
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
func getOrRunCoverage(t *taskConfig, root string, machine string, fuzzer string, corpus CorpusDef, limitStr string, iter int, iterDir string) (IterCoverage, error) {
	var cov IterCoverage

	taskCovDir := filepath.Join(root, "coverage", machine, fuzzer, corpus.Name, limitStr, fmt.Sprintf("iter_%d", iter))
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

	absTaskCovDir, err := filepath.Abs(taskCovDir)
	if err != nil {
		return cov, err
	}

	absBinDir, err := filepath.Abs(filepath.Join(root, "bin"))
	if err != nil {
		return cov, err
	}

	// Find .profraw files inside the original iteration directory
	profraws, err := findProfrawFiles(t, iterDir)
	if err != nil {
		return cov, fmt.Errorf("failed to scan for .profraw files inside %s: %w", iterDir, err)
	}

	if len(profraws) == 0 {
		return cov, fmt.Errorf("no .profraw file found inside iteration directory '%s' (make sure the experiment ran with coverage enabled)", iterDir)
	}

	// Merge .profraw files using llvm-profdata. We write the merged .profdata
	// into the fuzzer results folder (iterDir) because coverage.py completely
	// clears taskCovDir (shutil.rmtree) on startup, which would delete our merged file!
	llvmProfDataPath := filepath.Join(fileutils.DawnRoot(t.osWrapper), "third_party", "llvm-build", "Release+Asserts", "bin", "llvm-profdata"+fileutils.ExeExt)
	profdataPath := filepath.Join(iterDir, "coverage.profdata")

	absProfdataPath, err := filepath.Abs(profdataPath)
	if err != nil {
		return cov, err
	}

	mergeArgs := []string{"merge", "-o", absProfdataPath, "-sparse=true"}
	mergeArgs = append(mergeArgs, profraws...)
	if _, err := t.runCmd(llvmProfDataPath, mergeArgs...); err != nil {
		return cov, fmt.Errorf("failed to merge .profraw files using %s: %w", llvmProfDataPath, err)
	}

	// Run coverage.py using the already-generated profdata file and pointing the build-dir to our archived binaries directory
	scriptPath := filepath.Join(fileutils.DawnRoot(t.osWrapper), "tools", "code_coverage", "coverage.py")

	// Since we need to run coverage.py, we invoke it via vpython3
	cmdArgs := []string{
		scriptPath,
		fuzzer,
		"--no-component-view",
		"-b", absBinDir,
		"-o", absTaskCovDir,
		"--format", "lcov",
		"-p", absProfdataPath,
	}

	fmt.Printf("Generating coverage for %s/%s (%s) iter %d...\n", fuzzer, corpus.Name, limitStr, iter)
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

			inTintCore = strings.Contains(currentFile, "src/tint/") || strings.Contains(currentFile, "src/utils/")
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

func findProfrawFiles(t *taskConfig, dir string) ([]string, error) {
	var paths []string
	err := filepath.Walk(dir, func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}
		if !info.IsDir() && filepath.Ext(path) == ".profraw" {
			paths = append(paths, path)
		}
		return nil
	})
	return paths, err
}

func formatMarkdownTable(headers []string, rows [][]string) string {
	numCols := len(headers)
	colWidths := make([]int, numCols)

	// Calculate column widths from headers
	for i, h := range headers {
		colWidths[i] = utf8.RuneCountInString(h)
	}

	// Calculate column widths from rows
	for _, row := range rows {
		for i, val := range row {
			valLen := utf8.RuneCountInString(val)
			if i < numCols && valLen > colWidths[i] {
				colWidths[i] = valLen
			}
		}
	}

	var sb strings.Builder

	// Write header row
	sb.WriteString("|")
	for i, h := range headers {
		padding := colWidths[i] - utf8.RuneCountInString(h)
		sb.WriteString(" " + h + strings.Repeat(" ", padding) + " |")
	}
	sb.WriteString("\n")

	// Write separator row
	sb.WriteString("|")
	for i := 0; i < numCols; i++ {
		sb.WriteString(" " + strings.Repeat("-", colWidths[i]) + " |")
	}
	sb.WriteString("\n")

	// Write data rows
	for _, row := range rows {
		sb.WriteString("|")
		for i, val := range row {
			padding := 0
			if i < numCols {
				padding = colWidths[i] - utf8.RuneCountInString(val)
			}
			sb.WriteString(" " + val + strings.Repeat(" ", padding) + " |")
		}
		sb.WriteString("\n")
	}

	return sb.String()
}

type errorPoints struct {
	plotter.XYs
	Errors []float64
}

func (e errorPoints) YError(i int) (float64, float64) {
	return e.Errors[i], e.Errors[i]
}

func savePlot(p *plot.Plot, filename string, absoluteY bool) error {
	if absoluteY {
		p.Y.Min = 0.0
		p.Y.Max = 100.0
	}
	return p.Save(6*vg.Inch, 4*vg.Inch, filename)
}

func generatePlots(root string, aggregations map[string]*AggregatedResult, keys []string) error {
	plotsDir := filepath.Join(root, "plots")
	_ = os.MkdirAll(plotsDir, 0755)

	// 1. Group trajectory points by Fuzzer, Corpus, Segment
	type TrajKey struct {
		Fuzzer  string
		Corpus  string
		Segment string
	}

	type TrajPoint struct {
		NormSecs     float64
		Coverage     float64
		Sem          float64
		CovRate      float64
		CovRateError float64
	}

	trajPoints := make(map[TrajKey][]TrajPoint)
	for _, k := range keys {
		agg := aggregations[k]
		covAvg, covSd := stats(agg.CoveragePercent)
		normSecAvg, normSecSd := stats(agg.NormalizedSecs)
		N := len(agg.CoveragePercent)

		covSem := 0.0
		if N > 0 {
			covSem = covSd / math.Sqrt(float64(N))
		}

		normSecsSem := 0.0
		if N > 0 {
			normSecsSem = normSecSd / math.Sqrt(float64(N))
		}

		covRate := 0.0
		if normSecAvg > 0 {
			covRate = covAvg / normSecAvg
		}

		covRateError := 0.0
		if covRate > 0 && covAvg > 0 && normSecAvg > 0 {
			relCovErr := covSem / covAvg
			relTimeErr := normSecsSem / normSecAvg
			covRateError = covRate * math.Sqrt(relCovErr*relCovErr+relTimeErr*relTimeErr)
		}

		key := TrajKey{Fuzzer: agg.FuzzerName, Corpus: agg.CorpusName, Segment: agg.Segment}
		trajPoints[key] = append(trajPoints[key], TrajPoint{
			NormSecs:     normSecAvg,
			Coverage:     covAvg,
			Sem:          covSem,
			CovRate:      covRate,
			CovRateError: covRateError,
		})
	}

	// Sort points by NormSecs for each line
	for k := range trajPoints {
		sort.Slice(trajPoints[k], func(i, j int) bool {
			return trajPoints[k][i].NormSecs < trajPoints[k][j].NormSecs
		})
	}

	// 2. Generate individual fuzzer plots (Total Coverage & Coverage Rate)
	for k, pts := range trajPoints {
		sanitize := func(s string) string {
			s = strings.ReplaceAll(s, " ", "_")
			s = strings.ReplaceAll(s, "::", "_")
			return strings.ToLower(s)
		}
		basePrefix := filepath.Join(plotsDir, fmt.Sprintf("%s_%s_%s", sanitize(k.Fuzzer), sanitize(k.Corpus), sanitize(k.Segment)))

		// A. Total Coverage (Y = Coverage, X = NormSecs)
		{
			p := plot.New()
			p.Title.Text = fmt.Sprintf("Total Coverage for %s\n(%s on %s)", k.Fuzzer, k.Segment, k.Corpus)
			p.X.Label.Text = "Normalized CPU Seconds"
			p.Y.Label.Text = "Coverage %"

			xyPts := make(plotter.XYs, len(pts))
			sems := make([]float64, len(pts))
			for i, pt := range pts {
				xyPts[i].X = pt.NormSecs
				xyPts[i].Y = pt.Coverage
				sems[i] = pt.Sem
			}

			l, s, err := plotter.NewLinePoints(xyPts)
			if err != nil {
				return err
			}
			l.Color = color.Black
			s.Color = color.Black
			s.GlyphStyle.Shape = plotShapes[0]
			p.Add(l, s)

			eb, err := plotter.NewYErrorBars(errorPoints{XYs: xyPts, Errors: sems})
			if err != nil {
				return err
			}
			eb.Color = color.Black
			p.Add(eb)

			// Save Natural
			if err := savePlot(p, basePrefix+"_total_coverage_natural.svg", false); err != nil {
				return err
			}
			// Save Absolute (0 to 100)
			if err := savePlot(p, basePrefix+"_total_coverage_absolute.svg", true); err != nil {
				return err
			}
		}

		// B. Coverage Rate (Y = CovRate, X = NormSecs)
		{
			p := plot.New()
			p.Title.Text = fmt.Sprintf("Coverage Rate for %s\n(%s on %s)", k.Fuzzer, k.Segment, k.Corpus)
			p.X.Label.Text = "Normalized CPU Seconds"
			p.Y.Label.Text = "Coverage Rate (%/sec)"

			xyPts := make(plotter.XYs, len(pts))
			sems := make([]float64, len(pts))
			for i, pt := range pts {
				xyPts[i].X = pt.NormSecs
				xyPts[i].Y = pt.CovRate
				sems[i] = pt.CovRateError
			}

			l, s, err := plotter.NewLinePoints(xyPts)
			if err != nil {
				return err
			}
			l.Color = color.Black
			s.Color = color.Black
			s.GlyphStyle.Shape = plotShapes[0]
			p.Add(l, s)

			eb, err := plotter.NewYErrorBars(errorPoints{XYs: xyPts, Errors: sems})
			if err != nil {
				return err
			}
			eb.Color = color.Black
			p.Add(eb)

			// Save Natural only (absolute Y 0-100 doesn't apply to rates)
			if err := savePlot(p, basePrefix+"_coverage_rate_natural.svg", false); err != nil {
				return err
			}
		}
	}

	// 3. Generate comparison plots comparing different Fuzzers for the same Corpus & Segment
	type CompKey struct {
		Corpus  string
		Segment string
	}

	compGroup := make(map[CompKey][]string) // maps CompKey to list of Fuzzers
	for k := range trajPoints {
		ck := CompKey{Corpus: k.Corpus, Segment: k.Segment}
		found := false
		for _, f := range compGroup[ck] {
			if f == k.Fuzzer {
				found = true
				break
			}
		}
		if !found {
			compGroup[ck] = append(compGroup[ck], k.Fuzzer)
		}
	}

	for ck, fuzzers := range compGroup {
		if len(fuzzers) <= 1 {
			continue // only compare if multiple fuzzers exist
		}
		if len(fuzzers) > len(plotShapes) {
			return fmt.Errorf("cannot plot comparison of %d fuzzers on %s (%s): exceeds the maximum of %d highly-legible glyph symbols inside plotShapes. Please update the experiment config or add more symbols", len(fuzzers), ck.Corpus, ck.Segment, len(plotShapes))
		}

		sanitize := func(s string) string {
			s = strings.ReplaceAll(s, " ", "_")
			return strings.ToLower(s)
		}
		basePrefix := filepath.Join(plotsDir, fmt.Sprintf("compare_fuzzers_%s_%s", sanitize(ck.Corpus), sanitize(ck.Segment)))

		// A. Total Coverage Comparison
		{
			p := plot.New()
			p.Title.Text = fmt.Sprintf("Fuzzer Comparison: %s on %s (Total Coverage)", ck.Segment, ck.Corpus)
			p.X.Label.Text = "Normalized CPU Seconds"
			p.Y.Label.Text = "Coverage %"
			p.Legend.Left = true
			p.Legend.Top = true
			p.Legend.XOffs = 5
			p.Legend.YOffs = -5

			for idx, fuzzer := range fuzzers {
				tk := TrajKey{Fuzzer: fuzzer, Corpus: ck.Corpus, Segment: ck.Segment}
				pts := trajPoints[tk]

				xyPts := make(plotter.XYs, len(pts))
				sems := make([]float64, len(pts))
				for i, pt := range pts {
					xyPts[i].X = pt.NormSecs
					xyPts[i].Y = pt.Coverage
					sems[i] = pt.Sem
				}

				l, s, err := plotter.NewLinePoints(xyPts)
				if err != nil {
					return err
				}
				l.Color = color.Black
				s.Color = color.Black
				s.GlyphStyle.Shape = plotShapes[idx%len(plotShapes)]

				p.Add(l, s)
				p.Legend.Add(fuzzer, l, s)

				// Add black error bars
				eb, err := plotter.NewYErrorBars(errorPoints{XYs: xyPts, Errors: sems})
				if err != nil {
					return err
				}
				eb.Color = color.Black
				p.Add(eb)
			}

			if err := savePlot(p, basePrefix+"_total_coverage_natural.svg", false); err != nil {
				return err
			}
			if err := savePlot(p, basePrefix+"_total_coverage_absolute.svg", true); err != nil {
				return err
			}
		}

		// B. Coverage Rate Comparison
		{
			p := plot.New()
			p.Title.Text = fmt.Sprintf("Fuzzer Comparison: %s on %s (Coverage Rate)", ck.Segment, ck.Corpus)
			p.X.Label.Text = "Normalized CPU Seconds"
			p.Y.Label.Text = "Coverage Rate (%/sec)"
			p.Legend.Left = true
			p.Legend.Top = true
			p.Legend.XOffs = 5
			p.Legend.YOffs = -5

			for idx, fuzzer := range fuzzers {
				tk := TrajKey{Fuzzer: fuzzer, Corpus: ck.Corpus, Segment: ck.Segment}
				pts := trajPoints[tk]

				xyPts := make(plotter.XYs, len(pts))
				sems := make([]float64, len(pts))
				for i, pt := range pts {
					xyPts[i].X = pt.NormSecs
					xyPts[i].Y = pt.CovRate
					sems[i] = pt.CovRateError
				}

				l, s, err := plotter.NewLinePoints(xyPts)
				if err != nil {
					return err
				}
				l.Color = color.Black
				s.Color = color.Black
				s.GlyphStyle.Shape = plotShapes[idx%len(plotShapes)]

				p.Add(l, s)
				p.Legend.Add(fuzzer, l, s)

				// Add black error bars
				eb, err := plotter.NewYErrorBars(errorPoints{XYs: xyPts, Errors: sems})
				if err != nil {
					return err
				}
				eb.Color = color.Black
				p.Add(eb)
			}

			if err := savePlot(p, basePrefix+"_coverage_rate_natural.svg", false); err != nil {
				return err
			}
		}
	}

	// 4. Generate comparison plots comparing different Corpora for the same Fuzzer & Segment
	type CorpCompKey struct {
		Fuzzer  string
		Segment string
	}

	corpGroup := make(map[CorpCompKey][]string) // maps CorpCompKey to list of Corpora
	for k := range trajPoints {
		cck := CorpCompKey{Fuzzer: k.Fuzzer, Segment: k.Segment}
		found := false
		for _, c := range corpGroup[cck] {
			if c == k.Corpus {
				found = true
				break
			}
		}
		if !found {
			corpGroup[cck] = append(corpGroup[cck], k.Corpus)
		}
	}

	for cck, corpora := range corpGroup {
		if len(corpora) <= 1 {
			continue // only compare if multiple corpora exist
		}
		if len(corpora) > len(plotShapes) {
			return fmt.Errorf("cannot plot comparison of %d corpora for %s (%s): exceeds the maximum of %d highly-legible glyph symbols inside plotShapes. Please update the experiment config or add more symbols", len(corpora), cck.Fuzzer, cck.Segment, len(plotShapes))
		}

		sanitize := func(s string) string {
			s = strings.ReplaceAll(s, " ", "_")
			s = strings.ReplaceAll(s, "::", "_")
			return strings.ToLower(s)
		}
		basePrefix := filepath.Join(plotsDir, fmt.Sprintf("compare_corpora_%s_%s", sanitize(cck.Fuzzer), sanitize(cck.Segment)))

		// A. Total Coverage Corpora Comparison
		{
			p := plot.New()
			p.Title.Text = fmt.Sprintf("Corpora Comparison for %s (%s, Total Coverage)", cck.Fuzzer, cck.Segment)
			p.X.Label.Text = "Normalized CPU Seconds"
			p.Y.Label.Text = "Coverage %"
			p.Legend.Left = true
			p.Legend.Top = true
			p.Legend.XOffs = 5
			p.Legend.YOffs = -5

			for idx, corpus := range corpora {
				tk := TrajKey{Fuzzer: cck.Fuzzer, Corpus: corpus, Segment: cck.Segment}
				pts := trajPoints[tk]

				xyPts := make(plotter.XYs, len(pts))
				sems := make([]float64, len(pts))
				for i, pt := range pts {
					xyPts[i].X = pt.NormSecs
					xyPts[i].Y = pt.Coverage
					sems[i] = pt.Sem
				}

				l, s, err := plotter.NewLinePoints(xyPts)
				if err != nil {
					return err
				}
				l.Color = color.Black
				s.Color = color.Black
				s.GlyphStyle.Shape = plotShapes[idx%len(plotShapes)]

				p.Add(l, s)
				p.Legend.Add(corpus, l, s)

				// Add black error bars
				eb, err := plotter.NewYErrorBars(errorPoints{XYs: xyPts, Errors: sems})
				if err != nil {
					return err
				}
				eb.Color = color.Black
				p.Add(eb)
			}

			if err := savePlot(p, basePrefix+"_total_coverage_natural.svg", false); err != nil {
				return err
			}
			if err := savePlot(p, basePrefix+"_total_coverage_absolute.svg", true); err != nil {
				return err
			}
		}

		// B. Coverage Rate Corpora Comparison
		{
			p := plot.New()
			p.Title.Text = fmt.Sprintf("Corpora Comparison for %s (%s, Coverage Rate)", cck.Fuzzer, cck.Segment)
			p.X.Label.Text = "Normalized CPU Seconds"
			p.Y.Label.Text = "Coverage Rate (%/sec)"
			p.Legend.Left = true
			p.Legend.Top = true
			p.Legend.XOffs = 5
			p.Legend.YOffs = -5

			for idx, corpus := range corpora {
				tk := TrajKey{Fuzzer: cck.Fuzzer, Corpus: corpus, Segment: cck.Segment}
				pts := trajPoints[tk]

				xyPts := make(plotter.XYs, len(pts))
				sems := make([]float64, len(pts))
				for i, pt := range pts {
					xyPts[i].X = pt.NormSecs
					xyPts[i].Y = pt.CovRate
					sems[i] = pt.CovRateError
				}

				l, s, err := plotter.NewLinePoints(xyPts)
				if err != nil {
					return err
				}
				l.Color = color.Black
				s.Color = color.Black
				s.GlyphStyle.Shape = plotShapes[idx%len(plotShapes)]

				p.Add(l, s)
				p.Legend.Add(corpus, l, s)

				// Add black error bars
				eb, err := plotter.NewYErrorBars(errorPoints{XYs: xyPts, Errors: sems})
				if err != nil {
					return err
				}
				eb.Color = color.Black
				p.Add(eb)
			}

			if err := savePlot(p, basePrefix+"_coverage_rate_natural.svg", false); err != nil {
				return err
			}
		}
	}

	return nil
}
