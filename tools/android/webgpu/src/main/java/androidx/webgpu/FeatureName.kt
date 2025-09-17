package androidx.webgpu

@JvmInline
public value class FeatureName(public val value: Int) {
    public companion object {
        public val CoreFeaturesAndLimits: FeatureName = FeatureName(0x00000001)
        public val DepthClipControl: FeatureName = FeatureName(0x00000002)
        public val Depth32FloatStencil8: FeatureName = FeatureName(0x00000003)
        public val TextureCompressionBC: FeatureName = FeatureName(0x00000004)
        public val TextureCompressionBCSliced3D: FeatureName = FeatureName(0x00000005)
        public val TextureCompressionETC2: FeatureName = FeatureName(0x00000006)
        public val TextureCompressionASTC: FeatureName = FeatureName(0x00000007)
        public val TextureCompressionASTCSliced3D: FeatureName = FeatureName(0x00000008)
        public val TimestampQuery: FeatureName = FeatureName(0x00000009)
        public val IndirectFirstInstance: FeatureName = FeatureName(0x0000000a)
        public val ShaderF16: FeatureName = FeatureName(0x0000000b)
        public val RG11B10UfloatRenderable: FeatureName = FeatureName(0x0000000c)
        public val BGRA8UnormStorage: FeatureName = FeatureName(0x0000000d)
        public val Float32Filterable: FeatureName = FeatureName(0x0000000e)
        public val Float32Blendable: FeatureName = FeatureName(0x0000000f)
        public val ClipDistances: FeatureName = FeatureName(0x00000010)
        public val DualSourceBlending: FeatureName = FeatureName(0x00000011)
        public val Subgroups: FeatureName = FeatureName(0x00000012)
        public val TextureFormatsTier1: FeatureName = FeatureName(0x00000013)
        public val TextureFormatsTier2: FeatureName = FeatureName(0x00000014)
        public val PrimitiveIndex: FeatureName = FeatureName(0x00000015)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "CoreFeaturesAndLimits",
            0x00000002 to "DepthClipControl",
            0x00000003 to "Depth32FloatStencil8",
            0x00000004 to "TextureCompressionBC",
            0x00000005 to "TextureCompressionBCSliced3D",
            0x00000006 to "TextureCompressionETC2",
            0x00000007 to "TextureCompressionASTC",
            0x00000008 to "TextureCompressionASTCSliced3D",
            0x00000009 to "TimestampQuery",
            0x0000000a to "IndirectFirstInstance",
            0x0000000b to "ShaderF16",
            0x0000000c to "RG11B10UfloatRenderable",
            0x0000000d to "BGRA8UnormStorage",
            0x0000000e to "Float32Filterable",
            0x0000000f to "Float32Blendable",
            0x00000010 to "ClipDistances",
            0x00000011 to "DualSourceBlending",
            0x00000012 to "Subgroups",
            0x00000013 to "TextureFormatsTier1",
            0x00000014 to "TextureFormatsTier2",
            0x00000015 to "PrimitiveIndex",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
