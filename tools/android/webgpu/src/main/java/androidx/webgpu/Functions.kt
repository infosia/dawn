package androidx.webgpu

import dalvik.annotation.optimization.FastNative


@FastNative
public external fun createInstance(descriptor: InstanceDescriptor? = null, ): Instance
@FastNative
public external fun getInstanceFeatures(features: SupportedInstanceFeatures, ): SupportedInstanceFeatures
@FastNative
public external fun getInstanceLimits(limits: InstanceLimits = InstanceLimits(), ): InstanceLimits
@FastNative
public external fun hasInstanceFeature(feature: InstanceFeatureName, ): Boolean
