package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class ShaderModuleDescriptor(
    public var label: String? = null,
    public var shaderSourceSPIRV: ShaderSourceSPIRV? = null,
    public var shaderSourceWGSL: ShaderSourceWGSL? = null,
)
