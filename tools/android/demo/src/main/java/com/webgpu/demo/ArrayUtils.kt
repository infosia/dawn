package com.webgpu.demo

import java.nio.ByteBuffer
import java.nio.ByteOrder

fun FloatArray.toByteBuffer(): ByteBuffer =
    ByteBuffer.allocateDirect(size * Float.SIZE_BYTES).order(ByteOrder.LITTLE_ENDIAN)
        .apply { asFloatBuffer().put(this@toByteBuffer) }

fun ByteArray.toByteBuffer(): ByteBuffer =
    ByteBuffer.allocateDirect(size).also { it.put(this) }

