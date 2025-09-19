package androidx.webgpu

import kotlin.coroutines.resume
import kotlin.coroutines.suspendCoroutine


public class RequestDeviceReturn(
    public val status: RequestDeviceStatus,
    public val device: Device?,
    public val message: String,
) {
    public operator fun component1() : RequestDeviceStatus =status
    public operator fun component2() : Device? =device
    public operator fun component3() : String =message
}

public suspend fun Adapter.requestDevice(descriptor: DeviceDescriptor? = null,): RequestDeviceReturn = suspendCoroutine {
        requestDevice(descriptor, callbackInfo = RequestDeviceCallbackInfo(CallbackMode.AllowSpontaneous){status,device,message, -> it.resume(RequestDeviceReturn(status , device , message , )
            )
        })
    }

public class BufferMapReturn(
    public val status: MapAsyncStatus,
    public val message: String,
) {
    public operator fun component1() : MapAsyncStatus =status
    public operator fun component2() : String =message
}

public suspend fun Buffer.mapAsync(mode: MapMode,offset: Long,size: Long,): BufferMapReturn = suspendCoroutine {
        mapAsync(mode, offset, size, callbackInfo = BufferMapCallbackInfo(CallbackMode.AllowSpontaneous){status,message, -> it.resume(BufferMapReturn(status , message , )
            )
        })
    }

public class CreateComputePipelineAsyncReturn(
    public val status: CreatePipelineAsyncStatus,
    public val pipeline: ComputePipeline?,
    public val message: String,
) {
    public operator fun component1() : CreatePipelineAsyncStatus =status
    public operator fun component2() : ComputePipeline? =pipeline
    public operator fun component3() : String =message
}

public suspend fun Device.createComputePipelineAsync(descriptor: ComputePipelineDescriptor,): CreateComputePipelineAsyncReturn = suspendCoroutine {
        createComputePipelineAsync(descriptor, callbackInfo = CreateComputePipelineAsyncCallbackInfo(CallbackMode.AllowSpontaneous){status,pipeline,message, -> it.resume(CreateComputePipelineAsyncReturn(status , pipeline , message , )
            )
        })
    }

public class CreateRenderPipelineAsyncReturn(
    public val status: CreatePipelineAsyncStatus,
    public val pipeline: RenderPipeline?,
    public val message: String,
) {
    public operator fun component1() : CreatePipelineAsyncStatus =status
    public operator fun component2() : RenderPipeline? =pipeline
    public operator fun component3() : String =message
}

public suspend fun Device.createRenderPipelineAsync(descriptor: RenderPipelineDescriptor,): CreateRenderPipelineAsyncReturn = suspendCoroutine {
        createRenderPipelineAsync(descriptor, callbackInfo = CreateRenderPipelineAsyncCallbackInfo(CallbackMode.AllowSpontaneous){status,pipeline,message, -> it.resume(CreateRenderPipelineAsyncReturn(status , pipeline , message , )
            )
        })
    }

/**
 * @param status @see PopErrorScopeStatus.
 * @param type The type of the error caught by the scope, or @see ErrorType.NoError if there was none. If
 * the `status` is not @see PopErrorScopeStatus.Success, this is @see ErrorType.NoError.
 */
public class PopErrorScopeReturn(
    public val status: PopErrorScopeStatus,
    public val type: ErrorType,
    public val message: String,
) {
    public operator fun component1() : PopErrorScopeStatus =status
    public operator fun component2() : ErrorType =type
    public operator fun component3() : String =message
}

public suspend fun Device.popErrorScope(): PopErrorScopeReturn = suspendCoroutine {
        popErrorScope(callbackInfo = PopErrorScopeCallbackInfo(CallbackMode.AllowSpontaneous){status,type,message, -> it.resume(PopErrorScopeReturn(status , type , message , )
            )
        })
    }

public class RequestAdapterReturn(
    public val status: RequestAdapterStatus,
    public val adapter: Adapter?,
    public val message: String,
) {
    public operator fun component1() : RequestAdapterStatus =status
    public operator fun component2() : Adapter? =adapter
    public operator fun component3() : String =message
}

public suspend fun Instance.requestAdapter(options: RequestAdapterOptions? = RequestAdapterOptions(),): RequestAdapterReturn = suspendCoroutine {
        requestAdapter(options, callbackInfo = RequestAdapterCallbackInfo(CallbackMode.AllowSpontaneous){status,adapter,message, -> it.resume(RequestAdapterReturn(status , adapter , message , )
            )
        })
    }

/**
 * @param status @see QueueWorkDoneStatus.
 */
public class QueueWorkDoneReturn(
    public val status: QueueWorkDoneStatus,
    public val message: String,
) {
    public operator fun component1() : QueueWorkDoneStatus =status
    public operator fun component2() : String =message
}

public suspend fun Queue.onSubmittedWorkDone(): QueueWorkDoneReturn = suspendCoroutine {
        onSubmittedWorkDone(callbackInfo = QueueWorkDoneCallbackInfo(CallbackMode.AllowSpontaneous){status,message, -> it.resume(QueueWorkDoneReturn(status , message , )
            )
        })
    }

public class CompilationInfoReturn(
    public val status: CompilationInfoRequestStatus,
    public val compilationInfo: CompilationInfo,
) {
    public operator fun component1() : CompilationInfoRequestStatus =status
    public operator fun component2() : CompilationInfo =compilationInfo
}

public suspend fun ShaderModule.getCompilationInfo(): CompilationInfoReturn = suspendCoroutine {
        getCompilationInfo(callbackInfo = CompilationInfoCallbackInfo(CallbackMode.AllowSpontaneous){status,compilationInfo, -> it.resume(CompilationInfoReturn(status , compilationInfo , )
            )
        })
    }

