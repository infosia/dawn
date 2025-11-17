# Quick reviewer checklist

- API & spec
  - Matches WebGPU spec; feature/limit gating correct
  - Clear errors with DAWN_INVALID_IF and context
- Validation
  - Descriptor and usage validation in right layer; helpers reused
- Backends
  - Consistent translation across D3D12/Metal/Vulkan; deltas justified
  - Resource state transitions/barriers correct
- Performance
  - Cache hits encouraged; unnecessary work avoided; no accidental O(N^2)
- Testing
  - Unit + end2end added/updated; CTS coverage noted
- Docs & toggles
  - Debug toggles/doc updated if behavior changes
