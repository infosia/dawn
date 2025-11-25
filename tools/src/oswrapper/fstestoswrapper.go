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

// Contains implementation of oswrapper interfaces using fstest.
package oswrapper

import (
	"errors"
	"fmt"
	"io"
	"io/fs"
	"os"
	"path"
	"path/filepath"
	"strings"
	"sync/atomic"
	"syscall"
	"testing/fstest"
	"time"
)

var (
	ErrPwdNotSet  = errors.New("PWD not set in test environment")
	ErrHomeNotSet = errors.New("HOME not set in test environment")
)

// --- OSWrapper implementation ---

// FSTestOSWrapper is an in-memory implementation of OSWrapper for testing.
// It uses a map to simulate a filesystem, which can be used to create
// a read-only fstest.MapFS for read operations. Write operations
// manipulate the map directly.
// Caveats/known differences from real os.* functions:
//   - Paths in error messages will never have a leading / even if it was in
//     the provided path. For example, ReadFile("/root") when /root does not
//     exist will have the error message "open root: file does not exist". This
//     is a side-effect of fs.FS requiring that Open() reject names that do not
//     satisfy fs.ValidPath(), which requires that there is no leading /.
//     TODO(crbug.com/436025865): Update the implementation to correctly add
//     leading / back in.
//   - The current working directory is not taken into account. For example,
//     creating /root/subdir/file.txt, changing to /root/subdir, and attempting
//     to open file.txt will fail. This cannot currently be easily worked
//     around at the moment since the CWD-aware logic lives in EnvironProvider
//     while filesystem operations live in FilesystemReaderWriter.
//     TODO(crbug.com/436025865): Merge everything into FSTestOSWrapper so that
//     the filesystem can be CWD-aware.
type FSTestOSWrapper struct {
	FSTestEnvironProvider
	FSTestFilesystemReaderWriter
}

// CreateFSTestOSWrapper creates a new FSTestOSWrapper with an empty in-memory filesystem.
func CreateFSTestOSWrapper() FSTestOSWrapper {
	return FSTestOSWrapper{
		FSTestEnvironProvider{env: make(map[string]string)},
		FSTestFilesystemReaderWriter{
			FS: make(map[string]*fstest.MapFile),
		},
	}
}

// CreateFSTestOSWrapperWithRealEnv creates a new FSTestOSWrapper with an empty in-memory filesystem
// and an environment initialized from the current process's environment.
func CreateFSTestOSWrapperWithRealEnv() FSTestOSWrapper {
	envMap := make(map[string]string)
	for _, e := range os.Environ() {
		pair := strings.SplitN(e, "=", 2)
		if len(pair) == 2 {
			envMap[pair[0]] = pair[1]
		}
	}

	// os.Getwd() and os.UserHomeDir() are not always from environment variables on all systems, but
	// to simplify the test wrapper PWD and HOME will be populated from them.
	if _, ok := envMap["PWD"]; !ok {
		if wd, err := os.Getwd(); err == nil {
			envMap["PWD"] = wd
		}
	}
	if _, ok := envMap["HOME"]; !ok {
		if home, err := os.UserHomeDir(); err == nil {
			envMap["HOME"] = home
		}
	}

	return FSTestOSWrapper{
		FSTestEnvironProvider: FSTestEnvironProvider{env: envMap},
		FSTestFilesystemReaderWriter: FSTestFilesystemReaderWriter{
			FS: make(map[string]*fstest.MapFile),
		},
	}
}

// --- EnvironProvider implementation ---

// FSTestEnvironProvider is a stub implementation of EnvironProvider that uses a map.
type FSTestEnvironProvider struct {
	env map[string]string
}

// EnvMap returns the internal environment map.
// This is intended for testing purposes.
func (p FSTestEnvironProvider) EnvMap() map[string]string {
	return p.env
}

// Setenv sets an environment variable in the test provider.
func (p FSTestEnvironProvider) Setenv(key, value string) {
	p.env[key] = value
}

func (p FSTestEnvironProvider) Environ() []string {
	result := make([]string, 0, len(p.env))
	for k, v := range p.env {
		result = append(result, k+"="+v)
	}
	return result
}

func (p FSTestEnvironProvider) Getenv(key string) string {
	// os.Getenv returns "" for a missing key, which is the same as a map lookup.
	return p.env[key]
}

func (p FSTestEnvironProvider) Getwd() (string, error) {
	if wd, ok := p.env["PWD"]; ok {
		return wd, nil
	}
	// The specific error returned by os.Getwd is very system dependent, so just returning a package
	// error
	return "", ErrPwdNotSet
}

func (p FSTestEnvironProvider) UserHomeDir() (string, error) {
	if home, ok := p.env["HOME"]; ok {
		return home, nil
	}
	// The specific error returned by os.UserHomeDir() is very system dependent, so just returning
	// a package error
	return "", ErrHomeNotSet
}

// --- FilesystemReaderWriter implementation ---

// FSTestFilesystemReaderWriter provides an in-memory implementation of FilesystemReaderWriter.
// It holds the map that represents the filesystem.
type FSTestFilesystemReaderWriter struct {
	FS map[string]*fstest.MapFile
}

// CleanPath converts an OS dependent path into a fs.FS compatible path that can be used as a key
// within FSTestOSWrapper
func (w FSTestFilesystemReaderWriter) CleanPath(pathStr string) string {
	// Replace all backslashes with forward slashes to handle Windows paths on any OS.
	p := strings.ReplaceAll(pathStr, "\\", "/")
	// Use the platform-agnostic path package to clean the path.
	p = path.Clean(p)

	// Normalize the path to be compatible with fs.FS
	p = strings.TrimPrefix(p, "/")
	if p == "" {
		p = "." // The canonical representation of the root in a fs.FS.
	}
	return p
}

func (w FSTestFilesystemReaderWriter) resolvePath(pathStr string) (string, error) {
	const maxSymlinkLinks = 255
	linksWalked := 0

	// Use CleanPath to get the initial list of parts relative to the FS root
	cleanedPath := w.CleanPath(pathStr)
	if cleanedPath == "." {
		return ".", nil
	}
	currentParts := strings.Split(cleanedPath, "/")

	var resolvedParts []string

	for len(currentParts) > 0 {
		part := currentParts[0]
		currentParts = currentParts[1:]

		if part == "." {
			continue
		}

		// Construct path to check
		checkPath := ""
		if len(resolvedParts) > 0 {
			checkPath = strings.Join(resolvedParts, "/") + "/" + part
		} else {
			checkPath = part
		}

		entry, ok := w.FS[checkPath]
		if ok && (entry.Mode&os.ModeSymlink != 0) {
			linksWalked++
			if linksWalked > maxSymlinkLinks {
				return "", &os.PathError{Op: "resolve", Path: pathStr, Err: syscall.ELOOP}
			}

			target := string(entry.Data)

			var newPath string
			if filepath.IsAbs(target) {
				newPath = w.CleanPath(target)
			} else {
				dir := strings.Join(resolvedParts, "/")
				if dir == "" {
					newPath = w.CleanPath(target)
				} else {
					newPath = w.CleanPath(dir + "/" + target)
				}
			}

			// Restart resolution with the new path prepended to remaining parts
			resolvedParts = nil
			newParts := strings.Split(newPath, "/")
			currentParts = append(newParts, currentParts...)
		} else {
			resolvedParts = append(resolvedParts, part)
		}
	}

	if len(resolvedParts) == 0 {
		return ".", nil
	}
	return strings.Join(resolvedParts, "/"), nil
}

// fs returns a read-only fs.FS view of the underlying map.
func (w FSTestFilesystemReaderWriter) fs() fs.FS {
	return fstest.MapFS(w.FS)
}

// mapFileInfo wraps a fstest.MapFile to implement the os.FileInfo interface.
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
type fstestFileHandle struct {
	path         string
	originalPath string
	info         os.FileInfo
	fsMap        *map[string]*fstest.MapFile
	content      []byte
	offset       int64
	flag         int
	closed       bool
}

// --- FilesystemReader implementation ---

func (w FSTestFilesystemReaderWriter) Open(name string) (File, error) {
	return w.OpenFile(name, os.O_RDONLY, 0)
}

func (w FSTestFilesystemReaderWriter) OpenFile(name string, flag int, perm os.FileMode) (File, error) {
	// If O_CREATE and O_EXCL are set, we should not follow the final symlink if it exists.
	// But O_CREATE | O_EXCL implies that we expect the file to NOT exist.
	// If the path refers to a symlink, and O_EXCL is set, os.OpenFile should fail with EEXIST.
	// If O_NOFOLLOW was supported (it's not in the os package signature), we would check that.
	// Go's os.OpenFile documentation says:
	// "If the file already exists, OpenFile returns an error satisfying IsExist." for O_CREATE|O_EXCL.
	// So checking for existence of the symlink itself is correct behavior for O_EXCL.

	path := w.CleanPath(name)

	// Special case: if creating a file that ends with a symlink that points to a non-existent location,
	// OpenFile with O_CREATE should create the target file.
	// So we need to resolve the path, but potentially stop before the last component if we are creating.

	// However, resolvePath resolves fully.
	// Let's try resolving fully first.
	_, err := w.resolvePath(name)
	if err != nil {
		// If resolution failed because the target doesn't exist, that might be okay if we are creating.
		// But resolvePath returns ELOOP for loops, and other errors.
		// If it's a "path not found" error during resolution of intermediate components, that's a failure.
		// If it's just the final component missing, we might handle it.
		// For now, let's just propagate error and see if we need more complex logic.
		// Actually, resolvePath returns the path so far if it can't resolve? No, it returns error.
		// We might need to resolve the directory part if we are creating.
	}

	// Refined strategy:
	// 1. If O_CREATE is set, we need to handle the case where the file doesn't exist yet.
	//    We should resolve the directory part of the path.
	// 2. If O_CREATE is NOT set, the file must exist (or we fail). resolvePath handles this.

	var pathToCheck string
	if flag&os.O_CREATE != 0 {
		// We are creating. We need to check if the path exists.
		// If it exists and is a symlink, we follow it (unless O_EXCL, but let's assume standard behavior first).
		// If it doesn't exist, we resolve the parent directory.

		// Check if the exact path exists as a symlink first (for O_EXCL check)
		if _, exists := w.FS[path]; exists && (flag&os.O_EXCL != 0) {
			return nil, &os.PathError{Op: "open", Path: name, Err: os.ErrExist}
		}

		// Try to resolve the full path.
		r, err := w.resolvePath(name)
		if err == nil {
			// It resolved to something existing.
			pathToCheck = r
		} else {
			// It didn't resolve. It might be because the final component is missing.
			// Let's try resolving the parent.
			parent := filepath.Dir(name)
			resolvedParent, errParent := w.resolvePath(parent)
			if errParent != nil {
				// Parent resolution failed, so we can't create the file.
				return nil, errParent
			}
			base := filepath.Base(name)
			if resolvedParent == "." {
				pathToCheck = base
			} else {
				pathToCheck = resolvedParent + "/" + base
			}
			// Clean it again just in case
			pathToCheck = w.CleanPath(pathToCheck)
		}
	} else {
		// Not creating, so just resolve fully.
		var err error
		pathToCheck, err = w.resolvePath(name)
		if err != nil {
			return nil, err
		}
	}

	// Now proceed with pathToCheck as the actual file path in the map
	mapFile, exists := w.FS[pathToCheck]

	// Check if a parent component of the pathToCheck is a file.
	// (This check seems redundant if resolvePath worked correctly and checked components,
	// but resolvePath might not check if intermediate components are files if they are not symlinks?
	// The original implementation checked this. Let's keep a check but using the resolved path.)
	dir := filepath.Dir(pathToCheck)
	if dir != "." && dir != "" {
		parentInfo, parentExists := w.FS[dir]
		if parentExists && !parentInfo.Mode.IsDir() {
			return nil, &os.PathError{Op: "open", Path: name, Err: fmt.Errorf("not a directory")}
		}
	}

	if flag&os.O_CREATE != 0 {
		if exists {
			if flag&os.O_EXCL != 0 {
				return nil, &os.PathError{Op: "open", Path: name, Err: os.ErrExist}
			}
		} else {
			parent := filepath.Dir(pathToCheck)
			if parent != "." && parent != "" {
				parentInfo, parentExists := w.FS[parent]
				if !parentExists {
					return nil, &os.PathError{Op: "open", Path: name, Err: os.ErrNotExist}
				}
				if !parentInfo.Mode.IsDir() {
					return nil, &os.PathError{Op: "open", Path: name, Err: fmt.Errorf("not a directory")}
				}
			}
			newFile := &fstest.MapFile{Data: []byte{}, Mode: perm, ModTime: time.Now()}
			w.FS[pathToCheck] = newFile
			mapFile = newFile
			exists = true
		}
	}

	if !exists {
		return nil, &os.PathError{Op: "open", Path: name, Err: os.ErrNotExist}
	}

	if mapFile.Mode.IsDir() {
		if flag&(os.O_WRONLY|os.O_RDWR) != 0 {
			return nil, &os.PathError{Op: "open", Path: name, Err: fmt.Errorf("is a directory")}
		}
		if flag&os.O_TRUNC != 0 {
			return nil, &os.PathError{Op: "open", Path: name, Err: fmt.Errorf("is a directory")}
		}
	}

	// Create an os.FileInfo compatible wrapper for the fstest.MapFile.
	// NOTE: We use the original name in the FileInfo to match os.Open behavior?
	// Actually os.Stat returns the info of the target.
	// When opening a file, the file handle should probably point to the resolved path?
	// The implementation uses `path` which is the key in the map.
	info := &mapFileInfo{path: pathToCheck, file: mapFile}
	handle := &fstestFileHandle{
		path:         pathToCheck,
		originalPath: name,
		info:         info,
		fsMap:        &w.FS,
		flag:         flag,
		content:      nil, // Set below
		offset:       0,   // Set below
	}

	initialData := mapFile.Data
	if flag&os.O_TRUNC != 0 {
		initialData = []byte{}
	}

	handle.content = make([]byte, len(initialData))
	copy(handle.content, initialData)

	if flag&os.O_APPEND != 0 {
		handle.offset = int64(len(handle.content))
	}

	return handle, nil
}

func (w FSTestFilesystemReaderWriter) ReadFile(name string) ([]byte, error) {
	// ReadFile follows symlinks.
	p, err := w.resolvePath(name)
	if err != nil {
		return nil, err
	}

	// If the path is the root, it's always a directory.
	if p == "." {
		return nil, &os.PathError{Op: "read", Path: name, Err: fmt.Errorf("is a directory")}
	}

	// Check for an explicit entry
	if mapFile, exists := w.FS[p]; exists {
		if mapFile.Mode.IsDir() {
			return nil, &os.PathError{Op: "read", Path: name, Err: fmt.Errorf("is a directory")}
		}
		data := make([]byte, len(mapFile.Data))
		copy(data, mapFile.Data)
		return data, nil
	}

	// No check for an implicit entry, since MkdirAll should create all the intermediate
	// directories.

	return nil, &os.PathError{Op: "open", Path: name, Err: os.ErrNotExist}
}

func (w FSTestFilesystemReaderWriter) ReadDir(dir string) ([]os.DirEntry, error) {
	p, err := w.resolvePath(dir)
	if err != nil {
		return nil, err
	}

	if mapFile, exists := w.FS[p]; exists && !mapFile.Mode.IsDir() {
		return nil, &os.PathError{Op: "readdir", Path: dir, Err: fmt.Errorf("not a directory")}
	}
	return fs.ReadDir(w.fs(), p)
}

func (w FSTestFilesystemReaderWriter) Stat(name string) (os.FileInfo, error) {
	resolved, err := w.resolvePath(name)
	if err != nil {
		return nil, err
	}
	return fs.Stat(w.fs(), resolved)
}

func (w FSTestFilesystemReaderWriter) Walk(root string, fn filepath.WalkFunc) error {
	fsRoot := w.CleanPath(root)

	walkDirFn := func(path string, d fs.DirEntry, err error) error {
		// The path from fs.WalkDir is relative to the FS root.
		// We need to reconstruct the path that the user's filepath.WalkFunc expects.
		// The contract for filepath.Walk is that the paths passed to the callback
		// have the original `root` argument as a prefix.
		var fullPath string
		if path == fsRoot {
			fullPath = root
		} else {
			// fs.WalkDir gives a path from the FS root. We need the part relative
			// to the walk's root, and then join it with the original root string.
			rel, errRel := filepath.Rel(fsRoot, path)
			if errRel != nil {
				return fn(path, nil, fmt.Errorf("internal error creating relative path: %w", errRel))
			}
			fullPath = filepath.Join(root, rel)
		}

		if err != nil {
			return fn(fullPath, nil, err)
		}
		info, errInfo := d.Info()
		if errInfo != nil {
			return fn(fullPath, nil, errInfo)
		}
		return fn(fullPath, info, nil)
	}
	return fs.WalkDir(w.fs(), fsRoot, walkDirFn)
}

// --- FilesystemWriter implementation ---

func (w FSTestFilesystemReaderWriter) Create(name string) (File, error) {
	return w.OpenFile(name, os.O_RDWR|os.O_CREATE|os.O_TRUNC, 0666)
}

func (w FSTestFilesystemReaderWriter) Mkdir(dir string, perm os.FileMode) error {
	p := w.CleanPath(dir)

	if p == "." {
		// The root directory always exists in a fs.FS.
		// os.Mkdir returns EEXIST in this case.
		return &os.PathError{Op: "mkdir", Path: dir, Err: os.ErrExist}
	}

	if _, exists := w.FS[p]; exists {
		return &os.PathError{Op: "mkdir", Path: dir, Err: os.ErrExist}
	}

	// We need to resolve the parent path to handle symlinks in parents.
	// But we should NOT resolve the final component if it's the one we are creating.
	// Actually, Mkdir requires the parent to be a directory.

	parent := filepath.Dir(dir)
	var resolvedParent string
	var err error

	if parent == "." || parent == "" {
		resolvedParent = "."
	} else {
		resolvedParent, err = w.resolvePath(parent)
		if err != nil {
			// If parent fails to resolve, we assume it doesn't exist or is invalid.
			// Ideally resolvePath returns a specific error we can map to NotExist or NotDirectory.
			return &os.PathError{Op: "mkdir", Path: dir, Err: os.ErrNotExist}
		}
	}

	// Check that the resolved parent is indeed a directory
	parentInfo, parentExists := w.FS[resolvedParent]
	if !parentExists {
		// This handles the case where parent is "." but somehow missing from map?
		// Or resolving returned a path that doesn't exist (which shouldn't happen if resolvePath succeeded, unless race).
		// In fs.FS, "." is implicit usually, but we might rely on it being explicit or handled.
		if resolvedParent == "." {
			// root always exists
		} else {
			return &os.PathError{Op: "mkdir", Path: dir, Err: os.ErrNotExist}
		}
	} else if !parentInfo.Mode.IsDir() {
		return &os.PathError{Op: "mkdir", Path: dir, Err: fmt.Errorf("not a directory")}
	}

	// Construct the final key. Since we resolved the parent, we should use that base for the key?
	// No, the requirement is that if I create "a/b", and "a" is a symlink to "c", then "c/b" is created.
	// So we should use the resolved parent + base name.

	base := filepath.Base(dir)
	var finalPath string
	if resolvedParent == "." {
		finalPath = base
	} else {
		finalPath = resolvedParent + "/" + base
	}
	finalPath = w.CleanPath(finalPath)

	// Check existence at the resolved path
	if _, exists := w.FS[finalPath]; exists {
		return &os.PathError{Op: "mkdir", Path: dir, Err: os.ErrExist}
	}

	w.FS[finalPath] = &fstest.MapFile{Mode: os.ModeDir | perm, ModTime: time.Now()}
	return nil
}

func (w FSTestFilesystemReaderWriter) MkdirAll(dir string, perm os.FileMode) error {
	// For MkdirAll, we iterate through components.
	// If a component is missing, we create it.
	// If a component is a symlink, we follow it for subsequent components.

	// Implementation strategy:
	// Use standard library approach: try Mkdir. If it fails with EEXIST, check if it's a directory.
	// If it fails with ENOENT, MkdirAll parent, then Mkdir.

	err := w.Mkdir(dir, perm)
	if err == nil {
		return nil
	}

	if os.IsExist(err) {
		// It exists. Check if it's a directory.
		// Note: Mkdir (our implementation) follows symlinks for the parent but NOT the final component.
		// However, if the final component IS a symlink, Mkdir returns EEXIST.
		// In that case, we need to check if the symlink points to a directory.

		info, statErr := w.Stat(dir) // Stat follows symlinks
		if statErr == nil && info.IsDir() {
			return nil
		}
		return err
	}

	if !os.IsNotExist(err) {
		// Some other error (e.g. not a directory).
		return err
	}

	// NotExist. Recurse on parent.
	p := w.CleanPath(dir)
	parent := filepath.Dir(p)
	if parent == "." || parent == p {
		return err
	}

	if err := w.MkdirAll(parent, perm); err != nil {
		return err
	}

	// Parent created. Retry Mkdir.
	return w.Mkdir(p, perm)
}

// tempCounter is an incrementing value used for generating the 'random' part of  temporary file
// names.
var tempCounter atomic.Uint32

func (w FSTestFilesystemReaderWriter) MkdirTemp(dir, pattern string) (string, error) {
	prefix, suffix := pattern, ""
	if i := strings.LastIndex(pattern, "*"); i != -1 {
		prefix, suffix = pattern[:i], pattern[i+1:]
	}

	// Note: This is using deterministic values for the 'random' part of the path names. This code
	// should only be used for testing purposes, since an attacker might be able to exploit this.
	rndStr := fmt.Sprintf("%d", tempCounter.Add(1))

	name := prefix + rndStr + suffix
	path := filepath.Join(dir, name)
	err := w.Mkdir(path, 0700|os.ModeDir)
	if err != nil {
		return "", err
	}
	return path, nil
}

func (w FSTestFilesystemReaderWriter) Remove(name string) error {
	// Remove does NOT follow symlinks if the last component is a symlink.
	// But it follows symlinks for directory components.

	// Implementation strategy:
	// Resolve the parent directory.

	p := w.CleanPath(name)
	parent := filepath.Dir(p)
	resolvedParent := "."
	if parent != "." {
		var err error
		resolvedParent, err = w.resolvePath(parent)
		if err != nil {
			return &os.PathError{Op: "remove", Path: name, Err: err}
		}
	}

	base := filepath.Base(p)
	var finalPath string
	if resolvedParent == "." {
		finalPath = base
	} else {
		finalPath = resolvedParent + "/" + base
	}
	finalPath = w.CleanPath(finalPath)

	info, exists := w.FS[finalPath]
	if !exists {
		return &os.PathError{Op: "remove", Path: name, Err: os.ErrNotExist}
	}

	if info.Mode.IsDir() {
		// If it's a directory, check if empty.
		// Note: Use ReadDir on the resolved path directly to avoid recursion or extra resolving
		entries, err := fs.ReadDir(w.fs(), finalPath)
		if err != nil {
			return &os.PathError{Op: "remove", Path: name, Err: err}
		}
		if len(entries) > 0 {
			return &os.PathError{Op: "remove", Path: name, Err: syscall.ENOTEMPTY}
		}
	}

	// If it's a symlink, we just delete the entry (which is the symlink itself).
	delete(w.FS, finalPath)
	return nil
}

func (w FSTestFilesystemReaderWriter) RemoveAll(path string) error {
	// RemoveAll follows symlinks for parent components, but if the final component is a symlink,
	// it removes the symlink, not the target.

	// Similar to Remove, resolve parent.
	p := w.CleanPath(path)
	parent := filepath.Dir(p)
	resolvedParent := "."
	if parent != "." {
		var err error
		resolvedParent, err = w.resolvePath(parent)
		if err != nil {
			// If parent doesn't exist, RemoveAll returns nil (success).
			// But check if it's NotExist or something else.
			// os.RemoveAll returns nil if path does not exist.
			// But if a parent component is a file, it returns error.
			// resolvePath returns error if path resolution fails.

			// We need to differentiate "path does not exist" from "component is not a directory".
			// For now, let's just try to Stat the full path. If it doesn't exist, return nil.
			if os.IsNotExist(err) {
				return nil
			}
			// If intermediate component is a file, resolvePath might return "not a directory" error or similar?
			// resolvePath returns ELOOP or other errors.

			// Let's rely on standard behavior:
			// "If the path does not exist, RemoveAll returns nil."
			// "If the path exists, it removes it."

			// If we can't resolve parent, likely the path doesn't exist.
			return nil
		}
	}

	base := filepath.Base(p)
	var finalPath string
	if resolvedParent == "." {
		finalPath = base
	} else {
		finalPath = resolvedParent + "/" + base
	}
	finalPath = w.CleanPath(finalPath)

	// Check if any parent of finalPath is a file.
	// This is required because os.RemoveAll fails if a path component is a file.
	checkDir := filepath.Dir(finalPath)
	for checkDir != "." && checkDir != "" {
		info, exists := w.FS[checkDir]
		if exists && !info.Mode.IsDir() {
			return &os.PathError{Op: "removeall", Path: path, Err: fmt.Errorf("not a directory")}
		}
		checkDir = filepath.Dir(checkDir)
	}

	// Check if finalPath exists.
	// If it's a symlink, we remove it.
	// If it's a directory, we recurse (but on the map keys).

	if _, exists := w.FS[finalPath]; !exists {
		return nil
	}

	// We need to delete everything that has finalPath as a prefix.
	// BUT, we need to be careful. FSTestOSWrapper stores canonical paths.
	// If we have a symlink involved in the parent path, finalPath is the resolved path.
	// e.g. "a/b", "a" -> "c". resolved is "c/b".
	// The map contains keys starting with "c/b".
	// We should delete "c/b" and "c/b/..."

	prefix := finalPath + "/"
	for key := range w.FS {
		if key == finalPath || strings.HasPrefix(key, prefix) {
			delete(w.FS, key)
		}
	}
	return nil
}

func (w FSTestFilesystemReaderWriter) Symlink(oldname, newname string) error {
	p := w.CleanPath(newname)

	if _, exists := w.FS[p]; exists {
		return &os.PathError{Op: "symlink", Path: newname, Err: os.ErrExist}
	}

	dir := filepath.Dir(p)
	if dir != "." && dir != "" {
		parentInfo, parentExists := w.FS[dir]
		if !parentExists {
			return &os.PathError{Op: "symlink", Path: newname, Err: os.ErrNotExist}
		}
		if !parentInfo.Mode.IsDir() {
			return &os.PathError{Op: "symlink", Path: newname, Err: fmt.Errorf("not a directory")}
		}
	}

	w.FS[p] = &fstest.MapFile{Data: []byte(oldname), Mode: os.ModeSymlink | 0777, ModTime: time.Now()}
	return nil
}

func (w FSTestFilesystemReaderWriter) WriteFile(name string, data []byte, perm os.FileMode) error {
	// WriteFile should follow symlinks.
	// But similar to OpenFile with O_CREATE, we need to handle if the file doesn't exist yet,
	// vs if it exists as a symlink.

	// Since WriteFile = OpenFile(O_WRONLY|O_CREATE|O_TRUNC) + Write + Close,
	// and OpenFile already handles following symlinks and creating targets,
	// we should be able to rely on resolvePath logic similar to OpenFile.

	// However, we can't reuse OpenFile directly because we want to stick to the existing implementation style
	// or maybe we CAN reuse OpenFile?
	// The existing implementation writes directly to the map.

	// Let's adopt logic similar to OpenFile: resolve the path.
	// If the path resolves (existing file or existing target of symlink), we overwrite.
	// If the path does NOT resolve, we need to check if we can create it.
	// This means checking if the directory exists.

	pathToCheck := ""
	resolvedPath, err := w.resolvePath(name)
	if err == nil {
		pathToCheck = resolvedPath
	} else {
		// Resolution failed. This likely means the file doesn't exist.
		// We need to resolve the parent directory.
		parent := filepath.Dir(name)
		resolvedParent, errParent := w.resolvePath(parent)
		if errParent != nil {
			return errParent
		}

		base := filepath.Base(name)
		if resolvedParent == "." {
			pathToCheck = base
		} else {
			pathToCheck = resolvedParent + "/" + base
		}
		pathToCheck = w.CleanPath(pathToCheck)
	}

	if info, exists := w.FS[pathToCheck]; exists && info.Mode.IsDir() {
		return &os.PathError{Op: "open", Path: name, Err: fmt.Errorf("is a directory")}
	}

	dir := filepath.Dir(pathToCheck)
	if dir != "." && dir != "" {
		parentInfo, parentExists := w.FS[dir]
		if !parentExists {
			return &os.PathError{Op: "open", Path: name, Err: os.ErrNotExist}
		}
		if !parentInfo.Mode.IsDir() {
			return &os.PathError{Op: "open", Path: name, Err: fmt.Errorf("not a directory")}
		}
	}
	w.FS[pathToCheck] = &fstest.MapFile{Data: data, Mode: perm, ModTime: time.Now()}
	return nil
}

// --- File implementation ---

func (h *fstestFileHandle) Stat() (os.FileInfo, error) {
	if h.closed {
		return nil, &os.PathError{Op: "stat", Path: h.originalPath, Err: os.ErrClosed}
	}
	return h.info, nil
}

func (h *fstestFileHandle) Close() error {
	if h.closed {
		return &os.PathError{Op: "close", Path: h.originalPath, Err: os.ErrClosed}
	}
	h.closed = true

	// Only flush content back to the map if the file was opened for writing.
	if h.flag&(os.O_WRONLY|os.O_RDWR) != 0 {
		if mapFile, exists := (*h.fsMap)[h.path]; exists {
			mapFile.Data = h.content
			mapFile.ModTime = time.Now()
		}
	}
	return nil
}

func (h *fstestFileHandle) Read(p []byte) (int, error) {
	if h.closed {
		return 0, &os.PathError{Op: "read", Path: h.originalPath, Err: os.ErrClosed}
	}
	if h.flag&os.O_WRONLY != 0 {
		return 0, &os.PathError{Op: "read", Path: h.originalPath, Err: os.ErrInvalid}
	}

	if h.offset >= int64(len(h.content)) {
		return 0, io.EOF
	}

	n := copy(p, h.content[h.offset:])
	h.offset += int64(n)
	return n, nil
}

func (h *fstestFileHandle) Write(p []byte) (int, error) {
	if h.closed {
		return 0, &os.PathError{Op: "write", Path: h.originalPath, Err: os.ErrClosed}
	}
	if h.flag&(os.O_WRONLY|os.O_RDWR) == 0 {
		return 0, &os.PathError{Op: "write", Path: h.originalPath, Err: os.ErrInvalid}
	}

	if h.flag&os.O_APPEND != 0 {
		h.offset = int64(len(h.content))
	}

	n := len(p)
	end := h.offset + int64(n)
	if end > int64(len(h.content)) {
		newContent := make([]byte, end)
		copy(newContent, h.content)
		h.content = newContent
	}

	copy(h.content[h.offset:end], p)
	h.offset = end
	return n, nil
}

func (h *fstestFileHandle) Seek(offset int64, whence int) (int64, error) {
	if h.closed {
		return 0, &os.PathError{Op: "seek", Path: h.originalPath, Err: os.ErrClosed}
	}
	var newOffset int64
	switch whence {
	case io.SeekStart:
		newOffset = offset
	case io.SeekCurrent:
		newOffset = h.offset + offset
	case io.SeekEnd:
		newOffset = int64(len(h.content)) + offset
	default:
		return 0, &os.PathError{Op: "seek", Path: h.originalPath, Err: os.ErrInvalid}
	}

	if newOffset < 0 {
		return 0, &os.PathError{Op: "seek", Path: h.originalPath, Err: os.ErrInvalid}
	}
	h.offset = newOffset
	return h.offset, nil
}
