package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class PassTimestampWrites(
    public var querySet: QuerySet,
    public var beginningOfPassWriteIndex: Int = Constants.QUERY_SET_INDEX_UNDEFINED,
    public var endOfPassWriteIndex: Int = Constants.QUERY_SET_INDEX_UNDEFINED,
)
