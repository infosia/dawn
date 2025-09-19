package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class DeviceDescriptor(
    public var label: String? = null,
    @get:JvmName("getRequiredFeatures")
    public var requiredFeatures: Array<FeatureName> = arrayOf(),
    public var requiredLimits: Limits? = Limits(),
    public var defaultQueue: QueueDescriptor = QueueDescriptor(),
    public var deviceLostCallbackInfo: DeviceLostCallbackInfo,
    public var uncapturedErrorCallbackInfo: UncapturedErrorCallbackInfo = UncapturedErrorCallbackInfo(),
)
