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

// Contains implementations of oswrapper interfaces that actually interact with the OS.
package oswrapper

import (
	"os"
	"path/filepath"
)

// RealOSWrapper provides a concrete implementation of OSWrapper that interacts
// with the real operating system.
type RealOSWrapper struct {
	RealEnvironProvider
	RealFilesystemReaderWriter
}

// RealEnvironProvider provides a concrete implementation of EnvironProvider.
type RealEnvironProvider struct{}

// RealFilesystemReaderWriter provides a concrete implementation of FilesystemReaderWriter.
type RealFilesystemReaderWriter struct {
	RealFilesystemReader
	RealFilesystemWriter
}

// RealFilesystemReader provides a concrete implementation of FilesystemReader
// that interacts with the real operating system.
type RealFilesystemReader struct{}

// RealFilesystemWriter provides a concrete implementation of FilesystemWriter
// that interacts with the real operating system.
type RealFilesystemWriter struct{}

// GetRealOSWrapper creates a new RealOSWrapper that interacts with the real OS.
func GetRealOSWrapper() RealOSWrapper {
	return RealOSWrapper{
		RealEnvironProvider{},
		RealFilesystemReaderWriter{
			RealFilesystemReader{},
			RealFilesystemWriter{},
		},
	}
}

// Environ returns a copy of strings representing the environment, in the form "key=value".
func (RealEnvironProvider) Environ() []string {
	return os.Environ()
}

// Getenv retrieves the value of the environment variable named by the key.
func (RealEnvironProvider) Getenv(key string) string {
	return os.Getenv(key)
}

// Getwd returns a rooted path name corresponding to the current directory.
func (RealEnvironProvider) Getwd() (string, error) {
	return os.Getwd()
}

// UserHomeDir returns the current user's home directory.
func (RealEnvironProvider) UserHomeDir() (string, error) {
	return os.UserHomeDir()
}

// Open opens the named file for reading.
func (RealFilesystemReader) Open(name string) (File, error) {
	return os.Open(name)
}

// OpenFile is the generalized open call.
func (RealFilesystemReader) OpenFile(name string, flag int, perm os.FileMode) (File, error) {
	return os.OpenFile(name, flag, perm)
}

// ReadFile reads the file named by filename and returns the contents.
func (RealFilesystemReader) ReadFile(name string) ([]byte, error) {
	return os.ReadFile(name)
}

// ReadDir reads the named directory, returning all its directory entries.
func (RealFilesystemReader) ReadDir(name string) ([]os.DirEntry, error) {
	return os.ReadDir(name)
}

// Stat returns a FileInfo describing the named file.
func (RealFilesystemReader) Stat(name string) (os.FileInfo, error) {
	return os.Stat(name)
}

// Walk walks the file tree rooted at root, calling fn for each file or directory.
func (RealFilesystemReader) Walk(root string, fn filepath.WalkFunc) error {
	return filepath.Walk(root, fn)
}

// Create creates the named file.
func (RealFilesystemWriter) Create(name string) (File, error) {
	return os.Create(name)
}

// Mkdir creates a new directory with the specified name and permission bits.
func (RealFilesystemWriter) Mkdir(path string, perm os.FileMode) error {
	return os.Mkdir(path, perm)
}

// MkdirAll creates a directory named path, along with any necessary parents.
func (RealFilesystemWriter) MkdirAll(path string, perm os.FileMode) error {
	return os.MkdirAll(path, perm)
}

// MkdirTemp creates a new temporary directory in the directory dir.
func (RealFilesystemWriter) MkdirTemp(dir, pattern string) (string, error) {
	return os.MkdirTemp(dir, pattern)
}

// Remove removes the named file or (empty) directory.
func (RealFilesystemWriter) Remove(name string) error {
	return os.Remove(name)
}

// RemoveAll removes path and any children it contains.
func (RealFilesystemWriter) RemoveAll(path string) error {
	return os.RemoveAll(path)
}

// Symlink creates newname as a symbolic link to oldname.
func (RealFilesystemWriter) Symlink(oldname, newname string) error {
	return os.Symlink(oldname, newname)
}

// WriteFile writes data to a file named by filename.
func (RealFilesystemWriter) WriteFile(name string, data []byte, perm os.FileMode) error {
	return os.WriteFile(name, data, perm)
}
