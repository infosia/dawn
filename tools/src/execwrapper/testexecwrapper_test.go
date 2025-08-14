// Copyright 2025 The Dawn & Tint Authors
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
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

package execwrapper_test

import (
	"bytes"
	"context"
	"errors"
	"testing"

	"dawn.googlesource.com/dawn/tools/src/execwrapper"
	"github.com/stretchr/testify/require"
)

func TestTestExecWrapper_Command_NoOutput(t *testing.T) {
	t.Parallel()

	ew := execwrapper.NewTestExecWrapperForSuccess(nil, nil)
	cmd := ew.Command("test")
	var stdout, stderr bytes.Buffer
	cmd.WithStdout(&stdout).WithStderr(&stderr)

	err := cmd.Run()
	require.NoError(t, err)
	require.Empty(t, stdout.String())
	require.Empty(t, stderr.String())
}

func TestTestExecWrapper_Command_Stderr(t *testing.T) {
	t.Parallel()

	ew := execwrapper.NewTestExecWrapperForSuccess(nil, []byte("stderr"))
	cmd := ew.Command("test")
	var stdout, stderr bytes.Buffer
	cmd.WithStdout(&stdout).WithStderr(&stderr)

	err := cmd.Run()
	require.NoError(t, err)
	require.Empty(t, stdout.String())
	require.Equal(t, "stderr", stderr.String())
}

func TestTestExecWrapper_Command_Stdout(t *testing.T) {
	t.Parallel()

	ew := execwrapper.NewTestExecWrapperForSuccess([]byte("stdout"), nil)
	cmd := ew.Command("test")
	var stdout, stderr bytes.Buffer
	cmd.WithStdout(&stdout).WithStderr(&stderr)

	err := cmd.Run()
	require.NoError(t, err)
	require.Equal(t, "stdout", stdout.String())
	require.Empty(t, stderr.String())
}

func TestTestExecWrapper_Command_Error(t *testing.T) {
	t.Parallel()

	err := errors.New("test error")
	ew := execwrapper.NewTestExecWrapperForFailure(err)
	cmd := ew.Command("test")
	got := cmd.Run()
	require.ErrorIs(t, got, err)
}

func TestTestExecWrapper_CommandContext_NoOutput(t *testing.T) {
	t.Parallel()

	ew := execwrapper.NewTestExecWrapperForSuccess(nil, nil)
	cmd := ew.CommandContext(context.Background(), "test")
	var stdout, stderr bytes.Buffer
	cmd.WithStdout(&stdout).WithStderr(&stderr)

	err := cmd.Run()
	require.NoError(t, err)
	require.Empty(t, stdout.String())
	require.Empty(t, stderr.String())
}

func TestTestExecWrapper_CommandContext_Stderr(t *testing.T) {
	t.Parallel()

	ew := execwrapper.NewTestExecWrapperForSuccess(nil, []byte("stderr"))
	cmd := ew.CommandContext(context.Background(), "test")
	var stdout, stderr bytes.Buffer
	cmd.WithStdout(&stdout).WithStderr(&stderr)

	err := cmd.Run()
	require.NoError(t, err)
	require.Empty(t, stdout.String())
	require.Equal(t, "stderr", stderr.String())
}

func TestTestExecWrapper_CommandContext_Stdout(t *testing.T) {
	t.Parallel()

	ew := execwrapper.NewTestExecWrapperForSuccess([]byte("stdout"), nil)
	cmd := ew.CommandContext(context.Background(), "test")
	var stdout, stderr bytes.Buffer
	cmd.WithStdout(&stdout).WithStderr(&stderr)

	err := cmd.Run()
	require.NoError(t, err)
	require.Equal(t, "stdout", stdout.String())
	require.Empty(t, stderr.String())
}

func TestTestExecWrapper_CommandContext_Error(t *testing.T) {
	t.Parallel()

	err := errors.New("test error")
	ew := execwrapper.NewTestExecWrapperForFailure(err)
	cmd := ew.CommandContext(context.Background(), "test")
	got := cmd.Run()
	require.ErrorIs(t, got, err)
}

func TestTestCmdWrapper_RunCombinedOutput(t *testing.T) {
	t.Parallel()

	ew := execwrapper.NewTestExecWrapperForSuccess([]byte("stdout"), []byte("stderr"))
	cmd := ew.Command("test")
	out, err := cmd.RunWithCombinedOutput()
	require.NoError(t, err)
	require.Equal(t, "stdoutstderr", string(out))
}
