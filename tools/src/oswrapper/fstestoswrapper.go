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

// Contains implementations of oswrapper interfaces using fstest.
package oswrapper

import (
	"bytes"
	"fmt"
	"io"
	"io/fs"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"testing/fstest"
	"time"
)

// FSTestOSWrapper is an in-memory implementation of OSWrapper for testing.
// It uses a map to simulate a filesystem, which can be used to create
// a read-only fstest.MapFS for read operations. Write operations
// manipulate the map directly.
type FSTestOSWrapper struct {
	FSTestEnvironProvider
	FSTestFilesystemReaderWriter
}

// FSTestEnvironProvider provides an in-memory implementation of EnvironProvider.
type FSTestEnvironProvider struct {
	Environment map[string]string
}

// FSTestFilesystemReaderWriter provides an in-memory implementation of FilesystemReaderWriter.
// It holds the map that represents the filesystem.
type FSTestFilesystemReaderWriter struct {
	FS map[string]*fstest.MapFile
}

// CreateFSTestOSWrapper creates a new FSTestOSWrapper with an empty in-memory filesystem.
func CreateFSTestOSWrapper() FSTestOSWrapper {
	return FSTestOSWrapper{
		FSTestEnvironProvider{
			Environment: make(map[string]string),
		},
		FSTestFilesystemReaderWriter{
			FS: make(map[string]*fstest.MapFile),
		},
	}
}

// mapFileInfo wraps an fstest.MapFile to implement the os.FileInfo interface.
// It holds a pointer to the MapFile and the file's full path to derive its base name.
type mapFileInfo struct {
	path string
	file *fstest.MapFile
}

// Name returns the base name of the file.
func (i *mapFileInfo) Name() string {
	return filepath.Base(i.path)
}

// Size returns the length of the file's data in bytes.
func (i *mapFileInfo) Size() int64 {
	return int64(len(i.file.Data))
}

// Mode returns the file mode bits.
func (i *mapFileInfo) Mode() fs.FileMode {
	return i.file.Mode
}

// ModTime returns the modification time.
func (i *mapFileInfo) ModTime() time.Time {
	return i.file.ModTime
}

// IsDir returns true if the entry is a directory.
func (i *mapFileInfo) IsDir() bool {
	return i.file.Mode.IsDir()
}

// Sys returns the underlying data source (can be nil).
func (i *mapFileInfo) Sys() any {
	return i.file.Sys
}

// fstestFileHandle implements the oswrapper.File interface for the in-memory fstest map.
// It uses a bytes.Reader for read/seek operations and a bytes.Buffer for write operations.
// On Close(), the buffer's contents are written back to the underlying map.
type fstestFileHandle struct {
	path   string
	info   os.FileInfo
	fsMap  *map[string]*fstest.MapFile
	reader *bytes.Reader
	buffer *bytes.Buffer
}

// cleanPath prepares a path for use as a key in the fstest map.
// It ensures forward slashes and removes any leading slash.
func cleanPath(path string) string {
	p := filepath.ToSlash(filepath.Clean(path))
	return strings.TrimPrefix(p, "/")
}

// fs returns a read-only fs.FS view of the underlying map.
func (m FSTestFilesystemReaderWriter) fs() fs.FS {
	return fstest.MapFS(m.FS)
}

// Environ returns the environment variables as a sorted slice of "key=value" strings.
func (m FSTestEnvironProvider) Environ() []string {
	environment := make([]string, 0, len(m.Environment))
	for k, v := range m.Environment {
		environment = append(environment, fmt.Sprintf("%v=%v", k, v))
	}
	sort.Strings(environment)
	return environment
}

// Getenv retrieves the value of the environment variable named by the key.
func (m FSTestEnvironProvider) Getenv(key string) string {
	return m.Environment[key]
}

// Getwd returns a rooted path name corresponding to the current directory.
func (m FSTestEnvironProvider) Getwd() (string, error) {
	if wd, exists := m.Environment["PWD"]; exists {
		return wd, nil
	}
	return "/", nil
}

// UserHomeDir returns the current user's home directory.
func (m FSTestEnvironProvider) UserHomeDir() (string, error) {
	if homeDir, exists := m.Environment["HOME"]; exists {
		return homeDir, nil
	}
	if homeDir, exists := m.Environment["USERPROFILE"]; exists {
		return homeDir, nil
	}
	return "/", nil
}

// Stat returns a FileInfo describing the named file.
func (m FSTestFilesystemReaderWriter) Stat(name string) (os.FileInfo, error) {
	return fs.Stat(m.fs(), cleanPath(name))
}

// ReadFile reads the file named by filename and returns the contents.
func (m FSTestFilesystemReaderWriter) ReadFile(name string) ([]byte, error) {
	return fs.ReadFile(m.fs(), cleanPath(name))
}

// ReadDir reads the named directory, returning all its directory entries.
func (m FSTestFilesystemReaderWriter) ReadDir(name string) ([]os.DirEntry, error) {
	return fs.ReadDir(m.fs(), cleanPath(name))
}

// Walk walks the file tree rooted at root, calling fn for each file or directory.
func (m FSTestFilesystemReaderWriter) Walk(root string, fn filepath.WalkFunc) error {
	walkDirFn := func(path string, d fs.DirEntry, err error) error {
		if err != nil {
			return fn(path, nil, err)
		}
		info, err := d.Info()
		if err != nil {
			return fn(path, nil, err)
		}
		return fn(path, info, nil)
	}
	return fs.WalkDir(m.fs(), cleanPath(root), walkDirFn)
}

// WriteFile writes data to a file named by filename.
func (m FSTestFilesystemReaderWriter) WriteFile(name string, data []byte, perm os.FileMode) error {
	path := cleanPath(name)
	dir := filepath.Dir(path)
	if dir != "." && dir != "" {
		if err := m.MkdirAll(dir, 0755); err != nil {
			return err
		}
	}
	m.FS[path] = &fstest.MapFile{Data: data, Mode: perm, ModTime: time.Now()}
	return nil
}

// MkdirAll creates a directory named path, along with any necessary parents.
func (m FSTestFilesystemReaderWriter) MkdirAll(path string, perm os.FileMode) error {
	p := cleanPath(path)
	parts := strings.Split(p, "/")
	current := ""
	for _, part := range parts {
		if current == "" {
			current = part
		} else {
			current = current + "/" + part
		}
		if file, exists := m.FS[current]; !exists {
			m.FS[current] = &fstest.MapFile{Mode: os.ModeDir | perm, ModTime: time.Now()}
		} else if !file.Mode.IsDir() {
			return &os.PathError{Op: "mkdirall", Path: path, Err: fmt.Errorf("path component is a file")}
		}
	}
	return nil
}

// Mkdir creates a new directory with the specified name and permission bits.
func (m FSTestFilesystemReaderWriter) Mkdir(path string, perm os.FileMode) error {
	p := cleanPath(path)
	if _, exists := m.FS[p]; exists {
		return &os.PathError{Op: "mkdir", Path: path, Err: os.ErrExist}
	}
	parent := filepath.Dir(p)
	if parent != "." {
		if _, parentExists := m.FS[parent]; !parentExists {
			return &os.PathError{Op: "mkdir", Path: path, Err: os.ErrNotExist}
		}
	}
	m.FS[p] = &fstest.MapFile{Mode: os.ModeDir | perm, ModTime: time.Now()}
	return nil
}

// RemoveAll removes path and any children it contains.
func (m FSTestFilesystemReaderWriter) RemoveAll(path string) error {
	p := cleanPath(path)
	prefix := p + "/"
	for key := range m.FS {
		if key == p || strings.HasPrefix(key, prefix) {
			delete(m.FS, key)
		}
	}
	return nil
}

// Remove removes the named file or (empty) directory.
func (m FSTestFilesystemReaderWriter) Remove(name string) error {
	p := cleanPath(name)
	info, err := m.Stat(p)
	if err != nil {
		return &os.PathError{Op: "remove", Path: name, Err: err}
	}
	if info.IsDir() {
		entries, err := m.ReadDir(p)
		if err != nil {
			return &os.PathError{Op: "remove", Path: name, Err: err}
		}
		if len(entries) > 0 {
			return &os.PathError{Op: "remove", Path: name, Err: fmt.Errorf("directory not empty")}
		}
	}
	delete(m.FS, p)
	return nil
}

// MkdirTemp creates a new temporary directory in the directory dir.
func (m FSTestFilesystemReaderWriter) MkdirTemp(dir, pattern string) (string, error) {
	// fstest does not have a temp file concept, so we simulate it.
	for i := 0; i < 10000; i++ {
		name := pattern + time.Now().Format("20060102150405.000000")
		path := filepath.Join(dir, name)
		err := m.Mkdir(path, 0700|os.ModeDir)
		if os.IsExist(err) {
			continue
		}
		return path, err
	}
	return "", &os.PathError{Op: "mkdirtemp", Path: dir + string(filepath.Separator) + pattern, Err: os.ErrExist}
}

// Symlink is not supported by fstest.MapFS.
func (m FSTestFilesystemReaderWriter) Symlink(oldname, newname string) error {
	return fmt.Errorf("symlink not currently supported in fstest wrapper")
}

// Open opens the named file for reading.
func (m FSTestFilesystemReaderWriter) Open(name string) (File, error) {
	return m.OpenFile(name, os.O_RDONLY, 0)
}

// Create creates or truncates the named file.
func (m FSTestFilesystemReaderWriter) Create(name string) (File, error) {
	return m.OpenFile(name, os.O_RDWR|os.O_CREATE|os.O_TRUNC, 0666)
}

// OpenFile is the generalized open call.
func (m FSTestFilesystemReaderWriter) OpenFile(name string, flag int, perm os.FileMode) (File, error) {
	path := cleanPath(name)
	mapFile, exists := m.FS[path]

	if flag&os.O_CREATE != 0 && !exists {
		if err := m.WriteFile(path, []byte{}, perm); err != nil {
			return nil, err
		}
		mapFile = m.FS[path]
	} else if !exists {
		return nil, &os.PathError{Op: "open", Path: name, Err: os.ErrNotExist}
	}

	if mapFile.Mode.IsDir() {
		return nil, &os.PathError{Op: "open", Path: name, Err: fmt.Errorf("is a directory")}
	}

	// Create an os.FileInfo compatible wrapper for the fstest.MapFile.
	info := &mapFileInfo{path: name, file: mapFile}
	handle := &fstestFileHandle{path: path, info: info, fsMap: &m.FS}

	// Determine initial content for reader and writer buffer
	initialData := mapFile.Data
	if flag&os.O_TRUNC != 0 && flag&(os.O_RDWR|os.O_WRONLY) != 0 {
		initialData = []byte{}
	}

	// Setup reader if not write-only
	if flag&os.O_WRONLY == 0 {
		handle.reader = bytes.NewReader(initialData)
	}

	// Setup writer if not read-only
	if flag&(os.O_RDWR|os.O_WRONLY) != 0 {
		handle.buffer = bytes.NewBuffer(initialData)
		if flag&os.O_APPEND != 0 {
			// For a bytes.Buffer, writing is always appending, but we can
			// simulate the seek-to-end behavior for other operations.
			if handle.reader != nil {
				handle.reader.Seek(0, io.SeekEnd)
			}
		}
	}

	return handle, nil
}

// Stat returns the FileInfo structure describing the file.
func (h *fstestFileHandle) Stat() (os.FileInfo, error) {
	return h.info, nil
}

// Close writes the buffer's content back to the underlying map if the file was opened for writing.
func (h *fstestFileHandle) Close() error {
	if h.buffer != nil {
		mapFile := (*h.fsMap)[h.path]
		mapFile.Data = h.buffer.Bytes()
		mapFile.ModTime = time.Now()
	}
	return nil
}

// Read reads up to len(p) bytes into p.
func (h *fstestFileHandle) Read(p []byte) (int, error) {
	if h.reader == nil {
		return 0, &os.PathError{Op: "read", Path: h.path, Err: os.ErrInvalid}
	}
	return h.reader.Read(p)
}

// Write writes len(p) bytes from p to the underlying buffer.
// Note: Unlike os.File, this implementation always appends to the buffer,
// it does not write at the current seek position.
func (h *fstestFileHandle) Write(p []byte) (int, error) {
	if h.buffer == nil {
		return 0, &os.PathError{Op: "write", Path: h.path, Err: os.ErrInvalid}
	}
	return h.buffer.Write(p)
}

// Seek sets the offset for the next Read.
// Note: This only affects the read offset, not the write offset.
func (h *fstestFileHandle) Seek(offset int64, whence int) (int64, error) {
	if h.reader == nil {
		return 0, &os.PathError{Op: "seek", Path: h.path, Err: os.ErrInvalid}
	}
	return h.reader.Seek(offset, whence)
}
