package androidx.webgpu

@JvmInline
public value class QueueWorkDoneStatus(public val value: Int) {
    public companion object {
        public val Success: QueueWorkDoneStatus = QueueWorkDoneStatus(0x00000001)
        public val CallbackCancelled: QueueWorkDoneStatus = QueueWorkDoneStatus(0x00000002)
        /**
         * There was some deterministic error. (Note this is currently never used, but it
         * will be relevant when it's possible to create a queue object.)
         */
        public val Error: QueueWorkDoneStatus = QueueWorkDoneStatus(0x00000003)
        internal val names: Map<Int, String> = mapOf(
            0x00000001 to "Success",
            0x00000002 to "CallbackCancelled",
            0x00000003 to "Error",
        )
    }
    override fun toString(): String = names[value]?:value.toString()
}
