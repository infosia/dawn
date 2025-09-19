package androidx.webgpu

/**
 * An RGBA color. Represents a `f32`, `i32`, or `u32` color using @ref
 * DoubleAsSupertype.  If any channel is non-finite, produces a @ref
 * NonFiniteFloatValueError.
 */
public class Color(
    public var r: Double,
    public var g: Double,
    public var b: Double,
    public var a: Double,
)
