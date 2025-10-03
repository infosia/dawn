package androidx.webgpu

public object Constants {
    private const val UINT32_MAX: Int = -1   // -1 to max int is resolved at compile time
    private const val UINT64_MAX: Long = -1L   // -1L to max long is resolved at compile time
    private const val SIZE_MAX = UINT64_MAX

    public const val ARRAY_LAYER_COUNT_UNDEFINED: Int = UINT32_MAX
    public const val COPY_STRIDE_UNDEFINED: Int = UINT32_MAX
    public const val DEPTH_CLEAR_VALUE_UNDEFINED: Float = Float.NaN
    public const val DEPTH_SLICE_UNDEFINED: Int = UINT32_MAX
    public const val LIMIT_U32_UNDEFINED: Int = UINT32_MAX
    public const val LIMIT_U64_UNDEFINED: Long = UINT64_MAX
    public const val MIP_LEVEL_COUNT_UNDEFINED: Int = UINT32_MAX
    public const val QUERY_SET_INDEX_UNDEFINED: Int = UINT32_MAX
    public const val STRLEN: Long = SIZE_MAX
    public const val WHOLE_MAP_SIZE: Long = SIZE_MAX
    public const val WHOLE_SIZE: Long = UINT64_MAX
}
