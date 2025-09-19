package androidx.webgpu

import dalvik.annotation.optimization.FastNative


/**
* Create a WGPUInstance
*/
@FastNative
public external fun createInstance(descriptor: InstanceDescriptor? = null, ): Instance

/**
* Get the list of @ref WGPUInstanceFeatureName values supported by the instance.
*/
@FastNative
public external fun getInstanceFeatures(features: SupportedInstanceFeatures, ): SupportedInstanceFeatures

/**
* Get the limits supported by the instance.
* @return Indicates if there was an @ref OutStructChainError.
*/
@FastNative
public external fun getInstanceLimits(limits: InstanceLimits = InstanceLimits(), ): InstanceLimits

/**
* Check whether a particular @ref WGPUInstanceFeatureName is supported by the
* instance.
*/
@FastNative
public external fun hasInstanceFeature(feature: InstanceFeatureName, ): Boolean
