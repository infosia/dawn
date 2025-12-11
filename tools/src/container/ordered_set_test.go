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

package container_test

import (
	"fmt"
	"testing"

	"dawn.googlesource.com/dawn/tools/src/container"
)

func TestNewEmptyOrderedSet(t *testing.T) {
	s := container.NewOrderedSet[string]()
	expectEq(t, "s.Len()", s.Len(), 0)
}

func TestNewOrderedSet(t *testing.T) {
	s := container.NewOrderedSet("c", "a", "b")
	expectEq(t, "s.Len()", s.Len(), 3)
	expectEq(t, "s.List()", s.List(), []string{"a", "b", "c"})
}

func TestOrderedSetList(t *testing.T) {
	s := container.NewOrderedSet("c", "a", "b")
	expectEq(t, "s.List()", s.List(), []string{"a", "b", "c"})
}

func TestOrderedSetClone(t *testing.T) {
	a := container.NewOrderedSet("c", "a", "b")
	b := a.Clone()
	a.Remove("a")
	expectEq(t, "b.List()", b.List(), []string{"a", "b", "c"})
}

func TestOrderedSetAdd(t *testing.T) {
	s := container.NewOrderedSet[string]()
	s.Add("c")
	expectEq(t, "s.Len()", s.Len(), 1)
	expectEq(t, "s.List()", s.List(), []string{"c"})

	s.Add("a")
	expectEq(t, "s.Len()", s.Len(), 2)
	expectEq(t, "s.List()", s.List(), []string{"a", "c"})

	s.Add("b")
	expectEq(t, "s.Len()", s.Len(), 3)
	expectEq(t, "s.List()", s.List(), []string{"a", "b", "c"})

	s.Add("a") // Adding existing item should not change anything
	expectEq(t, "s.Len()", s.Len(), 3)
	expectEq(t, "s.List()", s.List(), []string{"a", "b", "c"})
}

func TestOrderedSetRemove(t *testing.T) {
	s := container.NewOrderedSet("c", "a", "b")
	s.Remove("a")
	expectEq(t, "s.Len()", s.Len(), 2)
	expectEq(t, "s.List()", s.List(), []string{"b", "c"})

	s.Remove("c")
	expectEq(t, "s.Len()", s.Len(), 1)
	expectEq(t, "s.List()", s.List(), []string{"b"})

	s.Remove("b")
	expectEq(t, "s.Len()", s.Len(), 0)
	expectEq(t, "s.List()", s.List(), []string{})
}

func TestOrderedSetContains(t *testing.T) {
	s := container.NewOrderedSet[string]()
	s.Add("c")
	expectEq(t, `s.Contains("a")`, s.Contains("a"), false)
	expectEq(t, `s.Contains("b")`, s.Contains("b"), false)
	expectEq(t, `s.Contains("c")`, s.Contains("c"), true)

	s.Add("a")
	expectEq(t, `s.Contains("a")`, s.Contains("a"), true)
	expectEq(t, `s.Contains("b")`, s.Contains("b"), false)
	expectEq(t, `s.Contains("c")`, s.Contains("c"), true)

	s.Add("b")
	expectEq(t, `s.Contains("a")`, s.Contains("a"), true)
	expectEq(t, `s.Contains("b")`, s.Contains("b"), true)
	expectEq(t, `s.Contains("c")`, s.Contains("c"), true)
}

func TestOrderedSetContainsAll(t *testing.T) {
	S := container.NewOrderedSet[string]

	s := container.NewOrderedSet[string]()
	s.Add("c")
	expectEq(t, `s.ContainsAll("a")`, s.ContainsAll(S("a")),
		false)
	expectEq(t, `s.ContainsAll("b")`, s.ContainsAll(S("b")),
		false)
	expectEq(t, `s.ContainsAll("c")`, s.ContainsAll(S("c")),
		true)
	expectEq(t, `s.ContainsAll("a", "b")`, s.ContainsAll(S("a", "b")),
		false)
	expectEq(t, `s.ContainsAll("b", "c")`, s.ContainsAll(S("b", "c")),
		false)
	expectEq(t, `s.ContainsAll("c", "a")`, s.ContainsAll(S("c", "a")),
		false)
	expectEq(t, `s.ContainsAll("c", "a", "b")`, s.ContainsAll(S("c", "a", "b")),
		false)

	s.Add("a")
	expectEq(t, `s.ContainsAll("a")`, s.ContainsAll(S("a")),
		true)
	expectEq(t, `s.ContainsAll("b")`, s.ContainsAll(S("b")),
		false)
	expectEq(t, `s.ContainsAll("c")`, s.ContainsAll(S("c")),
		true)
	expectEq(t, `s.ContainsAll("a", "b")`, s.ContainsAll(S("a", "b")),
		false)
	expectEq(t, `s.ContainsAll("b", "c")`, s.ContainsAll(S("b", "c")),
		false)
	expectEq(t, `s.ContainsAll("c", "a")`, s.ContainsAll(S("c", "a")),
		true)
	expectEq(t, `s.ContainsAll("c", "a", "b")`, s.ContainsAll(S("c", "a", "b")),
		false)

	s.Add("b")
	expectEq(t, `s.ContainsAll("a")`, s.ContainsAll(S("a")),
		true)
	expectEq(t, `s.ContainsAll("b")`, s.ContainsAll(S("b")),
		true)
	expectEq(t, `s.ContainsAll("c")`, s.ContainsAll(S("c")),
		true)
	expectEq(t, `s.ContainsAll("a", "b")`, s.ContainsAll(S("a", "b")),
		true)
	expectEq(t, `s.ContainsAll("b", "c")`, s.ContainsAll(S("b", "c")),
		true)
	expectEq(t, `s.ContainsAll("c", "a")`, s.ContainsAll(S("c", "a")),
		true)
	expectEq(t, `s.ContainsAll("c", "a", "b")`, s.ContainsAll(S("c", "a", "b")),
		true)
}

func TestOrderedSetContainsAny(t *testing.T) {
	S := container.NewOrderedSet[string]

	s := container.NewOrderedSet[string]()
	s.Add("c")
	expectEq(t, `s.ContainsAny("a")`, s.ContainsAny(S("a")),
		false)
	expectEq(t, `s.ContainsAny("b")`, s.ContainsAny(S("b")),
		false)
	expectEq(t, `s.ContainsAny("c")`, s.ContainsAny(S("c")),
		true)
	expectEq(t, `s.ContainsAny("a", "b")`, s.ContainsAny(S("a", "b")),
		false)
	expectEq(t, `s.ContainsAny("b", "c")`, s.ContainsAny(S("b", "c")),
		true)
	expectEq(t, `s.ContainsAny("c", "a")`, s.ContainsAny(S("c", "a")),
		true)
	expectEq(t, `s.ContainsAny("c", "a", "b")`, s.ContainsAny(S("c", "a", "b")),
		true)

	s.Add("a")
	expectEq(t, `s.ContainsAny("a")`, s.ContainsAny(S("a")),
		true)
	expectEq(t, `s.ContainsAny("b")`, s.ContainsAny(S("b")),
		false)
	expectEq(t, `s.ContainsAny("c")`, s.ContainsAny(S("c")),
		true)
	expectEq(t, `s.ContainsAny("a", "b")`, s.ContainsAny(S("a", "b")),
		true)
	expectEq(t, `s.ContainsAny("b", "c")`, s.ContainsAny(S("b", "c")),
		true)
	expectEq(t, `s.ContainsAny("c", "a")`, s.ContainsAny(S("c", "a")),
		true)
	expectEq(t, `s.ContainsAny("c", "a", "b")`, s.ContainsAny(S("c", "a", "b")),
		true)

	s.Remove("c")
	s.Add("b")
	expectEq(t, `s.ContainsAny("a")`, s.ContainsAny(S("a")),
		true)
	expectEq(t, `s.ContainsAny("b")`, s.ContainsAny(S("b")),
		true)
	expectEq(t, `s.ContainsAny("c")`, s.ContainsAny(S("c")),
		false)
	expectEq(t, `s.ContainsAny("a", "b")`, s.ContainsAny(S("a", "b")),
		true)
	expectEq(t, `s.ContainsAny("b", "c")`, s.ContainsAny(S("b", "c")),
		true)
	expectEq(t, `s.ContainsAny("c", "a")`, s.ContainsAny(S("c", "a")),
		true)
	expectEq(t, `s.ContainsAny("c", "a", "b")`, s.ContainsAny(S("c", "a", "b")),
		true)
}

func TestOrderedSetIntersection(t *testing.T) {
	a := container.NewOrderedSet(1, 3, 4, 6)
	b := container.NewOrderedSet(2, 3, 4, 5)

	i := a.Intersection(b)
	expectEq(t, `i.List()`, i.List(), []int{3, 4})
}

func TestOrderedSetAddAll(t *testing.T) {
	s := container.NewOrderedSet[string]()
	s.AddAll(container.NewOrderedSet("c", "a"))
	expectEq(t, "s.Len()", s.Len(), 2)
	expectEq(t, "s.List()", s.List(), []string{"a", "c"})
}

func TestOrderedSetRemoveAll(t *testing.T) {
	s := container.NewOrderedSet("c", "a", "b")
	s.RemoveAll(container.NewOrderedSet("a", "c"))
	expectEq(t, "s.Len()", s.Len(), 1)
	expectEq(t, "s.List()", s.List(), []string{"b"})
}

func TestOrderedSetOne(t *testing.T) {
	expectEq(t, "NewOrderedSet[string]().One()", container.NewOrderedSet[string]().One(), "")
	expectEq(t, `NewOrderedSet("x").One()`, container.NewOrderedSet("x").One(), "x")
	expectEq(t, `NewOrderedSet("x", "y").One()`, container.NewOrderedSet("x", "y").One(), "x")
	expectEq(t, `NewOrderedSet("y", "x").One()`, container.NewOrderedSet("y", "x").One(), "x")
}

func TestOrderedSetFormat(t *testing.T) {
	expectEq(t, "NewOrderedSet[string]()", fmt.Sprint(container.NewOrderedSet[string]()), "[]")
	expectEq(t, `NewOrderedSet("x")`, fmt.Sprint(container.NewOrderedSet("x")),
		`[x]`)
	expectEq(t, `NewOrderedSet(1)`, fmt.Sprint(container.NewOrderedSet(1)), `[1]`)
	expectEq(t, `NewOrderedSet("y", "x")`, fmt.Sprint(container.NewOrderedSet("y", "x")),
		`[x, y]`)
	expectEq(t, `NewOrderedSet(3, 1, 2)`, fmt.Sprint(container.NewOrderedSet(3, 1, 2)), `[1, 2, 3]`)
}
