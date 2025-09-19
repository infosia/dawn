package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class AdapterInfo(
    public var vendor: String,
    public var architecture: String,
    public var device: String,
    public var description: String,
    @get:JvmName("getBackendType")
    public var backendType: BackendType = BackendType.Undefined,
    @get:JvmName("getAdapterType")
    public var adapterType: AdapterType,
    public var vendorID: Int,
    public var deviceID: Int,
    public var subgroupMinSize: Int,
    public var subgroupMaxSize: Int,
)
