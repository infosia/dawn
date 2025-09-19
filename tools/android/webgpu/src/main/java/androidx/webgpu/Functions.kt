package androidx.webgpu

import dalvik.annotation.optimization.FastNative


/**
* Create a WGPUInstance
* @param descriptor
*/
@FastNative
public external fun createInstance(descriptor: InstanceDescriptor? = null, ): Instance

/**
* Get the list of @ref WGPUInstanceFeatureName values supported by the instance.
* @param features
*/
@FastNative
public external fun getInstanceFeatures(features: SupportedInstanceFeatures, ): SupportedInstanceFeatures

/**
* Get the limits supported by the instance.
* @param limits
* @return Indicates if there was an @ref OutStructChainError.
*/
@FastNative
public external fun getInstanceLimits(limits: InstanceLimits = InstanceLimits(), ): InstanceLimits

/**
* Check whether a particular @ref WGPUInstanceFeatureName is supported by the
* instance.
* @param feature
*/
@FastNative
public external fun hasInstanceFeature(feature: InstanceFeatureName, ): Boolean
