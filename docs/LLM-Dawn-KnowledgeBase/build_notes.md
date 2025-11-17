# Build notes (Windows)

- Sync: git fetch/rebase + gclient sync successful on origin/main.
- Toolchain: GN 2234; Ninja; Python 3.12.10; Go 1.23.4; Node v22.12.0; npm 11.4.2.
- Config: GN debug, use_siso=false. Output dir: out/active.
- Generation: gn gen out/active --add-export-compile-commands=//* --args="is_debug=true use_siso=false".
- Build: ninja -C out/active (linked tint_unittests.exe, dawn_unittests.exe, dawn_end2end_tests.exe).
- Notes: Bash scripts (tools/setup-build, tools/run) require CRLF stripping or Git Bash/WSL with LF.
