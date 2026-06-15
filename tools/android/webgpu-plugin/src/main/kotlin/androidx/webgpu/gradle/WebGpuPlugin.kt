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

package androidx.webgpu.gradle

import com.android.build.api.variant.AndroidComponentsExtension
import com.android.build.api.variant.BuildConfigField
import org.gradle.api.Plugin
import org.gradle.api.Project
import org.jetbrains.kotlin.gradle.tasks.KotlinCompile

import org.gradle.api.provider.Property

abstract class WebGpuExtension {
    abstract val useBundledDawn: Property<Boolean>
    abstract val enableExperimental: Property<Boolean>
}

class WebGpuPlugin : Plugin<Project> {
    override fun apply(project: Project) {
        val extension = project.extensions.create("webgpu", WebGpuExtension::class.java)

        val configureVariants = {
            val androidComponents = project.extensions.getByType(AndroidComponentsExtension::class.java)
            androidComponents.onVariants(androidComponents.selector().all()) { variant ->
                val useBundled = extension.useBundledDawn.getOrElse(false) || extension.enableExperimental.getOrElse(false)

                variant.buildConfigFields.put(
                    "USE_BUNDLED_DAWN",
                    BuildConfigField("boolean", useBundled.toString(), "Injected by WebGpuPlugin")
                )

                if (!useBundled) {
                    variant.packaging.jniLibs.excludes.add("**/libwebgpu_dawn_shared.so")
                }
            }
        }

        project.pluginManager.withPlugin("com.android.library") { configureVariants() }
        project.pluginManager.withPlugin("com.android.application") { configureVariants() }

        project.afterEvaluate {
            val useBundled = extension.useBundledDawn.getOrElse(false)
            val enableExp = extension.enableExperimental.getOrElse(false)

            if (useBundled || enableExp) {
                project.tasks.withType(KotlinCompile::class.java).configureEach { task ->
                    val args = task.kotlinOptions.freeCompilerArgs.toMutableList()
                    if (!args.contains("-opt-in=androidx.webgpu.ConditionalApi")) {
                        args.add("-opt-in=androidx.webgpu.ConditionalApi")
                    }
                    if (enableExp && !args.contains("-opt-in=androidx.webgpu.ExperimentalWebGpuApi")) {
                        args.add("-opt-in=androidx.webgpu.ExperimentalWebGpuApi")
                    }
                    task.kotlinOptions.freeCompilerArgs = args
                }
            }
        }
    }
}
