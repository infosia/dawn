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
package androidx.webgpu

import android.content.Context
import android.webkit.WebView
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.webgpu.helper.createWebGpu
import org.junit.Test
import org.junit.runner.RunWith
import org.junit.Assert.assertTrue
import kotlinx.coroutines.runBlocking

@RunWith(AndroidJUnit4::class)
class WebViewDawnTest {
  @Test
  fun testWebViewDawnInitialization() = runBlocking {
    val context = ApplicationProvider.getApplicationContext<Context>()

    // Initialize WebView to ensure native library is loaded and provider is initialized
    androidx.test.platform.app.InstrumentationRegistry.getInstrumentation().runOnMainSync {
      WebView(context)
    }

    createWebGpu()
    assertTrue("WebGpu should be successfully created without crashing", true)
  }
}
