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

#include "src/dawn/tests/TestExpectations.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>
#include <utility>

namespace {

std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

void TrimInPlace(std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        s.clear();
        return;
    }
    auto end = s.find_last_not_of(" \t\r\n");
    s = s.substr(start, end - start + 1);
}

// Strip "# ..." trailing comment from a logical line. Only a '#' at start-of-line or
// preceded by whitespace counts as a comment; this lets test names (none of Dawn's today,
// but defensive) contain '#' without confusing the parser.
std::string StripComment(const std::string& line) {
    for (size_t i = 0; i < line.size(); ++i) {
        if (line[i] != '#') {
            continue;
        }
        if (i == 0 || std::isspace(static_cast<unsigned char>(line[i - 1]))) {
            return line.substr(0, i);
        }
    }
    return line;
}

std::vector<std::string> Tokenize(const std::string& s) {
    std::vector<std::string> out;
    std::istringstream iss(s);
    std::string tok;
    while (iss >> tok) {
        out.push_back(tok);
    }
    return out;
}

bool ParseResultName(const std::string& tok, TestResult* out) {
    std::string lower = ToLower(tok);
    if (lower == "skip") {
        *out = TestResult::Skip;
    } else if (lower == "failure" || lower == "fail") {
        *out = TestResult::Failure;
    } else if (lower == "retryonfailure" || lower == "flaky") {
        *out = TestResult::RetryOnFailure;
    } else if (lower == "slow") {
        *out = TestResult::Slow;
    } else {
        return false;
    }
    return true;
}

bool LooksLikeBug(const std::string& tok) {
    // crbug.com/123, b/123, anglebug.com/123, dawn:123, webgpu.org/123, etc. Anything with
    // '/' or ':' before a digit, or starting with a known prefix.
    if (tok.empty()) {
        return false;
    }
    static const char* kPrefixes[] = {"crbug.com/", "b/",         "anglebug.com/",
                                      "dawn:",      "skbug.com/", "webgpu.org/"};
    for (const char* p : kPrefixes) {
        if (tok.compare(0, std::strlen(p), p) == 0) {
            return true;
        }
    }
    return false;
}

}  // namespace

// Matches ANGLE's NamesMatchWithWildcard. Wildcards anywhere; `\*` is a literal '*'.
// Iterative implementation using the classic two-pointer wildcard matcher with backtracking.
bool TestExpectationParser::MatchPattern(const std::string& name, const std::string& pat) {
    size_t ni = 0;
    size_t pi = 0;
    // Position of the last unmatched '*' in pat, and the position in name we should
    // backtrack to (incremented before retry).
    size_t starPi = std::string::npos;
    size_t starNi = 0;
    while (ni < name.size()) {
        // 1) Escaped '*' in pattern: matches a literal '*' in name and consumes both.
        if (pi + 1 < pat.size() && pat[pi] == '\\' && pat[pi + 1] == '*') {
            if (name[ni] == '*') {
                pi += 2;
                ni++;
                continue;
            }
            // Else fall through to the backtracking branch below.
        } else if (pi < pat.size() && pat[pi] == '*') {
            // 2) Wildcard: remember position, tentatively match zero characters.
            starPi = pi++;
            starNi = ni;
            continue;
        } else if (pi < pat.size() && pat[pi] == name[ni]) {
            // 3) Literal match.
            pi++;
            ni++;
            continue;
        }
        // 4) Mismatch: backtrack to the last '*', consuming one more character of name.
        if (starPi != std::string::npos) {
            pi = starPi + 1;
            ni = ++starNi;
            continue;
        }
        return false;
    }
    // Trailing '*'s in pat may match the empty tail.
    while (pi < pat.size() && pat[pi] == '*') {
        pi++;
    }
    return pi == pat.size();
}

bool TestExpectationParser::TagsMatch(const Entry& entry, const TestConditions& conditions) {
    for (const std::string& tag : entry.tags) {
        if (!conditions.Has(tag)) {
            return false;
        }
    }
    return true;
}

TestExpectationParser::TestExpectationParser() = default;
TestExpectationParser::~TestExpectationParser() = default;

bool TestExpectationParser::ParseTagHeader(const std::vector<std::string>& lines,
                                           size_t* outIndexAfterHeader,
                                           std::string* errorOut) {
    bool inHeader = false;
    bool sawEnd = false;
    size_t i = 0;
    for (; i < lines.size(); ++i) {
        std::string line = lines[i];
        TrimInPlace(line);
        if (line.empty()) {
            continue;
        }
        if (!inHeader) {
            if (line.find("BEGIN TAG HEADER") != std::string::npos) {
                inHeader = true;
                continue;
            }
            // No tag header at all -> non-strict mode, treat any tag as legal.
            *outIndexAfterHeader = i;
            mTagHeaderParsed = false;
            mLegalResults = ToResultSet(TestResult::Skip) | ToResultSet(TestResult::Failure) |
                            ToResultSet(TestResult::RetryOnFailure) | ToResultSet(TestResult::Slow);
            return true;
        }
        if (line.find("END TAG HEADER") != std::string::npos) {
            sawEnd = true;
            i++;
            break;
        }
        // Line must start with '#' inside the header.
        if (line.empty() || line[0] != '#') {
            continue;
        }
        // Look for "# tags: [ a b c ]" or "# results: [ a b ]".
        // Strip leading '#' and whitespace.
        std::string body = line.substr(1);
        TrimInPlace(body);
        if (body.compare(0, 5, "tags:") == 0) {
            auto open = body.find('[');
            if (open == std::string::npos) {
                if (errorOut) {
                    *errorOut += mPath + ":" + std::to_string(i + 1) + ": malformed tag set\n";
                }
                continue;
            }
            // Collect the tag content, potentially spanning multiple lines.
            std::string inner;
            auto close = body.find(']');
            if (close != std::string::npos) {
                inner = body.substr(open + 1, close - open - 1);
            } else {
                inner = body.substr(open + 1);
                for (size_t j = i + 1; j < lines.size(); ++j) {
                    std::string cont = lines[j];
                    TrimInPlace(cont);
                    if (cont.empty() || cont[0] != '#') {
                        break;
                    }
                    std::string cb = cont.substr(1);
                    TrimInPlace(cb);
                    auto closeCont = cb.find(']');
                    if (closeCont != std::string::npos) {
                        inner += " " + cb.substr(0, closeCont);
                        i = j;
                        break;
                    }
                    inner += " " + cb;
                    i = j;
                }
            }
            std::set<std::string> set;
            for (const auto& t : Tokenize(inner)) {
                std::string lower = ToLower(t);
                set.insert(lower);
                mLegalTags.insert(lower);
            }
            if (!set.empty()) {
                mTagSets.push_back(std::move(set));
            }
        } else if (body.compare(0, 8, "results:") == 0) {
            auto open = body.find('[');
            auto close = body.find(']');
            if (open == std::string::npos || close == std::string::npos || close <= open) {
                if (errorOut) {
                    *errorOut += mPath + ":" + std::to_string(i + 1) + ": malformed results set\n";
                }
                continue;
            }
            std::string inner = body.substr(open + 1, close - open - 1);
            for (const auto& t : Tokenize(inner)) {
                TestResult r;
                if (!ParseResultName(t, &r)) {
                    if (errorOut) {
                        *errorOut +=
                            mPath + ":" + std::to_string(i + 1) + ": unknown result '" + t + "'\n";
                    }
                    continue;
                }
                mLegalResults |= ToResultSet(r);
            }
        }
    }
    if (!sawEnd && inHeader) {
        if (errorOut) {
            *errorOut += mPath + ": missing END TAG HEADER\n";
        }
    }
    *outIndexAfterHeader = i;
    mTagHeaderParsed = inHeader;
    if (mLegalResults == 0) {
        // Header present but no results: fallback.
        mLegalResults = ToResultSet(TestResult::Skip) | ToResultSet(TestResult::Failure) |
                        ToResultSet(TestResult::RetryOnFailure) | ToResultSet(TestResult::Slow);
    }
    return true;
}

bool TestExpectationParser::ParseExpectationLine(const std::string& rawLine,
                                                 int lineNumber,
                                                 std::string* errorOut) {
    std::string line = StripComment(rawLine);
    TrimInPlace(line);
    if (line.empty()) {
        return true;
    }

    Entry entry;
    entry.lineNumber = lineNumber;

    // Parse:
    //   [bug] [[ tag1 tag2 ... ]] test-pattern [ result1 result2 ... ]
    size_t pos = 0;

    // Optional bug ID (anything that doesn't start with '[').
    // We tokenize whitespace-separated until we hit '['.
    while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos]))) {
        pos++;
    }
    if (pos < line.size() && line[pos] != '[') {
        size_t end = line.find_first_of(" \t", pos);
        if (end == std::string::npos) {
            end = line.size();
        }
        std::string firstTok = line.substr(pos, end - pos);
        if (LooksLikeBug(firstTok)) {
            entry.bug = firstTok;
            pos = end;
        }
    }
    while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos]))) {
        pos++;
    }

    // Optional tag block.
    if (pos < line.size() && line[pos] == '[') {
        auto close = line.find(']', pos);
        if (close == std::string::npos) {
            if (errorOut) {
                *errorOut +=
                    mPath + ":" + std::to_string(lineNumber) + ": unterminated tag block\n";
            }
            return false;
        }
        std::string inner = line.substr(pos + 1, close - pos - 1);
        std::set<size_t> seenSets;
        for (const auto& rawTok : Tokenize(inner)) {
            std::string tag = ToLower(rawTok);
            if (mTagHeaderParsed && !mLegalTags.count(tag)) {
                if (errorOut) {
                    *errorOut +=
                        mPath + ":" + std::to_string(lineNumber) + ": unknown tag '" + tag + "'\n";
                }
                return false;
            }
            // Make sure no two tags come from the same set on one line.
            for (size_t i = 0; i < mTagSets.size(); ++i) {
                if (mTagSets[i].count(tag)) {
                    if (seenSets.count(i)) {
                        if (errorOut) {
                            *errorOut += mPath + ":" + std::to_string(lineNumber) + ": tags '" +
                                         tag + "' and another from the same set\n";
                        }
                        return false;
                    }
                    seenSets.insert(i);
                    break;
                }
            }
            entry.tags.push_back(std::move(tag));
        }
        pos = close + 1;
    }
    while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos]))) {
        pos++;
    }

    // Test pattern.
    size_t patStart = pos;
    size_t patEnd = line.find_first_of(" \t", patStart);
    if (patEnd == std::string::npos) {
        if (errorOut) {
            *errorOut += mPath + ":" + std::to_string(lineNumber) + ": missing result block\n";
        }
        return false;
    }
    entry.testPattern = line.substr(patStart, patEnd - patStart);
    pos = patEnd;

    // Mark glob.
    for (size_t i = 0; i < entry.testPattern.size(); ++i) {
        if (entry.testPattern[i] == '*' && !(i > 0 && entry.testPattern[i - 1] == '\\')) {
            entry.isGlob = true;
            break;
        }
    }

    while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos]))) {
        pos++;
    }
    if (pos >= line.size() || line[pos] != '[') {
        if (errorOut) {
            *errorOut +=
                mPath + ":" + std::to_string(lineNumber) + ": expected '[' before results\n";
        }
        return false;
    }
    auto resClose = line.find(']', pos);
    if (resClose == std::string::npos) {
        if (errorOut) {
            *errorOut += mPath + ":" + std::to_string(lineNumber) + ": unterminated result block\n";
        }
        return false;
    }
    std::string resInner = line.substr(pos + 1, resClose - pos - 1);
    for (const auto& tok : Tokenize(resInner)) {
        TestResult r;
        if (!ParseResultName(tok, &r)) {
            if (errorOut) {
                *errorOut +=
                    mPath + ":" + std::to_string(lineNumber) + ": unknown result '" + tok + "'\n";
            }
            return false;
        }
        // mLegalResults is always populated by ParseTagHeader (fallback to the full set when
        // the header is absent), so this check actually gates Slow/etc. when a future header
        // restricts the legal set.
        if (!HasResult(mLegalResults, r)) {
            if (errorOut) {
                *errorOut += mPath + ":" + std::to_string(lineNumber) + ": result '" + tok +
                             "' not in legal set\n";
            }
            return false;
        }
        entry.results |= ToResultSet(r);
    }
    if (entry.results == 0) {
        if (errorOut) {
            *errorOut += mPath + ":" + std::to_string(lineNumber) + ": empty result block\n";
        }
        return false;
    }

    mEntries.push_back(std::move(entry));
    return true;
}

bool TestExpectationParser::LoadFile(const std::string& path, std::string* errorOut) {
    mPath = path;
    mEntries.clear();
    mExactByName.clear();
    mTagSets.clear();
    mLegalTags.clear();
    mLegalResults = 0;
    mTagHeaderParsed = false;

    std::ifstream file(path);
    if (!file.is_open()) {
        if (errorOut) {
            *errorOut += "Failed to open expectations file: " + path + "\n";
        }
        return false;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }

    size_t startIdx = 0;
    ParseTagHeader(lines, &startIdx, errorOut);

    // Per-line expectation parse errors are non-fatal: log into errorOut, drop the bad
    // entry, keep going. A single typo shouldn't crash the entire test binary on a bot
    // when the file is edited out-of-source.
    for (size_t i = startIdx; i < lines.size(); ++i) {
        ParseExpectationLine(lines[i], static_cast<int>(i + 1), errorOut);
    }

    // Sort entries: exact (non-glob) first (lookup uses them as overrides), then globs by
    // descending pattern length (most specific first).
    std::stable_sort(mEntries.begin(), mEntries.end(), [](const Entry& a, const Entry& b) {
        if (a.isGlob != b.isGlob) {
            return !a.isGlob;
        }
        if (a.isGlob && b.isGlob) {
            return a.testPattern.size() > b.testPattern.size();
        }
        return false;
    });

    // Index exact entries by name so the common exact-match lookup is O(1) amortized
    // rather than O(N) per test. The key is the *unescaped* pattern (`\*` → `*`), so that
    // a test whose name literally contains an asterisk can be found via this index.
    auto unescape = [](const std::string& pat) {
        std::string out;
        out.reserve(pat.size());
        for (size_t k = 0; k < pat.size(); ++k) {
            if (pat[k] == '\\' && k + 1 < pat.size() && pat[k + 1] == '*') {
                out += '*';
                ++k;
            } else {
                out += pat[k];
            }
        }
        return out;
    };
    for (size_t i = 0; i < mEntries.size(); ++i) {
        if (mEntries[i].isGlob) {
            break;
        }
        mExactByName[unescape(mEntries[i].testPattern)].push_back(i);
    }

    return true;
}

TestResultSet TestExpectationParser::GetExpectation(const std::string& testName,
                                                    const TestConditions& conditions,
                                                    std::vector<std::string>* outBugs) {
    TestResultSet merged = 0;
    auto recordBug = [&](const Entry& e) {
        if (outBugs && !e.bug.empty()) {
            outBugs->push_back(e.bug);
        }
    };
    // Exact matches first via the hash index.
    auto it = mExactByName.find(testName);
    if (it != mExactByName.end()) {
        for (size_t idx : it->second) {
            Entry& entry = mEntries[idx];
            if (!TagsMatch(entry, conditions)) {
                continue;
            }
            merged |= entry.results;
            entry.used = true;
            recordBug(entry);
        }
    }
    if (merged != 0) {
        return merged;
    }
    // Globs in descending length order. Once a length tier produces a match, later
    // shorter-tier globs are skipped (typ semantics: longest match wins, UNION among ties).
    size_t lastMatchedLen = std::string::npos;
    for (auto& entry : mEntries) {
        if (!entry.isGlob) {
            continue;
        }
        if (merged != 0 && entry.testPattern.size() < lastMatchedLen) {
            break;
        }
        if (!MatchPattern(testName, entry.testPattern)) {
            continue;
        }
        if (!TagsMatch(entry, conditions)) {
            continue;
        }
        merged |= entry.results;
        entry.used = true;
        recordBug(entry);
        lastMatchedLen = entry.testPattern.size();
    }
    return merged;
}

std::vector<std::string> TestExpectationParser::GetUnusedExpectationsMessages() const {
    std::vector<std::string> out;
    for (const auto& entry : mEntries) {
        if (!entry.used) {
            out.push_back(mPath + ":" + std::to_string(entry.lineNumber) + ": expectation '" +
                          entry.testPattern + "' was unused");
        }
    }
    return out;
}

std::vector<std::string> TestExpectationParser::GetPlaceholderBugMessages() const {
    std::vector<std::string> out;
    for (const auto& entry : mEntries) {
        if (entry.bug == "crbug.com/0") {
            out.push_back(mPath + ":" + std::to_string(entry.lineNumber) + ": '" +
                          entry.testPattern + "' uses placeholder bug 'crbug.com/0'");
        }
    }
    return out;
}

size_t TestExpectationParser::GetEntryCount() const {
    return mEntries.size();
}
