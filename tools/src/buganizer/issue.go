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

package buganizer

import (
	"encoding/json"
	"errors"
	//"fmt"
)

type IssueStatus string

const (
	StatusNew IssueStatus = "NEW"
	StatusAssigned IssueStatus = "ASSIGNED"
	StatusAccepted IssueStatus = "ACCEPTED"
	StatusFixed IssueStatus = "FIXED"
	StatusVerified IssueStatus = "VERIFIED"
	StatusNotReproducible IssueStatus = "NOT_REPRODUCIBLE"
	StatusIntendedBehavior IssueStatus = "INTENDED_BEHAVIOR"
	StatusObsolete IssueStatus = "OBSOLETE"
	StatusInfeasible IssueStatus = "INFEASIBLE"
	StatusDuplicate IssueStatus = "DUPLICATE"
)

type IssuePriority string

const (
	Priority0 IssuePriority = "P0"
	Priority1 IssuePriority = "P1"
	Priority2 IssuePriority = "P2"
	Priority3 IssuePriority = "P3"
	Priority4 IssuePriority = "P4"
)

type IssueSeverity string

const (
	Severity0 IssueSeverity = "S0"
	Severity1 IssueSeverity = "S1"
	Severity2 IssueSeverity = "S2"
	Severity3 IssueSeverity = "S3"
	Severity4 IssueSeverity = "S4"
)

type FormatMode string

const (
	FormatPlain FormatMode = "PLAIN"
	FormatMarkdown FormatMode = "MARKDOWN"
)

//type CCList []string

type NewIssueInfo struct {
	Title string
	Description string
	ComponentId string
	Cc []string
	Status IssueStatus
	Severity IssueSeverity
	Priority IssuePriority
	Format FormatMode
}

type NewCommentInfo struct {
	Comment string
	Format FormatMode
}

type issueStateJson struct {
	Title string  `json:"title"`
	ComponentId string `json:"componentId"`
	Status string `json:"status"`
	Type string `json:"type"`
	Severity string `json:"severity"`
	Priority string `json:"priority"`
	Ccs []map[string]string `json:"ccs"`
}

type issueCommentJson struct {
	Comment string `json:"comment"`
	FormattingMode string `json:"formattingMode"`
}

// We need to have separate structs for new issues and new comments since there
// is no good way to have IssueState omitted for comments.
type newIssueJson struct {
	IssueState issueStateJson `json:"issueState"`
	IssueComment issueCommentJson `json:"issuecomment"`
}

type newCommentJson struct {
	IssueComment issueCommentJson `json:"issuecomment"`
}

func (i NewIssueInfo) AsJSONString() (string, error) {
	// Validate.
	if i.Title == "" {
		return "", errors.New("Title not set")
	}
	if i.Description == "" {
		return "", errors.New("Description not set")
	}
	if i.ComponentId == "" {
		return "", errors.New("Component ID not set")
	}
	if i.Status == "" {
		return "", errors.New("Status not set")
	}

	// Set defaults.
	format := i.Format
	if format == "" {
		format = FormatPlain
	}

	// Construct values.
	ccList := []map[string]string{}
	for _, email := range i.Cc {
		ccList = append(ccList, map[string]string{"emailAddress": email})
	}

	// Construct JSON.
	jsonState := newIssueJson{
		IssueState: issueStateJson{
			Title: i.Title,
			ComponentId: i.ComponentId,
			Status: string(i.Status),
			Type: "BUG",
			Severity: string(i.Severity),
			Priority: string(i.Priority),
			Ccs: ccList,
		},
		IssueComment: issueCommentJson{
			Comment: i.Description,
			FormattingMode: string(format),
		},
	}

	jsonBytes, err := json.Marshal(jsonState)
	if err != nil {
		return "", err
	}

	return string(jsonBytes[:]), nil
}

func (c NewCommentInfo) AsJSONString() (string, error) {
	// Validate.
	if c.Comment == "" {
		return "", errors.New("Comment not set")
	}

	// Set defaults.
	format := c.Format
	if format == "" {
		format = FormatPlain
	}

	// Construct JSON
	jsonComment := newCommentJson{
		IssueComment: issueCommentJson{
			Comment: c.Comment,
			FormattingMode: string(format),
		},
	}

	jsonBytes, err := json.Marshal(jsonComment)
	if err != nil {
		return "", err
	}

	return string(jsonBytes[:]), nil
}
