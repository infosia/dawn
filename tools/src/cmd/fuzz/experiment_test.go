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
	"path/filepath"
	"testing"

	"dawn.googlesource.com/dawn/tools/src/execwrapper"
	"dawn.googlesource.com/dawn/tools/src/oswrapper"
	"github.com/stretchr/testify/require"
)

func TestCheckGnArgs(t *testing.T) {
	tests := []struct {
		name    string
		gnJson  string
		fuzzers []string
		wantErr bool
	}{
		{
			name: "Correct baseline flags",
			gnJson: `[
				{"name": "use_libfuzzer", "current": {"value": "true"}},
				{"name": "tint_build_wgsl_reader", "current": {"value": "true"}},
				{"name": "use_clang_coverage", "current": {"value": "true"}},
				{"name": "optimize_for_fuzzing", "current": {"value": "false"}}
			]`,
			fuzzers: []string{"tint_wgsl_fuzzer"},
			wantErr: false,
		},
		{
			name: "Missing use_clang_coverage",
			gnJson: `[
				{"name": "use_libfuzzer", "current": {"value": "true"}},
				{"name": "tint_build_wgsl_reader", "current": {"value": "true"}},
				{"name": "optimize_for_fuzzing", "current": {"value": "false"}}
			]`,
			fuzzers: []string{"tint_wgsl_fuzzer"},
			wantErr: true,
		},
		{
			name: "optimize_for_fuzzing is true",
			gnJson: `[
				{"name": "use_libfuzzer", "current": {"value": "true"}},
				{"name": "tint_build_wgsl_reader", "current": {"value": "true"}},
				{"name": "use_clang_coverage", "current": {"value": "true"}},
				{"name": "optimize_for_fuzzing", "current": {"value": "true"}}
			]`,
			fuzzers: []string{"tint_wgsl_fuzzer"},
			wantErr: true,
		},
		{
			name: "Correct IR flags",
			gnJson: `[
				{"name": "use_libfuzzer", "current": {"value": "true"}},
				{"name": "tint_build_wgsl_reader", "current": {"value": "true"}},
				{"name": "use_clang_coverage", "current": {"value": "true"}},
				{"name": "optimize_for_fuzzing", "current": {"value": "false"}},
				{"name": "tint_build_ir_binary", "current": {"value": "true"}},
				{"name": "tint_has_protobuf", "current": {"value": "true"}}
			]`,
			fuzzers: []string{"tint_ir_fuzzer"},
			wantErr: false,
		},
		{
			name: "Missing IR protobuf flag",
			gnJson: `[
				{"name": "use_libfuzzer", "current": {"value": "true"}},
				{"name": "tint_build_wgsl_reader", "current": {"value": "true"}},
				{"name": "use_clang_coverage", "current": {"value": "true"}},
				{"name": "optimize_for_fuzzing", "current": {"value": "false"}},
				{"name": "tint_build_ir_binary", "current": {"value": "true"}}
			]`,
			fuzzers: []string{"tint_ir_fuzzer"},
			wantErr: true,
		},
	}

	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			fs := oswrapper.CreateFSTestOSWrapper()
			ew := execwrapper.NewTestExecWrapperForSuccess([]byte(tc.gnJson), nil)

			cfg := &taskConfig{
				mainConfig: mainConfig{
					build:       "/build",
					osWrapper:   fs,
					execWrapper: ew,
				},
			}

			_, err := checkGnArgs(cfg, tc.fuzzers)
			if tc.wantErr {
				require.Error(t, err)
			} else {
				require.NoError(t, err)
			}
		})
	}
}

func TestGenerateTasksForFuzzer(t *testing.T) {
	secs := 3600
	runs := 100000
	customIters := 2

	config := &ExperimentConfig{
		DefaultIterations: 5,
		Durations: []DurationDef{
			{Seconds: &secs},
			{Runs: &runs, Iterations: &customIters},
		},
	}

	corpus := CorpusDef{Name: "tint_tests", Path: "tint_tests_dir"}
	tasks := generateTasksForFuzzer("tint_wgsl_fuzzer", corpus, "/root/corpora", "/root/results/default", config)

	// We expect 5 iterations for seconds (3600) + 2 iterations for runs (100000) = 7 tasks total
	require.Len(t, tasks, 7)

	// Verify details of first task
	require.Equal(t, "tint_wgsl_fuzzer", tasks[0].FuzzerName)
	require.Equal(t, "tint_tests", tasks[0].CorpusName)
	require.Equal(t, filepath.Join("/root/corpora", "tint_tests_dir"), tasks[0].CorpusPath)
	require.Equal(t, "seconds", tasks[0].LimitType)
	require.Equal(t, 3600, tasks[0].LimitValue)
	require.Equal(t, 1, tasks[0].Iteration)
	require.Equal(t, filepath.Join("/root/results/default", "tint_wgsl_fuzzer", "tint_tests", "seconds_3600", "iter_1"), tasks[0].TaskDir)

	// Verify details of last task (custom iterations)
	lastIdx := len(tasks) - 1
	require.Equal(t, "runs", tasks[lastIdx].LimitType)
	require.Equal(t, 100000, tasks[lastIdx].LimitValue)
	require.Equal(t, 2, tasks[lastIdx].Iteration)
	require.Equal(t, filepath.Join("/root/results/default", "tint_wgsl_fuzzer", "tint_tests", "runs_100000", "iter_2"), tasks[lastIdx].TaskDir)
}
