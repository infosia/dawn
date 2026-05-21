// Copyright 2026 The Dawn & Tint Authors
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

#ifndef SRC_DAWN_TESTS_TESTEXPECTATIONS_H_
#define SRC_DAWN_TESTS_TESTEXPECTATIONS_H_

#include <cstdint>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// File format (typ-style, modeled on webgpu-cts/expectations.txt):
//
//   # BEGIN TAG HEADER
//   # OS
//   # tags: [ win linux mac android chromeos ]
//   # GPU
//   # tags: [ amd apple arm intel nvidia qualcomm imgtec mesa-software ]
//   # Backend
//   # tags: [ d3d11 d3d12 metal vulkan opengl opengles ]
//   # Adapter
//   # tags: [ swiftshader warp angle ]
//   # Sanitizer
//   # tags: [ asan tsan ]
//   # Mode
//   # tags: [ compat wire backend-validation full-backend-validation
//   #         implicit-device-sync capture-replay webgpu-on-webgpu ]
//   # Intel-Gen
//   # tags: [ intel-gen9 intel-gen12 intel-gen12-or-later ]
//   # results: [ Skip Failure RetryOnFailure Slow ]
//   # END TAG HEADER
//
//   crbug.com/1234 [ vulkan nvidia ] BufferTests.MapRead_NonZeroOffset/* [ Skip ]
//   crbug.com/5678 [ d3d12 win ] BindGroupTests.* [ Failure ]
//
// Semantics:
//   - Tags are AND-combined within one line. The runner's active tag set must contain
//     all of the line's tags for the line to match.
//   - Wildcards `*` may appear anywhere in the test name (prefix/middle/end), multiple
//     times. Use `\*` for a literal asterisk. Implementation matches ANGLE's
//     NamesMatchWithWildcard. Note: this is a deliberate divergence from typ, which only
//     allows a single trailing `*`.
//   - When multiple lines match the same test, the longest specificity wins and results
//     are UNION-merged among ties. Exact (non-glob) matches beat any glob; among globs,
//     the longest pattern wins and any equal-length globs union their results. This
//     matches the typ runner's behavior.
//   - Empty tag list `[ ]` (or no tag block at all) matches every config.
//   - Bug ID is optional. Recognized prefixes: crbug.com/, b/, anglebug.com/, dawn:,
//     skbug.com/, webgpu.org/. The placeholder `crbug.com/0` is allowed but warned
//     during load so it can be triaged.
//   - Comments start with `#` at the beginning of a line or after whitespace and run
//     to end of line. Inline `#` comments are allowed.
//   - Two tags from the same tag set on one line is a parse error.

enum class TestResult {
    Skip,
    Failure,
    RetryOnFailure,
    Slow,
};

using TestResultSet = uint32_t;  // bitmask of (1u << static_cast<uint32_t>(TestResult))

inline TestResultSet ToResultSet(TestResult r) {
    return 1u << static_cast<uint32_t>(r);
}

inline bool HasResult(TestResultSet set, TestResult r) {
    return (set & ToResultSet(r)) != 0;
}

// Active tag set for a particular test instance. Populated from DawnTestBase Is*() methods
// before each test runs. Tags are lowercase strings matching those in the file header.
struct TestConditions {
    std::unordered_set<std::string> tags;

    void Add(const std::string& tag) { tags.insert(tag); }
    bool Has(const std::string& tag) const { return tags.count(tag) > 0; }
};

class TestExpectationParser {
  public:
    TestExpectationParser();
    ~TestExpectationParser();

    // Parses the file. Returns true if the file is syntactically valid. Errors are appended
    // to errorOut as multi-line text. Header errors are fatal (returns false). Per-line
    // expectation errors are non-fatal: the bad entry is dropped, the error is appended to
    // errorOut, and parsing continues.
    bool LoadFile(const std::string& path, std::string* errorOut);

    // Returns the set of results that apply to (testName, conditions). When no expectation
    // matches, returns 0. Marks each contributing entry as "used" for later reporting.
    // When `outBugs` is non-null, the bug IDs from each contributing entry are appended
    // (in entry order, possibly with duplicates filtered by the caller).
    TestResultSet GetExpectation(const std::string& testName,
                                 const TestConditions& conditions,
                                 std::vector<std::string>* outBugs = nullptr);

    // Returns "file:line: pattern" for each expectation line that was never selected by any
    // GetExpectation() call. Useful to flag stale entries after the run.
    std::vector<std::string> GetUnusedExpectationsMessages() const;

    // Returns "file:line" for each expectation line whose bug ID is the `crbug.com/0`
    // placeholder. Reported by DawnTestEnvironment so the count visibly decays as entries
    // get triaged.
    std::vector<std::string> GetPlaceholderBugMessages() const;

    // Returns the count of expectation entries loaded (for diagnostics).
    size_t GetEntryCount() const;

  private:
    struct Entry {
        std::string bug;
        std::vector<std::string> tags;  // all lowercase
        std::string testPattern;
        TestResultSet results = 0;
        int lineNumber = 0;
        bool isGlob = false;  // contains an unescaped '*'
        bool used = false;
    };

    // Tag header information.
    std::vector<std::set<std::string>> mTagSets;  // legal tag sets, lowercase
    std::set<std::string> mLegalTags;             // union, for quick lookup
    TestResultSet mLegalResults = 0;              // mask of legal results
    bool mTagHeaderParsed = false;

    std::vector<Entry> mEntries;
    // Index of exact (non-glob) entries by testPattern -> indices into mEntries. Built in
    // LoadFile() after stable_sort so it stays consistent with the storage. Lets
    // GetExpectation() avoid an O(N) scan for the common exact-match case.
    std::unordered_map<std::string, std::vector<size_t>> mExactByName;
    std::string mPath;

    bool ParseTagHeader(const std::vector<std::string>& lines,
                        size_t* outIndexAfterHeader,
                        std::string* errorOut);
    bool ParseExpectationLine(const std::string& line, int lineNumber, std::string* errorOut);

    // True if testName matches pattern. Wildcards anywhere; `\*` is literal '*'.
    static bool MatchPattern(const std::string& testName, const std::string& pattern);

    // True if all of the entry's tags appear in conditions.
    static bool TagsMatch(const Entry& entry, const TestConditions& conditions);
};

#endif  // SRC_DAWN_TESTS_TESTEXPECTATIONS_H_
