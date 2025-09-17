package androidx.webgpu

@JvmInline
public value class QueryType(public val value: Int) {
    public companion object {
        public val Occlusion: QueryType = QueryType(0x00000001)
        public val Timestamp: QueryType = QueryType(0x00000002)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "Occlusion",
            0x00000002 to "Timestamp",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
