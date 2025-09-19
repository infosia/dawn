package androidx.webgpu

@JvmInline
public value class StorageTextureAccess(public val value: Int) {
    public companion object {
        /**
         * Indicates that this @ref WGPUStorageTextureBindingLayout member of its parent
         * @ref WGPUBindGroupLayoutEntry is not used. (See also @ref SentinelValues.)
         */
        public val BindingNotUsed: StorageTextureAccess = StorageTextureAccess(0x00000000)
        /**
         * `1`. Indicates no value is passed for this argument. See @ref SentinelValues.
         */
        public val Undefined: StorageTextureAccess = StorageTextureAccess(0x00000001)
        public val WriteOnly: StorageTextureAccess = StorageTextureAccess(0x00000002)
        public val ReadOnly: StorageTextureAccess = StorageTextureAccess(0x00000003)
        public val ReadWrite: StorageTextureAccess = StorageTextureAccess(0x00000004)
        internal val names: Map<Int, String> = mapOf(
            0x00000000 to "BindingNotUsed",
            0x00000001 to "Undefined",
            0x00000002 to "WriteOnly",
            0x00000003 to "ReadOnly",
            0x00000004 to "ReadWrite",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
