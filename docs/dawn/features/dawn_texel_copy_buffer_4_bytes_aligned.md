# Dawn Texel Copy Buffer 4 Bytes Aligned

The `dawn-texel-copy-buffer-4-bytes-aligned` feature loosens the alignment restriction of `byetsPerRow` in `wgpu::TexelCopyBufferLayout` from 256 to 4.

Notes:
 - Even with this feature enabled, the actual alignment also needs to respect 'bytes-per-texel-block', and should be 'max(bytes-per-texel-block, 4)'.
 - The feature currently is only available on D3D11 backend.
