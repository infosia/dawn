# WebGPU Android Zero-Copy Video Demos

This directory contains demos demonstrating zero-copy video playback using WebGPU on Android. Zero-copy means that video frames are passed from the video decoder to the GPU without copying pixel data on the CPU, maximizing performance and battery life.

## Demos

### 1. Zero-Copy Video (YUV)

**File:** `src/main/java/com/webgpu/demo/ZeroCopyVideoYuv.kt`

This demo renders video frames directly from YUV `HardwareBuffer`s using WebGPU's External Texture feature.

- **Minimum API Level:** Android Q (API 29)
- **Approach:** Direct sampling of YUV data in WebGPU.
- **Workflow:**
  1. `MediaPlayer` plays video and renders to the surface of an `ImageReader`.
  2. `ImageReader` captures frames as `Image` objects containing `HardwareBuffer`s in private format.
  3. The `HardwareBuffer` is wrapped into a `GPUHardwareBufferExternalTexture` using `GPUAndroidHardwareBufferUtil.createExternalTexture`.
  4. A WebGPU `GPUExternalTexture` is bound to the pipeline.
  5. The fragment shader samples directly from the external texture. The YUV-to-RGB conversion is handled automatically by the driver/WebGPU implementation based on specified color space parameters.
- **Shaders:**
  - Vertex: `cameraInput/vert.wgsl`
  - Fragment (Normal): `cameraInput/frag_yuv.wgsl`
  - Fragment (Mosaic Filter): `cameraInput/frag_yuv_mosaic.wgsl`

### 2. Zero-Copy Video (RGB)

**File:** `src/main/java/com/webgpu/demo/ZeroCopyRGBVideo.kt`

This demo converts YUV video frames to RGB using Android's `HardwareBufferRenderer` before passing the RGB `HardwareBuffer` to WebGPU.

- **Minimum API Level:** Android 14 (API 34)
- **Approach:** Hardware-accelerated blit to RGB followed by standard RGB sampling in WebGPU.
- **Workflow:**
  1. `MediaPlayer` plays video and renders to the surface of an `ImageReader`.
  2. `ImageReader` captures frames as `Image` objects containing `HardwareBuffer`s.
  3. `HardwareBufferRenderer` is used to draw the YUV buffer into an RGB `HardwareBuffer` (`rgbOutputAhb`) via a `RenderNode` (blit operation).
  4. The RGB `HardwareBuffer` is wrapped into a `GPUHardwareBufferTexture` using `GPUAndroidHardwareBufferUtil.createTexture`.
  5. A standard WebGPU `GPUTexture` is bound to the pipeline.
  6. The shader samples from the RGB texture.
- **Shaders:**
  - Vertex: `cameraInput/vert.wgsl`
  - Fragment: `cameraInput/frag.wgsl`
  - Compute (Mosaic Filter): `cameraInput/mosaic.wgsl` (Applied before rendering if enabled)

## Key APIs Used

- **Android Media & Hardware:**
  - `android.media.MediaPlayer`: For video playback.
  - `android.media.ImageReader`: For acquiring video frames.
  - `android.hardware.HardwareBuffer`: The core object holding image data shared between processes/APIs.
  - `android.graphics.HardwareBufferRenderer` (API 34+): Used in the RGB demo for conversion.
- **WebGPU Android Bindings:**
  - `GPUAndroidHardwareBufferUtil`: Helper utility to create WebGPU textures from Android `HardwareBuffer`s.
  - `GPUExternalTexture`: Used in mapping YUV buffers directly.
  - `GPUHardwareBufferTexture` / `GPUHardwareBufferExternalTexture`: Wrappers handling buffer access synchronization.

## Performance Metrics

Both demos display a UI with real-time stats and a graph showing CPU overhead:
- **FPS:** Frames Per Second.
- **Render Loop Time:** Total time spent in the render loop.
- **CPU Overhead:** Time spent on CPU setup and submission for WebGPU. This metric isolates the overhead of managing the shared buffers versus actual GPU rendering work.

The graph helps visualize spikes in CPU overhead, which are critical to minimize for smooth video playback.
