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

package container

import (
	"fmt"
	"sort"
)

// OrderedSet is a generic ordered set, which preserves insertion order.
// T is the set key, which must match the 'key' constraint.
type OrderedSet[T key] struct {
	index map[T]int
	list  []T
}

// Returns a new ordered set with the given items
func NewOrderedSet[T key](items ...T) *OrderedSet[T] {
	out := &OrderedSet[T]{
		index: make(map[T]int),
		list:  make([]T, 0, len(items)),
	}
	for _, item := range items {
		out.Add(item)
	}
	return out
}

// Clone returns a new OrderedSet populated with s
func (s *OrderedSet[T]) Clone() *OrderedSet[T] {
	if s == nil {
		return nil
	}
	out := &OrderedSet[T]{
		index: make(map[T]int, len(s.list)),
		list:  make([]T, len(s.list)),
	}
	copy(out.list, s.list)
	for i, item := range out.list {
		out.index[item] = i
	}
	return out
}

// Add adds an item to the set.
// If s is nil, this will panic.
func (s *OrderedSet[T]) Add(item T) {
	if _, exists := s.index[item]; !exists {
		s.index[item] = len(s.list)
		s.list = append(s.list, item)
	}
}

// AddAll adds all the items of o to the set.
// If s is nil, this will panic.
func (s *OrderedSet[T]) AddAll(o *OrderedSet[T]) {
	if o == nil {
		return
	}
	for _, item := range o.list {
		s.Add(item)
	}
}

// Remove removes an item from the set
func (s *OrderedSet[T]) Remove(item T) {
	if s == nil {
		return
	}
	if idx, exists := s.index[item]; exists {
		delete(s.index, item)
		s.list = append(s.list[:idx], s.list[idx+1:]...)
		// Update indices of subsequent elements
		for i := idx; i < len(s.list); i++ {
			s.index[s.list[i]] = i
		}
	}
}

// RemoveAll removes all the items of o from the set.
func (s *OrderedSet[T]) RemoveAll(o *OrderedSet[T]) {
	if s == nil || o == nil {
		return
	}
	for _, item := range o.list {
		s.Remove(item)
	}
}

// Contains returns true if the set contains the given item
func (s *OrderedSet[T]) Contains(item T) bool {
	if s == nil {
		return false
	}
	_, found := s.index[item]
	return found
}

// ContainsAll returns true if the set contains all the items in o
func (s *OrderedSet[T]) ContainsAll(o *OrderedSet[T]) bool {
	if o == nil {
		return true
	}
	if s == nil {
		return o.Len() == 0
	}
	for _, item := range o.list {
		if !s.Contains(item) {
			return false
		}
	}
	return true
}

// ContainsAny returns true if the set contains any of the items in o
func (s *OrderedSet[T]) ContainsAny(o *OrderedSet[T]) bool {
	if s == nil || o == nil {
		return false
	}
	for _, item := range o.list {
		if s.Contains(item) {
			return true
		}
	}
	return false
}

// Intersection returns a new set containing items present in both s and o.
// The order of elements in the returned set will be from s.
func (s *OrderedSet[T]) Intersection(o *OrderedSet[T]) *OrderedSet[T] {
	out := NewOrderedSet[T]()
	if s == nil || o == nil {
		return out
	}
	for _, item := range s.list {
		if o.Contains(item) {
			out.Add(item)
		}
	}
	return out
}

// List returns the entries of the set as a slice, sorted lexicographically.
func (s *OrderedSet[T]) List() []T {
	if s == nil {
		return nil
	}
	// Clone the list to prevent external modifications
	list := make([]T, len(s.list))
	copy(list, s.list)
	sort.Slice(list, func(i, j int) bool {
		return fmt.Sprint(list[i]) < fmt.Sprint(list[j])
	})
	return list
}

// Len returns the number of items in the set.
func (s *OrderedSet[T]) Len() int {
	if s == nil {
		return 0
	}
	return len(s.list)
}

// One returns the lexicographically first item from the set, or an empty item if the set is empty.
func (s *OrderedSet[T]) One() T {
	if s != nil && len(s.list) > 0 {
		return s.List()[0]
	}
	var zero T
	return zero
}

// Format writes the Target to the fmt.State. The items are sorted lexicographically.
func (s *OrderedSet[T]) Format(f fmt.State, verb rune) {
	fmt.Fprint(f, "[")
	if s != nil {
		for i, item := range s.List() {
			if i > 0 {
				fmt.Fprint(f, ", ")
			}
			fmt.Fprint(f, item)
		}
	}
	fmt.Fprint(f, "]")
}
