package {{ kotlin_package }}

public class GPUHardwareBufferTexture(
    handle: Long,
    public val texture: GPUTexture,
) : GPUHardwareBufferWrapper(handle) {

    override fun close() {
        texture.close()
        super.close()
    }
}