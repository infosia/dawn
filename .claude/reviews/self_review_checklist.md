# Self-Review Checklist
Distilled from dawn project reviews on 2026-04-15

1. Does every behavior change described in the CL description have corresponding test coverage (both success and error paths)?
2. Are test setups minimal — no extra bind groups, uniform buffers, or shader modules beyond what's needed to exercise the specific behavior?
3. Does every non-obvious test case have a comment explaining what it tests and why?
4. When a workaround or TODO is needed, is there a tracking bug filed and referenced in the comment?
5. Are existing utility helpers (e.g. `utils::MakePipelineLayout`) used or extended, rather than duplicating setup inline?
6. Are existing framework mechanisms (e.g. `DelayedInvalidIf`, `DAWN_SUPPRESS_TEST_IF`) used instead of inventing custom alternatives?
7. Do variable and type names encode their semantic distinctions (e.g. `WithPadding` vs `NoPadding`), rather than relying on context?
8. Are return types self-documenting — named structs instead of `std::pair<T, T>` when the two fields have different meanings?
9. When multiple pipelines share a shader, is a single module reused with different entry points rather than duplicated?
10. Is each intermediate member variable actually needed beyond initialization, or can it be computed inline?
11. When adding VVL-related tests, has `--enable-backend-validation` been verified to catch the issue?
12. Does the CL avoid modifying `src/dawn/native/d3d/` without considering the impact on both D3D11 and D3D12 backends?
