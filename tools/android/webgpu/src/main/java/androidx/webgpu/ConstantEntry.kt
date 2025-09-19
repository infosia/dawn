package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class ConstantEntry(
    public var key: String,
    /**
     * Represents a WGSL numeric or boolean value using @ref DoubleAsSupertype.  If
     * non-finite, produces a @ref NonFiniteFloatValueError.
     */
    public var value: Double,
)
