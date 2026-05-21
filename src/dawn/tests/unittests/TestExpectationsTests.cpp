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

#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "dawn/tests/TestExpectations.h"
#include "gtest/gtest.h"

namespace dawn {
namespace {

// Standard tag header used by most tests. Mirrors the layout the runner uses.
const char kStdHeader[] =
    "# BEGIN TAG HEADER\n"
    "# OS\n"
    "# tags: [ win linux mac ]\n"
    "# GPU\n"
    "# tags: [ amd intel nvidia ]\n"
    "# Backend\n"
    "# tags: [ d3d11 d3d12 metal vulkan opengl opengles ]\n"
    "# results: [ Skip Failure RetryOnFailure Slow ]\n"
    "# END TAG HEADER\n";

// Writes the given file body into a unique temporary file in the OS temp dir and removes
// it on destruction. Uses a process-unique counter to avoid races with std::tmpnam (which
// triggers -Wdeprecated-declarations under Chromium's build flags).
class ScopedTempFile {
  public:
    explicit ScopedTempFile(const std::string& contents) {
        static std::atomic<uint64_t> sCounter{0};
        const auto stamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        std::filesystem::path p = std::filesystem::temp_directory_path() /
                                  ("dawn_expect_" + std::to_string(stamp) + "_" +
                                   std::to_string(sCounter.fetch_add(1)) + ".txt");
        mPath = p.string();
        std::ofstream out(mPath);
        out << contents;
    }
    ~ScopedTempFile() {
        std::error_code ec;
        std::filesystem::remove(mPath, ec);
    }
    ScopedTempFile(const ScopedTempFile&) = delete;
    ScopedTempFile& operator=(const ScopedTempFile&) = delete;

    const std::string& path() const { return mPath; }

  private:
    std::string mPath;
};

TestConditions MakeConditions(std::initializer_list<std::string> tags) {
    TestConditions c;
    for (const auto& t : tags) {
        c.Add(t);
    }
    return c;
}

// Returns true if `set` contains result `r`.
bool Contains(TestResultSet set, TestResult r) {
    return HasResult(set, r);
}

TEST(TestExpectations, EmptyFile) {
    ScopedTempFile f{std::string(kStdHeader)};
    TestExpectationParser p;
    std::string err;
    ASSERT_TRUE(p.LoadFile(f.path(), &err)) << err;
    EXPECT_EQ(p.GetEntryCount(), 0u);
}

TEST(TestExpectations, ExactMatchBeatsGlob) {
    ScopedTempFile f(std::string(kStdHeader) +
                     "crbug.com/1 [ vulkan ] Suite.A/foo [ Skip ]\n"
                     "crbug.com/2 [ vulkan ] Suite.A/* [ Failure ]\n");
    TestExpectationParser p;
    std::string err;
    ASSERT_TRUE(p.LoadFile(f.path(), &err)) << err;
    auto cond = MakeConditions({"vulkan"});
    auto r = p.GetExpectation("Suite.A/foo", cond);
    EXPECT_TRUE(Contains(r, TestResult::Skip));
    EXPECT_FALSE(Contains(r, TestResult::Failure));
}

TEST(TestExpectations, LongestGlobWinsUnionOnTies) {
    ScopedTempFile f(std::string(kStdHeader) +
                     "crbug.com/1 [ vulkan ] Suite.* [ Skip ]\n"
                     "crbug.com/2 [ vulkan ] Suite.A/* [ Failure ]\n"
                     "crbug.com/3 [ vulkan ] Suite.A/* [ Slow ]\n");
    TestExpectationParser p;
    std::string err;
    ASSERT_TRUE(p.LoadFile(f.path(), &err)) << err;
    auto cond = MakeConditions({"vulkan"});
    auto r = p.GetExpectation("Suite.A/bar", cond);
    EXPECT_FALSE(Contains(r, TestResult::Skip));
    EXPECT_TRUE(Contains(r, TestResult::Failure));
    EXPECT_TRUE(Contains(r, TestResult::Slow));
}

TEST(TestExpectations, TagsAreAndCombined) {
    ScopedTempFile f(std::string(kStdHeader) + "crbug.com/1 [ vulkan nvidia ] Suite.* [ Skip ]\n");
    TestExpectationParser p;
    std::string err;
    ASSERT_TRUE(p.LoadFile(f.path(), &err)) << err;
    EXPECT_NE(p.GetExpectation("Suite.X", MakeConditions({"vulkan", "nvidia"})), 0u);
    EXPECT_EQ(p.GetExpectation("Suite.X", MakeConditions({"vulkan"})), 0u);
    EXPECT_EQ(p.GetExpectation("Suite.X", MakeConditions({"nvidia"})), 0u);
}

TEST(TestExpectations, EmptyTagBlockMatchesAll) {
    ScopedTempFile f(std::string(kStdHeader) + "crbug.com/1 [ ] Suite.X [ Failure ]\n");
    TestExpectationParser p;
    std::string err;
    ASSERT_TRUE(p.LoadFile(f.path(), &err)) << err;
    EXPECT_TRUE(
        Contains(p.GetExpectation("Suite.X", MakeConditions({"vulkan"})), TestResult::Failure));
    EXPECT_TRUE(Contains(p.GetExpectation("Suite.X", TestConditions{}), TestResult::Failure));
}

TEST(TestExpectations, EscapedAsteriskIsLiteral) {
    ScopedTempFile f(std::string(kStdHeader) + "crbug.com/1 [ ] Suite.Foo\\* [ Failure ]\n");
    TestExpectationParser p;
    std::string err;
    ASSERT_TRUE(p.LoadFile(f.path(), &err)) << err;
    // The literal asterisk should match exactly "Suite.Foo*", not "Suite.FooBar".
    EXPECT_TRUE(Contains(p.GetExpectation("Suite.Foo*", TestConditions{}), TestResult::Failure));
    EXPECT_EQ(p.GetExpectation("Suite.FooBar", TestConditions{}), 0u);
}

TEST(TestExpectations, MultipleAsterisksMatchAnywhere) {
    ScopedTempFile f(std::string(kStdHeader) + "crbug.com/1 [ ] *.A/*Vulkan* [ Failure ]\n");
    TestExpectationParser p;
    std::string err;
    ASSERT_TRUE(p.LoadFile(f.path(), &err)) << err;
    EXPECT_TRUE(Contains(p.GetExpectation("Suite.A/SomeTest_Vulkan_AMD", TestConditions{}),
                         TestResult::Failure));
    EXPECT_EQ(p.GetExpectation("Suite.B/foo", TestConditions{}), 0u);
}

TEST(TestExpectations, BadLineIsNonFatal) {
    // First line has an unknown result; second line is valid. Loading should still succeed
    // (no fatal error) and the good entry should be present.
    ScopedTempFile f(std::string(kStdHeader) +
                     "crbug.com/1 [ vulkan ] Suite.Bad [ Nonsense ]\n"
                     "crbug.com/2 [ vulkan ] Suite.Good [ Failure ]\n");
    TestExpectationParser p;
    std::string err;
    ASSERT_TRUE(p.LoadFile(f.path(), &err)) << err;
    EXPECT_FALSE(err.empty());
    EXPECT_TRUE(
        Contains(p.GetExpectation("Suite.Good", MakeConditions({"vulkan"})), TestResult::Failure));
    EXPECT_EQ(p.GetExpectation("Suite.Bad", MakeConditions({"vulkan"})), 0u);
}

TEST(TestExpectations, CommentsAreStripped) {
    ScopedTempFile f(std::string(kStdHeader) +
                     "# whole-line comment\n"
                     "crbug.com/1 [ vulkan ] Suite.X [ Failure ]  # trailing comment\n");
    TestExpectationParser p;
    std::string err;
    ASSERT_TRUE(p.LoadFile(f.path(), &err)) << err;
    EXPECT_TRUE(
        Contains(p.GetExpectation("Suite.X", MakeConditions({"vulkan"})), TestResult::Failure));
}

TEST(TestExpectations, HashInsideTestNameNotStripped) {
    // The parser strips `#` only at start-of-line or after whitespace, so a `#` inside a
    // glob like `Foo#bar` should remain part of the test pattern.
    ScopedTempFile f(std::string(kStdHeader) + "crbug.com/1 [ ] Suite.Foo#bar/* [ Failure ]\n");
    TestExpectationParser p;
    std::string err;
    ASSERT_TRUE(p.LoadFile(f.path(), &err)) << err;
    EXPECT_TRUE(
        Contains(p.GetExpectation("Suite.Foo#bar/abc", TestConditions{}), TestResult::Failure));
}

TEST(TestExpectations, BugIdPrefixesAllRecognized) {
    ScopedTempFile f(std::string(kStdHeader) +
                     "crbug.com/1 [ ] Suite.A [ Failure ]\n"
                     "b/2 [ ] Suite.B [ Failure ]\n"
                     "anglebug.com/3 [ ] Suite.C [ Failure ]\n"
                     "dawn:4 [ ] Suite.D [ Failure ]\n"
                     "skbug.com/5 [ ] Suite.E [ Failure ]\n"
                     "webgpu.org/6 [ ] Suite.F [ Failure ]\n");
    TestExpectationParser p;
    std::string err;
    ASSERT_TRUE(p.LoadFile(f.path(), &err)) << err;
    for (const std::string& name :
         {"Suite.A", "Suite.B", "Suite.C", "Suite.D", "Suite.E", "Suite.F"}) {
        std::vector<std::string> bugs;
        auto r = p.GetExpectation(name, TestConditions{}, &bugs);
        EXPECT_TRUE(Contains(r, TestResult::Failure)) << name;
        ASSERT_EQ(bugs.size(), 1u) << name;
        EXPECT_FALSE(bugs[0].empty()) << name;
    }
}

TEST(TestExpectations, PlaceholderBugDetected) {
    ScopedTempFile f(std::string(kStdHeader) +
                     "crbug.com/0 [ ] Suite.A [ Failure ]\n"
                     "crbug.com/123 [ ] Suite.B [ Failure ]\n");
    TestExpectationParser p;
    std::string err;
    ASSERT_TRUE(p.LoadFile(f.path(), &err)) << err;
    auto placeholders = p.GetPlaceholderBugMessages();
    ASSERT_EQ(placeholders.size(), 1u);
    EXPECT_NE(placeholders[0].find("Suite.A"), std::string::npos);
}

TEST(TestExpectations, UnionAcrossExactMatches) {
    // Multiple exact-match lines for the same name with different tag sets should union
    // their results when both tag sets match.
    ScopedTempFile f(std::string(kStdHeader) +
                     "crbug.com/1 [ vulkan ] Suite.X [ Skip ]\n"
                     "crbug.com/2 [ nvidia ] Suite.X [ Failure ]\n");
    TestExpectationParser p;
    std::string err;
    ASSERT_TRUE(p.LoadFile(f.path(), &err)) << err;
    auto r = p.GetExpectation("Suite.X", MakeConditions({"vulkan", "nvidia"}));
    EXPECT_TRUE(Contains(r, TestResult::Skip));
    EXPECT_TRUE(Contains(r, TestResult::Failure));
}

TEST(TestExpectations, UnusedExpectationReported) {
    ScopedTempFile f(std::string(kStdHeader) +
                     "crbug.com/1 [ vulkan ] Suite.Unused [ Failure ]\n"
                     "crbug.com/2 [ vulkan ] Suite.Used [ Failure ]\n");
    TestExpectationParser p;
    std::string err;
    ASSERT_TRUE(p.LoadFile(f.path(), &err)) << err;
    (void)p.GetExpectation("Suite.Used", MakeConditions({"vulkan"}));
    auto unused = p.GetUnusedExpectationsMessages();
    ASSERT_EQ(unused.size(), 1u);
    EXPECT_NE(unused[0].find("Suite.Unused"), std::string::npos);
}

TEST(TestExpectations, MissingFileIsFatal) {
    TestExpectationParser p;
    std::string err;
    EXPECT_FALSE(p.LoadFile("/nonexistent/path/that/does/not/exist.txt", &err));
    EXPECT_FALSE(err.empty());
}

}  // namespace
}  // namespace dawn
