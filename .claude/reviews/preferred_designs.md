# Preferred Designs
Distilled from dawn project reviews on 2026-04-15

- **Leverage Tint transforms for backend-specific data layout** — when both a native approach (tracking two sizes) and a Tint IR transform approach exist, prefer the Tint transform to decompose data uniformly across backends, avoiding per-backend size divergence
- **Keep dirty-state tracking co-located with the mutation** — when state changes cause invalidation (e.g. resource table rebinding after push constant range changes), place the dirtying logic next to the mutation site, not in a separate block that could drift
- **Unconditional initialization over conditional guards** — when a struct field has a zero default, set it unconditionally rather than wrapping it in an if-check; the unconditional path is clearer and less error-prone
- **Factor shared shader modules across pipeline variants** — when two pipelines share the same vertex shader, use one module and differentiate via entry point names rather than duplicating shader source
- **Simplify test assertions by returning expected values directly** — if a shader can directly output the expected value (e.g. from a bind group), prefer that over computing expected values with math in the test harness
- **Use code suggestions with `auto` and clear parameter names** — reviewers prefer concise lambda signatures (e.g. `auto MakePipeline = [&](wgpu::ShaderModule mod, bool useDepthWrite)`) that name parameters meaningfully
- **Reference existing parallel code for test patterns** — when a new test class mirrors an existing one (e.g. resource table tests), copy the established setup pattern (suppress conditions, required features) for consistency
