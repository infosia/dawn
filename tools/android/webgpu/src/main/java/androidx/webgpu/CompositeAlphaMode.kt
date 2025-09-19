package androidx.webgpu

@JvmInline
public value class CompositeAlphaMode(public val value: Int) {
    public companion object {
        /**
         * Lets the WebGPU implementation choose the best mode (supported, and with the
         * best performance) between @see CompositeAlphaMode.Opaque or @see
         * CompositeAlphaMode.Inherit .
         */
        public val Auto: CompositeAlphaMode = CompositeAlphaMode(0x00000000)
        /**
         * The alpha component of the image is ignored and teated as if it is always 1.0.
         */
        public val Opaque: CompositeAlphaMode = CompositeAlphaMode(0x00000001)
        /**
         * The alpha component is respected and non-alpha components are assumed to be
         * already multiplied with the alpha component. For example, (0.5, 0, 0, 0.5) is
         * semi-transparent bright red.
         */
        public val Premultiplied: CompositeAlphaMode = CompositeAlphaMode(0x00000002)
        /**
         * The alpha component is respected and non-alpha components are assumed to NOT be
         * already multiplied with the alpha component. For example, (1.0, 0, 0, 0.5) is
         * semi-transparent bright red.
         */
        public val Unpremultiplied: CompositeAlphaMode = CompositeAlphaMode(0x00000003)
        /**
         * The handling of the alpha component is unknown to WebGPU and should be handled
         * by the application using system-specific APIs. This mode may be unavailable (for
         * example on Wasm).
         */
        public val Inherit: CompositeAlphaMode = CompositeAlphaMode(0x00000004)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "Auto",
            0x00000001 to "Opaque",
            0x00000002 to "Premultiplied",
            0x00000003 to "Unpremultiplied",
            0x00000004 to "Inherit",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
