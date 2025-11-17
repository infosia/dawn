# How to test Dawn

- Build tests
  - GN: `gn gen out/active --args="is_debug=true"`; `ninja -C out/active`
- Unit tests
  - Run: `out/active/dawn_unittests.exe` (supports `--gtest_filter=...`, `--enable-toggles=...`)
- End2End tests
  - Run: `out/active/dawn_end2end_tests.exe` (supports `--gtest_filter=...`, `--use-wire`)
- Debug toggles
  - Example: `--enable-toggles=dump_shaders,disable_symbol_renaming`
- CTS (WebGPU Conformance)
  - Requires Node/npm and Go; see `docs/rolling_cts.md`.
  - Runner supports subcommands `node` and `chrome`. For a tiny slice on Windows PowerShell:
    - `go run .\tools\src\cmd\run-cts node -j 1 --bin .\out\active --skip-vs-code-info "webgpu:api,operation,command_buffer,copyBufferToBuffer:single:*"`
  - Tip: list queries from CTS metadata if unsure; avoid unsupported flags like `--limit`.
