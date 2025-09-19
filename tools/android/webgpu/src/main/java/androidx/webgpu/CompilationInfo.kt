package androidx.webgpu

import java.util.concurrent.Executor
import java.util.concurrent.ForkJoinPool

public class CompilationInfo(
    public var messages: Array<CompilationMessage> = arrayOf(),
)
