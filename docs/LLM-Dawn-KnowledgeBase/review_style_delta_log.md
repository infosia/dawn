# Reviewer style calibration: Before vs After

Track concrete deltas inspired by sampled reviews (cwallez@chromium.org). Fill with short code excerpts or pseudo.

## 1) Validation factoring
- Before: inline checks repeated across call sites; mixed assertions and errors.
- After: `ValidateFoo(...)` helper + `DAWN_TRY(...)` at the call site; assertions only for invariants.

## 2) Spec-aligned naming
- Before: legacy builder names, ambiguous descriptor naming.
- After: `*Descriptor`/`*State` consistent with spec; consistent `entryPoint`, `ShaderModule*` patterns.

## 3) Tests migrated
- Before: builder-era patterns relying on missing properties.
- After: modern descriptors; explicit success/failure tests; wire expectations when relevant.
