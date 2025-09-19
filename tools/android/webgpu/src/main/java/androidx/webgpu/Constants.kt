package androidx.webgpu

public object Constants {
    /**
     * -1 to max int is resolved at compile time
     */
    private const val UINT32_MAX: Int = -1

    /**
     * -1L to max long is resolved at compile time
     */
    private const val UINT64_MAX: Long = -1L
    private const val SIZE_MAX = UINT64_MAX

    /**
     * Indicates no array layer count is specified. For more info, see @ref
     * SentinelValues and the places that use this sentinel value.
     */
    public const val ARRAY_LAYER_COUNT_UNDEFINED: Int = UINT32_MAX

    /**
     * Indicates no copy stride is specified. For more info, see @ref SentinelValues
     * and the places that use this sentinel value.
     */
    public const val COPY_STRIDE_UNDEFINED: Int = UINT32_MAX

    /**
     * Indicates no depth clear value is specified. For more info, see @ref
     * SentinelValues and the places that use this sentinel value.
     */
    public const val DEPTH_CLEAR_VALUE_UNDEFINED: Float = Float.NaN

    /**
     * Indicates no depth slice is specified. For more info, see @ref SentinelValues
     * and the places that use this sentinel value.
     */
    public const val DEPTH_SLICE_UNDEFINED: Int = UINT32_MAX

    /**
     * For `uint32_t` limits, indicates no limit value is specified. For more info, see
     * @ref SentinelValues and the places that use this sentinel value.
     */
    public const val LIMIT_U32_UNDEFINED: Int = UINT32_MAX

    /**
     * For `uint64_t` limits, indicates no limit value is specified. For more info, see
     * @ref SentinelValues and the places that use this sentinel value.
     */
    public const val LIMIT_U64_UNDEFINED: Long = UINT64_MAX

    /**
     * Indicates no mip level count is specified. For more info, see @ref
     * SentinelValues and the places that use this sentinel value.
     */
    public const val MIP_LEVEL_COUNT_UNDEFINED: Int = UINT32_MAX

    /**
     * Indicates no query set index is specified. For more info, see @ref
     * SentinelValues and the places that use this sentinel value.
     */
    public const val QUERY_SET_INDEX_UNDEFINED: Int = UINT32_MAX

    /**
     * Sentinel value used in @ref WGPUStringView to indicate that the pointer is to a
     * null-terminated string, rather than an explicitly-sized string.
     */
    public const val STRLEN: Long = SIZE_MAX

    /**
     * Indicates a size extending to the end of the buffer. For more info, see @ref
     * SentinelValues and the places that use this sentinel value.
     */
    public const val WHOLE_MAP_SIZE: Long = SIZE_MAX

    /**
     * Indicates a size extending to the end of the buffer. For more info, see @ref
     * SentinelValues and the places that use this sentinel value.
     */
    public const val WHOLE_SIZE: Long = UINT64_MAX
}
