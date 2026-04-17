# Review Patterns
Distilled from dawn project reviews on 2026-04-15

- **Missing test coverage for stated behavior changes** — when a CL description claims a behavior change (e.g. "trailing padding no longer required"), reviewers expect at least minimal test coverage proving that claim, including both success and error cases
- **Tests are over-complicated with unnecessary resources** — tests frequently include extra bind groups, uniform buffers, or redundant shader modules that aren't needed to exercise the bug. Reviewers consistently push to simplify test setups to the minimum that reproduces the issue
- **Missing explanatory comments on non-obvious test logic** — test cases that exercise subtle backend behavior (e.g. descriptor set invalidation on push constant range change) need comments explaining *what* they test and *why*, not just the mechanics
- **Reuse existing test utilities instead of inline setup** — when utility helpers like `utils::MakePipelineLayout` already exist or can be trivially extended, reviewers prefer extending them over duplicating setup logic in individual tests
- **Prefer existing framework mechanisms over manual workarounds** — when the codebase already provides a mechanism for a pattern (e.g. `DelayedInvalidIf` for deferring validation errors to pipeline creation), use it rather than inventing a new mechanism (e.g. custom overflow bits)
- **Naming should encode semantic distinctions** — when a type or variable has two related variants (e.g. size with/without padding), the names should make the distinction explicit (e.g. `immediateDataByteSizeNoPadding/WithPadding`), not rely on context
- **Avoid confusing return types** — pairs or tuples with the same type repeated (e.g. `std::pair<uint32_t, uint32_t>`) are hard to read; prefer a named struct
- **Avoid keeping intermediate state that's only used once** — if a field is only read during initialization, compute it inline rather than storing it as a member variable
