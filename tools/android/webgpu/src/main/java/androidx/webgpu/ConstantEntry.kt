package androidx.webgpu

public class ConstantEntry(
    public var key: String,
    /**
     * Represents a WGSL numeric or boolean value using @ref DoubleAsSupertype.  If
     * non-finite, produces a @ref NonFiniteFloatValueError.
     */
    public var value: Double,
)
