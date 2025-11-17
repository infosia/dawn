# Testing Dawn

This page points to the main test harnesses and shows quick patterns to get productive.

## End-to-end and unit tests

- Base classes and macros live in `src/dawn/tests/DawnTest.h`:
  - `using DawnTest = DawnTestWithParams<>;`
  - Expectations: `EXPECT_BUFFER_*`, `EXPECT_PIXEL_*`, `EXPECT_TEXTURE_*`
  - Skips/suppression: `DAWN_TEST_UNSUPPORTED_IF`, `DAWN_SUPPRESS_TEST_IF`
  - Instantiation helpers: `DAWN_INSTANTIATE_TEST(...)`

Minimal example:

// ...existing code...
// class MyFeatureTests : public DawnTest { /* SetUp uses DawnTestBase */ };
// TEST_P(MyFeatureTests, DoesThing) { EXPECT_TRUE(device); }
// DAWN_INSTANTIATE_TEST(MyFeatureTests, D3D12Backend(), VulkanBackend(), MetalBackend())

Run on Windows:
- `out/active/dawn_unittests.exe --gtest_filter=MyFeatureTests.*`
- `out/active/dawn_end2end_tests.exe --gtest_filter=MyFeatureTests.*`

## Perf tests

See `./perf_memory.md` and repo docs for perf test harness details.
