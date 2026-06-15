/*
 * Copyright 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package {{kotlin_package}}

/**
 * A marker for conditional WebGPU APIs that are not yet universally supported.
 */
@RequiresOptIn(message = "This WebGPU API is conditional and may not be supported on all WebView implementations.")
@Retention(AnnotationRetention.BINARY)
@Target(AnnotationTarget.FUNCTION, AnnotationTarget.CLASS)
public annotation class ConditionalApi

/**
 * Features that can be explicitly required at library initialization.
 */
public enum class WebGpuFeature(internal val id: Int) {
{% for feature_name, feature_id in webgpu_features.items() %}
    {{ feature_name }}({{ feature_id }}),
{% endfor %}
}
