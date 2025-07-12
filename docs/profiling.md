# Profiling

General profiling tips:

- CMake builds should use `Release` or `MinSizeRel` builds which enable CMake's
  default LTO (`-flto=thin` in Clang).
- GN builds should use `use_thin_lto=true`
  (or perhaps `is_official_build=true chrome_pgo_phase=0`).

For profiling individual subprojects, check their profiling docs if any
(e.g. [tint/profiling.md](tint/profiling.md)).
