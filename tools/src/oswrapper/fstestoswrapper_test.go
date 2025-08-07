// Copyright 2025 The Dawn & Tint Authors
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
package oswrapper_test

import (
	"errors"
	"os"
	"path/filepath"
	"strings"
	"syscall"
	"testing"
	"testing/fstest"

	"dawn.googlesource.com/dawn/tools/src/oswrapper"
	"github.com/stretchr/testify/require"
)

// NOTE: There are two types of tests in this file, those suffixed with _MatchesReal and those that
// are not. Those that are suffixed are meant to be testing the behaviour of the FSTestOSWrapper
// against the RealOSWrapper to confirm that it is a drop in replacement. Those that are not
// suffixed are traditional unittests that test the implementation functions in isolation against
// defined expectations.

// --- Generic helpers ---

// stringPtr is a helper to get a pointer to a string, used so test cases can have nil to indicate a
// string value isn't set, instead of testing for "".
func stringPtr(s string) *string {
	return &s
}

// --- Unittest specific helpers ---

// unittestSetup is a helper for setting up the FSTestOSWrapper for a unittest.
type unittestSetup struct {
	initialFiles map[string]string
	initialDirs  []string
}

// setup initializes the FSTestOSWrapper with the files and directories specified
// in the unittestSetup struct.
func (s unittestSetup) setup(t *testing.T) oswrapper.FSTestOSWrapper {
	t.Helper()
	testFS := oswrapper.CreateFSTestOSWrapper()

	for path, content := range s.initialFiles {
		// All paths in unittests are rooted at "/"
		testPath := path
		if !strings.HasPrefix(testPath, "/") {
			testPath = "/" + testPath
		}
		// MkdirAll will handle the case where the parent is the root.
		require.NoError(t, testFS.MkdirAll(filepath.Dir(testPath), 0755))
		require.NoError(t, testFS.WriteFile(testPath, []byte(content), 0666))
	}
	for _, path := range s.initialDirs {
		testPath := path
		if !strings.HasPrefix(testPath, "/") {
			testPath = "/" + testPath
		}
		require.NoError(t, testFS.MkdirAll(testPath, 0755))
	}
	return testFS
}

// expectedError is a helper struct for testing error conditions.
// Note: This is meant for usage in the non-*_MatchesReal unittests.
type expectedError struct {
	wantErrIs  error
	wantErrMsg string
}

// Check verifies that the given error `err` matches the expected error conditions.
// It returns true if an error was expected and found, indicating the test should stop.
func (e expectedError) Check(t *testing.T, err error) (stopTest bool) {
	t.Helper()
	if e.wantErrIs == nil && e.wantErrMsg == "" {
		require.NoError(t, err)
		return false
	}

	require.Error(t, err)
	if e.wantErrIs != nil {
		require.ErrorIs(t, err, e.wantErrIs)
	}
	if e.wantErrMsg != "" {
		require.ErrorContains(t, err, e.wantErrMsg)
	}
	return true
}

// --- *_MatchesReal specific helpers ---

// matchesRealSetup is a helper for setting up the filesystem for a test case that
// matches against the real OS.
// Note: This is meant for usage in the *_MatchesReal tests.
type matchesRealSetup struct {
	unittestSetup
}

// setup creates a temporary directory for the real OS, and initializes both the
// real and test filesystems with the files and directories specified in the
// matchesRealSetup struct.
func (s matchesRealSetup) setup(t *testing.T) (string, oswrapper.OSWrapper, oswrapper.FSTestOSWrapper) {
	t.Helper()

	// Setup the test wrapper using the embedded setup method.
	testFS := s.unittestSetup.setup(t)

	// Setup the real FS
	realRoot, err := os.MkdirTemp("", "real_fs_test_*")
	require.NoError(t, err)
	realFS := oswrapper.GetRealOSWrapper()

	for path, content := range s.initialFiles {
		realPath := filepath.Join(realRoot, path)
		require.NoError(t, os.MkdirAll(filepath.Dir(realPath), 0755))
		require.NoError(t, os.WriteFile(realPath, []byte(content), 0666))
	}
	for _, path := range s.initialDirs {
		require.NoError(t, os.MkdirAll(filepath.Join(realRoot, path), 0755))
	}

	return realRoot, realFS, testFS
}

// requireFileSystemsMatch walks the real filesystem at realRoot and compares its
// structure and file content against the provided FSTestOSWrapper.
// Note: This is meant for usage in *_MatchesReal tests.
func requireFileSystemsMatch(t *testing.T, realRoot string, testFS oswrapper.FSTestOSWrapper) {
	t.Helper()

	realMap := make(map[string]*fstest.MapFile)
	err := filepath.Walk(realRoot, func(path string, info os.FileInfo, err error) error {
		require.NoError(t, err)
		if path == realRoot {
			return nil
		}

		relPath, err := filepath.Rel(realRoot, path)
		require.NoError(t, err)

		// Use the same path cleaning logic as FSTestOSWrapper.
		mapKey := testFS.FSTestFilesystemReaderWriter.CleanPath(relPath)

		mapFile := &fstest.MapFile{Mode: info.Mode()}
		if !info.IsDir() {
			data, err := os.ReadFile(path)
			require.NoError(t, err)
			mapFile.Data = data
		}
		realMap[mapKey] = mapFile
		return nil
	})
	require.NoError(t, err)

	testMap := testFS.FS
	require.Equal(t, len(realMap), len(testMap), "Filesystems have a different number of entries")

	for key, realFile := range realMap {
		testFile, ok := testMap[key]
		require.True(t, ok, "Path '%s' exists in real FS but not in FS under test", key)

		require.Equal(t, realFile.Mode.IsDir(), testFile.Mode.IsDir(), "IsDir mismatch for '%s'", key)

		if !realFile.Mode.IsDir() {
			require.Equal(t, realFile.Data, testFile.Data, "Content mismatch for file '%s'", key)
		}
	}
}

// requireErrorsMatch asserts that the real and test errors are compatible.
// Both can be nil. If the real error is a well-known os error, the test error
// must be the same. Otherwise, it just checks that both are non-nil.
// Note: This is meant for usage in *_MatchesReal tests.
func requireErrorsMatch(t *testing.T, realErr, testErr error) {
	t.Helper()
	if realErr != nil {
		require.Error(t, testErr, "Real FS errored but FS under test did not.\nReal error: %v", realErr)

		// For certain well-defined errors, we expect the test wrapper to return
		// the exact same error type.
		if errors.Is(realErr, os.ErrNotExist) {
			require.ErrorIs(t, testErr, os.ErrNotExist, "Real error is os.ErrNotExist, but test error is not")
		} else if errors.Is(realErr, os.ErrExist) {
			require.ErrorIs(t, testErr, os.ErrExist, "Real error is os.ErrExist, but test error is not")
		} else if errors.Is(realErr, syscall.ENOTEMPTY) {
			require.ErrorIs(t, testErr, syscall.ENOTEMPTY, "Real error is syscall.ENOTEMPTY, but test error is not")
		}
		// For other errors (e.g., 'is a directory', 'directory not empty'),
		// the exact error message can be OS-dependent. In these cases, just
		// checking that *an* error occurred is sufficient.
	} else {
		require.NoError(t, testErr, "FS under test errored but Real FS did not.\nTest error: %v", testErr)
	}
}

// --- FilesystemReader tests ---

func TestFSTestOSWrapper_ReadFile(t *testing.T) {
	tests := []struct {
		name            string
		setup           unittestSetup
		path            string
		expectedContent []byte
		expectedError
	}{
		{
			name: "Read existing file",
			path: filepath.Join("/", "file.txt"),
			setup: unittestSetup{
				initialFiles: map[string]string{"/file.txt": "hello world"},
			},
			expectedContent: []byte("hello world"),
		},
		{
			name: "Read non-existent file",
			path: filepath.Join("/", "nonexistent.txt"),
			expectedError: expectedError{
				wantErrIs: os.ErrNotExist,
			},
		},
		{
			name: "Read a directory",
			path: filepath.Join("/", "mydir"),
			setup: unittestSetup{
				initialDirs: []string{"/mydir"},
			},
			expectedError: expectedError{
				wantErrMsg: "is a directory",
			},
		},
	}

	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			wrapper := tc.setup.setup(t)
			content, err := wrapper.ReadFile(tc.path)

			if tc.expectedError.Check(t, err) {
				return
			}

			require.Equal(t, tc.expectedContent, content)
		})
	}
}

func TestFSTestOSWrapper_ReadFile_MatchesReal(t *testing.T) {
	tests := []struct {
		name  string
		setup matchesRealSetup
		path  string
	}{
		{
			name: "Read existing file",
			setup: matchesRealSetup{unittestSetup{
				initialFiles: map[string]string{
					"file.txt": "hello world",
				},
			}},
			path: "file.txt",
		},
		{
			name: "Error on non-existent file",
			path: "nonexistent.txt",
		},
		{
			name: "Error on path is a directory",
			setup: matchesRealSetup{unittestSetup{
				initialDirs: []string{
					"mydir",
				},
			}},
			path: "mydir",
		},
	}

	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			realRoot, realFS, testFS := tc.setup.setup(t)
			defer os.RemoveAll(realRoot)

			// Execute
			realContent, realErr := realFS.ReadFile(filepath.Join(realRoot, tc.path))
			testContent, testErr := testFS.ReadFile("/" + tc.path)

			requireErrorsMatch(t, realErr, testErr)
			if realErr == nil {
				require.Equal(t, realContent, testContent)
			}
		})
	}
}

func TestFSTestOSWrapper_Stat(t *testing.T) {
	tests := []struct {
		name   string
		setup  unittestSetup
		path   string
		verify func(t *testing.T, info os.FileInfo)
		expectedError
	}{
		{
			name: "Stat a file",
			path: filepath.Join("/", "file.txt"),
			setup: unittestSetup{
				initialFiles: map[string]string{"/file.txt": "content"},
			},
			verify: func(t *testing.T, info os.FileInfo) {
				require.False(t, info.IsDir())
				require.Equal(t, "file.txt", info.Name())
				require.Equal(t, int64(7), info.Size())
			},
		},
		{
			name: "Stat a directory",
			path: filepath.Join("/", "dir"),
			setup: unittestSetup{
				initialDirs: []string{"/dir"},
			},
			verify: func(t *testing.T, info os.FileInfo) {
				require.True(t, info.IsDir())
				require.Equal(t, "dir", info.Name())
			},
		},
		{
			name: "Stat non-existent path",
			path: filepath.Join("/", "nonexistent"),
			expectedError: expectedError{
				wantErrIs: os.ErrNotExist,
			},
		},
	}

	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			wrapper := tc.setup.setup(t)
			info, err := wrapper.Stat(tc.path)

			if tc.expectedError.Check(t, err) {
				return
			}

			if tc.verify != nil {
				tc.verify(t, info)
			}
		})
	}
}

func TestFSTestOSWrapper_Stat_MatchesReal(t *testing.T) {
	tests := []struct {
		name  string
		setup matchesRealSetup
		path  string
	}{
		{
			name: "Stat a file",
			setup: matchesRealSetup{unittestSetup{
				initialFiles: map[string]string{
					"file.txt": "content",
				},
			}},
			path: "file.txt",
		},
		{
			name: "Stat a directory",
			setup: matchesRealSetup{unittestSetup{
				initialDirs: []string{
					"dir",
				},
			}},
			path: "dir",
		},
		{
			name: "Stat non-existent path",
			path: "nonexistent",
		},
	}

	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			realRoot, realFS, testFS := tc.setup.setup(t)
			defer os.RemoveAll(realRoot)

			// Execute
			realInfo, realErr := realFS.Stat(filepath.Join(realRoot, tc.path))
			testInfo, testErr := testFS.Stat("/" + tc.path)

			requireErrorsMatch(t, realErr, testErr)
			if realErr == nil {
				require.Equal(t, realInfo.Name(), testInfo.Name())
				require.Equal(t, realInfo.IsDir(), testInfo.IsDir())
				// The size of a directory is system-dependent, so only compare sizes for files.
				if !realInfo.IsDir() {
					require.Equal(t, realInfo.Size(), testInfo.Size())
				}
			}
		})
	}
}

// --- FilesystemWriter tests ---

func TestFSTestOSWrapper_Mkdir(t *testing.T) {
	tests := []struct {
		name   string
		setup  unittestSetup
		path   string
		verify func(t *testing.T, fs oswrapper.FSTestOSWrapper)
		expectedError
	}{
		{
			name: "Create new directory",
			path: filepath.Join("/", "newdir"),
			verify: func(t *testing.T, fs oswrapper.FSTestOSWrapper) {
				info, err := fs.Stat(filepath.Join("/", "newdir"))
				require.NoError(t, err)
				require.True(t, info.IsDir())
			},
		},
		{
			name: "Directory already exists",
			path: filepath.Join("/", "mydir"),
			setup: unittestSetup{
				initialDirs: []string{"/mydir"},
			},
			expectedError: expectedError{
				wantErrIs: os.ErrExist,
			},
		},
		{
			name: "Path is a file",
			path: filepath.Join("/", "myfile.txt"),
			setup: unittestSetup{
				initialFiles: map[string]string{"/myfile.txt": ""},
			},
			expectedError: expectedError{
				wantErrIs: os.ErrExist,
			},
		},
		{
			name: "Parent directory does not exist",
			path: filepath.Join("/", "nonexistent", "newdir"),
			expectedError: expectedError{
				wantErrIs: os.ErrNotExist,
			},
		},
		{
			name: "Parent path is a file",
			path: filepath.Join("/", "file.txt", "newdir"),
			setup: unittestSetup{
				initialFiles: map[string]string{"/file.txt": ""},
			},
			expectedError: expectedError{
				wantErrMsg: "not a directory",
			},
		},
	}

	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			wrapper := tc.setup.setup(t)
			err := wrapper.Mkdir(tc.path, 0755)

			if tc.expectedError.Check(t, err) {
				return
			}

			if tc.verify != nil {
				tc.verify(t, wrapper)
			}
		})
	}
}

func TestFSTestOSWrapper_Mkdir_MatchesReal(t *testing.T) {
	tests := []struct {
		name  string
		setup matchesRealSetup
		path  string // path to Mkdir
	}{
		{
			name: "Create new directory",
			path: "newdir",
		},
		{
			name: "Create in existing subdirectory",
			setup: matchesRealSetup{unittestSetup{
				initialDirs: []string{"foo"},
			}},
			path: filepath.Join("foo", "newdir"),
		},
		{
			name: "Error on existing directory",
			setup: matchesRealSetup{unittestSetup{
				initialDirs: []string{"mydir"},
			}},
			path: "mydir",
		},
		{
			name: "Error on existing file",
			setup: matchesRealSetup{unittestSetup{
				initialFiles: map[string]string{"myfile": "content"},
			}},
			path: "myfile",
		},
		{
			name: "Error on non-existent parent",
			path: filepath.Join("nonexistent", "newdir"),
		},
		{
			name: "Error on parent is file",
			setup: matchesRealSetup{unittestSetup{
				initialFiles: map[string]string{"file.txt": "content"},
			}},
			path: filepath.Join("file.txt", "newdir"),
		},
	}

	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			realRoot, realFS, testFS := tc.setup.setup(t)
			defer os.RemoveAll(realRoot)

			// Execute
			realErr := realFS.Mkdir(filepath.Join(realRoot, tc.path), 0755)
			testErr := testFS.Mkdir("/"+tc.path, 0755)

			requireErrorsMatch(t, realErr, testErr)
			if realErr == nil {
				requireFileSystemsMatch(t, realRoot, testFS)
			}
		})
	}
}

func TestFSTestOSWrapper_MkdirAll(t *testing.T) {
	tests := []struct {
		name   string
		setup  unittestSetup
		path   string
		verify func(t *testing.T, fs oswrapper.FSTestOSWrapper)
		expectedError
	}{
		{
			name: "Create new nested directory",
			path: filepath.Join("/", "a", "b", "c"),
			verify: func(t *testing.T, fs oswrapper.FSTestOSWrapper) {
				info, err := fs.Stat(filepath.Join("/", "a", "b", "c"))
				require.NoError(t, err)
				require.True(t, info.IsDir())
			},
		},
		{
			name: "Path already exists as directory",
			path: filepath.Join("/", "a", "b"),
			setup: unittestSetup{
				initialDirs: []string{"/a/b"},
			},
		},
		{
			name: "Part of path is a file",
			path: filepath.Join("/", "a", "b", "c"),
			setup: unittestSetup{
				initialFiles: map[string]string{"/a/b": "i am a file"},
				initialDirs:  []string{"/a"},
			},
			expectedError: expectedError{
				wantErrMsg: "not a directory",
			},
		},
		{
			name: "Destination path is a file",
			path: filepath.Join("/", "a", "b"),
			setup: unittestSetup{
				initialFiles: map[string]string{"/a/b": "i am a file"},
				initialDirs:  []string{"/a"},
			},
			expectedError: expectedError{
				wantErrIs: os.ErrExist,
			},
		},
	}

	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			wrapper := tc.setup.setup(t)
			err := wrapper.MkdirAll(tc.path, 0755)

			if tc.expectedError.Check(t, err) {
				return
			}

			if tc.verify != nil {
				tc.verify(t, wrapper)
			}
		})
	}
}

func TestFSTestOSWrapper_MkdirAll_MatchesReal(t *testing.T) {
	tests := []struct {
		name  string
		setup matchesRealSetup
		path  string // path to MkdirAll
	}{
		{
			name: "Create new nested directory",
			path: filepath.Join("a", "b", "c"),
		},
		{
			name: "Path already exists as directory",
			setup: matchesRealSetup{unittestSetup{
				initialDirs: []string{filepath.Join("a", "b", "c")},
			}},
			path: filepath.Join("a", "b", "c"),
		},
		{
			name: "Part of path is a file",
			setup: matchesRealSetup{unittestSetup{
				initialFiles: map[string]string{filepath.Join("a", "b"): "i am a file"},
				initialDirs:  []string{"a"},
			}},
			path: filepath.Join("a", "b", "c"),
		},
		{
			name: "Destination is a file",
			setup: matchesRealSetup{unittestSetup{
				initialFiles: map[string]string{filepath.Join("a", "b"): "i am a file"},
				initialDirs:  []string{"a"},
			}},
			path: filepath.Join("a", "b"),
		},
	}

	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			realRoot, realFS, testFS := tc.setup.setup(t)
			defer os.RemoveAll(realRoot)

			// Execute
			realErr := realFS.MkdirAll(filepath.Join(realRoot, tc.path), 0755)
			testErr := testFS.MkdirAll("/"+tc.path, 0755)

			requireErrorsMatch(t, realErr, testErr)
			if realErr == nil {
				requireFileSystemsMatch(t, realRoot, testFS)
			}
		})
	}
}

func TestFSTestOSWrapper_WriteFile(t *testing.T) {
	tests := []struct {
		name          string
		setup         unittestSetup
		path          string
		content       []byte
		expectContent *string
		expectedError
	}{
		{
			name:          "Create new file",
			path:          filepath.Join("/", "newfile.txt"),
			content:       []byte("new content"),
			expectContent: stringPtr("new content"),
		},
		{
			name: "Overwrite existing file",
			path: filepath.Join("/", "existing.txt"),
			setup: unittestSetup{
				initialFiles: map[string]string{"/existing.txt": "old content"},
			},
			content:       []byte("overwritten"),
			expectContent: stringPtr("overwritten"),
		},
		{
			name: "Create file in existing subdirectory",
			path: filepath.Join("/", "foo", "bar.txt"),
			setup: unittestSetup{
				initialDirs: []string{"/foo"},
			},
			content:       []byte("sub content"),
			expectContent: stringPtr("sub content"),
		},
		{
			name:    "Error on non-existent directory",
			path:    filepath.Join("/", "new", "dir", "file.txt"),
			content: []byte("some data"),
			expectedError: expectedError{
				wantErrIs: os.ErrNotExist,
			},
		},
		{
			name: "Error on path is a directory",
			path: filepath.Join("/", "mydir"),
			setup: unittestSetup{
				initialDirs: []string{"/mydir"},
			},
			content: []byte("some data"),
			expectedError: expectedError{
				wantErrMsg: "is a directory",
			},
		},
		{
			name: "Error on parent path is a file",
			path: filepath.Join("/", "file.txt", "another.txt"),
			setup: unittestSetup{
				initialFiles: map[string]string{"/file.txt": "i am a file"},
			},
			content: []byte("some data"),
			expectedError: expectedError{
				wantErrMsg: "not a directory",
			},
		},
	}

	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			wrapper := tc.setup.setup(t)
			err := wrapper.WriteFile(tc.path, tc.content, 0666)

			if tc.expectedError.Check(t, err) {
				return
			}

			if tc.expectContent != nil {
				content, err := wrapper.ReadFile(tc.path)
				require.NoError(t, err)
				require.Equal(t, *tc.expectContent, string(content))
			}
		})
	}
}

func TestFSTestOSWrapper_WriteFile_MatchesReal(t *testing.T) {
	tests := []struct {
		name    string
		setup   matchesRealSetup
		path    string
		content []byte
	}{
		{
			name:    "Create new file",
			path:    "newfile.txt",
			content: []byte("new content"),
		},
		{
			name: "Overwrite existing file",
			setup: matchesRealSetup{unittestSetup{
				initialFiles: map[string]string{
					"existing.txt": "old content",
				},
			}},
			path:    "existing.txt",
			content: []byte("overwritten"),
		},
		{
			name: "Create file in existing subdirectory",
			setup: matchesRealSetup{unittestSetup{
				initialDirs: []string{"foo"},
			}},
			path:    filepath.Join("foo", "bar.txt"),
			content: []byte("sub content"),
		},
		{
			name:    "Error on non-existent directory",
			path:    filepath.Join("new", "dir", "file.txt"),
			content: []byte("some data"),
		},
		{
			name: "Error on path is a directory",
			setup: matchesRealSetup{unittestSetup{
				initialDirs: []string{"mydir"},
			}},
			path:    "mydir",
			content: []byte("some data"),
		},
		{
			name: "Error on parent path is a file",
			setup: matchesRealSetup{unittestSetup{
				initialFiles: map[string]string{
					"file.txt": "i am a file",
				},
			}},
			path:    filepath.Join("file.txt", "another.txt"),
			content: []byte("some data"),
		},
	}

	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			realRoot, realFS, testFS := tc.setup.setup(t)
			defer os.RemoveAll(realRoot)

			// Execute
			realErr := realFS.WriteFile(filepath.Join(realRoot, tc.path), tc.content, 0666)
			testErr := testFS.WriteFile("/"+tc.path, tc.content, 0666)

			requireErrorsMatch(t, realErr, testErr)
			if realErr == nil {
				requireFileSystemsMatch(t, realRoot, testFS)
			}
		})
	}
}
