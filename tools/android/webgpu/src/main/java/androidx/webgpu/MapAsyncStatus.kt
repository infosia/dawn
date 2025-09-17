package androidx.webgpu

@JvmInline
public value class MapAsyncStatus(public val value: Int) {
    public companion object {
        public val Success: MapAsyncStatus = MapAsyncStatus(0x00000001)
        public val CallbackCancelled: MapAsyncStatus = MapAsyncStatus(0x00000002)
        public val Error: MapAsyncStatus = MapAsyncStatus(0x00000003)
        public val Aborted: MapAsyncStatus = MapAsyncStatus(0x00000004)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "Success",
            0x00000002 to "CallbackCancelled",
            0x00000003 to "Error",
            0x00000004 to "Aborted",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
