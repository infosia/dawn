# Design: WebGPU Backend Capture and Replay - Multi-frame and Back Buffer Support

## Overview
This document describes the design for enhancing the WebGPU backend's capture and replay capabilities to support multi-frame command streams and identify surface back buffer textures.

## 1. Identifying the Back Buffer Texture
To replay a captured stream visually or to extract frame-by-frame images, the replayer needs to know which texture represents the "output" that was intended for display.

### Strategy
- **SharedTextureMemory (STM)**: In Chromium, textures used for the WebGPU canvas are imported via `SharedTextureMemory`. Any texture created through `SharedTextureMemory::CreateTexture` is a back buffer.
- **Surface**: In native Dawn environments, the texture returned by `wgpu::Surface::GetCurrentTexture()` is the back buffer.

### Implementation
- **Schema Update**: Add a boolean `isBackBuffer` to `schema::Texture`.
- **Capture**:
  - In `src/dawn/native/webgpu/TextureWGPU.cpp`, the constructor for textures created from `SharedTextureMemory` will set `isBackBuffer = true` in the captured creation parameters.
  - When `Surface` support is added to `src/dawn/native/webgpu`, `GetCurrentTexture` will similarly mark its textures.
- **Replay**: The replayer (`src/dawn/replay/Replay.cpp`) can use this flag to identify which textures should be presented to a window or captured as frames.

## 2. Marking Frame Boundaries (Multi-frame Support)
A captured stream may contain multiple frames. We need explicit markers in the command stream to denote frame boundaries.

### Frame Marker Command
Add a new `RootCommand` called `Present` (or `FrameEnd`).

### Native Dawn Environment
- The signal for frame completion is `wgpu::Surface::Present()` or `wgpu::SwapChain::Present()`.
- The WebGPU backend will intercept these calls and serialize a `RootCommandPresent` into the command stream.

### Chromium / SharedTextureMemory Environment
- Chromium's WebGPU canvas uses `beginAccess` and `endAccess` to wrap the usage of a back buffer texture for a single frame.
- `EndAccess` on a texture marked as `isBackBuffer` effectively serves as the `Present` call.

## 3. Capturing SharedTextureMemory Access
To support frame marking in Chromium and to correctly replay workloads that depend on external synchronization, `beginAccess` and `endAccess` must be captured.

### Schema Updates
Add the following commands to `schema::RootCommand`:
- `SharedTextureMemoryBeginAccess`: Captures `textureId`, `concurrentRead`, and `initialized` state.
- `SharedTextureMemoryEndAccess`: Captures `textureId` and the updated `initialized` state.

### Implementation in WebGPU Backend
- Update `SharedTextureMemory::BeginAccessImpl` and `SharedTextureMemory::EndAccessImpl` in `src/dawn/native/webgpu/SharedTextureMemoryWGPU.cpp`.
- These methods will:
  1. Retrieve the `Queue` from the `Device`.
  2. Check if the queue `IsCapturing()`.
  3. Use the `CaptureContext` to serialize the corresponding `BeginAccess` or `EndAccess` command.

## 4. Replay Logic
The replayer needs to handle these new commands to support multi-frame playback.

- **Frame Advancement**: Add a `Replay::PlayFrame()` method that executes commands until the next `Present` or `EndAccess` (on a back buffer) is encountered.
- **Visual Replay**: When replaying `Present` or `EndAccess(backBuffer)`, the replayer can:
  - Copy the content of the replayed texture to its own swapchain/surface for display.
  - Advance its internal frame counter.
  - Call `device.Tick()` to process asynchronous work.
- **Headless Replay**: Frame markers can be used to trigger per-frame validation or performance measurements.

## 5. Summary of Marker Usage
| Environment | Frame Start Marker | Frame End / Present Marker |
| :--- | :--- | :--- |
| **Native Dawn** | First command after previous Present | `Surface::Present()` |
| **Chromium (Canvas)** | `SharedTextureMemory::BeginAccess()` | `SharedTextureMemory::EndAccess()` |
| **Headless/Compute** | N/A | `Device::Tick()` (Optional) |

## 6. Reference: Industry Standards & Native Tools
Native graphics debugging tools use similar mechanisms to manage backbuffers and frames:

- **RenderDoc/PIX (DX12/Vulkan)**: These tools hook the `Present` API calls (e.g., `IDXGISwapChain::Present` or `vkQueuePresentKHR`). They identify backbuffers by tracking resources returned by the Swapchain factory.
- **Xcode (Metal)**: Uses `[commandBuffer presentDrawable:]`. Metal also provides an explicit `MTLCaptureScope` API that allows developers to manually define "frames" via `beginScope` and `endScope` calls, which closely mirrors our use of `BeginAccess` and `EndAccess` for `SharedTextureMemory`.
- **Headless/Compute**: In the absence of a `Present` call, tools often use `device.Tick()` or a manual "Capture" button to define a boundary.
- **Capture Triggers**: Tools typically record from the end of `Present(N)` to the start of `Present(N+1)` to capture a single frame's commands and its initial resource state.
