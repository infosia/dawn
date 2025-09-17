package androidx.webgpu

public class ShaderModuleDescriptor(
    public var label: String? = null,
    public var shaderSourceSPIRV: ShaderSourceSPIRV? = null,
    public var shaderSourceWGSL: ShaderSourceWGSL? = null,
)
