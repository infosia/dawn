package androidx.webgpu

public class PassTimestampWrites(
    public var querySet: QuerySet,
    public var beginningOfPassWriteIndex: Int = Constants.QUERY_SET_INDEX_UNDEFINED,
    public var endOfPassWriteIndex: Int = Constants.QUERY_SET_INDEX_UNDEFINED,
)
