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

// Contains implementations of oswrapper interfaces that are fully in memory.
package oswrapper

import (
	"fmt"
	"io/fs"
	"os"
	"path/filepath"
	"sort"

	"github.com/spf13/afero"
)

// MemMapOSWrapper is an in-memory implementation of OSWrapper for testing.
// It uses an afero.MemMapFs to simulate a filesystem.
type MemMapOSWrapper struct {
	MemMapEnvironProvider
	MemMapFilesystemReaderWriter
}

// MemMapEnvironProvider provides an in-memory implementation of EnvironProvider.
type MemMapEnvironProvider struct {
	Environment map[string]string
}

// MemMapFilesystemReaderWriter provides an in-memory implementation of FilesystemReaderWriter.
type MemMapFilesystemReaderWriter struct {
	MemMapFilesystemReader
	MemMapFilesystemWriter
}

// MemMapFilesystemReader provides an in-memory implementation of FilesystemReader.
type MemMapFilesystemReader struct {
	fs afero.Fs
}

// MemMapFilesystemWriter provides an in-memory implementation of FilesystemWriter.
type MemMapFilesystemWriter struct {
	fs afero.Fs
}

// CreateMemMapOSWrapper creates a new MemMapOSWrapper with an in-memory filesystem.
func CreateMemMapOSWrapper() MemMapOSWrapper {
	filesystem := afero.NewMemMapFs()
	return MemMapOSWrapper{
		MemMapEnvironProvider{
			Environment: make(map[string]string),
		},
		MemMapFilesystemReaderWriter{
			MemMapFilesystemReader{
				fs: filesystem,
			},
			MemMapFilesystemWriter{
				fs: filesystem,
			},
		},
	}
}

// Environ returns the environment variables as a sorted slice of "key=value" strings.
func (m MemMapEnvironProvider) Environ() []string {
	environment := []string{}
	for k, v := range m.Environment {
		environment = append(environment, fmt.Sprintf("%v=%v", k, v))
	}
	sort.Strings(environment)
	return environment
}

// Getenv retrieves the value of the environment variable named by the key.
func (m MemMapEnvironProvider) Getenv(key string) string {
	return m.Environment[key]
}

// Getwd returns a rooted path name corresponding to the current directory.
func (m MemMapEnvironProvider) Getwd() (string, error) {
	if wd, exists := m.Environment["PWD"]; exists {
		return wd, nil
	}
	return "/", nil
}

// UserHomeDir returns the current user's home directory.
func (m MemMapEnvironProvider) UserHomeDir() (string, error) {
	if homeDir, exists := m.Environment["HOME"]; exists {
		return homeDir, nil
	}
	if homeDir, exists := m.Environment["USERPROFILE"]; exists {
		return homeDir, nil
	}
	return "/", nil
}

// Open opens the named file for reading.
func (m MemMapFilesystemReader) Open(name string) (File, error) {
	return m.fs.Open(name)
}

// OpenFile is the generalized open call.
func (m MemMapFilesystemReader) OpenFile(name string, flag int, perm os.FileMode) (File, error) {
	return m.fs.OpenFile(name, flag, perm)
}

// ReadFile reads the file named by filename and returns the contents.
func (m MemMapFilesystemReader) ReadFile(name string) ([]byte, error) {
	return afero.ReadFile(m.fs, name)
}

// ReadDir reads the named directory, returning all its directory entries.
func (m MemMapFilesystemReader) ReadDir(name string) ([]os.DirEntry, error) {
	fileInfos, err := afero.ReadDir(m.fs, name)
	if err != nil {
		return nil, err
	}
	dirEntries := make([]os.DirEntry, len(fileInfos))
	for i, fi := range fileInfos {
		dirEntries[i] = fs.FileInfoToDirEntry(fi)
	}
	return dirEntries, nil
}

// Stat returns a FileInfo describing the named file.
func (m MemMapFilesystemReader) Stat(name string) (os.FileInfo, error) {
	// KI with afero, https://github.com/spf13/afero/issues/522
	if name == "" {
		return nil, fmt.Errorf("no such file or directory")
	}

	return m.fs.Stat(name)
}

// Walk walks the file tree rooted at root, calling fn for each file or directory.
func (m MemMapFilesystemReader) Walk(root string, fn filepath.WalkFunc) error {
	return afero.Walk(m.fs, root, fn)
}

// Create creates the named file.
func (m MemMapFilesystemWriter) Create(name string) (File, error) {
	return m.fs.Create(name)
}

// Mkdir creates a new directory with the specified name and permission bits.
func (m MemMapFilesystemWriter) Mkdir(path string, perm os.FileMode) error {
	return m.fs.Mkdir(path, perm)
}

// MkdirAll creates a directory named path, along with any necessary parents.
func (m MemMapFilesystemWriter) MkdirAll(path string, perm os.FileMode) error {
	return m.fs.MkdirAll(path, perm)
}

// MkdirTemp creates a new temporary directory in the directory dir.
func (m MemMapFilesystemWriter) MkdirTemp(dir, pattern string) (string, error) {
	return afero.TempDir(m.fs, dir, pattern)
}

// Remove removes the named file or (empty) directory.
func (m MemMapFilesystemWriter) Remove(name string) error {
	// This appears to not actually function identically to os' implementation,
	// as it successfully removes non-empty directories.
	return m.fs.Remove(name)
}

// RemoveAll removes path and any children it contains.
func (m MemMapFilesystemWriter) RemoveAll(path string) error {
	return m.fs.RemoveAll(path)
}

// Symlink is not currently supported in the test wrapper.
func (m MemMapFilesystemWriter) Symlink(oldname, newname string) error {
	return fmt.Errorf("%s", "Symlink not currently supported in tests")
}

// WriteFile writes data to a file named by filename.
func (m MemMapFilesystemWriter) WriteFile(name string, data []byte, perm os.FileMode) error {
	return afero.WriteFile(m.fs, name, data, perm)
}
