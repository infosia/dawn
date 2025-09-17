package androidx.webgpu

@JvmInline
public value class WGSLLanguageFeatureName(public val value: Int) {
    public companion object {
        public val ReadonlyAndReadwriteStorageTextures: WGSLLanguageFeatureName = WGSLLanguageFeatureName(0x00000001)
        public val Packed4x8IntegerDotProduct: WGSLLanguageFeatureName = WGSLLanguageFeatureName(0x00000002)
        public val UnrestrictedPointerParameters: WGSLLanguageFeatureName = WGSLLanguageFeatureName(0x00000003)
        public val PointerCompositeAccess: WGSLLanguageFeatureName = WGSLLanguageFeatureName(0x00000004)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "ReadonlyAndReadwriteStorageTextures",
            0x00000002 to "Packed4x8IntegerDotProduct",
            0x00000003 to "UnrestrictedPointerParameters",
            0x00000004 to "PointerCompositeAccess",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
