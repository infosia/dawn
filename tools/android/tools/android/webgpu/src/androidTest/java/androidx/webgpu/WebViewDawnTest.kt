package androidx.webgpu

import android.content.Context
import android.webkit.WebView
import androidx.test.core.app.ApplicationProvider
import androidx.test.ext.junit.runners.AndroidJUnit4

import androidx.webgpu.helper.initLibrary
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
        
        // initLibrary calls WebViewDawnLoader.getDawnLoader() and maps the address
        initLibrary()
        
        val webGpu = createWebGpu()
        assertTrue("WebGpu should be successfully created without crashing", webGpu != null)
    }
}
