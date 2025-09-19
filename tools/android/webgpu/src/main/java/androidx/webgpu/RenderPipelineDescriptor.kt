package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class RenderPipelineDescriptor(
    public var label: String? = null,
    public var layout: PipelineLayout? = null,
    public var vertex: VertexState,
    public var primitive: PrimitiveState = PrimitiveState(),
    public var depthStencil: DepthStencilState? = null,
    public var multisample: MultisampleState = MultisampleState(),
    public var fragment: FragmentState? = null,
)
