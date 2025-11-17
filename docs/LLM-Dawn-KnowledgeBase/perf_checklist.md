# Performance checklist

- Pipelines & caching
  - Reuse PipelineLayout / render bundles; avoid thrash; ensure caches keyed correctly.
  - See also: `docs/dawn/perf_memory.md` for cache keys, BlobCache, and isolation keys.
- Resource lifetime
  - Avoid unnecessary transitions; batch barriers; correct usage flags.
- CPU overhead
  - Minimize validation in hot loops; precompute where possible.
- Memory
  - Watch transient allocations; alignments; map/unmap patterns; avoid O(N^2).
- Diagnostics
  - Perf tests; traces; timing toggles; CTS perf shards where applicable.
