import android.dawn.*
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class AsyncHelperTest {
    @Test
    fun asyncMethodTest() {
        dawnTestLauncher() { device ->
            val shaderModule = device.createShaderModule(
                ShaderModuleDescriptor(shaderSourceWGSL = ShaderSourceWGSL(""))
            )
            /* This is an asynchronous method, converted from a callback pattern by a helper. */
            val result = device.createRenderPipelineAsync(
                RenderPipelineDescriptor(vertex = VertexState(module = shaderModule))
            )
            assert(result.status == CreatePipelineAsyncStatus.ValidationError) {
                "Create render pipeline (async) should fail when no shader entry point exists."
            }
        }
    }
}
