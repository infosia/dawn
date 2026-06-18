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
	"fmt"
	"strings"
)

// WithCheckout saves the current git state, checks out a specific hash, executes a
// function, and then restores the original git state.
func WithCheckout(t *taskConfig, hash string, fn func() error) error {
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

	defer func() {
		fmt.Printf("Restoring repository to original state (%s)...\n", origRef)
		_, _ = t.runCmd("git", "checkout", origRef)
		_, _ = t.runCmd("gclient", "sync")
	}()

	fmt.Printf("Syncing repository to hash %s...\n", hash)
	if _, err := t.runCmd("git", "checkout", hash); err != nil {
		return fmt.Errorf("failed to checkout %s: %w", hash, err)
	}

	return fn()
}

// SyncAndBuildTargets runs gclient sync and then builds a list of ninja targets.
func SyncAndBuildTargets(t *taskConfig, targets []string) error {
	if _, err := t.runCmd("gclient", "sync"); err != nil {
		return fmt.Errorf("gclient sync failed: %w", err)
	}

	if len(targets) > 0 {
		fmt.Printf("Building %v...\n", targets)
		args := []string{"-C", t.build}
		args = append(args, targets...)
		if _, err := t.runCmd("autoninja", args...); err != nil {
			return fmt.Errorf("build failed for %v: %w", targets, err)
		}
	}
	return nil
}
