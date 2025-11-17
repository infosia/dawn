# Minimal test log

- tint_unittests.exe --gtest_filter=WGSLParserTest.*:NumberSuffixTest.* --gtest_brief=1
  - Result: 567 passed (56 ms)
- dawn_unittests.exe --gtest_filter=*Adapter* --gtest_brief=1
  - Result: 54 passed, 1 skipped (~151 ms)
- Notes: End2End small subset can be run similarly; CTS tooling available via tools/src/cmd/run-cts.

## CTS slice (node runner) — command_buffer copyBufferToBuffer:single

- When: 2025-08-29
- Env: Windows, Node v22.12.0, npm 11.4.2, Go 1.23.4
- Prep:
  - Enabled GN arg `dawn_build_node_bindings=true` and regenerated build files.
  - Synced submodules: `third_party/gpuweb`, `third_party/node-addon-api`, `third_party/node-api-headers`.
  - Built `dawn.node` and copied `cts.js` into `out/active`.
  - Ran `npm ci` in `third_party/webgpu-cts`.
- Run:
  - go run tools/src/cmd/run-cts node -j 1 --bin .\out\active --skip-vs-code-info "webgpu:api,operation,command_buffer,copyBufferToBuffer:single:*"
- Result: PASS 2/2 in ~4.25s
