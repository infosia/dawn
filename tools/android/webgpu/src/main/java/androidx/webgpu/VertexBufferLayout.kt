package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

/**
 * If `attributes` is empty *and* `stepMode` is @see VertexStepMode.Undefined,
 * indicates a "hole" in the parent @see VertexState `buffers` array, with behavior
 * equivalent to `null` in the JS API.  If `attributes` is empty but `stepMode` is
 * *not* @see VertexStepMode.Undefined, indicates a vertex buffer with no
 * attributes, with behavior equivalent to `{ attributes: [] }` in the JS API. (:
 * If the JS API changes not to distinguish these cases, then this distinction
 * doesn't matter and we can remove this documentation.)  If `stepMode` is @see
 * VertexStepMode.Undefined but `attributes` is *not* empty, `stepMode` defaults to
 * @see VertexStepMode.Vertex.
 */
public class VertexBufferLayout(
    @get:JvmName("getStepMode")
    public var stepMode: VertexStepMode = VertexStepMode.Undefined,
    public var arrayStride: Long,
    public var attributes: Array<VertexAttribute> = arrayOf(),
)
