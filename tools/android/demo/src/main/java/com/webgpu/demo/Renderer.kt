package com.webgpu.demo

import java.io.Closeable

// Each client of GraphicsSurface returns a Renderer to handling rendering per-frame and to handle
// closing.
abstract class Renderer : Closeable {
    open suspend fun render(frameTimeNanos: Long) {}
}
