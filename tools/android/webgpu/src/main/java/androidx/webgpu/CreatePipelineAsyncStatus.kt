package androidx.webgpu

@JvmInline
public value class CreatePipelineAsyncStatus(public val value: Int) {
    public companion object {
        public val Success: CreatePipelineAsyncStatus = CreatePipelineAsyncStatus(0x00000001)
        public val CallbackCancelled: CreatePipelineAsyncStatus = CreatePipelineAsyncStatus(0x00000002)
        public val ValidationError: CreatePipelineAsyncStatus = CreatePipelineAsyncStatus(0x00000003)
        public val InternalError: CreatePipelineAsyncStatus = CreatePipelineAsyncStatus(0x00000004)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "Success",
            0x00000002 to "CallbackCancelled",
            0x00000003 to "ValidationError",
            0x00000004 to "InternalError",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
